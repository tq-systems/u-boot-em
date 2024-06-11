// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2023 - 2024 TQ-Systems GmbH <u-boot@ew.tq-group.com>,
 * D-82229 Seefeld, Germany.
 * Author: Michael Krummsdorf
 */

#include <common.h>
#include <dm.h>
#include <dm/device-internal.h>
#include <dm/lists.h>
#include <dm/uclass-internal.h>
#include <asm/arch/imx8mn_pins.h>
#include <malloc.h>
#include <errno.h>
#include <asm/io.h>
#include <asm/mach-imx/iomux-v3.h>
#include <asm-generic/gpio.h>
#include <env.h>
#include <fdt_support.h>
#include <fsl_esdhc.h>
#include <init.h>
#include <led.h>
#include <miiphy.h>
#include <mmc.h>
#include <netdev.h>
#include <phy.h>
#include <asm/arch/sys_proto.h>
#include <asm/mach-imx/boot_mode.h>
#include <asm/mach-imx/gpio.h>
#include <asm/mach-imx/mxc_i2c.h>
#include <asm/arch/clock.h>
#include <asm/arch/ddr.h>
#include <linux/stringify.h>

#include "../common/tq_board_gpio.h"
#include "../common/tq_rtc.h"

DECLARE_GLOBAL_DATA_PTR;

static bool em4xx_lan_detected;

static const unsigned int hw_rev_tbl[] = {
	0x0100,		/* "REV0100" */
	0x0101,		/* "REV0101" */
	0x0200,		/* "REV0200" */
	0x0201,
	0x0202,
	0x0203,
	0x0204,
	0x0205,
};

static const char *const hw_ver_tbl[] = {
	"EM4XX-CB-LLRR",
	"EM4XX-L-CB-LRR",
	"EM4XX-L-CB-LL",
	"UNKNOWN",
	"UNKNOWN",
	"UNKNOWN",
	"UNKNOWN",
	"UNKNOWN",
	"EM4XX-CB-ULRR",
	"EM4XX-CB-URR",
	"EM4XX-CB-LRR",
	"EM4XX-CB-UL",
	"EM4XX-CB-U",
	"EM4XX-CB-L",
	"UNKNOWN",
	"EEBUS-GW-L"
};

enum {
	HW_REV0,
	HW_REV1,
	HW_REV2,
	HW_VER0,
	HW_VER1,
	HW_VER2,
	HW_VER3,
	INT_5V_N,
	FACTORY_DFLT_N,
	USB_EN,
};

/* LED[1|2|3]_[GN|RD] registered via DT */
static struct tq_gpio_init_data em4xx_gid[] = {
	GPIO_INIT_DATA_ENTRY(HW_REV2, "GPIO4_30", GPIOD_IS_IN),
	GPIO_INIT_DATA_ENTRY(HW_REV1, "GPIO4_28", GPIOD_IS_IN),
	GPIO_INIT_DATA_ENTRY(HW_REV0, "GPIO4_27", GPIOD_IS_IN),

	GPIO_INIT_DATA_ENTRY(HW_VER3, "GPIO4_25", GPIOD_IS_IN),
	GPIO_INIT_DATA_ENTRY(HW_VER2, "GPIO4_24", GPIOD_IS_IN),
	GPIO_INIT_DATA_ENTRY(HW_VER1, "GPIO4_23", GPIOD_IS_IN),
	GPIO_INIT_DATA_ENTRY(HW_VER0, "GPIO4_22", GPIOD_IS_IN),

	GPIO_INIT_DATA_ENTRY(INT_5V_N, "GPIO5_0", GPIOD_IS_IN),
	GPIO_INIT_DATA_ENTRY(FACTORY_DFLT_N, "GPIO4_31", GPIOD_IS_IN),

	GPIO_INIT_DATA_ENTRY(USB_EN, "GPIO1_12", GPIOD_IS_OUT),
};

static void print_hw_info(void)
{
	unsigned int hw_rev = tq_board_gpio_data(em4xx_gid, HW_REV0, HW_REV2);
	unsigned int hw_ver = tq_board_gpio_data(em4xx_gid, HW_VER0, HW_VER3);

	printf("HW:    %s | REV%04x\n", hw_ver_tbl[hw_ver], hw_rev_tbl[hw_rev]);
};

#if defined(CONFIG_OF_BOARD_SETUP)
static int em4xx_set_revision(void *blob)
{
	unsigned int hw_rev;

	hw_rev = tq_board_gpio_data(em4xx_gid, HW_REV0, HW_REV2);

	do_fixup_by_path_u32(blob, "/", "tq,revision", hw_rev_tbl[hw_rev], 1);

	return 0;
}

static int em4xx_set_serial(void *blob)
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

	ret = em4xx_set_revision(blob);
	if (ret)
		printf("Failed to set board revision in FDT: %d\n", ret);

	ret = em4xx_set_serial(blob);
	if (ret)
		printf("Failed to set serial number in FDT: %d\n", ret);

	return 0;
}
#endif

/* Keep setup_fec() to revert behaviour by
 * 4bdc3524d7 ("net: fec_mxc: Add board_interface_eth_init() for i.MX8M Mini/Nano/Plus")
 * which set ENET_REF clk to internally generated (and output to pin) although
 * we have externally generated clock and need the pin to be input, which is not considered
 * with our phy-mode = "rmii"
 */
static void setup_fec(void)
{
	struct iomuxc_gpr_base_regs *gpr =
		(struct iomuxc_gpr_base_regs *)IOMUXC_GPR_BASE_ADDR;

	/* Use external REF_CLK input for ENET1 actually */
	clrsetbits_le32(&gpr->gpr[1], BIT(13), 0);
}

int board_init(void)
{
	struct udevice *status_led;

	if (led_get_by_label("energymanager:red:status", &status_led) == 0)
		led_set_state(status_led, LEDST_ON);

	tq_board_gpio_init(em4xx_gid, ARRAY_SIZE(em4xx_gid));
	print_hw_info();

	return 0;
}

static int print_bootinfo(void)
{
	enum boot_device bt_dev;

	bt_dev = get_boot_device();

	puts("Boot:  ");
	switch (bt_dev) {
	case MMC3_BOOT:
		puts("MMC\n");
		break;
	case USB_BOOT:
		puts("USB\n");
		break;
	default:
		printf("Unknown/Unsupported device %u\n", bt_dev);
		break;
	}

	return 0;
}

static bool has_usb(void)
{
	return dm_gpio_get_value(&em4xx_gid[HW_VER3].desc);
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

int configure_micrel_switch(struct phy_device *phydev)
{
	int ret;

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

#define MICREL_PHY_ID 0x00221430
#define MARVELL_SWITCH_ID 0x0205
int board_phy_config(struct phy_device *phydev)
{
	int ret;
	u32 id;
	char *fdtfile = NULL;

	/* Overwrite 'output' enet_ref direction from imx8mp_fec_interface_init() to 'input'
	 * because the clock is externally generated by the switch
	 */
	setup_fec();

	/* No switch in USB variant */
	if (has_usb())
		return 0;

	if (!phydev->bus)
		phydev->bus = miiphy_get_dev_by_name("FEC0");

	if (phydev->drv->config)
		phydev->drv->config(phydev);

	/* Identify switch chip as A or B */
	/* (A) Micrel KSZ8863 by reading PHY ID register, addr 0x3 reg 0x2..0x3 */
	ret = get_phy_id(phydev->bus, 0x3, MDIO_DEVAD_NONE, &id);
	if (!ret && id == MICREL_PHY_ID) {
		printf("Found Micrel PHY ID: %x\n", id);

		ret = configure_micrel_switch(phydev);
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
	print_bootinfo();
	return 0;
}
