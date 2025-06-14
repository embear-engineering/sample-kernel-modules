// SPDX-License-Identifier: GPL-2.0+
#define DEBUG
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

static void ts070wsh02ce_init_sequence(struct mipi_dsi_device *dsi)
{
	MIPI_DSI_SEQ(dsi, 0x80, 0x8B);
	MIPI_DSI_SEQ(dsi, 0x81, 0x78);
	MIPI_DSI_SEQ(dsi, 0x82, 0x84);
	MIPI_DSI_SEQ(dsi, 0x83, 0x88);
	MIPI_DSI_SEQ(dsi, 0x84, 0xA8);
	MIPI_DSI_SEQ(dsi, 0x85, 0xE3);
	MIPI_DSI_SEQ(dsi, 0x86, 0x88);
}


static void wf70a8syahmngb_init_sequence(struct mipi_dsi_device *dsi)
{
	MIPI_DSI_SEQ(dsi, 0xB1, 0x30);
	msleep(5);

	MIPI_DSI_SEQ(dsi, 0x80, 0x5B);
	MIPI_DSI_SEQ(dsi, 0x81, 0x47);
	MIPI_DSI_SEQ(dsi, 0x82, 0x84);
	MIPI_DSI_SEQ(dsi, 0x83, 0x88);
	MIPI_DSI_SEQ(dsi, 0x84, 0x88);
	MIPI_DSI_SEQ(dsi, 0x85, 0x23);
	MIPI_DSI_SEQ(dsi, 0x86, 0xB6);
}

static void wf40eswaa6_init_sequence(struct mipi_dsi_device *dsi)
{
	MIPI_DSI_SEQ(dsi, 0x11);
	msleep(120);

	MIPI_DSI_SEQ(dsi, 0xFF, 0x77, 0x01, 0x00, 0x00, 0x10);
	msleep(5);
	MIPI_DSI_SEQ(dsi, 0xC0, 0x3B, 0x00);
	msleep(5);
	MIPI_DSI_SEQ(dsi, 0xC1, 0x0D, 0x02);
	msleep(5);
	MIPI_DSI_SEQ(dsi, 0xC2, 0x30, 0x05);
	msleep(5);
	MIPI_DSI_SEQ(dsi, 0xB0, 0x01, 0x08, 0x10, 0x0C, 0x10, 0x06, 0x07, 0x08, 0x07, 0x22, 0x04, 0x14, 0x12, 0xB3, 0x3A, 0x1F);
	msleep(5);
	MIPI_DSI_SEQ(dsi, 0xB1, 0x13, 0x19, 0x1F, 0x0F, 0x14, 0x07, 0x07, 0x08, 0x07, 0x22, 0x02, 0x0F, 0x0F, 0xA3, 0x29, 0x0D);
	msleep(5);
	MIPI_DSI_SEQ(dsi, 0xFF, 0x77, 0x01, 0x00, 0x00, 0x11);
	msleep(5);
	MIPI_DSI_SEQ(dsi, 0xB0, 0x60);
	msleep(5);
	MIPI_DSI_SEQ(dsi, 0xB1, 0x2D);
	msleep(5);
	MIPI_DSI_SEQ(dsi, 0xB2, 0x07);
	msleep(5);
	MIPI_DSI_SEQ(dsi, 0xB3, 0x80);
	msleep(5);
	MIPI_DSI_SEQ(dsi, 0xB5, 0x49);
	msleep(5);
	MIPI_DSI_SEQ(dsi, 0xB7, 0x85);
	msleep(5);
	MIPI_DSI_SEQ(dsi, 0xB8, 0x21);
	msleep(5);
	MIPI_DSI_SEQ(dsi, 0xC1, 0x78);
	msleep(5);
	MIPI_DSI_SEQ(dsi, 0xC2, 0x78);
	msleep(100);
	MIPI_DSI_SEQ(dsi, 0xE0, 0x00, 0x28, 0x02);
	msleep(5);
	MIPI_DSI_SEQ(dsi, 0xE1, 0x08, 0xA0, 0x00, 0x00, 0x07, 0xA0, 0x00, 0x00, 0x00, 0x44, 0x44);
	msleep(5);
	MIPI_DSI_SEQ(dsi, 0xE2, 0x11, 0x11, 0x44, 0x44, 0xED, 0xA0, 0x00, 0x00, 0xEC, 0xA0, 0x00, 0x00);
	msleep(5);
	MIPI_DSI_SEQ(dsi, 0xE3, 0x00, 0x00, 0x11, 0x11);
	msleep(5);
	MIPI_DSI_SEQ(dsi, 0xE4, 0x44, 0x44);
	msleep(5);
	MIPI_DSI_SEQ(dsi, 0xE5, 0x0A, 0xE9, 0xD8, 0xA0, 0x0C, 0xEB, 0xD8, 0xA0, 0x0E, 0xED, 0xD8, 0xA0, 0x10, 0xEF, 0xD8, 0xA0);
	msleep(5);
	MIPI_DSI_SEQ(dsi, 0xE6, 0x00, 0x00, 0x11, 0x11);
	msleep(5);
	MIPI_DSI_SEQ(dsi, 0xE7, 0x44, 0x44);
	msleep(5);
	MIPI_DSI_SEQ(dsi, 0xE8, 0x09, 0xE8, 0xD8, 0xA0, 0x0B, 0xEA, 0xD8, 0xA0, 0x0D, 0xEC, 0xD8, 0xA0, 0x0F, 0xEE, 0xD8, 0xA0);
	msleep(5);
	MIPI_DSI_SEQ(dsi, 0xEB, 0x00, 0x00, 0xE4, 0xE4, 0x88, 0x00, 0x40);
	msleep(5);
	MIPI_DSI_SEQ(dsi, 0xEC, 0x3C, 0x00);
	msleep(5);
	MIPI_DSI_SEQ(dsi, 0xED, 0xAB, 0x89, 0x76, 0x54, 0x02, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x20, 0x45, 0x67, 0x98, 0xBA);
	msleep(5);

	MIPI_DSI_SEQ(dsi, 0x36, 0x00);
	MIPI_DSI_SEQ(dsi, 0x29);
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
	MIPI_DSI_SEQ(dsi_panel->dsi, MIPI_DCS_ENTER_SLEEP_MODE, 0x00);

	msleep(dsi_panel->sleep_delay);

	gpiod_set_value(dsi_panel->reset, 0);

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

static const struct drm_display_mode wf40eswaa6mnn0_mode = {
	.clock		= 30000,

	.hdisplay	= 480,
	.hsync_start	= 480 + 200, /* width + front porch */
	.hsync_end	= 480 + 200 + 200, /* width + front porch + hsync */
	.htotal		= 480 + 200 + 200 + 90, /* width + front porch + hsync + back porch */

	.vdisplay	= 480,
	.vsync_start	= 480 + 25,
	.vsync_end	= 480 + 25 + 20,
	.vtotal		= 480 + 25 + 20 + 25,

	.width_mm	= 69,
	.height_mm	= 69,

	.type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED,
	.flags = DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC,
};


static const struct drm_display_mode wf70a8syahmngb_mode = {
	.clock		= 50000,

	.hdisplay	= 1024,
	.hsync_start	= 1024 + 90,
	.hsync_end	= 1024 + 90 + 70,
	.htotal		= 1024 + 90 + 70 + 160,

	.vdisplay	= 600,
	.vsync_start	= 600 + 3,
	.vsync_end	= 600 + 3 + 20,
	.vtotal		= 600 + 3 + 20 + 12,

	.width_mm	= 154,
	.height_mm	= 86,

	.type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED,
	.flags = DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC,
};

static const struct drm_display_mode ts070wsh02ce_mode = {
	.clock		= 46000,

	.hdisplay	= 1024,
	.hsync_start	= 1024 + 90,
	.hsync_end	= 1024 + 90 + 10,
	.htotal		= 1024 + 90 + 10 + 80,

	.vdisplay	= 600,
	.vsync_start	= 600 + 16,
	.vsync_end	= 600 + 16 + 10,
	.vtotal		= 600 + 16 + 10 + 6,

	.width_mm	= 154,
	.height_mm	= 86,

	.type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED,
	.flags = DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC,
};

static const char * const ts8550b_supply_names[] = {
	"VCC",
	"IOVCC",
};

	//.flags = MIPI_DSI_MODE_VIDEO | MIPI_DSI_MODE_VIDEO_HSE | MIPI_DSI_MODE_VIDEO_BURST | MIPI_DSI_MODE_LPM | MIPI_DSI_CLOCK_NON_CONTINUOUS,
static const struct mipi_dsi_panel_panel_desc wf40eswaa6mnn0_desc = {
	.mode = &wf40eswaa6mnn0_mode,
	.lanes = 2,
	//.flags = MIPI_DSI_MODE_VIDEO | MIPI_DSI_MODE_VIDEO_HSE | MIPI_DSI_MODE_VIDEO_BURST,
	.flags = MIPI_DSI_MODE_VIDEO | MIPI_DSI_MODE_VIDEO_HSE | MIPI_DSI_MODE_VIDEO_BURST | MIPI_DSI_MODE_LPM | MIPI_DSI_CLOCK_NON_CONTINUOUS,
	.format = MIPI_DSI_FMT_RGB888,
	.supply_names = ts8550b_supply_names,
	.num_supplies = ARRAY_SIZE(ts8550b_supply_names),
	.panel_sleep_delay = 200,
	.init_sequence = wf40eswaa6_init_sequence,
};

static const struct mipi_dsi_panel_panel_desc wf70a8syahmngb_desc = {
	.mode = &wf70a8syahmngb_mode,
	.lanes = 4,
	.flags = MIPI_DSI_MODE_VIDEO | MIPI_DSI_MODE_VIDEO_HSE | MIPI_DSI_MODE_VIDEO_BURST,
	.format = MIPI_DSI_FMT_RGB888,
	.supply_names = ts8550b_supply_names,
	.num_supplies = ARRAY_SIZE(ts8550b_supply_names),
	.panel_sleep_delay = 200,
	.init_sequence = wf70a8syahmngb_init_sequence,
};

static const struct mipi_dsi_panel_panel_desc ts070wsh02ce_desc= {
	.mode = &ts070wsh02ce_mode,
	.lanes = 4,
	.flags = MIPI_DSI_MODE_VIDEO | MIPI_DSI_MODE_VIDEO_HSE | MIPI_DSI_MODE_VIDEO_BURST,
	.format = MIPI_DSI_FMT_RGB888,
	.supply_names = ts8550b_supply_names,
	.num_supplies = ARRAY_SIZE(ts8550b_supply_names),
	.panel_sleep_delay = 200,
	.init_sequence = ts070wsh02ce_init_sequence,
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
	{ .compatible = "winstar,wf40eswaa6mnn0", .data = &wf40eswaa6mnn0_desc},
	{ .compatible = "winstar,wf70a8syahmngb", .data = &wf70a8syahmngb_desc},
	{ .compatible = "tdo,ts070wsh02ce", .data = &ts070wsh02ce_desc},
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
MODULE_DESCRIPTION("MIPI DSI sample driver supporting various panels");
MODULE_LICENSE("GPL");
