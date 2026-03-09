// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2023 - 2024 TQ-Systems GmbH <u-boot@ew.tq-group.com>,
 * D-82229 Seefeld, Germany.
 * Author: Michael Krummsdorf
 */

#include <asm/gpio.h>
#include <asm/io.h>
#include <asm/arch/imx-regs.h>
#include <asm/arch/iomux-mx28.h>
#include <asm/arch/gpio.h>
#include <asm/arch/clock.h>
#include <asm/arch/sys_proto.h>
#include <env.h>
#include <linux/mii.h>
#include <fdt_support.h>
#include <miiphy.h>
#include <netdev.h>
#include <sysinfo.h>
#include <errno.h>
#include <mmc.h>
#include <eth_phy.h>
#include <linux/stringify.h>
#include "../common/tq_rtc.h"

DECLARE_GLOBAL_DATA_PTR;

static int em_em310_board_model(const char *path, size_t size, char *val)
{
	struct udevice *dev;
	int ret;

	ret = uclass_get_device_by_of_path(UCLASS_SYSINFO, path, &dev);
	if (ret)
		return ret;

	ret = sysinfo_detect(dev);
	if (ret)
		return ret;

	return sysinfo_get_str(dev, SYSID_BOARD_MODEL, size, val);
}

void print_hw_info(void)
{
	char verbuf[30] = "unknown", revbuf[8] = "unknown";

	em_em310_board_model("/sysinfo-ver", sizeof(verbuf), verbuf);
	em_em310_board_model("/sysinfo-rev", sizeof(revbuf), revbuf);

	printf("HW:    %s | REV%s\n", verbuf, revbuf);
};

/*
 * Functions
 */
int board_early_init_f(void)
{
	/* IO0 clock at 480MHz */
	mxs_set_ioclk(MXC_IOCLK0, 480000);

	/* SSP0 clock at 96MHz */
	mxs_set_sspclk(MXC_SSPCLK0, 96000, 0);
	/* SSP3 clock at 96MHz */
	mxs_set_sspclk(MXC_SSPCLK3, 96000, 0);

	return 0;
}

int dram_init(void)
{
	return mxs_dram_init();
}

int board_init(void)
{
	struct mxs_clkctrl_regs *clkctrl_regs =
		(struct mxs_clkctrl_regs *)MXS_CLKCTRL_BASE;
	int ret;

	/* Address of boot parameters */
	gd->bd->bi_boot_params = PHYS_SDRAM_1 + 0x100;

	ret = cpu_eth_init(NULL);
	if (ret)
		return ret;

	/* Disable pad output - external clock */
	/* select source for IEEE 1588 timer */
	clrsetbits_le32(&clkctrl_regs->hw_clkctrl_enet,
			CLKCTRL_ENET_TIME_SEL_MASK | CLKCTRL_ENET_CLK_OUT_EN,
			CLKCTRL_ENET_TIME_SEL_RMII_CLK);

	print_hw_info();

	return 0;
}

int board_phy_config(struct phy_device *phydev)
{
	int ret;

	/* Make sure MDIO bus is probed */
	dm_mdio_probe_devices();

	if (!phydev->bus)
		phydev->bus = miiphy_get_dev_by_name("mdio");

	if (phydev->drv->config)
		phydev->drv->config(phydev);

	if (!phydev->bus) {
		printf("mdio bus not found\n");
		return 0;
	}

	/* Set KSZ8863 driver strength to 8mA */
	ret = mdio_gpio_write_smi(phydev->bus, 0x0E, 0x07);
	if (ret) {
		printf("FEC MXS: Unable to set KSZ8863 driver strength\n");
		return ret;
	}

	/* Change KSZ8863 to use internal source for RMII clock */
	ret = mdio_gpio_write_smi(phydev->bus, 0xC6, 0x0B);
	if (ret) {
		printf("FEC MXS: Unable to change KSZ8863 RMII clock settings\n");
		return ret;
	}

	return 0;
}

#ifdef CONFIG_OF_BOARD_SETUP
static int em310_set_revision(void *blob)
{
	struct udevice *dev;
	int hw_rev;
	int ret;

	ret = uclass_get_device_by_of_path(UCLASS_SYSINFO, "/sysinfo-rev", &dev);
	if (ret)
		return ret;

	ret = sysinfo_detect(dev);
	if (ret)
		return ret;

	sysinfo_get_int(dev, SYSID_BOARD_MODEL, &hw_rev);
	do_fixup_by_path_u32(blob, "/", "tq,revision", hw_rev, 1);

	return 0;
}

static int em310_set_serial(void *blob)
{
	const char *serial = env_get("serial");

	if (!serial)
		return -ENOENT;

	do_fixup_by_path_string(blob, "/", "tq,serial-number", serial);

	return 0;
}

int ft_board_setup(void *blob, struct bd_info *bd)
{
	int ret;

	ret = em310_set_revision(blob);
	if (ret)
		printf("Failed to set board revision in FDT: %d\n", ret);

	ret = em310_set_serial(blob);
	if (ret)
		printf("Failed to set serial number in FDT: %d\n", ret);

	return 0;
}
#endif

int board_late_init(void)
{
	const char *serial = env_get("serial");

	if (serial)
		printf("Serial: %s\n", serial);

	/* set quartz load to 12500 femtofarads */
	tq_pcf85063_init_capacity(0, 0x51, 12500);
	tq_pcf85063_init_clkout(0, 0x51, TQ_PCF85063_CLKOUT_OFF);
	tq_pcf85063_init_offset(0, 0x51, true, 5);

	return 0;
}
