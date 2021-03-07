# LED control module

This module shows how you can use the parent child relation in a kernel driver to gain access to an LED. You can find a description of the driver at [embear.ch](https://embear.ch/blog/using-parent-child-relations).

Please note that you should better implement a led-trigger driver if you really want to access an LED in a kernel module. This driver just showcases how you can use the parent child relation.

# How to use

To use the driver you need to create a device tree node for the led-control driver:
```
/ {
    leds {
            compatible = "gpio-leds";

            pwr_led: pwr {
                    label = "pwr";
                    gpios = <&expgpio 2 GPIO_ACTIVE_LOW>;
            };
    };

    led-control {
            compatible = "led-control";
            led = <&pwr_led>;
    };
};
```
led: is a phandle to a LED (e.g. a gpio-led).
