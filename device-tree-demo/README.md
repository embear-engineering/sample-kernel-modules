# Device Tree Demo Driver

This driver shows how you can use the device tree to give some properties to a Linux kernel driver.

# How to use

Compile the driver as follows:
```
export ARCH=<arch>
export CROSS_COMPILE=<architecture prefix>
export KDIR=<kernel directory>
make
make dtb
```

The device tree overlay will only work with imx6q-apalis-eval.dts.

To toggle the LED you can write to /sys/devices/demo-driver/demo_gpio:
```
echo 0 > /sys/devices/demo-driver/demo_gpio
```
