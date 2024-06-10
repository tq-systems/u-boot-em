// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2023 - 2024 TQ-Systems GmbH <u-boot@ew.tq-group.com>,
 * D-82229 Seefeld, Germany.
 * Author: Michael Krummsdorf
 */

#include <common.h>
#include <asm/gpio.h>
#include <asm/io.h>
#include <asm/arch/imx-regs.h>
#include <asm/arch/iomux-mx28.h>
#include <asm/arch/gpio.h>
#include <asm/arch/clock.h>
#include <asm/arch/sys_proto.h>
#include <linux/mii.h>
#include <fdt_support.h>
#include <miiphy.h>
#include <netdev.h>
#include <errno.h>
#include <mmc.h>
#include <eth_phy.h>
#include <linux/stringify.h>
#include "../common/tq_board_gpio.h"
#include "../common/tq_rtc.h"

DECLARE_GLOBAL_DATA_PTR;


/*
 *          EM310_REV2 | EM310_REV1 | EM310_REV0
 * REV0100:  pull-up   |  pull-up   |  pull-down -> 110 -> 0x0100
 * REV0200:  pull-up   |  pull-down |  pull-up   -> 101 -> 0x0200
 *
 *          EM310_VER2 | EM310_VER1 | EM310_VER0
 * L      :  pull-up   |  pull-up   |  pull-down -> 110 -> 0x0001
 * LL     :  pull-up   |  pull-down |  pull-up   -> 101 -> 0x0002
 * LR     :  pull-up   |  pull-down |  pull-down -> 100 -> 0x0003
 * LLR    :  pull-down |  pull-up   |  pull-up   -> 011 -> 0x0004
 * LLRR   :  pull-down |  pull-up   |  pull-down -> 010 -> 0x0005
 */
static const unsigned int hw_rev_tbl[] = {
	0x0700,		/* 000   PD PD PD */
	0x0600,		/* 001   PD PD PU */
	0x0500,		/* 010   PD PU PD */
	0x0400,		/* 011   PD PU PU */
	0x0300,		/* 100   PU PD PD */
	0x0200,		/* 101   PU PD PU */
	0x0100,		/* 110   PU PU PD */
	0xffff,		/* 111   PU PU PU */
};

static const char *const hw_ver_tbl[] = {
	"UNKNOWN",	/* 000   PD PD PD */
	"UNKNOWN",	/* 001   PD PD PU */
	"EM310-LLRR",	/* 010   PD PU PD */
	"EM310-LLR",	/* 011   PD PU PU */
	"EM310-LR",	/* 100   PU PD PD */
	"EM310-LL",	/* 101   PU PD PU */
	"EM310-L",	/* 110   PU PU PD */
	"UNKNOWN",	/* 111   PU PU PU */
};

enum {
	HW_REV0,
	HW_REV1,
	HW_REV2,
	HW_VER0,
	HW_VER1,
	HW_VER2
};

static struct tq_gpio_init_data em310_gid[] = {
	GPIO_INIT_DATA_ENTRY(HW_REV2, "GPIO0_2", GPIOD_IS_IN),
	GPIO_INIT_DATA_ENTRY(HW_REV1, "GPIO0_1", GPIOD_IS_IN),
	GPIO_INIT_DATA_ENTRY(HW_REV0, "GPIO0_0", GPIOD_IS_IN),

	GPIO_INIT_DATA_ENTRY(HW_VER2, "GPIO0_5", GPIOD_IS_IN),
	GPIO_INIT_DATA_ENTRY(HW_VER1, "GPIO0_4", GPIOD_IS_IN),
	GPIO_INIT_DATA_ENTRY(HW_VER0, "GPIO0_3", GPIOD_IS_IN),
};

void print_hw_info(void)
{
	unsigned int hw_rev = tq_board_gpio_data(em310_gid, HW_REV0, HW_REV2);
	unsigned int hw_ver = tq_board_gpio_data(em310_gid, HW_VER0, HW_VER2);

	printf("HW:    %s | REV%04x\n", hw_ver_tbl[hw_ver], hw_rev_tbl[hw_rev]);
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

	/* Adress of boot parameters */
	gd->bd->bi_boot_params = PHYS_SDRAM_1 + 0x100;

	ret = cpu_eth_init(NULL);
	if (ret)
		return ret;

	/* Disable pad output - external clock */
	/* select source for IEEE 1588 timer */
	clrsetbits_le32(&clkctrl_regs->hw_clkctrl_enet,
		CLKCTRL_ENET_TIME_SEL_MASK | CLKCTRL_ENET_CLK_OUT_EN,
		CLKCTRL_ENET_TIME_SEL_RMII_CLK);

	tq_board_gpio_init(em310_gid, ARRAY_SIZE(em310_gid));
	print_hw_info();

	return 0;
}

int board_phy_config(struct phy_device *phydev)
{
	int ret;

	if (!phydev->bus)
		phydev->bus = miiphy_get_dev_by_name("FEC0");

	if (phydev->drv->config)
		phydev->drv->config(phydev);

	/* Set KSZ8863 driver strength to 8mA */
	ret = fec_smi_write(phydev, 0x0E, 0x07);
	if (ret) {
		printf("FEC MXS: Unable to set KSZ8863 driver strength\n");
		return ret;
	}

	/* Change KSZ8863 to use internal source for RMII clock */
	ret = fec_smi_write(phydev, 0xC6, 0x0B);
	if (ret) {
		printf("FEC MXS: Unable to change KSZ8863 RMII clock settings\n");
		return ret;
	}

	return 0;
}

#ifdef CONFIG_OF_BOARD_SETUP
static int em310_set_revision(void *blob)
{
	unsigned int hw_rev;

	hw_rev = tq_board_gpio_data(em310_gid, HW_REV0, HW_REV2);

	do_fixup_by_path_u32(blob, "/", "tq,revision", hw_rev_tbl[hw_rev], 1);

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

int misc_init_r(void)
{
	char *s = env_get("serial");

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
	/* power-on reset on rtc is sometimes incomplete and leave corrupted registers */
	tq_pcf85063_clear_reg(0, 0x51, 0x00);
	tq_pcf85063_clear_reg(0, 0x51, 0x01);
	tq_pcf85063_clear_reg(0, 0x51, 0x02);

	/* set quartz load to 12500 femtofarads */
	tq_pcf85063_adjust_capacity(0, 0x51, 12500);
	tq_pcf85063_set_clkout(0, 0x51, TQ_PCF85063_CLKOUT_OFF);
	tq_pcf85063_set_offset(0, 0x51, true, 5);

	return 0;
}
