#include <linux/leds.h>
#include <linux/of.h>
#include <linux/module.h>
#include <linux/platform_device.h>

static int skeleton_probe(struct platform_device *pdev)
{
	return 0;
}

static void skeleton_remove(struct platform_device *pdev)
{
}

static const struct of_device_id of_skeleton_match[] = {
	{ .compatible = "skeleton", },
	{},
};

static struct platform_driver skeleton_driver = {
	.probe		= skeleton_probe,
	.shutdown	= skeleton_remove,
	.driver		= {
		.name	= "skeleton",
		.of_match_table = of_skeleton_match,
	},
};

module_platform_driver(skeleton_driver);

MODULE_AUTHOR("Stefan Eichenberger <stefan@embear.ch>");
MODULE_DESCRIPTION("Skeleton driver");
MODULE_LICENSE("GPL");
