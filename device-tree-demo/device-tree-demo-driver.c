// SPDX-License-Identifier: GPL-2.0-only
/*
 * How to use device trees, demo driver
 *
 * Copyright (C) Stefan Eichenberger <stefan@embear.ch>
 */
#include <linux/platform_device.h>
#include <linux/of_gpio.h>

struct device_tree_demo_driver_data
{
	int gpio;
};

static ssize_t demo_gpio_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct device_tree_demo_driver_data *data = dev_get_drvdata(dev);
	int val;

	if (sscanf(buf, "%d", &val) != 1)
	    return -EINVAL;

	gpio_set_value(data->gpio, val);

	return count;
}

static DEVICE_ATTR(demo_gpio, S_IWUSR, NULL, demo_gpio_store);

static int device_tree_demo_driver_probe(struct platform_device *pdev)
{
    struct device_node *np = pdev->dev.of_node;
    struct device_tree_demo_driver_data *data;
    const char *str;
    int gpio;

    if (!np)
    {
	dev_err(&pdev->dev, "no device tree node\n");
	return -ENODEV;
    }

    if (of_property_read_string(np, "demo-string", &str))
    {
	dev_err(&pdev->dev, "no demo-string property\n");
	return -EINVAL;
    }

    dev_info(&pdev->dev, "demo-string: %s\n", str);

    // Get a gpio from the device tree
    gpio = of_get_named_gpio(np, "demo-gpio", 0);
    if (!gpio_is_valid(gpio))
	return -EINVAL;

    if (gpio_request(gpio, "demo-gpio"))
	return -EINVAL;

    // Set the gpio to output and set it to 1
    gpio_direction_output(gpio, 1);

    // Create a sysfs entry to set the gpio
    if (device_create_file(&pdev->dev, &dev_attr_demo_gpio))
	return -EINVAL;

    // Create a private data structure to hold the gpio
    data = devm_kzalloc(&pdev->dev, sizeof(struct device_tree_demo_driver_data), GFP_KERNEL);
    if (!data)
	return -ENOMEM;

    data->gpio = gpio;

    // Set the private data structure to the platform device
    dev_set_drvdata(&pdev->dev, data);

    return 0;
}

static int device_tree_demo_driver_remove(struct platform_device *pdev)
{
    return 0;
}

static const struct of_device_id device_tree_demo_driver_of_match[] = {
    { .compatible = "device-tree-demo", },
    { },
};

MODULE_DEVICE_TABLE(of, device_tree_demo_driver_of_match);

static struct platform_driver device_tree_demo_driver_driver = {
	.probe = device_tree_demo_driver_probe,
	.remove = device_tree_demo_driver_remove,
	.driver = {
	.name = "device-tree-demo-driver",
	.owner = THIS_MODULE,
	.of_match_table = device_tree_demo_driver_of_match,
	},
};

module_platform_driver(device_tree_demo_driver_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Stefan Eichenberger <stefan@embear.ch>");
MODULE_DESCRIPTION("A demo driver for using device trees");
