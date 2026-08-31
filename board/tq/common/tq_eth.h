// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2026 TQ-Systems GmbH <license@tq-group.com>, D-82229 Seefeld, Germany. All rights reserved.
 * Author: Michael Krummsdorf
 */

#ifndef __TQ_ETH_H__
#define __TQ_ETH_H__

#include <miiphy.h>

struct phy_device;

void tq_eth_setup_fec(void);
int tq_eth_probe_mdio_bus(struct phy_device *phydev);
int tq_eth_configure_micrel_switch(struct phy_device *phydev);

#endif /* __TQ_ETH_H__ */
