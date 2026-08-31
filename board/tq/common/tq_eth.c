// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2026 TQ-Systems GmbH <license@tq-group.com>, D-82229 Seefeld, Germany. All rights reserved.
 * Author: Michael Krummsdorf
 */

#include <asm/io.h>
#include <linux/bitops.h>
#include <linux/errno.h>
#include <miiphy.h>
#include <netdev.h>
#include <phy.h>
#include <stdio.h>

#include "tq_eth.h"

#if CONFIG_IS_ENABLED(ARCH_IMX8M)
/* Keep setup_fec() to revert behaviour by
 * 4bdc3524d7 ("net: fec_mxc: Add board_interface_eth_init() for i.MX8M Mini/Nano/Plus")
 * which set ENET_REF clk to internally generated (and output to pin) although
 * we have externally generated clock and need the pin to be input, which is not considered
 * with our phy-mode = "rmii"
 */
void tq_eth_setup_fec(void)
{
	struct iomuxc_gpr_base_regs *gpr =
		(struct iomuxc_gpr_base_regs *)IOMUXC_GPR_BASE_ADDR;

	/* Use external REF_CLK input for ENET1 actually */
	clrsetbits_le32(&gpr->gpr[1], BIT(13), 0);
}
#else
/* dummy implementation */
void tq_eth_setup_fec(void)
{
}
#endif

int tq_eth_configure_micrel_switch(struct phy_device *phydev)
{
	int ret;

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

int tq_eth_probe_mdio_bus(struct phy_device *phydev)
{
	/* Make sure MDIO bus is probed */
	dm_mdio_probe_devices();

	if (!phydev->bus)
		phydev->bus = miiphy_get_dev_by_name("mdio");

	if (!phydev->bus)
		return -ENODEV;

	if (phydev->drv->config)
		phydev->drv->config(phydev);

	return 0;
}
