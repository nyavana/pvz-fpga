/*
 * PvZ GPU kernel driver
 *
 * Misc device at /dev/pvz with a single ioctl, PVZ_WRITE_REG, that
 * writes one 32-bit value into the FPGA register file.  Avalon
 * addressing is in WORDS (set in pvz_top_hw.tcl) so the byte offset
 * is `word_index * 4`.
 *
 * Compatible: csee4840,pvz_gpu-1.0
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include "pvz.h"
#include "pvz_driver.h"

static struct pvz_dev dev;

static long pvz_ioctl(struct file *f, unsigned int cmd, unsigned long arg)
{
    pvz_write_arg_t w;

    switch (cmd) {
    case PVZ_WRITE_REG:
        if (copy_from_user(&w, (pvz_write_arg_t *)arg, sizeof(w)))
            return -EACCES;
        if (w.word_index >= PVZ_NUM_REGS)
            return -EINVAL;
        iowrite32(w.value, dev.virtbase + (w.word_index * 4));
        break;

    default:
        return -EINVAL;
    }

    return 0;
}

static const struct file_operations pvz_fops = {
    .owner          = THIS_MODULE,
    .unlocked_ioctl = pvz_ioctl,
};

static struct miscdevice pvz_misc_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name  = DRIVER_NAME,
    .fops  = &pvz_fops,
};

static int __init pvz_probe(struct platform_device *pdev)
{
    int ret;

    ret = misc_register(&pvz_misc_device);
    if (ret) {
        pr_err(DRIVER_NAME ": misc_register failed\n");
        return ret;
    }

    ret = of_address_to_resource(pdev->dev.of_node, 0, &dev.res);
    if (ret) {
        ret = -ENOENT;
        goto out_deregister;
    }

    if (request_mem_region(dev.res.start, resource_size(&dev.res),
                           DRIVER_NAME) == NULL) {
        ret = -EBUSY;
        goto out_deregister;
    }

    dev.virtbase = of_iomap(pdev->dev.of_node, 0);
    if (dev.virtbase == NULL) {
        ret = -ENOMEM;
        goto out_release_mem_region;
    }

    pr_info(DRIVER_NAME ": initialized at 0x%08lx\n",
            (unsigned long)dev.res.start);

    return 0;

out_release_mem_region:
    release_mem_region(dev.res.start, resource_size(&dev.res));
out_deregister:
    misc_deregister(&pvz_misc_device);
    return ret;
}

static int pvz_remove(struct platform_device *pdev)
{
    iounmap(dev.virtbase);
    release_mem_region(dev.res.start, resource_size(&dev.res));
    misc_deregister(&pvz_misc_device);
    pr_info(DRIVER_NAME ": removed\n");
    return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id pvz_of_match[] = {
    { .compatible = "csee4840,pvz_gpu-1.0" },
    {},
};
MODULE_DEVICE_TABLE(of, pvz_of_match);
#endif

static struct platform_driver pvz_driver = {
    .driver = {
        .name           = DRIVER_NAME,
        .owner          = THIS_MODULE,
        .of_match_table = of_match_ptr(pvz_of_match),
    },
    .remove = __exit_p(pvz_remove),
};

static int __init pvz_init(void)
{
    pr_info(DRIVER_NAME ": init\n");
    return platform_driver_probe(&pvz_driver, pvz_probe);
}

static void __exit pvz_exit(void)
{
    platform_driver_unregister(&pvz_driver);
    pr_info(DRIVER_NAME ": exit\n");
}

module_init(pvz_init);
module_exit(pvz_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("CSEE4840 Team");
MODULE_DESCRIPTION("PvZ GPU display engine driver");
