// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2023 - 2026 TQ-Systems GmbH <license@tq-group.com>, D-82229 Seefeld, Germany. All rights reserved.
 * Author: Michael Krummsdorf
 */

#include <asm/arch/sys_proto.h>
#include <asm/mach-imx/boot_mode.h>
#include <dm/uclass.h>
#include <env.h>
#include <led.h>
#include <linux/mdio.h>
#include <linux/types.h>
#include <miiphy.h>
#include <phy.h>
#include <string.h>
#include <sysinfo.h>

#include "../common/tq_bdi.h"
#include "../common/tq_eth.h"
#include "../common/tq_rtc.h"

static bool em4xx_lan_detected;

int board_init(void)
{
	struct udevice *status_led;

	if (led_get_by_label("energymanager:red:status", &status_led) == 0)
		led_set_state(status_led, LEDST_ON);

	tq_bdi_print_hw_info();

	return 0;
}

static bool has_usb(void)
{
	struct udevice *dev;
	int hw_ver;
	int ret;

	ret = uclass_get_device_by_of_path(UCLASS_SYSINFO, "/sysinfo-ver", &dev);
	if (ret)
		return ret;

	ret = sysinfo_detect(dev);
	if (ret)
		return ret;

	sysinfo_get_int(dev, SYSID_BOARD_MODEL, &hw_ver);

	return hw_ver & 0x8;
}

static void adjust_env(void)
{
	enum boot_device bt_dev = get_boot_device();

	/* MACHINE imx8mn-egw is a TARGET_IMX8MN_EM4XX but with a different DEFAULT_FDT_FILE config.
	 * So, in case of a different config, use it for fdtfile.
	 *
	 * When DEFAULT_FDT_FILE is the default for EM4XX (== USB), we are probably on a
	 * MACHINE imx8mn-em4xx. So, for fdtfile, evaluate the hardware variant (USB or LAN)
	 * via a designated GPIO.
	 *
	 * When USB variant is identified, fdtfile must also be set to EM4xx's DEFAULT_FDT_FILE
	 * (USB). When USB variant is *not* identified, we must be on one of two LAN variants.
	 * In that case fdtfile will be set to one of two LAN alternatives in board_phy_config()
	 * later.
	 */
	if ((strcmp(CONFIG_DEFAULT_FDT_FILE, "imx8mn-em4xx-u.dtb") != 0) || has_usb())
		env_set("fdt_file", CONFIG_DEFAULT_FDT_FILE);
	else
		em4xx_lan_detected = true;

	/* disable autoboot in serial download mode*/
	if (bt_dev == USB_BOOT)
		env_set("bootdelay", "-1");
}

#define MICREL_PHY_ID 0x00221430
#define MARVELL_SWITCH_ID 0x0205
int board_phy_config(struct phy_device *phydev)
{
	int ret;
	u32 id;
	char *fdtfile = NULL;

	/* No switch in USB variant */
	if (has_usb())
		return 0;

	tq_eth_setup_fec();

	ret = tq_eth_probe_mdio_bus(phydev);
	if (ret)
		return ret;

	/* Identify switch chip as A or B */
	/* (A) Micrel KSZ8863 by reading PHY ID register, addr 0x3 reg 0x2..0x3 */
	ret = get_phy_id(phydev->bus, 0x3, MDIO_DEVAD_NONE, &id);
	if (!ret && id == MICREL_PHY_ID) {
		printf("Found Micrel PHY ID: %x\n", id);

		ret = tq_eth_configure_micrel_switch(phydev);
		if (ret)
			return ret;

		fdtfile = "imx8mn-em4xx-l-ksz8863.dtb";
	} else {
		/* (B) Marvell MV88E6020 by reading Switch ID register, addr 0x18 reg 0x3 */
		ret = phydev->bus->read(phydev->bus, 0x18, MDIO_DEVAD_NONE, 0x3);
		if (ret == MARVELL_SWITCH_ID) {
			printf("Found Marvell Switch ID: %04x\n", ret);
			fdtfile = "imx8mn-em4xx-l-mv88e6020.dtb";
		}
	}

	if (em4xx_lan_detected && fdtfile)
		env_set("fdt_file", fdtfile);

	return 0;
}

int board_late_init(void)
{
	adjust_env();

	/* set quartz load to 12500 femtofarads */
	tq_pcf85063_init_capacity(0, 0x51, 12500);
	tq_pcf85063_init_clkout(0, 0x51, TQ_PCF85063_CLKOUT_OFF);
	tq_pcf85063_init_offset(0, 0x51, true, 5);

	return 0;
}

int checkboard(void)
{
	tq_bdi_print_bootinfo();
	return 0;
}
