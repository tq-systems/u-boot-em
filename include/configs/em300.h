/*
 * EM300 (Energymanager 300)
 * Copyright (C) 2017, TQ Systems
 * Christoph Krutz <christoph.krutz@tq-group.com>
 *
 * Based on mx28evk.h
 * (C) Copyright 2011 Freescale Semiconductor, Inc.
 * Author: Fabio Estevam <fabio.estevam@freescale.com>
 *
 * Based on m28evk.h:
 * Copyright (C) 2011 Marek Vasut <marek.vasut@gmail.com>
 * on behalf of DENX Software Engineering GmbH
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __CONFIGS_EM300_H__
#define __CONFIGS_EM300_H__

/* System configurations */
#define CONFIG_MX28				/* i.MX28 SoC */
#define CONFIG_MACH_TYPE	MACH_TYPE_MX28EVK	/* TODO: set own type or use device tree */

/* Extra board configuration */
#define CONFIG_BOARD_LATE_INIT		/* revision and led init */
#define CONFIG_MISC_INIT_R		/* set emmc_dsr */

/* U-Boot Commands */

/* Memory configuration */
#define CONFIG_NR_DRAM_BANKS		1		/* 1 bank of DRAM */
#define PHYS_SDRAM_1			0x40000000	/* Base address */
#define PHYS_SDRAM_1_SIZE		0x40000000	/* Max 1 GB RAM */
#define CONFIG_SYS_SDRAM_BASE		PHYS_SDRAM_1

/* Environment */
#define CONFIG_ENV_SIZE			(16 * 1024)
#define CONFIG_ENV_OVERWRITE

/* Environment is in MMC */
#if defined(CONFIG_CMD_MMC) && defined(CONFIG_ENV_IS_IN_MMC)
#define CONFIG_ENV_OFFSET		(256 * 1024)
#define CONFIG_SYS_MMC_ENV_DEV		0
#endif

/* FEC Ethernet on SoC */
#ifdef	CONFIG_CMD_NET
#define CONFIG_FEC_MXC
#define CONFIG_MX28_FEC_MAC_IN_OCOTP
#endif

/* File System */
#define CONFIG_FS_EXT4

/* Boot Linux */
#define CONFIG_BOOTFILE		"zImage"
#define CONFIG_LOADADDR		0x42000000
#define CONFIG_SYS_LOAD_ADDR	CONFIG_LOADADDR

/* Extra Environment */
#define CONFIG_EXTRA_ENV_SETTINGS \
	"boot_fdt=try\0" \
	"mmcdev=0\0" \
	"mmcpart=2\0" \
	"console_fsl=ttyAM0\0" \
	"console_mainline=ttyAMA0\0" \
	"fdtfile=imx28-em300.dtb\0" \
	"fdtaddr=0x41000000\0" \
	"miscargs=setenv bootargs ${bootargs} panic=1\0" \
	"mmcargs=setenv bootargs ${bootargs} root=/dev/mmcblk${mmcdev}p${mmcpart} rw rootwait\0" \
	"ttyargs=setenv bootargs ${bootargs} console=${console_mainline},${baudrate}\0" \
	"erase_env=mw.b ${loadaddr} 0 512; mmc write ${loadaddr} 2 1\0" \
	"erase_mmc=mw.b ${loadaddr} 0 512; mmc write ${loadaddr} 0 2\0" \
	"loadimage=ext4load mmc ${mmcdev}:${mmcpart} ${loadaddr} /boot/${bootfile}\0" \
	"loadfdt=ext4load mmc ${mmcdev}:${mmcpart} ${fdtaddr} /boot/${fdtfile}\0" \
	"mmcboot=echo Booting from mmc ...; " \
		"run miscargs mmcargs ttyargs; " \
		"if test ${boot_fdt} = yes || test ${boot_fdt} = try; then " \
			"if run loadfdt; then " \
				"bootz ${loadaddr} - ${fdtaddr}; " \
			"else " \
				"if test ${boot_fdt} = try; then " \
					"bootz; " \
				"else " \
					"echo WARN: Cannot load the DT; " \
				"fi; " \
			"fi; " \
		"else " \
			"bootz; " \
		"fi;\0" \
	"netboot=echo Booting netconsole for production process TODO...\0"

#define CONFIG_BOOTCOMMAND \
	"mmc dev ${mmcdev} ${mmcpart}; if mmc rescan; then " \
		"if run loadimage; then " \
			"run mmcboot; " \
		"else " \
			"run netboot; " \
		"fi; " \
	"else " \
		"run netboot; " \
	"fi;"

/* The rest of the configuration is shared */
#include <configs/mxs.h>

#endif /* __CONFIGS_EM300_H__ */
