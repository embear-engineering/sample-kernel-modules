// SPDX-License-Identifier: GPL-2.0+
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_fdt.h>
#include <linux/of_reserved_mem.h>
#include <linux/crc32.h>
#include <linux/reboot.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/seq_file.h>

/* Reset reasons */
#define POWEROFF_PATTERN			(0x0)
#define BOOT_PATTERN				(0x424f4f54)
#define REBOOT_PATTERN				(0x5245424f)
#define OOPS_PATTERN				(0x4f4f5053)
#define WATCHDOG_PATTERN			(0x781f9ce2)

/* Make sure you patch and enable the pretimeout_notifier governor.
 * Else you should uncomment ENABLE_WATCHDOG
 */
#define ENABLE_WATCHDOG
#ifdef ENABLE_WATCHDOG
extern struct atomic_notifier_head watchdog_notifier_list;
#endif

struct reset_reason_platform_data *pdata;

struct reset_registers {
	uint32_t rr_value;
	uint32_t rr_value_crc;
};

struct reset_reason_platform_data {
	uint32_t last_reset_pattern;
	bool oops_pending;

	struct reset_registers *regs;	/* write needs to happen in memory */
};

static const char *get_reset_reason(struct reset_reason_platform_data *pdata)
{
	if (pdata == NULL) {
		pr_err("reset-reason: module not initialized yet\n");
		return "unknown";
	}

	switch (pdata->last_reset_pattern) {
	case BOOT_PATTERN:
		return "unknown (e.g. voltage dip)";
	case REBOOT_PATTERN:
		return "reboot";
	case OOPS_PATTERN:
		return "panic";
	case WATCHDOG_PATTERN:
		return "watchdog";
	default:
		return "power-cycle";
	}
}

static ssize_t reset_reason_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct reset_reason_platform_data *pdata = dev->platform_data;

	return snprintf(buf, PAGE_SIZE, "%s\n", get_reset_reason(pdata));
}
DEVICE_ATTR_RO(reset_reason);

static struct attribute *reset_reasons_attrs[] = {
	&dev_attr_reset_reason.attr,
	NULL,
};
ATTRIBUTE_GROUPS(reset_reasons);

static void read_reset_reg(struct reset_reason_platform_data *pdata)
{
	unsigned int val = ioread32(&pdata->regs->rr_value);
	unsigned int crc = ioread32(&pdata->regs->rr_value_crc);
	uint32_t actual_crc = ether_crc(sizeof(pdata->regs->rr_value), (unsigned char *)&val);

	pdata->last_reset_pattern = 0;

	if (actual_crc == crc) {
		pdata->last_reset_pattern = val;
	} else {
		pr_debug("reset-reason: value: 0x%08X crc is 0x%08X should be 0x%08X",
				val, crc, actual_crc);
	}
}

static void write_reset_reg(struct reset_registers *reg, uint32_t value)
{
	/* Use ethernet crc which is crc32 with 0xFFFFFFFFF as seed and in little endian */
	unsigned int crc = ether_crc(sizeof(reg->rr_value), (unsigned char *)&value);

	/* Use iowrite to be sure the write actually happens */
	iowrite32(value, &reg->rr_value);
	iowrite32(crc, &reg->rr_value_crc);
}

static int reboot_notify(struct notifier_block *this, unsigned long code, void *cmd)
{
	if (code == SYS_RESTART) {
		/* If an oops was happending before we keep it as an OOPS */
		if (!pdata->oops_pending)
			write_reset_reg(pdata->regs, REBOOT_PATTERN);
	} else {
		/* Make sure we have a proper 0 if an imediate power on would follow */
		write_reset_reg(pdata->regs, POWEROFF_PATTERN);
	}
	return 0;
}

static struct notifier_block reboot_notify_block = {
	.notifier_call = reboot_notify,
};

static int panic_notify(struct notifier_block *this, unsigned long event, void *ptr)
{
	write_reset_reg(pdata->regs, OOPS_PATTERN);
	pdata->oops_pending = true;
	return 0;
}

static struct notifier_block panic_notify_block = {
	.notifier_call = panic_notify,
};

#ifdef ENABLE_WATCHDOG
static int watchdog_notify(struct notifier_block *this, unsigned long event, void *ptr)
{
	write_reset_reg(pdata->regs, WATCHDOG_PATTERN);
	return 0;
}

static struct notifier_block watchdog_notify_block = {
	.notifier_call = watchdog_notify,
};
#endif

static int reset_reason_probe(struct platform_device *pdev)
{
	int ret;
	struct reserved_mem *rmem = NULL;
	struct device_node *node;

	pdata = pdev->dev.platform_data;
	if (pdata) {
		dev_err(&pdev->dev, "Platform data already initialized\n");
		return -EINVAL;
	}

	pdata = devm_kzalloc(&pdev->dev, sizeof(*pdata), GFP_KERNEL);
	if (!pdata)
		return -ENOMEM;

	pdata->oops_pending = false;
	pdev->dev.platform_data = pdata;

	node = of_parse_phandle(pdev->dev.of_node, "memory-region", 0);
	if (!node) {
		dev_err(&pdev->dev, "Memory region missing\n");
		return -EINVAL;
	}
	rmem = of_reserved_mem_lookup(node);
	of_node_put(node);

	pdata->regs = ioremap(rmem->base, rmem->size);
	if (pdata->regs == NULL) {
		dev_err(&pdev->dev, "Can not remap memory\n");
		return -ENOMEM;
	}

	/* Register the callbacks */
	register_reboot_notifier(&reboot_notify_block);
	atomic_notifier_chain_register(&panic_notifier_list, &panic_notify_block);
#ifdef ENABLE_WATCHDOG
	/* Make sure you patch the kernel with the pertimeout_notifier governor,
	 * else this will not work
	 */
	atomic_notifier_chain_register(&watchdog_notifier_list, &watchdog_notify_block);
#endif

	read_reset_reg(pdata);

	ret = devm_device_add_groups(&pdev->dev, reset_reasons_groups);
	if (ret) {
		dev_err(&pdev->dev, "Could not create sysfs groups: %i\n", ret);
		return -EINVAL;
	}

	dev_dbg(&pdev->dev, "Last reset pattern: 0x%08X\n", pdata->last_reset_pattern);
	dev_info(&pdev->dev, "Last reset reason: %s\n", get_reset_reason(pdata));

	/* This pattern is set during boot, if it still available after reboot,
	 * we had an unknown reset
	 */
	write_reset_reg(pdata->regs, BOOT_PATTERN);

	return 0;
}

static int reset_reason_remove(struct platform_device *pdev)
{
	struct reset_reason_platform_data *pdata = pdev->dev.platform_data;

	unregister_reboot_notifier(&reboot_notify_block);
	atomic_notifier_chain_unregister(&panic_notifier_list, &panic_notify_block);
#ifdef ENABLE_WATCHDOG
	atomic_notifier_chain_unregister(&watchdog_notifier_list, &watchdog_notify_block);
#endif

	iounmap(pdata->regs);

	pdata = NULL;
	pdev->dev.platform_data = NULL;

	return 0;
}

static const struct of_device_id dt_match[] = {
	{ .compatible = "reset-reason" },
	{}
};

static struct platform_driver reset_reason_driver = {
	.probe		= reset_reason_probe,
	.remove		= reset_reason_remove,
	.driver		= {
		.name = "reset-reason",
		.of_match_table	= dt_match,
	},
};

module_platform_driver(reset_reason_driver);

MODULE_AUTHOR("Stefan Eichenberger <stefan@embear.ch>");
MODULE_DESCRIPTION("Software based reset reason detection");
MODULE_LICENSE("GPL v2");
