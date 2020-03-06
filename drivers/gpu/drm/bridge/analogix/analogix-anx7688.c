// SPDX-License-Identifier: GPL-2.0-only
/*
 * ANX7688 HDMI->DP bridge driver
 *
 * Copyright 2020 Google LLC
 */

#include <linux/mfd/anx7688.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <drm/drm_bridge.h>
#include <drm/drm_print.h>

struct anx7688_bridge_data {
	struct drm_bridge bridge;
	struct regmap *regmap;

	bool filter;
};

static inline struct anx7688_bridge_data *
bridge_to_anx7688(struct drm_bridge *bridge)
{
	return container_of(bridge, struct anx7688_bridge_data, bridge);
}

static bool anx7688_bridge_mode_fixup(struct drm_bridge *bridge,
				      const struct drm_display_mode *mode,
				      struct drm_display_mode *adjusted_mode)
{
	struct anx7688_bridge_data *data = bridge_to_anx7688(bridge);
	int totalbw, requiredbw;
	u8 dpbw, lanecount;
	u8 regs[2];
	int ret;

	if (!data->filter)
		return true;

	/* Read both regs 0x85 (bandwidth) and 0x86 (lane count). */
	ret = regmap_bulk_read(data->regmap, ANX7688_DP_BANDWIDTH_REG, regs,
			       2);
	if (ret < 0) {
		DRM_ERROR("Failed to read bandwidth/lane count\n");
		return false;
	}
	dpbw = regs[0];
	lanecount = regs[1];

	/* Maximum 0x19 bandwidth (6.75 Gbps Turbo mode), 2 lanes */
	if (dpbw > 0x19 || lanecount > 2) {
		DRM_ERROR("Invalid bandwidth/lane count (%02x/%d)\n", dpbw,
			  lanecount);
		return false;
	}

	/* Compute available bandwidth (kHz) */
	totalbw = dpbw * lanecount * 270000 * 8 / 10;

	/* Required bandwidth (8 bpc, kHz) */
	requiredbw = mode->clock * 8 * 3;

	DRM_DEBUG_KMS("DP bandwidth: %d kHz (%02x/%d); mode requires %d Khz\n",
		      totalbw, dpbw, lanecount, requiredbw);

	if (totalbw == 0) {
		DRM_ERROR("Bandwidth/lane count are 0, not rejecting modes\n");
		return true;
	}

	return totalbw >= requiredbw;
}

static const struct drm_bridge_funcs anx7688_bridge_funcs = {
	.mode_fixup = anx7688_bridge_mode_fixup,
};

static int anx7688_bridge_probe(struct platform_device *pdev)
{
	struct anx7688 *anx7688 = dev_get_drvdata(pdev->dev.parent);
	struct anx7688_bridge_data *data;
	struct device *dev = &pdev->dev;

	data = devm_kzalloc(dev, sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	data->bridge.of_node = dev->of_node;
	data->regmap = anx7688->regmap;

	/* FW version >= 0.85 supports bandwidth/lane count registers */
	if (anx7688->fw_version >= ANX7688_MINIMUM_FW_VERSION)
		data->filter = true;
	else
		/* Warn, but not fail, for backwards compatibility */
		DRM_WARN("Old ANX7688 FW version (0x%04x), not filtering\n",
			 anx7688->fw_version);

	data->bridge.funcs = &anx7688_bridge_funcs;
	drm_bridge_add(&data->bridge);

	return 0;
}

static int anx7688_bridge_remove(struct platform_device *pdev)
{
	struct anx7688_bridge_data *data = dev_get_drvdata(&pdev->dev);

	drm_bridge_remove(&data->bridge);

	return 0;
}

static const struct of_device_id anx7688_bridge_match_table[] = {
	{ .compatible = "analogix,anx7688-bridge", },
	{ }
};
MODULE_DEVICE_TABLE(of, anx7688_bridge_match_table);

static struct platform_driver anx7688_bridge_driver = {
	.probe = anx7688_bridge_probe,
	.remove = anx7688_bridge_remove,
	.driver = {
		.name = "anx7688-bridge",
		.of_match_table = anx7688_bridge_match_table,
	},
};

module_platform_driver(anx7688_bridge_driver);

MODULE_DESCRIPTION("ANX7688 HDMI->DP bridge driver");
MODULE_AUTHOR("Nicolas Boichat <drinkcat@chromium.org>");
MODULE_AUTHOR("Enric Balletbo i Serra <enric.balletbo@collabora.com>");
MODULE_LICENSE("GPL");
