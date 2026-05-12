#ifndef _PVZ_DRIVER_H
#define _PVZ_DRIVER_H

/*
 * 这个头文件是 pvz_driver.c 内部用的"私有"声明，不会暴露给用户态。
 * pvz.h 那一份是 hw/sw 共享的 ABI（寄存器布局 + ioctl 编号），
 * 这里则只关心"我们这个 Linux 驱动自己要怎么记账"。
 *
 * 高层：
 *   - DRIVER_NAME 是这个驱动在 Linux 系统里的"名字"，会出现在
 *     /dev/pvz、/sys/class/misc/pvz、dmesg 的前缀里。
 *   - struct pvz_dev 是这个 platform driver 的"每设备状态"。
 *     probe 时填好，runtime 时 ioctl 拿来算 MMIO 地址，remove 时拿来清理。
 *
 * 低层：
 *   - <linux/miscdevice.h> 提供 misc_register / struct miscdevice，
 *     让我们能"借用" misc 框架直接拿到一个 /dev/pvz 节点，不用自己
 *     操心 char device major/minor 分配。
 *   - <linux/platform_device.h> 提供 platform_driver / platform_device，
 *     是 Linux 里给"通过 device tree 描述、挂在 SoC 内部总线上的外设"
 *     用的标准框架。我们这个 FPGA 外设就是这种角色。
 */

#include <linux/miscdevice.h>
#include <linux/platform_device.h>

/*
 * DRIVER_NAME 同时被三个地方用到：
 *   1. miscdevice.name —— 决定 /dev/pvz 的设备节点名（udev 据此命名）。
 *   2. request_mem_region 的标签 —— 在 /proc/iomem 里能看到 "pvz"。
 *   3. pr_info(DRIVER_NAME ": ...") —— dmesg 输出里所有日志前缀。
 * 改这个字符串等于同时改了用户态打开的路径、/proc 显示和日志前缀。
 */
#define DRIVER_NAME "pvz"

/*
 * struct pvz_dev —— 驱动的私有状态包。
 *
 * 高层：probe 阶段我们从 device tree 拿到 FPGA 外设的物理地址范围，
 *      然后把它 ioremap 成内核虚拟地址，之后 ioctl 写寄存器就用
 *      这个虚拟地址当基址。remove 阶段反过来把这两样还回去。
 *
 * 字段：
 *   res        —— 物理 MMIO 范围（start / end），从 device tree 的
 *                 "reg" 属性 + of_address_to_resource() 拿到。
 *                 在 DE1-SoC 上大概是 0xff20xxxx 起，正好对应
 *                 hw/pvz_top.sv 通过 Avalon-MM bridge 暴露出来的
 *                 那段 64 字（每字 4 字节）的寄存器窗口。
 *   virtbase   —— 内核虚拟地址，ioremap 出来的"指针"。
 *                 __iomem 这个 sparse 注解告诉静态检查器：
 *                 "这块地址不能直接 *p 解引用，必须用 iowrite32/ioread32
 *                 等带屏障的 IO 访问器"。pvz_ioctl 里就是用 iowrite32
 *                 往 virtbase + word_index*4 写值，最终落到 FPGA 的
 *                 Avalon-MM 寄存器文件里（参考 hw/pvz_top.sv 的写解码）。
 *
 * 备注：这个项目只挂一个外设实例，所以 pvz_driver.c 里直接搞了一个
 *      static 全局 struct pvz_dev dev; 没用 platform_set_drvdata 的
 *      指针挂载方式。多实例化需要换成动态分配 + drvdata。
 */
struct pvz_dev {
    struct resource res;
    void __iomem *virtbase;
};

#endif /* _PVZ_DRIVER_H */
