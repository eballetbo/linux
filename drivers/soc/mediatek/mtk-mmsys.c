// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2014 MediaTek Inc.
 * Author: James Liao <jamesjj.liao@mediatek.com>
 */

#include <linux/device.h>
#include <linux/io.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/soc/mediatek/mtk-mmsys.h>

#define DISP_REG_CONFIG_DISP_OVL0_MOUT_EN	0x040
#define DISP_REG_CONFIG_DISP_OVL1_MOUT_EN	0x044
#define DISP_REG_CONFIG_DISP_OD_MOUT_EN		0x048
#define DISP_REG_CONFIG_DISP_GAMMA_MOUT_EN	0x04c
#define DISP_REG_CONFIG_DISP_UFOE_MOUT_EN	0x050
#define DISP_REG_CONFIG_DISP_COLOR0_SEL_IN	0x084
#define DISP_REG_CONFIG_DISP_COLOR1_SEL_IN	0x088
#define DISP_REG_CONFIG_DSIE_SEL_IN		0x0a4
#define DISP_REG_CONFIG_DSIO_SEL_IN		0x0a8
#define DISP_REG_CONFIG_DPI_SEL_IN		0x0ac
#define DISP_REG_CONFIG_DISP_RDMA2_SOUT		0x0b8
#define DISP_REG_CONFIG_DISP_RDMA0_SOUT_EN	0x0c4
#define DISP_REG_CONFIG_DISP_RDMA1_SOUT_EN	0x0c8
#define DISP_REG_CONFIG_MMSYS_CG_CON0		0x100

#define DISP_REG_CONFIG_DISP_OVL_MOUT_EN	0x030
#define DISP_REG_CONFIG_OUT_SEL			0x04c
#define DISP_REG_CONFIG_DSI_SEL			0x050
#define DISP_REG_CONFIG_DPI_SEL			0x064

#define OVL0_MOUT_EN_COLOR0			0x1
#define OD_MOUT_EN_RDMA0			0x1
#define OD1_MOUT_EN_RDMA1			BIT(16)
#define UFOE_MOUT_EN_DSI0			0x1
#define COLOR0_SEL_IN_OVL0			0x1
#define OVL1_MOUT_EN_COLOR1			0x1
#define GAMMA_MOUT_EN_RDMA1			0x1
#define RDMA0_SOUT_DPI0				0x2
#define RDMA0_SOUT_DPI1				0x3
#define RDMA0_SOUT_DSI1				0x1
#define RDMA0_SOUT_DSI2				0x4
#define RDMA0_SOUT_DSI3				0x5
#define RDMA0_SOUT_MASK				0x7
#define RDMA1_SOUT_DPI0				0x2
#define RDMA1_SOUT_DPI1				0x3
#define RDMA1_SOUT_DSI1				0x1
#define RDMA1_SOUT_DSI2				0x4
#define RDMA1_SOUT_DSI3				0x5
#define RDMA1_SOUT_MASK				0x7
#define RDMA2_SOUT_DPI0				0x2
#define RDMA2_SOUT_DPI1				0x3
#define RDMA2_SOUT_DSI1				0x1
#define RDMA2_SOUT_DSI2				0x4
#define RDMA2_SOUT_DSI3				0x5
#define RDMA2_SOUT_MASK				0x7
#define DPI0_SEL_IN_RDMA1			0x1
#define DPI0_SEL_IN_RDMA2			0x3
#define DPI0_SEL_IN_MASK			0x3
#define DPI1_SEL_IN_RDMA1			(0x1 << 8)
#define DPI1_SEL_IN_RDMA2			(0x3 << 8)
#define DPI1_SEL_IN_MASK			(0x3 << 8)
#define DSI0_SEL_IN_RDMA1			0x1
#define DSI0_SEL_IN_RDMA2			0x4
#define DSI0_SEL_IN_MASK			0x7
#define DSI1_SEL_IN_RDMA1			0x1
#define DSI1_SEL_IN_RDMA2			0x4
#define DSI1_SEL_IN_MASK			0x7
#define DSI2_SEL_IN_RDMA1			(0x1 << 16)
#define DSI2_SEL_IN_RDMA2			(0x4 << 16)
#define DSI2_SEL_IN_MASK			(0x7 << 16)
#define DSI3_SEL_IN_RDMA1			(0x1 << 16)
#define DSI3_SEL_IN_RDMA2			(0x4 << 16)
#define DSI3_SEL_IN_MASK			(0x7 << 16)
#define COLOR1_SEL_IN_OVL1			0x1

#define OVL_MOUT_EN_RDMA			0x1
#define BLS_TO_DSI_RDMA1_TO_DPI1		0x8
#define BLS_TO_DPI_RDMA1_TO_DSI			0x2
#define BLS_RDMA1_DSI_DPI_MASK			0xf
#define DSI_SEL_IN_BLS				0x0
#define DPI_SEL_IN_BLS				0x0
#define DPI_SEL_IN_MASK				0x1
#define DSI_SEL_IN_RDMA				0x1
#define DSI_SEL_IN_MASK				0x1

struct mtk_mmsys_routes {
	u32 from_comp;
	u32 to_comp;
	u32 addr;
	u32 mask;
	u32 val;
};

struct mtk_mmsys_driver_data {
	const char *clk_driver;
	const struct mtk_mmsys_routes *routes;
	const unsigned int num_routes;
};

static const struct mtk_mmsys_driver_data mt2701_mmsys_driver_data = {
	.clk_driver = "clk-mt2701-mm",
};

static const struct mtk_mmsys_driver_data mt2712_mmsys_driver_data = {
	.clk_driver = "clk-mt2712-mm",
};

static const struct mtk_mmsys_driver_data mt6779_mmsys_driver_data = {
	.clk_driver = "clk-mt6779-mm",
};

static const struct mtk_mmsys_driver_data mt6797_mmsys_driver_data = {
	.clk_driver = "clk-mt6797-mm",
};

static const struct mtk_mmsys_driver_data mt8183_mmsys_driver_data = {
	.clk_driver = "clk-mt8183-mm",
};

struct mtk_mmsys {
	void __iomem *regs;
	const struct mtk_mmsys_driver_data *data;
};

static const struct mtk_mmsys_routes mt8173_mmsys_routing_table[] = {
	{
		DDP_COMPONENT_BLS, DDP_COMPONENT_DSI0,
		DISP_REG_CONFIG_OUT_SEL,
		BLS_RDMA1_DSI_DPI_MASK, BLS_TO_DSI_RDMA1_TO_DPI1
	}, {
		DDP_COMPONENT_BLS, DDP_COMPONENT_DSI0,
		DISP_REG_CONFIG_DSI_SEL,
		DSI_SEL_IN_MASK, DSI_SEL_IN_BLS
	}, {
		DDP_COMPONENT_BLS, DDP_COMPONENT_DPI0,
		DISP_REG_CONFIG_OUT_SEL,
		BLS_RDMA1_DSI_DPI_MASK, BLS_TO_DPI_RDMA1_TO_DSI
	}, {
		DDP_COMPONENT_BLS, DDP_COMPONENT_DPI0,
		DISP_REG_CONFIG_DSI_SEL,
		DSI_SEL_IN_MASK, DSI_SEL_IN_RDMA
	}, {
		DDP_COMPONENT_BLS, DDP_COMPONENT_DPI0,
		DISP_REG_CONFIG_DPI_SEL,
		DPI_SEL_IN_MASK, DPI_SEL_IN_BLS
	}, {
		DDP_COMPONENT_GAMMA, DDP_COMPONENT_RDMA1,
		DISP_REG_CONFIG_DISP_GAMMA_MOUT_EN,
		GAMMA_MOUT_EN_RDMA1, GAMMA_MOUT_EN_RDMA1
	}, {
		DDP_COMPONENT_OD0, DDP_COMPONENT_RDMA0,
		DISP_REG_CONFIG_DISP_OD_MOUT_EN,
		OD_MOUT_EN_RDMA0, OD_MOUT_EN_RDMA0
	}, {
		DDP_COMPONENT_OD1, DDP_COMPONENT_RDMA1,
		DISP_REG_CONFIG_DISP_OD_MOUT_EN,
		OD1_MOUT_EN_RDMA1, OD1_MOUT_EN_RDMA1
	}, {
		DDP_COMPONENT_OVL0, DDP_COMPONENT_COLOR0,
		DISP_REG_CONFIG_DISP_OVL0_MOUT_EN,
		OVL0_MOUT_EN_COLOR0, OVL0_MOUT_EN_COLOR0
	}, {
		DDP_COMPONENT_OVL0, DDP_COMPONENT_COLOR0,
		DISP_REG_CONFIG_DISP_COLOR0_SEL_IN,
		COLOR0_SEL_IN_OVL0, COLOR0_SEL_IN_OVL0
	}, {
		DDP_COMPONENT_OVL0, DDP_COMPONENT_RDMA0,
		DISP_REG_CONFIG_DISP_OVL_MOUT_EN,
		OVL_MOUT_EN_RDMA, OVL_MOUT_EN_RDMA
	}, {
		DDP_COMPONENT_OVL1, DDP_COMPONENT_COLOR1,
		DISP_REG_CONFIG_DISP_OVL1_MOUT_EN,
		OVL1_MOUT_EN_COLOR1, OVL1_MOUT_EN_COLOR1
	}, {
		DDP_COMPONENT_OVL1, DDP_COMPONENT_COLOR1,
		DISP_REG_CONFIG_DISP_COLOR1_SEL_IN,
		COLOR1_SEL_IN_OVL1, COLOR1_SEL_IN_OVL1
	}, {
		DDP_COMPONENT_RDMA0, DDP_COMPONENT_DPI0,
		DISP_REG_CONFIG_DISP_RDMA0_SOUT_EN,
		RDMA0_SOUT_MASK, RDMA0_SOUT_DPI0
	}, {
		DDP_COMPONENT_RDMA0, DDP_COMPONENT_DPI1,
		DISP_REG_CONFIG_DISP_RDMA0_SOUT_EN,
		RDMA0_SOUT_MASK, RDMA0_SOUT_DPI1
	}, {
		DDP_COMPONENT_RDMA0, DDP_COMPONENT_DSI1,
		DISP_REG_CONFIG_DISP_RDMA0_SOUT_EN,
		RDMA0_SOUT_MASK, RDMA0_SOUT_DSI1
	}, {
		DDP_COMPONENT_RDMA0, DDP_COMPONENT_DSI2,
		DISP_REG_CONFIG_DISP_RDMA0_SOUT_EN,
		RDMA0_SOUT_MASK, RDMA0_SOUT_DSI2
	}, {
		DDP_COMPONENT_RDMA0, DDP_COMPONENT_DSI3,
		DISP_REG_CONFIG_DISP_RDMA0_SOUT_EN,
		RDMA0_SOUT_MASK, RDMA0_SOUT_DSI3
	}, {
		DDP_COMPONENT_RDMA1, DDP_COMPONENT_DPI0,
		DISP_REG_CONFIG_DISP_RDMA1_SOUT_EN,
		RDMA1_SOUT_MASK, RDMA1_SOUT_DPI0
	}, {
		DDP_COMPONENT_RDMA1, DDP_COMPONENT_DPI0,
		DISP_REG_CONFIG_DPI_SEL_IN,
		DPI0_SEL_IN_MASK, DPI0_SEL_IN_RDMA1
	}, {
		DDP_COMPONENT_RDMA1, DDP_COMPONENT_DPI1,
		DISP_REG_CONFIG_DISP_RDMA1_SOUT_EN,
		RDMA1_SOUT_MASK, RDMA1_SOUT_DPI1
	}, {
		DDP_COMPONENT_RDMA1, DDP_COMPONENT_DPI1,
		DISP_REG_CONFIG_DPI_SEL_IN,
		DPI1_SEL_IN_MASK, DPI1_SEL_IN_RDMA1
	}, {
		DDP_COMPONENT_RDMA1, DDP_COMPONENT_DSI0,
		DISP_REG_CONFIG_DSIE_SEL_IN,
		DSI0_SEL_IN_MASK, DSI0_SEL_IN_RDMA1
	}, {
		DDP_COMPONENT_RDMA1, DDP_COMPONENT_DSI1,
		DISP_REG_CONFIG_DISP_RDMA1_SOUT_EN,
		RDMA1_SOUT_MASK, RDMA1_SOUT_DSI1
	}, {
		DDP_COMPONENT_RDMA1, DDP_COMPONENT_DSI1,
		DISP_REG_CONFIG_DSIO_SEL_IN,
		DSI1_SEL_IN_MASK, DSI1_SEL_IN_RDMA1
	}, {
		DDP_COMPONENT_RDMA1, DDP_COMPONENT_DSI2,
		DISP_REG_CONFIG_DISP_RDMA1_SOUT_EN,
		RDMA1_SOUT_MASK, RDMA1_SOUT_DSI2
	}, {
		DDP_COMPONENT_RDMA1, DDP_COMPONENT_DSI2,
		DISP_REG_CONFIG_DSIE_SEL_IN,
		DSI2_SEL_IN_MASK, DSI2_SEL_IN_RDMA1
	}, {
		DDP_COMPONENT_RDMA1, DDP_COMPONENT_DSI3,
		DISP_REG_CONFIG_DISP_RDMA1_SOUT_EN,
		RDMA1_SOUT_MASK, RDMA1_SOUT_DSI3
	}, {
		DDP_COMPONENT_RDMA1, DDP_COMPONENT_DSI3,
		DISP_REG_CONFIG_DSIO_SEL_IN,
		DSI3_SEL_IN_MASK, DSI3_SEL_IN_RDMA1
	}, {
		DDP_COMPONENT_RDMA2, DDP_COMPONENT_DPI0,
		DISP_REG_CONFIG_DISP_RDMA2_SOUT,
		RDMA2_SOUT_MASK, RDMA2_SOUT_DPI0
	}, {
		DDP_COMPONENT_RDMA2, DDP_COMPONENT_DPI0,
		DISP_REG_CONFIG_DPI_SEL_IN,
		DPI0_SEL_IN_MASK, DPI0_SEL_IN_RDMA2
	}, {
		DDP_COMPONENT_RDMA2, DDP_COMPONENT_DPI1,
		DISP_REG_CONFIG_DISP_RDMA2_SOUT,
		RDMA2_SOUT_MASK, RDMA2_SOUT_DPI1
	}, {
		DDP_COMPONENT_RDMA2, DDP_COMPONENT_DPI1,
		DISP_REG_CONFIG_DPI_SEL_IN,
		DPI1_SEL_IN_MASK, DPI1_SEL_IN_RDMA2
	}, {
		DDP_COMPONENT_RDMA2, DDP_COMPONENT_DSI0,
		DISP_REG_CONFIG_DSIE_SEL_IN,
		DSI0_SEL_IN_MASK, DSI0_SEL_IN_RDMA2
	}, {
		DDP_COMPONENT_RDMA2, DDP_COMPONENT_DSI1,
		DISP_REG_CONFIG_DISP_RDMA2_SOUT,
		RDMA2_SOUT_MASK, RDMA2_SOUT_DSI1
	}, {
		DDP_COMPONENT_RDMA2, DDP_COMPONENT_DSI1,
		DISP_REG_CONFIG_DSIO_SEL_IN,
		DSI1_SEL_IN_MASK, DSI1_SEL_IN_RDMA2
	}, {
		DDP_COMPONENT_RDMA2, DDP_COMPONENT_DSI2,
		DISP_REG_CONFIG_DISP_RDMA2_SOUT,
		RDMA2_SOUT_MASK, RDMA2_SOUT_DSI2
	}, {
		DDP_COMPONENT_RDMA2, DDP_COMPONENT_DSI2,
		DISP_REG_CONFIG_DSIE_SEL_IN,
		DSI2_SEL_IN_MASK, DSI2_SEL_IN_RDMA2
	}, {
		DDP_COMPONENT_RDMA2, DDP_COMPONENT_DSI3,
		DISP_REG_CONFIG_DISP_RDMA2_SOUT,
		RDMA2_SOUT_MASK, RDMA2_SOUT_DSI3
	}, {
		DDP_COMPONENT_RDMA2, DDP_COMPONENT_DSI3,
		DISP_REG_CONFIG_DSIO_SEL_IN,
		DSI3_SEL_IN_MASK, DSI3_SEL_IN_RDMA2
	}
};

static const struct mtk_mmsys_driver_data mt8173_mmsys_driver_data = {
	.clk_driver = "clk-mt8173-mm",
	.routes = mt8173_mmsys_routing_table,
	.num_routes = ARRAY_SIZE(mt8173_mmsys_routing_table),
};

void mtk_mmsys_ddp_connect(struct device *dev,
			   enum mtk_ddp_comp_id cur,
			   enum mtk_ddp_comp_id next)
{
	struct mtk_mmsys *mmsys = dev_get_drvdata(dev);
	const struct mtk_mmsys_routes *routes = mmsys->data->routes;
	u32 reg;
	int i;

	for (i = 0; i < mmsys->data->num_routes; i++)
		if (cur == routes[i].from_comp && next == routes[i].to_comp) {
			reg = readl(mmsys->regs + routes[i].addr);
			reg &= ~routes[i].mask;
			reg |= routes[i].val;
			writel(reg, mmsys->regs + routes[i].addr);
		}
}
EXPORT_SYMBOL_GPL(mtk_mmsys_ddp_connect);

void mtk_mmsys_ddp_disconnect(struct device *dev,
			      enum mtk_ddp_comp_id cur,
			      enum mtk_ddp_comp_id next)
{
	struct mtk_mmsys *mmsys = dev_get_drvdata(dev);
	const struct mtk_mmsys_routes *routes = mmsys->data->routes;
	u32 reg;
	int i;

	for (i = 0; i < mmsys->data->num_routes; i++)
		if (cur == routes[i].from_comp && next == routes[i].to_comp) {
			reg = readl(mmsys->regs + routes[i].addr);
			reg &= ~routes[i].mask;
			writel(reg, mmsys->regs + routes[i].addr);
		}
}
EXPORT_SYMBOL_GPL(mtk_mmsys_ddp_disconnect);

static int mtk_mmsys_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct platform_device *clks;
	struct platform_device *drm;
	struct mtk_mmsys *mmsys;
	int ret;

	mmsys = devm_kzalloc(dev, sizeof(*mmsys), GFP_KERNEL);
	if (!mmsys)
		return -ENOMEM;

	mmsys->regs = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(mmsys->regs)) {
		ret = PTR_ERR(mmsys->regs);
		dev_err(dev, "Failed to ioremap mmsys registers: %d\n", ret);
		return ret;
	}

	mmsys->data = of_device_get_match_data(&pdev->dev);
	platform_set_drvdata(pdev, mmsys);

	clks = platform_device_register_data(&pdev->dev, mmsys->data->clk_driver,
					     PLATFORM_DEVID_AUTO, NULL, 0);
	if (IS_ERR(clks))
		return PTR_ERR(clks);

	drm = platform_device_register_data(&pdev->dev, "mediatek-drm",
					    PLATFORM_DEVID_AUTO, NULL, 0);
	if (IS_ERR(drm)) {
		platform_device_unregister(clks);
		return PTR_ERR(drm);
	}

	return 0;
}

static const struct of_device_id of_match_mtk_mmsys[] = {
	{
		.compatible = "mediatek,mt2701-mmsys",
		.data = &mt2701_mmsys_driver_data,
	},
	{
		.compatible = "mediatek,mt2712-mmsys",
		.data = &mt2712_mmsys_driver_data,
	},
	{
		.compatible = "mediatek,mt6779-mmsys",
		.data = &mt6779_mmsys_driver_data,
	},
	{
		.compatible = "mediatek,mt6797-mmsys",
		.data = &mt6797_mmsys_driver_data,
	},
	{
		.compatible = "mediatek,mt8173-mmsys",
		.data = &mt8173_mmsys_driver_data,
	},
	{
		.compatible = "mediatek,mt8183-mmsys",
		.data = &mt8183_mmsys_driver_data,
	},
	{ }
};

static struct platform_driver mtk_mmsys_drv = {
	.driver = {
		.name = "mtk-mmsys",
		.of_match_table = of_match_mtk_mmsys,
	},
	.probe = mtk_mmsys_probe,
};

builtin_platform_driver(mtk_mmsys_drv);
