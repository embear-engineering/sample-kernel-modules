// SPDX-License-Identifier: GPL-2.0-only
/*
 * LEDs driver for GPIOs
 *
 * Copyright (C) Stefan Eichenberger <stefan@embear.ch>
 */
#include <linux/leds.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/module.h>
#include <linux/platform_device.h>

struct led_control_data {
	struct device *dev;
	struct led_classdev *led_cdev;
	const char *state;
};

struct device_state_led_blink_entry {
	const char *device_state;
	unsigned long time_on;
	unsigned long time_off;
};

#define FIRST_STATE "booting"
static struct device_state_led_blink_entry device_state_led_blink_table[] = {
	{FIRST_STATE, 500, 100},
	{"running", 500, 0},
	{"shutdown", 100, 500}
};

static ssize_t set_device_state(struct led_control_data *pdata,
		const char *state, size_t count)
{
	unsigned int i;

	pr_info("Set state: %s\n", state);

	/* Set the blink timing found in the table */
	for (i = 0; i < ARRAY_SIZE(device_state_led_blink_table); i++) {
		struct device_state_led_blink_entry *entry =
			&device_state_led_blink_table[i];
		pr_info("cmp %s with %s\n", state, entry->device_state);
		if (!strncmp(entry->device_state, state, count-1)) {
			led_blink_set(pdata->led_cdev,
					&entry->time_on, &entry->time_off);
			pdata->state = entry->device_state;
			break;
		}
	}

	if (i == ARRAY_SIZE(device_state_led_blink_table))
			return -EINVAL;

	return count;
}

static ssize_t device_state_show(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	struct led_control_data *pdata = dev_get_drvdata(dev);

	strcpy(buf, pdata->state);

	return strlen(buf);
}

static ssize_t device_state_store(struct device *dev, struct device_attribute *attr,
		 const char *buf, size_t count)
{
	struct led_control_data *pdata = dev_get_drvdata(dev);

	return set_device_state(pdata, buf, count);
}


DEVICE_ATTR_RW(device_state);

static int match_led(struct device *dev, void *data)
{
	return dev->of_node->phandle == ((struct device_node*)data)->phandle;
}

static int led_control_probe(struct platform_device *pdev)
{
	struct device_node *np;
	struct device_node *led_node;
	struct device_node *leds_node;
	struct platform_device *led_pdev;
	struct led_control_data *data = NULL;
	struct device *led_dev;
	int err = 0;

	data = devm_kzalloc(&pdev->dev, sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	data->dev = &pdev->dev;
	np = data->dev->of_node;
	dev_set_drvdata(&pdev->dev, data);

	/* Now we need to find the led class device from the phandle provided
	 * in the device tree:
	 * led = <&led>;
	 */
	led_node = of_parse_phandle(np, "led", 0);
	if (!led_node) {
		pr_err("No led node found\n");
		return -EINVAL;
	}

	/* This gives us the leds platform node */
	leds_node = of_get_parent(led_node);
	if (!leds_node) {
		of_node_put(led_node);
		pr_err("Can not get parent node\n");
		return -EINVAL;
	}

	/* This gives us the leds device */
	led_pdev = of_find_device_by_node(leds_node);
	if (!led_pdev) {
		err = -EINVAL;
		pr_err("Cannot convert node to platform device\n");
		goto error;
	}

	/* Now we search the actual led which is a child of the leds device */
	led_dev = device_find_child(&led_pdev->dev, led_node, match_led);
	if (!led_dev) {
		err = -EINVAL;
		pr_err("Cannot find led device\n");
		goto error;
	}

	/* From leds-class driver we can see that they put the led class
	 * device into driver_data of the device device. This is what we are
	 * looking for and allows us to control the state of the LED from kernel.
	 */
	data->led_cdev = (struct led_classdev*) led_dev->driver_data;
	if (!data->led_cdev) {
		err = -EINVAL;
		pr_err("Cannot get led class device\n");
		goto error;
	}

	/* Create the sysfs entry */
	err = device_create_file(&pdev->dev, &dev_attr_device_state);
	if (err) {
		pr_err("Could not create sysfs file: %d\n", err);
		goto error;
	}

	set_device_state(data, FIRST_STATE, sizeof(FIRST_STATE));

	return 0;

error:
	/* Return all nodes on error */
	of_node_put(led_node);
	of_node_put(leds_node);

	return err;
}

static int led_control_remove(struct platform_device *pdev)
{
	device_remove_file(&pdev->dev, &dev_attr_device_state);

	return 0;
}

static const struct of_device_id of_led_control_match[] = {
	{ .compatible = "led-control", },
	{},
};

static struct platform_driver gpio_led_driver = {
	.probe		= led_control_probe,
	.remove		= led_control_remove,
	.driver		= {
		.name	= "led-control",
		.of_match_table = of_led_control_match,
	},
};

module_platform_driver(gpio_led_driver);

MODULE_AUTHOR("Stefan Eichenberger <stefan@embear.ch>");
MODULE_DESCRIPTION("LED control driver");
MODULE_LICENSE("GPL");
