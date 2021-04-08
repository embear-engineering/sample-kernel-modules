# Reset reason module

This module provides a way to get the reason of why a device was rebooting. The information is made available under /sys/devices/platform/reset-reason/reset_reason. It can be read by doing a:
```
cat /sys/devices/platform/reset-reason/reset_reason
```

Possible values are "power-cycle", "reboot", "panic", "watchdog" or "unknown (e.g. voltag dip)".

# How to use

To use the driver you need to create a device tree node for the reset-reason driver:
```
/ {
	reserved-memory {
		#address-cells = <2>;
		#size-cells = <2>;
		ranges;

		reset_reason_mem: reset_reason@880100000 {
			#address-cells = <2>;
			#size-cells = <2>;
			no-map;
			reg = <0x00000008 0x80100000 0x0 0x1000>; // 4kB at 0x880100000
		};
	};

	reset-reason {
		compatible = "reset-reason";
		memory-region = <&reset_reason_mem>;
	};
};
```

It is necessary to apply the patch 0001-watchdog-pretimeout-add-an-atomic-notifier-governor.patch available in this directory to the kernel. It will add a new pretimeout governor. It will add an atomic notifier call chain list which is used to register a pretimeout callback. Make sure you enable the governor in the kernel configuration by setting the following configs:
```
CONFIG_WATCHDOG_PRETIMEOUT_GOV=y
CONFIG_WATCHDOG_PRETIMEOUT_GOV_NOOP=y
CONFIG_WATCHDOG_PRETIMEOUT_GOV_PANIC=y
CONFIG_WATCHDOG_PRETIMEOUT_GOV_NOTIFIER=y
CONFIG_WATCHDOG_PRETIMEOUT_DEFAULT_GOV_NOTIFIER=y
```
To check the proper installation of the governor under cat /sys/class/watchdog/watchdog0/pretimeout_governor (should be notifier) also enable the following config:
```
CONFIG_WATCHDOG_SYSFS=y
```

After that you can compile the module by following [compiling a kernel module](https://embear.ch/blog/compiling-a-kernel-module).
