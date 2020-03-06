// SPDX-License-Identifier: GPL-2.0-only
/*
 * HDMI to USB Type-C Bridge and Port Controller with MUX
 *
 * Copyright 2020 Google LLC
 */

#include <linux/i2c.h>
#include <linux/gpio/consumer.h>
#include <linux/mfd/anx7688.h>
#include <linux/module.h>
#include <linux/of_platform.h>
#include <linux/regulator/consumer.h>
#include <linux/regmap.h>

static const struct regmap_config anx7688_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
};

static int anx7688_i2c_probe(struct i2c_client *client)
{
	struct device *dev = &client->dev;
	struct anx7688 *anx7688;
	u16 vendor, device;
	u8 buffer[4];
	int ret;

	anx7688 = devm_kzalloc(dev, sizeof(*anx7688), GFP_KERNEL);
	if (!anx7688)
		return -ENOMEM;

	anx7688->client = client;
	i2c_set_clientdata(client, anx7688);

	anx7688->regmap = devm_regmap_init_i2c(client, &anx7688_regmap_config);

	/* Read both vendor and device id (4 bytes). */
	ret = regmap_bulk_read(anx7688->regmap, ANX7688_VENDOR_ID_REG,
			       buffer, 4);
	if (ret) {
		dev_err(dev, "Failed to read chip vendor/device id\n");
		return ret;
	}

	vendor = (u16)buffer[1] << 8 | buffer[0];
	device = (u16)buffer[3] << 8 | buffer[2];
	if (vendor != ANX7688_VENDOR_ID || device != ANX7688_DEVICE_ID) {
		dev_err(dev, "Invalid vendor/device id %04x/%04x\n",
			vendor, device);
		return -ENODEV;
	}

	ret = regmap_bulk_read(anx7688->regmap, ANX7688_FW_VERSION_REG,
			       buffer, 2);
	if (ret) {
		dev_err(&client->dev, "Failed to read firmware version\n");
		return ret;
	}

	anx7688->fw_version = (u16)buffer[0] << 8 | buffer[1];
	dev_info(dev, "ANX7688 firwmare version 0x%04x\n",
		 anx7688->fw_version);

	return devm_of_platform_populate(dev);
}

static const struct of_device_id anx7688_match_table[] = {
	{ .compatible = "analogix,anx7688", },
	{ }
};
MODULE_DEVICE_TABLE(of, anx7688_match_table);

static struct i2c_driver anx7688_driver = {
	.probe_new = anx7688_i2c_probe,
	.driver = {
		.name = "anx7688",
		.of_match_table = anx7688_match_table,
	},
};

module_i2c_driver(anx7688_driver);

MODULE_DESCRIPTION("HDMI to USB Type-C Bridge and Port Controller with MUX driver");
MODULE_AUTHOR("Nicolas Boichat <drinkcat@chromium.org>");
MODULE_AUTHOR("Enric Balletbo i Serra <enric.balletbo@collabora.com>");
MODULE_LICENSE("GPL");
