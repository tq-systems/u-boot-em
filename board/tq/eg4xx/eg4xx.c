// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2026 TQ-Systems GmbH <u-boot@ew.tq-group.com>,
 * D-82229 Seefeld, Germany.
 * Author: Paul Gerber
 */

#include <asm/arch/sys_proto.h>
#include <asm-generic/gpio.h>
#include <asm/mach-imx/boot_mode.h>
#include <asm/arch/sys_proto.h>
#include <asm/global_data.h>
#include <env.h>
#include <led.h>
#include <linux/delay.h>
#include <linux/mdio.h>
#include <linux/types.h>
#include <miiphy.h>
#include <phy.h>

#include "../common/tq_rtc.h"

DECLARE_GLOBAL_DATA_PTR;

static int is_factory_button_pressed(void)
{
	struct gpio_desc factory_button;
	int ret;

	ret = dm_gpio_lookup_name("GPIO4_31", &factory_button);
	if (ret)
		return 0;

	ret = dm_gpio_request(&factory_button, "factory-dflt-n");
	if (ret && ret != -EBUSY)
		return 0;

	dm_gpio_set_dir_flags(&factory_button, GPIOD_IS_IN);

	return dm_gpio_get_value(&factory_button) == 0;
}

int board_init(void)
{
	struct udevice *red_status_led = NULL;
	struct udevice *green_status_led = NULL;
	int i;

	if (led_get_by_label("energymanager:red:status", &red_status_led))
		red_status_led = NULL;
	if (led_get_by_label("energymanager:green:status", &green_status_led))
		green_status_led = NULL;

	if (is_factory_button_pressed() && red_status_led && green_status_led) {
		printf("Test: Button pressed, starting 1s LED test\n");

		for (i = 0; i < 20; i++) {
			if (i & 1) {
				led_set_state(red_status_led, LEDST_OFF);
				led_set_state(green_status_led, LEDST_ON);
			} else {
				led_set_state(red_status_led, LEDST_ON);
				led_set_state(green_status_led, LEDST_OFF);
			}
			mdelay(50);
		}
	}

	if (red_status_led)
		led_set_state(red_status_led, LEDST_ON);

	if (green_status_led)
		led_set_state(green_status_led, LEDST_OFF);

	tq_bdi_print_hw_info();

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

int board_late_init(void)
{
	if (is_usb_boot()) {
		env_set("bootcmd", "run fastbootcmd");
		env_set("bootdelay", "0");
	}

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
