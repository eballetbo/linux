/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * HDMI to USB Type-C Bridge and Port Controller with MUX
 *
 * Copyright 2020 Google LLC
 */
#ifndef __LINUX_MFD_ANX7688_H
#define __LINUX_MFD_ANX7688_H

#include <linux/types.h>

/* Register addresses */
#define ANX7688_VENDOR_ID_REG		0x00
#define ANX7688_DEVICE_ID_REG		0x02

#define ANX7688_FW_VERSION_REG		0x80

#define ANX7688_DP_BANDWIDTH_REG	0x85
#define ANX7688_DP_LANE_COUNT_REG	0x86

#define ANX7688_VENDOR_ID		0x1f29
#define ANX7688_DEVICE_ID		0x7688

/* First supported firmware version (0.85) */
#define ANX7688_MINIMUM_FW_VERSION	0x0085

struct gpio_desc;
struct i2c_client;
struct regulator;
struct regmap;

struct anx7688 {
	struct i2c_client *client;
	struct regmap *regmap;

	u16 fw_version;
};

#endif /* __LINUX_MFD_ANX7688_H */
