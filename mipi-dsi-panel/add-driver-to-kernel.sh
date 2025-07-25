#!/bin/sh

# Expects the kernel directory as the first argument
if [ -z "$1" ]; then
    echo "Usage: $0 <kernel_directory>"
    exit 1
fi
KERNEL_DIR="$1"
if [ ! -d "$KERNEL_DIR" ]; then
    echo "Error: Directory $KERNEL_DIR does not exist."
    exit 1
fi

# Check if panel-mipi-dsi.c is already present
if [ -f "$KERNEL_DIR/drivers/gpu/drm/panel/panel-mipi-dsi.c" ]; then
    echo "panel-mipi-dsi.c already exists in $KERNEL_DIR/drivers/gpu/drm/panel/"
    echo "Do you want to overwrite it? (y/n)"
    read -r answer
    if [ "$answer" != "y" ]; then
	echo "Aborting."
	exit 0
    else
	echo "Overwriting panel-mipi-dsi.c"
    fi
fi
cp panel-mipi-dsi.c "$KERNEL_DIR/drivers/gpu/drm/panel/"

# Add it to the Makefile
MAKEFILE="$KERNEL_DIR/drivers/gpu/drm/panel/Makefile"
if grep -q "panel-mipi-dsi.o" "$MAKEFILE"; then
    echo "panel-mipi-dsi.o is already in $MAKEFILE"
else
    echo "Adding panel-mipi-dsi.o to $MAKEFILE"
    echo "obj-\$(CONFIG_DRM_PANEL_MIPI_DSI) += panel-mipi-dsi.o" >> "$MAKEFILE"
fi
# Add it to the Kconfig
KCONFIG="$KERNEL_DIR/drivers/gpu/drm/panel/Kconfig"
if grep -q "config DRM_PANEL_MIPI_DSI" "$KCONFIG"; then
    echo "config DRM_PANEL_MIPI_DSI is already in $KCONFIG"
else
    echo "Adding config DRM_PANEL_MIPI_DSI to $KCONFIG"
    # delete the endmenu line if it exists
    sed -i '/^endmenu/d' "$KCONFIG"
    echo "" >> "$KCONFIG"
    echo "config DRM_PANEL_MIPI_DSI" >> "$KCONFIG"
    echo "	tristate \"MIPI DSI panel support\"" >> "$KCONFIG"
    echo "	depends on DRM_MIPI_DSI" >> "$KCONFIG"
    echo "	default n" >> "$KCONFIG"
    echo "	help" >> "$KCONFIG"
    echo "	  Support for MIPI DSI panels." >> "$KCONFIG"
    echo "endmenu" >> "$KCONFIG"
fi
