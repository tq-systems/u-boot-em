// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2026 TQ-Systems GmbH <license@tq-group.com>, D-82229 Seefeld, Germany. All rights reserved.
 * Author: Michael Krummsdorf
 */

#include <asm/arch/sys_proto.h>
#include <asm/mach-imx/boot_mode.h>
#include <dm/uclass.h>
#include <env.h>
#include <fdt_support.h>
#include <linux/errno.h>
#include <stdio.h>
#include <sysinfo.h>

#include "tq_bdi.h"

#if defined(CONFIG_OF_BOARD_SETUP)
static int tq_bdi_set_revision(void *blob)
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

static int tq_bdi_set_serial(void *blob)
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

	ret = tq_bdi_set_revision(blob);
	if (ret)
		printf("Failed to set board revision in FDT: %d\n", ret);

	ret = tq_bdi_set_serial(blob);
	if (ret)
		printf("Failed to set serial number in FDT: %d\n", ret);

	return 0;
}
#endif

static int tq_bdi_board_model(const char *path, size_t size, char *val)
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

void tq_bdi_print_hw_info(void)
{
	char verbuf[30] = "unknown", revbuf[8] = "unknown";

	tq_bdi_board_model("/sysinfo-ver", sizeof(verbuf), verbuf);
	tq_bdi_board_model("/sysinfo-rev", sizeof(revbuf), revbuf);

	printf("HW:    %s | REV%s\n", verbuf, revbuf);
};

int tq_bdi_print_bootinfo(void)
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
