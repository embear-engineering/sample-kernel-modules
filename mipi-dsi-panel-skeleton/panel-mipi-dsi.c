// SPDX-License-Identifier: GPL-2.0+
#include <drm/drm_mipi_dsi.h>
#include <drm/drm_modes.h>
#include <drm/drm_panel.h>
#include <drm/drm_print.h>
#include <drm/drm_crtc.h>

#include <linux/backlight.h>
#include <linux/gpio/consumer.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/regulator/consumer.h>

#include <video/mipi_display.h>

struct mipi_dsi_panel_panel_desc {
	const struct drm_display_mode *mode;
	unsigned int lanes;
	unsigned long flags;
	enum mipi_dsi_pixel_format format;
	const char *const *supply_names;
	unsigned int num_supplies;
	unsigned int panel_sleep_delay;

	void (*init_sequence)(struct mipi_dsi_device *dev);
};

struct mipi_dsi_panel {
	struct drm_panel panel;
	struct mipi_dsi_device *dsi;
	const struct mipi_dsi_panel_panel_desc *desc;

	struct backlight_device *backlight;
	struct regulator_bulk_data *supplies;
	struct gpio_desc *reset;
	unsigned int sleep_delay;
};

static inline struct mipi_dsi_panel *panel_to_dsi_panel(struct drm_panel *panel)
{
	return container_of(panel, struct mipi_dsi_panel, panel);
}

static inline int mipi_dsi_panel_dsi_write(struct mipi_dsi_device *dsi, const void *seq,
				   size_t len)
{
	return mipi_dsi_dcs_write_buffer(dsi, seq, len);
}

#define MIPI_DSI_SEQ(dsi, seq...)				\
	{							\
		const u8 d[] = { seq };				\
		mipi_dsi_panel_dsi_write(dsi, d, ARRAY_SIZE(d));	\
	}

static void template_init_sequence(struct mipi_dsi_device *dsi)
{
	/* Add the initisequence of teh panel here */
}


static int mipi_dsi_panel_prepare(struct drm_panel *panel)
{
	struct mipi_dsi_panel *dsi_panel = panel_to_dsi_panel(panel);
	int ret;

	pr_debug("%s - %s:%d\n", __func__, __FILE__, __LINE__);

	gpiod_set_value(dsi_panel->reset, 0);

	ret = regulator_bulk_enable(dsi_panel->desc->num_supplies,
				    dsi_panel->supplies);
	if (ret < 0)
		return ret;
	msleep(20);

	gpiod_set_value(dsi_panel->reset, 1);
	msleep(150);

	dsi_panel->desc->init_sequence(dsi_panel->dsi);

	MIPI_DSI_SEQ(dsi_panel->dsi, MIPI_DCS_EXIT_SLEEP_MODE, 0x00);
	pr_debug("%s - %s:%d\n", __func__, __FILE__, __LINE__);

	return 0;
}

static int mipi_dsi_panel_enable(struct drm_panel *panel)
{
	struct mipi_dsi_panel *dsi_panel = panel_to_dsi_panel(panel);


	MIPI_DSI_SEQ(dsi_panel->dsi, MIPI_DCS_SET_DISPLAY_ON, 0x00);
	backlight_enable(dsi_panel->backlight);

	return 0;
}

static int mipi_dsi_panel_disable(struct drm_panel *panel)
{
	struct mipi_dsi_panel *dsi_panel = panel_to_dsi_panel(panel);

	pr_debug("%s - %s:%d\n", __func__, __FILE__, __LINE__);

	backlight_disable(dsi_panel->backlight);

	MIPI_DSI_SEQ(dsi_panel->dsi, MIPI_DCS_SET_DISPLAY_OFF, 0x00);

	pr_debug("%s - %s:%d\n", __func__, __FILE__, __LINE__);

	return 0;
}

static int mipi_dsi_panel_unprepare(struct drm_panel *panel)
{
	struct mipi_dsi_panel *dsi_panel = panel_to_dsi_panel(panel);

	pr_debug("%s - %s:%d\n", __func__, __FILE__, __LINE__);

	MIPI_DSI_SEQ(dsi_panel->dsi, MIPI_DCS_ENTER_SLEEP_MODE, 0x00);

	msleep(dsi_panel->sleep_delay);

	gpiod_set_value(dsi_panel->reset, 0);

	regulator_bulk_disable(dsi_panel->desc->num_supplies, dsi_panel->supplies);

	pr_debug("%s - %s:%d\n", __func__, __FILE__, __LINE__);


	return 0;
}

static int mipi_dsi_panel_get_modes(struct drm_panel *panel,
		struct drm_connector *connector)
{
	struct mipi_dsi_panel *dsi_panel = panel_to_dsi_panel(panel);
	const struct drm_display_mode *desc_mode = dsi_panel->desc->mode;
	struct drm_display_mode *mode;

	mode = drm_mode_duplicate(connector->dev, desc_mode);
	if (!mode) {
		DRM_DEV_ERROR(&dsi_panel->dsi->dev,
			      "failed to add mode %ux%ux@%u\n",
			      desc_mode->hdisplay, desc_mode->vdisplay,
			      drm_mode_vrefresh(desc_mode));
		return -ENOMEM;
	}

	drm_mode_set_name(mode);
	drm_mode_probed_add(connector, mode);

	connector->display_info.width_mm = desc_mode->width_mm;
	connector->display_info.height_mm = desc_mode->height_mm;

	return 1;
}

static const struct drm_panel_funcs mipi_dsi_panel_funcs = {
	.disable	= mipi_dsi_panel_disable,
	.unprepare	= mipi_dsi_panel_unprepare,
	.prepare	= mipi_dsi_panel_prepare,
	.enable		= mipi_dsi_panel_enable,
	.get_modes	= mipi_dsi_panel_get_modes,
};

static const struct drm_display_mode template_mode = {
	/* Add the settings from the panel datasheet here */
	.clock		= 18306,

	.hdisplay	= 480,
	.hsync_start	= 480 + 2,
	.hsync_end	= 480 + 2 + 45,
	.htotal		= 480 + 2 + 45  + 13,

	.vdisplay	= 480,
	.vsync_start	= 480 + 2,
	.vsync_end	= 480 + 2 + 70,
	.vtotal		= 480 + 2 + 70 + 13,

	.width_mm	= 72,
	.height_mm	= 70,

	.flags		= DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC,

	.type		= DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED,
};

static const char * const template_supply_names[] = {
	"VCC",
	"IOVCC",
};

static const struct mipi_dsi_panel_panel_desc template_desc = {
	.mode = &template_mode,
	.lanes = 2,
	.flags = MIPI_DSI_MODE_VIDEO | MIPI_DSI_MODE_VIDEO_HSE | MIPI_DSI_MODE_VIDEO_BURST | MIPI_DSI_MODE_LPM | MIPI_DSI_CLOCK_NON_CONTINUOUS,
	.format = MIPI_DSI_FMT_RGB888,
	.supply_names = template_supply_names,
	.num_supplies = ARRAY_SIZE(template_supply_names),
	.panel_sleep_delay = 200,
	.init_sequence = template_init_sequence,
};

static int mipi_dsi_panel_probe(struct mipi_dsi_device *dsi)
{
	const struct mipi_dsi_panel_panel_desc *desc;
	struct mipi_dsi_panel *dsi_panel;
	int ret, i;

	pr_debug("%s - %s:%d\n", __func__, __FILE__, __LINE__);
	dsi_panel = devm_kzalloc(&dsi->dev, sizeof(*dsi_panel), GFP_KERNEL);
	if (!dsi_panel)
		return -ENOMEM;

	pr_debug("%s - %s:%d\n", __func__, __FILE__, __LINE__);
	desc = of_device_get_match_data(&dsi->dev);
	dsi->mode_flags = desc->flags;
	dsi->format = desc->format;
	dsi->lanes = desc->lanes;

	dsi_panel->supplies = devm_kcalloc(&dsi->dev, desc->num_supplies,
					sizeof(*dsi_panel->supplies),
					GFP_KERNEL);
	if (!dsi_panel->supplies)
		return -ENOMEM;

	pr_debug("%s - %s:%d\n", __func__, __FILE__, __LINE__);
	for (i = 0; i < desc->num_supplies; i++)
		dsi_panel->supplies[i].supply = desc->supply_names[i];

	ret = devm_regulator_bulk_get(&dsi->dev, desc->num_supplies,
				      dsi_panel->supplies);
	if (ret < 0)
		return ret;

	pr_debug("%s - %s:%d\n", __func__, __FILE__, __LINE__);
	dsi_panel->reset = devm_gpiod_get(&dsi->dev, "reset", GPIOD_OUT_LOW);
	if (IS_ERR(dsi_panel->reset)) {
		DRM_DEV_ERROR(&dsi->dev, "Couldn't get our reset GPIO\n");
		return PTR_ERR(dsi_panel->reset);
	}

	pr_debug("%s - %s:%d\n", __func__, __FILE__, __LINE__);
	dsi_panel->backlight = devm_of_find_backlight(&dsi->dev);
	if (IS_ERR(dsi_panel->backlight))
		return PTR_ERR(dsi_panel->backlight);

	pr_debug("%s - %s:%d\n", __func__, __FILE__, __LINE__);

	drm_panel_init(&dsi_panel->panel, &dsi->dev, &mipi_dsi_panel_funcs, DRM_MODE_CONNECTOR_DSI);

	pr_debug("%s - %s:%d\n", __func__, __FILE__, __LINE__);
	drm_panel_add(&dsi_panel->panel);

	pr_debug("%s - %s:%d\n", __func__, __FILE__, __LINE__);
	mipi_dsi_set_drvdata(dsi, dsi_panel);
	dsi_panel->dsi = dsi;
	dsi_panel->desc = desc;
	dsi_panel->sleep_delay = desc->panel_sleep_delay;

	return mipi_dsi_attach(dsi);
}

static void mipi_dsi_panel_remove(struct mipi_dsi_device *dsi)
{
	struct mipi_dsi_panel *dsi_panel = mipi_dsi_get_drvdata(dsi);

	mipi_dsi_detach(dsi);
	drm_panel_remove(&dsi_panel->panel);
}

static const struct of_device_id mipi_dsi_panel_of_match[] = {
	{ .compatible = "template", .data = &template_desc},
	{ }
};
MODULE_DEVICE_TABLE(of, mipi_dsi_panel_of_match);

static struct mipi_dsi_driver mipi_dsi_panel_dsi_driver = {
	.probe		= mipi_dsi_panel_probe,
	.remove		= mipi_dsi_panel_remove,
	.driver = {
		.name		= "mipi-dsi-panel",
		.of_match_table	= mipi_dsi_panel_of_match,
	},
};
module_mipi_dsi_driver(mipi_dsi_panel_dsi_driver);

MODULE_AUTHOR("Stefan Eichenberger <stefan@embear.ch>");
MODULE_DESCRIPTION("MIPI DSI skeleton driver");
MODULE_LICENSE("GPL");
