// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2023 - 2026 TQ-Systems GmbH <license@tq-group.com>, D-82229 Seefeld, Germany. All rights reserved.
 * Author: Michael Krummsdorf
 */
#include <asm/io.h>
#include <asm/arch/clock.h>
#include <asm/arch/imx-regs.h>
#include <asm/arch/sys_proto.h>
#include <asm/global_data.h>
#include <env.h>
#include <netdev.h>
#include <stdio.h>

#include "../common/tq_bdi.h"
#include "../common/tq_eth.h"
#include "../common/tq_rtc.h"

DECLARE_GLOBAL_DATA_PTR;

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

	tq_bdi_print_hw_info();

	return 0;
}

int board_phy_config(struct phy_device *phydev)
{
	int ret;

	ret = tq_eth_probe_mdio_bus(phydev);
	if (ret)
		return ret;

	ret = tq_eth_configure_micrel_switch(phydev);
	if (ret)
		return ret;

	return 0;
}

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
