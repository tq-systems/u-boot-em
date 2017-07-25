/*
 * EM300 (Energymanager 300)
 * Copyright (C) 2017, TQ Systems
 * Christoph Krutz <christoph.krutz@tq-group.com>
 *
 * Based on Freescale mx28evk.c:
 * (C) Copyright 2011 Freescale Semiconductor, Inc.
 * Author: Fabio Estevam <fabio.estevam@freescale.com>
 *
 * Based on m28evk.c:
 * Copyright (C) 2011 Marek Vasut <marek.vasut@gmail.com>
 * on behalf of DENX Software Engineering GmbH
 *
 * SPDX-License-Identifier:    GPL-2.0+
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
static uint16_t tqma28l_emmc_dsr = 0x0100;

#define SYSTEM_REV_OFFSET    0x8
#define HW3REV0100 0x100
#define HW3REV0200 0x200
#define HW3REV0300 0x300
#define HW3REV0400 0x400

#define MUX_CONFIG_REV (MXS_PAD_3V3 | MXS_PAD_4MA | MXS_PAD_NOPULL)
#define MUX_CONFIG_LED (MXS_PAD_3V3 | MXS_PAD_4MA | MXS_PAD_NOPULL)

#define GPIO_PHY_RESET MX28_PAD_LCD_D00__GPIO_1_0

/* LEDs on HW3 Rev 0100 */
static const iomux_cfg_t leds_hw0100_pads[] = {
	MX28_PAD_GPMI_RDY0__GPIO_0_20 | MUX_CONFIG_LED,
	MX28_PAD_GPMI_RDN__GPIO_0_24 | MUX_CONFIG_LED,
	MX28_PAD_GPMI_ALE__GPIO_0_26 | MUX_CONFIG_LED,
};

/* LEDs on HW3 Rev 0200 and later */
static const iomux_cfg_t leds_hw0200_pads[] = {
	MX28_PAD_LCD_D18__GPIO_1_18 | MUX_CONFIG_LED,
	MX28_PAD_LCD_D19__GPIO_1_19 | MUX_CONFIG_LED,
	MX28_PAD_LCD_D20__GPIO_1_20 | MUX_CONFIG_LED,
	MX28_PAD_LCD_D21__GPIO_1_21 | MUX_CONFIG_LED,
	MX28_PAD_LCD_D22__GPIO_1_22 | MUX_CONFIG_LED,
	MX28_PAD_LCD_D23__GPIO_1_23 | MUX_CONFIG_LED,
};

static const iomux_cfg_t revision_pads[] = {
	MX28_PAD_LCD_D05__GPIO_1_5 | MUX_CONFIG_REV,
	MX28_PAD_LCD_D06__GPIO_1_6 | MUX_CONFIG_REV,
	MX28_PAD_LCD_D07__GPIO_1_7 | MUX_CONFIG_REV,
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
	/* SSP1 clock at 96MHz */
	mxs_set_sspclk(MXC_SSPCLK2, 96000, 0);

	/* Setup pads for board revision detection */
	mxs_iomux_setup_multiple_pads(
		revision_pads, ARRAY_SIZE(revision_pads));

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

void board_rev_init(void)
{
	/*
	 * REV0100^: all pins floating               -> 111 -> 000 -> 0x0100
	 * REV0200^: floating, pull-down, floating   -> 101 -> 010 -> 0x0200
	 * REV0300:  pull-up, pull-down, pull-down   -> 100 -> 011 -> 0x0300
	 * REV0400:  pull-down, pull-up, pull-up     -> 011 -> 100 -> 0x0400
	 * unusable: pull-down, pull-down, pull-down -> 000 -> 111 -> 0x0700
	 *
	 * ^ Sampling the pins might be unreliable due to missing pull-ups/pull-downs.
	 *   In this case the board revision is typically detected as
	 *   000 -> 111 -> 0x0700 so simply assume that it is REV0200 for real
	 *   as newer board revisions should not suffer from floating pins anymore.
	 */
	system_rev =
		((!gpio_get_value(MX28_PAD_LCD_D05__GPIO_1_5)) << 2) |
		((!gpio_get_value(MX28_PAD_LCD_D06__GPIO_1_6)) << 1) |
		( !gpio_get_value(MX28_PAD_LCD_D07__GPIO_1_7));

	if (system_rev == 0)
		system_rev = 1;
	if (system_rev == 7)
		system_rev = 2;

	system_rev <<= SYSTEM_REV_OFFSET;
}

u32 get_board_rev(void)
{
	return system_rev;
}

void board_leds_init(void)
{
	/* Disable LEDs after reset ... */
	switch (system_rev) {
	case HW3REV0100:
		mxs_iomux_setup_multiple_pads(
			leds_hw0100_pads, ARRAY_SIZE(leds_hw0100_pads));
		gpio_direction_output(MX28_PAD_GPMI_RDY0__GPIO_0_20, 0);
		gpio_direction_output(MX28_PAD_GPMI_RDN__GPIO_0_24, 0);
		gpio_direction_output(MX28_PAD_GPMI_ALE__GPIO_0_26, 0);
		break;
	case HW3REV0200:
	case HW3REV0300:
	case HW3REV0400:
	default:
		mxs_iomux_setup_multiple_pads(
			leds_hw0200_pads, ARRAY_SIZE(leds_hw0200_pads));
		gpio_direction_output(MX28_PAD_LCD_D18__GPIO_1_18, 0);
		gpio_direction_output(MX28_PAD_LCD_D19__GPIO_1_19, 0);
		gpio_direction_output(MX28_PAD_LCD_D20__GPIO_1_20, 0);
		gpio_direction_output(MX28_PAD_LCD_D21__GPIO_1_21, 0);
		gpio_direction_output(MX28_PAD_LCD_D22__GPIO_1_22, 0);
		gpio_direction_output(MX28_PAD_LCD_D23__GPIO_1_23, 0);
		break;
	}

	/* ... but switch on one to indicate a running bootloader */
	switch (system_rev) {
	case HW3REV0100:
		gpio_set_value(MX28_PAD_GPMI_ALE__GPIO_0_26, 1);
		break;
	case HW3REV0200:
	case HW3REV0300:
	case HW3REV0400:
	default:
		gpio_set_value(MX28_PAD_LCD_D18__GPIO_1_18, 1);
		gpio_set_value(MX28_PAD_LCD_D19__GPIO_1_19, 1);
		break;
	}
}

#ifdef	CONFIG_CMD_MMC
int em300_emmc_cd(int id)
{
	return 1;
}

int board_mmc_init(bd_t *bis)
{
	struct mmc *mmc;
	int ret;

	ret = mxsmmc_initialize(bis, 1, NULL, em300_emmc_cd);

	mmc = find_mmc_device(CONFIG_SYS_MMC_ENV_DEV);
	if (mmc) {
		mmc->block_dev.removable = 0;
		mmc_set_dsr(mmc, tqma28l_emmc_dsr);
	}

	return ret;
}
#endif

#ifdef	CONFIG_CMD_NET
int mxs_eth_enable_clock_out(void)
{
	printf("mxs_eth_enable_clock_out\n");
	if (system_rev >= HW3REV0300)
		return 0;

	return 1;
}

int board_eth_init(bd_t *bis)
{
	struct mxs_clkctrl_regs *clkctrl_regs =
		(struct mxs_clkctrl_regs *)MXS_CLKCTRL_BASE;
	struct eth_device *dev;
	int ret;

	ret = cpu_eth_init(bis);

	/* select source for IEEE 1588 timer */
	setbits_le32(&clkctrl_regs->hw_clkctrl_enet, CLKCTRL_ENET_TIME_SEL_RMII_CLK);

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
		puts("FEC MXS: Unable to init FEC0\n");
		return ret;
	}

	dev = eth_get_dev_by_name("FEC0");
	if (!dev) {
		puts("FEC MXS: Unable to get FEC0 device entry\n");
		return -EINVAL;
	}

	return ret;
}

#endif

int misc_init_r(void)
{
	char buffer[16];
	char *s = getenv("serial");

	puts("Board: EM300\n");

	if (s && s[0]) {
		puts("Serial: ");
		puts(s);
		putc('\n');
	}

	sprintf(buffer, "%d", tqma28l_emmc_dsr);
	setenv("tq_dsr", buffer);

	return 0;
}

int board_late_init(void)
{
	board_rev_init();
	board_leds_init();

	printf("Revision: %04x\n", system_rev);

	return 0;
}
