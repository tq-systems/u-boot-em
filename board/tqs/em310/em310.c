/*
 * TQ Systems Energy Manager 310 platform
 * (C) Copyright 2015 TQ Systems GmbH
 * Jeremias Schneider <jeremias.schneider@tqs.de>
 *
 * Based on:
 *
 * (C) Copyright 2011 Freescale Semiconductor, Inc.
 *
 * Author: Fabio Estevam <fabio.estevam@freescale.com>
 *
 * Based on m28evk.c:
 * Copyright (C) 2011 Marek Vasut <marek.vasut@gmail.com>
 * on behalf of DENX Software Engineering GmbH
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <common.h>
#include <asm/gpio.h>
#include <asm/io.h>
#include <asm/arch/imx-regs.h>
#include <asm/arch/iomux-mx28.h>
#include <asm/imx-common/gpio.h>
#include <asm/arch/clock.h>
#include <asm/arch/sys_proto.h>
#include <linux/mii.h>
#include <miiphy.h>
#include <netdev.h>
#include <errno.h>
#include <mmc.h>

DECLARE_GLOBAL_DATA_PTR;

static u32 system_rev;
static u32 system_ver;

#define SYSTEM_REV_OFFSET 0x8
#define EM310REV0100      0x100
#define EM310REV0200      0x200

#define MUX_CONFIG_REV (MXS_PAD_3V3 | MXS_PAD_4MA | MXS_PAD_NOPULL)
#define MUX_CONFIG_VER (MXS_PAD_3V3 | MXS_PAD_4MA | MXS_PAD_NOPULL)
#define MUX_CONFIG_LED (MXS_PAD_3V3 | MXS_PAD_4MA | MXS_PAD_NOPULL)

#define GPIO_PHY_INT    MX28_PAD_GPMI_D06__GPIO_0_6
#define GPIO_PHY_RESET  MX28_PAD_GPMI_D07__GPIO_0_7
#define GPIO_PHY_ENABLE MX28_PAD_SAIF0_MCLK__GPIO_3_20

static const iomux_cfg_t leds_pads[] = {
	MX28_PAD_SSP2_SCK__GPIO_2_16  | MUX_CONFIG_LED,
	MX28_PAD_SSP2_MOSI__GPIO_2_17 | MUX_CONFIG_LED,
	MX28_PAD_SSP2_MISO__GPIO_2_18 | MUX_CONFIG_LED,
	MX28_PAD_SSP2_SS0__GPIO_2_19  | MUX_CONFIG_LED,
	MX28_PAD_SSP2_SS1__GPIO_2_20  | MUX_CONFIG_LED,
	MX28_PAD_SSP2_SS2__GPIO_2_21  | MUX_CONFIG_LED,
};

static const iomux_cfg_t revision_version_pads[] = {
	MX28_PAD_GPMI_D00__GPIO_0_0 | MUX_CONFIG_REV,
	MX28_PAD_GPMI_D01__GPIO_0_1 | MUX_CONFIG_REV,
	MX28_PAD_GPMI_D02__GPIO_0_2 | MUX_CONFIG_REV,
	MX28_PAD_GPMI_D03__GPIO_0_3 | MUX_CONFIG_VER,
	MX28_PAD_GPMI_D04__GPIO_0_4 | MUX_CONFIG_VER,
	MX28_PAD_GPMI_D05__GPIO_0_5 | MUX_CONFIG_VER,
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

	/* Setup pads for board revision detection */
	mxs_iomux_setup_multiple_pads(
		revision_version_pads, ARRAY_SIZE(revision_version_pads));

	return 0;
}

int dram_init(void)
{
	return mxs_dram_init();
}

int board_init(void)
{
	/* Adress of boot parameters */
	gd->bd->bi_boot_params = PHYS_SDRAM_1 + 0x100;

	return 0;
}

void board_rev_ver_init(void)
{
	/*
	 *          EM310_REV0 | EM310_REV1 | EM310_REV0
	 * REV0100:  pull-up   |  pull-up   |  pull-down -> 110 -> 001 -> 0x0100
	 * REV0200:  pull-up   |  pull-down |  pull-up   -> 101 -> 010 -> 0x0200
	 *
	 *          EM310_VER0 | EM310_VER1 | EM310_VER0
	 * L      :  pull-up   |  pull-up   |  pull-down -> 110 -> 001 -> 0x0001
	 * LL     :  pull-up   |  pull-down |  pull-up   -> 101 -> 010 -> 0x0002
	 * LR     :  pull-up   |  pull-down |  pull-down -> 100 -> 011 -> 0x0003
	 * LLR    :  pull-down |  pull-up   |  pull-up   -> 011 -> 100 -> 0x0004
	 * LLRR   :  pull-down |  pull-up   |  pull-down -> 010 -> 101 -> 0x0005
	 */

	system_rev =	 (!gpio_get_value(MX28_PAD_GPMI_D00__GPIO_0_0))       |
			((!gpio_get_value(MX28_PAD_GPMI_D01__GPIO_0_1)) << 1) |
			((!gpio_get_value(MX28_PAD_GPMI_D02__GPIO_0_2)) << 2);

	system_rev <<= SYSTEM_REV_OFFSET;

	system_ver =	 (!gpio_get_value(MX28_PAD_GPMI_D03__GPIO_0_3))       |
			((!gpio_get_value(MX28_PAD_GPMI_D04__GPIO_0_4)) << 1) |
			((!gpio_get_value(MX28_PAD_GPMI_D05__GPIO_0_5)) << 2);
}

u32 get_board_rev(void)
{
	return system_rev;
}

void board_leds_init(void)
{
	/* Disable LEDs after reset ... */

	mxs_iomux_setup_multiple_pads(
		leds_pads, ARRAY_SIZE(leds_pads));
	gpio_direction_output(MX28_PAD_SSP2_SCK__GPIO_2_16,  0);
	gpio_direction_output(MX28_PAD_SSP2_MOSI__GPIO_2_17, 0);
	gpio_direction_output(MX28_PAD_SSP2_MISO__GPIO_2_18, 0);
	gpio_direction_output(MX28_PAD_SSP2_SS0__GPIO_2_19,  0);
	gpio_direction_output(MX28_PAD_SSP2_SS1__GPIO_2_20,  0);
	gpio_direction_output(MX28_PAD_SSP2_SS2__GPIO_2_21,  0);

	/* ... but switch on one to indicate a running bootloader */

	gpio_set_value(MX28_PAD_SSP2_SCK__GPIO_2_16,  1);
	gpio_set_value(MX28_PAD_SSP2_MOSI__GPIO_2_17, 1);

}

#ifdef	CONFIG_CMD_MMC

static int em310_emmc_cd(int id)
{
	return 1;
}

int board_mmc_init(bd_t *bis)
{
	struct mmc *mmc;
	int ret;

	ret = mxsmmc_initialize(bis, 0, NULL, em310_emmc_cd);

	mmc = find_mmc_device(CONFIG_SYS_MMC_ENV_DEV);
	if (mmc) {
		mmc->block_dev.removable = 0;
	}

	return ret;
}

#endif

#ifdef	CONFIG_CMD_NET

int mxs_eth_enable_clock_out(void)
{
	return 0;
}

static int fecmxc_mii_postcall(int phy)
{
	struct eth_device *dev;
	int ret;

	dev = eth_get_dev_by_name("FEC0");
	if (!dev) {
		puts("FEC MXS: Unable to get FEC device entry\n");
		return -EINVAL;
	}

	/* Set KSZ8863 driver strength to 8mA */
	ret = fecmxc_smi_write(dev, 0x0E, 0xFA07);
	if(ret) {
		printf("FEC MXS: Unable to set KSZ8863 driver strength\n");
		return ret;
	}

	/* Change KSZ8863 RMII clock setting for no feedback to REFCLKI_3 */
	ret = fecmxc_smi_write(dev, 0xC6, 0x0007);
	if(ret) {
		printf("FEC MXS: Unable to change KSZ8863 RMII clock settings\n");
		return ret;
	}

	return 0;
}

int board_eth_init(bd_t *bis)
{
	struct mxs_clkctrl_regs *clkctrl_regs =
		(struct mxs_clkctrl_regs *)MXS_CLKCTRL_BASE;
	struct eth_device *dev;
	int ret;

	ret = cpu_eth_init(bis);
	if (ret)
		return ret;

	/* select source for IEEE 1588 timer */
	clrsetbits_le32(&clkctrl_regs->hw_clkctrl_enet,
		CLKCTRL_ENET_TIME_SEL_MASK | CLKCTRL_ENET_CLK_OUT_EN,
		CLKCTRL_ENET_TIME_SEL_RMII_CLK);

	gpio_set_value(GPIO_PHY_ENABLE, 0);
	gpio_direction_output(GPIO_PHY_ENABLE, 1);
	udelay(200);
	gpio_set_value(GPIO_PHY_ENABLE, 1);
	udelay(200);
	/*
	 * Reset PHY
	 * We first drive the pin HIGH before we switch to output. This should
	 * prevent a tiny glitch which could cause a short drop of the clock output
	 * of the PHY (PHY is running in REFCLKO mode).
	 */
	gpio_set_value(GPIO_PHY_RESET, 1);
	gpio_direction_output(GPIO_PHY_RESET, 1);
	udelay(50);
	gpio_set_value(GPIO_PHY_RESET, 0);
	udelay(200);
	gpio_set_value(GPIO_PHY_RESET, 1);

	/* give PHY some time to get out of the reset */
	udelay(10000);

	ret = fecmxc_initialize_multi(bis, 0, 0, MXS_ENET0_BASE);
	if (ret) {
		puts("FEC MXS: Unable to init FEC\n");
		return ret;
	}

	dev = eth_get_dev_by_name("FEC0");
	if (!dev) {
		puts("FEC MXS: Unable to get FEC device entry\n");
		return -EINVAL;
	}

	ret = fecmxc_register_mii_postcall(dev, fecmxc_mii_postcall);
	if (ret) {
		printf("FEC MXS: Unable to register FEC0 mii postcall\n");
		return ret;
	}

	return ret;
}

#endif

int misc_init_r(void)
{
	char *s = getenv("serial");

	puts("Board: Energy Manager 310\n");

	if (s && s[0]) {
		puts("Serial: ");
		puts(s);
		putc('\n');
	}

	return 0;
}

int board_late_init(void)
{
	board_rev_ver_init();
	board_leds_init();

	printf("Revision: %04x\n", system_rev);
	printf("Version: %04x\n", system_ver);

	return 0;
}
