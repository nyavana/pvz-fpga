/*
 * PvZ GPU kernel driver
 *
 * Misc device at /dev/pvz with a single ioctl, PVZ_WRITE_REG, that
 * writes one 32-bit value into the FPGA register file.  Avalon
 * addressing is in WORDS (set in pvz_top_hw.tcl) so the byte offset
 * is `word_index * 4`.
 *
 * Compatible: csee4840,pvz_gpu-1.0
 *
 * ===================================================================
 * 中文概览 —— 这是整个 SW 栈里唯一一段跑在 kernel space 的代码。
 * ===================================================================
 *
 * 这个驱动干的事其实就一件：把"用户态发过来的 32 位值"写到 FPGA
 * 那个 Avalon-MM 寄存器文件的某个字里。流程：
 *
 *   用户态 game loop
 *      |  ioctl(fd, PVZ_WRITE_REG, &{word_index, value})
 *      v
 *   pvz_ioctl()
 *      |  copy_from_user 把 struct 搬进内核
 *      |  边界检查 word_index < PVZ_NUM_REGS
 *      |  iowrite32(value, virtbase + word_index * 4)
 *      v
 *   Avalon-MM bridge (HPS -> FPGA)
 *      v
 *   hw/pvz_top.sv 的 always_ff 解码，把值塞进对应寄存器
 *
 * 关键设计选择：
 *   - misc device 框架（不是手动注册 chrdev）：省事，自动分配 minor，
 *     udev 会自动创建 /dev/pvz 节点。
 *   - platform_driver + device tree 匹配：FPGA 外设挂在 SoC 内部总线上，
 *     物理地址（约 0xff20xxxx）在 device tree 里描述，靠 compatible
 *     字符串 "csee4840,pvz_gpu-1.0" 找到我们这个驱动。
 *     同样的字符串还要出现在 hw/pvz_top_hw.tcl 和 .dtb 里 —— 三处必须一致！
 *   - 没有 read/write fops：只暴露 ioctl，因为我们其实写的是稀疏寄存器，
 *     用 seek+write 的语义反而别扭。
 *   - 没有 vsync 同步：写下去立刻生效，可能产生 1 帧 tearing，
 *     对 60Hz 游戏可以接受（参考 hw/pvz_top.sv 顶部说明）。
 */

#include <linux/module.h>            /* 模块基本宏 MODULE_LICENSE 等 */
#include <linux/init.h>              /* __init / __exit 段标记 */
#include <linux/errno.h>             /* -EACCES, -EINVAL, -ENOMEM 等错误码 */
#include <linux/version.h>           /* LINUX_VERSION_CODE（这里没直接用，但常见保留） */
#include <linux/kernel.h>            /* pr_info / pr_err 等内核打印 */
#include <linux/platform_device.h>   /* platform_driver / platform_device 框架 */
#include <linux/miscdevice.h>        /* miscdevice / misc_register —— 拿 /dev/pvz */
#include <linux/slab.h>              /* kmalloc/kfree（这里没动态分配，但很多模块习惯带上） */
#include <linux/io.h>                /* ioremap / iowrite32 / ioread32 等 MMIO API */
#include <linux/of.h>                /* Open Firmware / device tree 解析 */
#include <linux/of_address.h>        /* of_address_to_resource / of_iomap */
#include <linux/fs.h>                /* struct file_operations */
#include <linux/uaccess.h>           /* copy_from_user / copy_to_user */
#include "pvz.h"                     /* 寄存器布局 + ioctl 编号（和 hw/pvz_top.sv 共享） */
#include "pvz_driver.h"              /* struct pvz_dev、DRIVER_NAME */

/*
 * 单例全局：因为整个项目只挂一个 PvZ GPU 外设，所以直接用 static
 * 全局存设备状态，不用 platform_set_drvdata + 动态分配的复杂玩法。
 * 如果以后要支持多实例（不太可能），需要把它改成 per-pdev 分配。
 */
static struct pvz_dev dev;

/*
 * pvz_ioctl —— 这个驱动唯一对用户态开放的入口。
 *
 * 高层：用户态调 ioctl(fd, PVZ_WRITE_REG, &arg) 时内核就跳到这里。
 *      sw/render.c 里那个 write_reg() 小工具就是不停在调它，每帧
 *      把 51 个寄存器字推下去。
 *
 * 参数：
 *   f    —— 内核给的 struct file *，对应用户那边的 fd（这里没用）。
 *   cmd  —— ioctl 命令号，对照 pvz.h 里 _IOW 定义的 PVZ_WRITE_REG。
 *   arg  —— 用户态传过来的指针（unsigned long 是约定，需要强转）。
 *
 * 注意为什么签名是 long 而不是 int：
 *   现代内核用 .unlocked_ioctl（旧的 .ioctl 已经被去掉了），
 *   规范返回类型是 long。返回 0 = 成功，负数 = -errno。
 *
 * 用户/内核空间隔离：
 *   arg 是用户态地址，绝对不能直接 *((pvz_write_arg_t *)arg)，
 *   一旦用户给了非法地址内核就 panic。所以必须 copy_from_user。
 *   它会做完整的地址校验 + 失败时返回拷贝失败字节数（非 0 即错）。
 */
static long pvz_ioctl(struct file *f, unsigned int cmd, unsigned long arg)
{
    /* 内核栈上的副本，从用户态拷贝过来后只在内核内访问 */
    pvz_write_arg_t w;

    switch (cmd) {
    case PVZ_WRITE_REG:
        /*
         * 把用户态的 pvz_write_arg_t struct 拷到内核栈上的 w。
         * 失败（用户给了非法/不可访问的指针）就返回 -EACCES。
         */
        if (copy_from_user(&w, (pvz_write_arg_t *)arg, sizeof(w)))
            return -EACCES;
        /*
         * 边界检查。PVZ_NUM_REGS = 51（见 pvz.h）。
         * 没这个检查的话越界写就会污染 FPGA 寄存器空间外的地址，
         * 在 DE1-SoC 上可能访到 H2F bridge 上别的外设，后果不可控。
         */
        if (w.word_index >= PVZ_NUM_REGS)
            return -EINVAL;
        /*
         * 核心一行：真正把数据写到 FPGA。
         *   - dev.virtbase 是 ioremap 给出来的内核虚拟地址（probe 时设的）
         *   - word_index * 4 把"字索引"变成"字节偏移"，因为 ARM 地址是字节寻址
         *     但 Avalon-MM 那边 addressUnits=WORDS（hw/pvz_top_hw.tcl 配置），
         *     所以硬件最终拿到的地址 = (word_index*4) >> 2 = word_index，
         *     正好命中 hw/pvz_top.sv 的 always_ff 解码 (address == 6'd0..50)。
         *   - iowrite32 自带 memory barrier，保证写完才返回。
         *
         * 举个完整例子：用户写 word_index=48 (PVZ_REG_CURSOR), value=0x80000C01：
         *   iowrite32 写到 virtbase + 192 (= 0xC0 字节偏移)
         *   -> Avalon address = 0xC0 >> 2 = 48
         *   -> hw/pvz_top.sv 命中 `address == 6'd48` 分支
         *   -> cursor_visible <= 1, cursor_col <= 0, cursor_row <= 1
         *   -> hw/entity_drawer.sv 在 (col=0,row=1) 那个格子画黄色边框。
         */
        iowrite32(w.value, dev.virtbase + (w.word_index * 4));
        break;

    default:
        /* 不认识的 cmd 一律返回 -EINVAL，避免用户拿其他 ioctl 进来乱搞 */
        return -EINVAL;
    }

    return 0;
}

/*
 * pvz_fops —— file_operations 表，告诉 VFS"用户对 /dev/pvz 做什么操作时
 * 应该跳到哪个函数"。
 *
 * 我们只定义两个字段：
 *   .owner          THIS_MODULE，让内核知道这个 fops 属于 pvz_driver 模块，
 *                   有人持有 fd 的时候模块不会被 rmmod 掉。
 *   .unlocked_ioctl pvz_ioctl，唯一的入口（没有 .read/.write/.mmap）。
 *
 * 用户 open("/dev/pvz") 时内核会用默认的 simple_open（misc 框架提供），
 * close 时同理走默认释放，所以这里不需要 .open/.release。
 */
static const struct file_operations pvz_fops = {
    .owner          = THIS_MODULE,
    .unlocked_ioctl = pvz_ioctl,
};

/*
 * pvz_misc_device —— misc device 注册信息。
 *
 * 高层：misc device 是 Linux 提供的"轻量级 char device 框架"，
 *      省得我们自己 alloc_chrdev_region + cdev_init + device_create。
 *      注册成功后会自动出现 /dev/<name> 节点（udev 创建），
 *      major 号固定是 10，minor 由内核动态分配。
 *
 * 字段：
 *   .minor MISC_DYNAMIC_MINOR
 *          让内核自己挑一个空闲 minor 号，不会和别的 misc 设备冲突。
 *   .name  DRIVER_NAME ("pvz")
 *          决定设备节点名 —— udev 看到这个 name 就会创建 /dev/pvz。
 *   .fops  &pvz_fops
 *          指向上面那个 fops 表，让 VFS 知道操作回调。
 */
static struct miscdevice pvz_misc_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name  = DRIVER_NAME,
    .fops  = &pvz_fops,
};

/*
 * pvz_probe —— 当 platform 总线发现一个 compatible 字符串匹配的
 * device tree 节点时被调用。这是整个驱动的"安装"流程。
 *
 * 触发条件：
 *   .dtb 里有这样一段（具体由 hw/Makefile 的 make dtb 通过 sopc2dts 生成）：
 *     pvz_gpu@ff20xxxx {
 *         compatible = "csee4840,pvz_gpu-1.0";
 *         reg = <0xff20xxxx 0x1000>;
 *     };
 *   内核启动 / insmod 时遍历 device tree，找到这个节点，
 *   match 到我们的 pvz_of_match 表，就调到这里。
 *
 * 步骤（按顺序，每步失败都要回滚已经做过的）：
 *   1. misc_register —— 注册 /dev/pvz 字符设备
 *   2. of_address_to_resource —— 从 device tree 读出物理 MMIO 范围
 *   3. request_mem_region —— 在 /proc/iomem 占一段，防止别的驱动抢
 *   4. of_iomap —— 把物理地址 ioremap 成内核虚拟地址，存在 dev.virtbase
 *   5. pr_info 打个日志方便从 dmesg 确认绑定成功
 *
 * 错误处理：经典的 goto-out 风格 —— C 没有 try/catch，需要把已分配的
 *           资源按相反顺序释放掉。Linux 内核源码里几乎所有 probe 都长这样。
 */
static int __init pvz_probe(struct platform_device *pdev)
{
    int ret;

    /* 第 1 步：创建 /dev/pvz。要是这步失败用户态根本进不来，所以最先做。 */
    ret = misc_register(&pvz_misc_device);
    if (ret) {
        pr_err(DRIVER_NAME ": misc_register failed\n");
        return ret;
    }

    /*
     * 第 2 步：解析 device tree 节点的 "reg" 属性，填到 dev.res.start / .end。
     *   pdev->dev.of_node 指向匹配上的 DT 节点
     *   第二个参数 0 表示取 reg 属性里的第 0 段地址范围
     *   失败一般是 DT 写错了或没有 reg 属性 —— 返回 -ENOENT 让用户知道
     */
    ret = of_address_to_resource(pdev->dev.of_node, 0, &dev.res);
    if (ret) {
        ret = -ENOENT;
        goto out_deregister;
    }

    /*
     * 第 3 步：在 /proc/iomem 里把这段物理地址登记成 "pvz" 占用。
     * 这一步不会真的让 CPU 能访问到地址，它只是个"互斥锁"：
     * 防止另一个驱动也来 ioremap 同一段地址造成冲突。
     * 失败说明地址被别人占了 —— 返回 -EBUSY。
     */
    if (request_mem_region(dev.res.start, resource_size(&dev.res),
                           DRIVER_NAME) == NULL) {
        ret = -EBUSY;
        goto out_deregister;
    }

    /*
     * 第 4 步：把物理地址映射成 CPU 可访问的虚拟地址。
     * of_iomap = 从 device tree 节点直接 ioremap 的便捷版。
     * 结果存到 dev.virtbase 里，pvz_ioctl 用它当基址做 iowrite32。
     * 失败一般是地址空间分配不出来 —— 返回 -ENOMEM。
     */
    dev.virtbase = of_iomap(pdev->dev.of_node, 0);
    if (dev.virtbase == NULL) {
        ret = -ENOMEM;
        goto out_release_mem_region;
    }

    /*
     * 一切就绪，打个 info 日志。运行时可以 dmesg | grep pvz 来确认
     * 驱动绑定成功，并看清楚 FPGA 外设的物理地址（一般是 0xff20xxxx）。
     */
    pr_info(DRIVER_NAME ": initialized at 0x%08lx\n",
            (unsigned long)dev.res.start);

    return 0;

/*
 * 下面是回滚标签。注意顺序：越靠下的标签做的事越少（只回滚到刚发生失败那一步）。
 * out_release_mem_region：第 4 步 ioremap 失败时进来 —— 释放第 3 步占的内存区。
 * out_deregister：       第 2 或第 3 步失败时进来 —— 卸载第 1 步注册的 misc 设备。
 */
out_release_mem_region:
    release_mem_region(dev.res.start, resource_size(&dev.res));
out_deregister:
    misc_deregister(&pvz_misc_device);
    return ret;
}

/*
 * pvz_remove —— probe 的反向操作。在 rmmod pvz_driver 或者
 * 设备消失（DT 节点被去掉，几乎不会发生）的时候被调用。
 *
 * 顺序必须严格反着来：
 *   1. iounmap            —— 把 virtbase 那段虚拟地址映射撤掉
 *   2. release_mem_region —— 把 /proc/iomem 里占的那段还回去
 *   3. misc_deregister    —— 删掉 /dev/pvz 节点
 *
 * 顺序很重要：如果先 misc_deregister 再 iounmap，期间如果有最后一个
 * 还没退出的 ioctl 在跑就会访问已经被回收的地址。先把 IO 关掉再断 fop
 * 入口才是稳的。当然 misc_deregister 内部会等所有 fd 都关掉再返回，
 * 实际上 unloaded module 的引用计数也防住了，但保持反向顺序是好习惯。
 */
static int pvz_remove(struct platform_device *pdev)
{
    iounmap(dev.virtbase);
    release_mem_region(dev.res.start, resource_size(&dev.res));
    misc_deregister(&pvz_misc_device);
    pr_info(DRIVER_NAME ": removed\n");
    return 0;
}

/*
 * Device Tree 匹配表。
 *
 * compatible 字符串是 device tree 和驱动之间的"绑定钥匙"。
 * 必须和下面三处保持完全一致：
 *   1. hw/pvz_top_hw.tcl 里 set_module_property COMPATIBLE 那一行
 *   2. .dtb 里那个外设节点的 compatible 属性
 *   3. 这里 pvz_of_match[] 的字符串
 * 任何一处写错，insmod 之后 probe 都不会被触发，/dev/pvz 也不会出现。
 * README 的 Debugging tips 里专门提到过这个坑。
 *
 * 末尾的空项 { } 是惯例，告诉内核"表到这里结束"。
 *
 * MODULE_DEVICE_TABLE 让 depmod / udev 能从 .ko 里读出我们支持哪些
 * device tree 节点，自动加载驱动。
 *
 * CONFIG_OF 是内核 Open Firmware (设备树) 支持的编译开关，在 ARM Linux
 * 上几乎一定为 true。
 */
#ifdef CONFIG_OF
static const struct of_device_id pvz_of_match[] = {
    { .compatible = "csee4840,pvz_gpu-1.0" },
    {},
};
MODULE_DEVICE_TABLE(of, pvz_of_match);
#endif

/*
 * platform_driver 结构 —— 把上面所有零件串起来注册给 platform 子系统。
 *
 * 字段：
 *   .driver.name           DRIVER_NAME ("pvz")，会出现在
 *                          /sys/bus/platform/drivers/ 下面，方便调试。
 *   .driver.owner          THIS_MODULE，引用计数用。
 *   .driver.of_match_table 上面的 pvz_of_match 表，用于 DT 匹配。
 *                          of_match_ptr() 在 CONFIG_OF 关掉时会展开成 NULL，
 *                          这样不开 DT 的内核也能编过。
 *   .remove                __exit_p(pvz_remove) —— 让 pvz_remove 落到
 *                          .exit 段，模块卸载时才用得到，节省常驻内存。
 *   注意 .probe 在这里没有直接填：而是通过 platform_driver_probe()
 *        在 pvz_init 里显式传进去。这样写有个好处：probe 函数用了
 *        __init 标记，平时常驻内存里可以没有它。
 */
static struct platform_driver pvz_driver = {
    .driver = {
        .name           = DRIVER_NAME,
        .owner          = THIS_MODULE,
        .of_match_table = of_match_ptr(pvz_of_match),
    },
    .remove = __exit_p(pvz_remove),
};

/*
 * pvz_init —— 模块加载入口。insmod pvz_driver.ko 时被调用。
 *
 * 用 platform_driver_probe 而不是 platform_driver_register 是有道理的：
 *   - register 表示"以后只要出现匹配的设备都尝试 probe"，常用于热插拔。
 *   - probe   表示"现在立刻尝试 probe 一次，以后不再监听"，适合像
 *     FPGA 这种"开机时就已经在 DT 里描述好、不会插拔"的外设。
 *     用它还能让 probe 函数走 __init 段，加载完就释放内存。
 */
static int __init pvz_init(void)
{
    pr_info(DRIVER_NAME ": init\n");
    return platform_driver_probe(&pvz_driver, pvz_probe);
}

/*
 * pvz_exit —— 模块卸载入口。rmmod pvz_driver 时被调用。
 * 注销 platform driver 会触发 pvz_remove 把资源都还回去。
 */
static void __exit pvz_exit(void)
{
    platform_driver_unregister(&pvz_driver);
    pr_info(DRIVER_NAME ": exit\n");
}

/* 把 init/exit 跟内核模块框架挂钩。这两行少了模块根本不动。 */
module_init(pvz_init);
module_exit(pvz_exit);

/*
 * 模块元数据 —— modinfo pvz_driver.ko 时能看见。
 * LICENSE 必须填"GPL"才能用很多 GPL-only 的内核符号
 * （比如某些 ioremap 变种）。这也是 Linus 提的政治正确要求。
 */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("CSEE4840 Team");
MODULE_DESCRIPTION("PvZ GPU display engine driver");
