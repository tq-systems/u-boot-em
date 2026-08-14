/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2023 - 2024 TQ-Systems GmbH <u-boot@ew.tq-group.com>,
 * D-82229 Seefeld, Germany.
 * Author: Michael Krummsdorf
 */

#ifndef __EM4XX_H
#define __EM4XX_H

#include <linux/sizes.h>
#include <asm/arch/imx-regs.h>
#include <configs/em_set_bootsys.h>

#define CONSOLE_DEV			"ttymxc0"

#define BB_ENV_SETTINGS \
	"console=" CONSOLE_DEV "," __stringify(CONFIG_BAUDRATE)  "\0" \
	"fdt_file=" CONFIG_DEFAULT_FDT_FILE "\0"

/* Initial environment variables */
/* TODO: Environment unification/variable renaming of em310/em4xx
 *       must be able to handle saved envs on field devices.
 *       We will take care of that step at a later date.
 */
#define EM4XX_ENV_SETTINGS \
	"hwtype=" CONFIG_IMX8MN_EM4XX_HWTYPE "\0" \
	"image=Image.gz\0" \
	"loadaddr=" __stringify(CONFIG_SYS_LOAD_ADDR) "\0" \
	"fdt_addr=0x46480000\0" \
	"mmcdev=" __stringify(CONFIG_ENV_MMC_DEVICE_INDEX) "\0" \
	"mmcblkdev=" __stringify(CONFIG_ENV_MMC_DEVICE_INDEX) "\0" \
	"mmcpart=1\0" \
	"unzipimage=unzip ${fdt_addr} ${loadaddr}\0" \
	"loadimage=load mmc ${mmcdev}:${mmcpart} ${fdt_addr} boot/${image}\0" \
	"loadfdt=load mmc ${mmcdev}:${mmcpart} ${fdt_addr} boot/${fdt_file}\0" \
	"mmcboot=echo Booting from mmc ...; " \
		"setenv bootargs; " \
		"run mmcargs; " \
		"mmc dev ${mmcdev}; mmc rescan; " \
		"run loadimage && " \
		"run unzipimage && " \
		"run loadfdt && " \
		"booti ${loadaddr} - ${fdt_addr}\0" \
	"boot_net=echo Booting from net ...; " \
		"setenv bootargs; " \
		"run netargs;  " \
		"run set_getcmd; " \
		"${get_cmd} ${fdt_addr} ${hwtype}/${image} && " \
		"run unzipimage && " \
		"${get_cmd} ${fdt_addr} ${hwtype}/${fdt_file} && " \
		"echo 'Loaded kernel and device tree via tftp' && " \
		"booti ${loadaddr} - ${fdt_addr}\0" \
	"set_getcmd=if test \"${ipmode}\" != static; then "                    \
			"setenv get_cmd dhcp; "                                \
		"else "                                                        \
			"setenv get_cmd tftp; "                                \
		"fi; \0"                                                       \
	"rootfsmode=ro\0"                                                      \
	"addtty=setenv bootargs ${bootargs} console=${console}\0"              \
	"addmmc=setenv bootargs ${bootargs} "                                  \
		"root=/dev/mmcblk${mmcblkdev}p${mmcpart} ${rootfsmode} "       \
		"rootwait "                                                    \
		"rauc.slot=${raucslot}\0"                                      \
	"mmcargs=run addtty addmmc\0"                                          \
	"netargs=run addtty\0"                                    \
	"netconsole=echo Starting netconsole...; "                             \
		"setenv ncip ${serverip}; "                                    \
		"setenv stderr nc; setenv stdout nc; setenv stdin nc\0"        \
	"serialconsole=setenv stderr serial; setenv stdout serial; setenv stdin serial; " \
		"setenv ncip\0"                                                \
	"ipmode=static\0"                                                      \
	""

/* Link Definitions */
#define CFG_SYS_INIT_RAM_ADDR	0x40000000
#define CFG_SYS_INIT_RAM_SIZE	0x80000
#define CFG_SYS_SDRAM_BASE	0x40000000
#define PHYS_SDRAM		0x40000000

/* Minimum size - only used during init */
#if IS_ENABLED(CONFIG_IMX8MN_EM4XX_MEMORY_1G)
#define PHYS_SDRAM_SIZE		SZ_1G
#elif IS_ENABLED(CONFIG_IMX8MN_EM4XX_MEMORY_512M)
#define PHYS_SDRAM_SIZE		SZ_512M
#else
# error "No RAM timing configured"
#endif

#define CFG_EXTRA_ENV_SETTINGS		\
	EM4XX_ENV_SETTINGS		\
	EM_SET_BOOTSYS			\
	BB_ENV_SETTINGS

#endif /* __EM4XX_H */
