/*
 * EM300 (Energymanager 300)
 * Copyright (C) 2018, TQ Systems
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

#ifndef __CONFIGS_EM3X0_H__
#define __CONFIGS_EM3X0_H__

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
#define CONFIG_ENV_SIZE			(256 * 1024)
#define CONFIG_ENV_OVERWRITE

/* Environment is in MMC */
#if defined(CONFIG_CMD_MMC) && defined(CONFIG_ENV_IS_IN_MMC)
#define CONFIG_ENV_OFFSET		(2 * 1024)
#define CONFIG_SYS_MMC_ENV_DEV		0
#define CONFIG_SYS_REDUNDAND_ENVIRONMENT
#define CONFIG_ENV_OFFSET_REDUND	(260 * 1024)
#endif

/* FEC Ethernet on SoC */
#ifdef	CONFIG_CMD_NET
#define CONFIG_FEC_MXC
#define CONFIG_MX28_FEC_MAC_IN_OCOTP
#endif

/* IP config for boot_net */
#define CONFIG_IPADDR		192.168.9.100
#define CONFIG_SERVERIP		192.168.9.133
#define CONFIG_NETMASK		255.255.255.0

/* */
#define CONFIG_NETCONSOLE

/* File System */
#define CONFIG_FS_EXT4

/* Boot Linux */
#define CONFIG_BOOTFILE		"zImage"
#define CONFIG_LOADADDR		0x42000000
#define CONFIG_SYS_LOAD_ADDR	CONFIG_LOADADDR

/* Extra Environment */
#define CONFIG_EXTRA_ENV_SETTINGS_COMMON \
	"bootdelay=1\0" \
	"BOOT_1_LEFT=3\0" \
	"BOOT_2_LEFT=3\0" \
	"BOOT_ORDER=1 2\0" \
	"console_fsl=ttyAM0\0" \
	"console_mainline=ttyAMA0\0" \
	"ethaddr=00:d0:93:00:00:00\0" \
	"fdtaddr=0x41000000\0" \
	"mmcdev=0\0" \
	"mmcpart=2\0" \
	"raucslot=1\0" \
	"args_misc=setenv bootargs ${bootargs} rauc.slot=${raucslot} panic=1\0" \
	"args_mmc=setenv bootargs ${bootargs} root=/dev/mmcblk${mmcdev}p${mmcpart} " \
		"rootfstype=ext4 ro rootwait\0" \
	"args_nc=run netconsole; setenv ncip 192.168.9.133\0" \
	"args_nc_unset=run serialconsole; setenv ncip; saveenv\0" \
	"args_tty=setenv bootargs ${bootargs} console=${console_mainline},${baudrate}\0" \
	"boot_kernel=bootz ${loadaddr} - ${fdtaddr}\0" \
	"boot_mmc=if run load_mmc_kernel && run load_mmc_dt; then " \
			"echo Loaded kernel and device tree from mmc; " \
			"run args_misc args_mmc args_tty boot_kernel; " \
		"else " \
			"echo Could not load kernel and device tree from mmc; " \
			"run args_nc; " \
		"fi\0" \
	"boot_net=if run load_tftp_kernel && run load_tftp_dt; then " \
			"echo Loaded kernel and device tree via tftp; " \
			"run args_nc_unset args_tty boot_kernel; " \
		"else " \
			"echo Could not load kernel and device tree via tftp; " \
		"fi\0" \
	"erase_env1=mw.b ${loadaddr} 0 512; mmc write ${loadaddr} 4 200\0" \
	"erase_env2=mw.b ${loadaddr} 0 512; mmc write ${loadaddr} 208 200\0" \
	"erase_mbr=mw.b ${loadaddr} 0 512; mmc write ${loadaddr} 0 2\0" \
	"load_mmc_kernel=ext4load mmc ${mmcdev}:${mmcpart} ${loadaddr} /boot/${bootfile}\0" \
	"load_mmc_dt=ext4load mmc ${mmcdev}:${mmcpart} ${fdtaddr} /boot/${fdtfile}\0" \
	"load_tftp_kernel=tftpboot ${loadaddr} ${serverip}:${hwtype}/${bootfile}\0" \
	"load_tftp_dt=tftpboot ${fdtaddr} ${serverip}:${hwtype}/${fdtfile}\0" \
	"netconsole=setenv stderr nc; setenv stdout nc; setenv stdin nc\0" \
	"serialconsole=setenv stderr serial; setenv stdout serial; setenv stdin serial\0" \
	"set_bootsys=echo Setting booting system; " \
		"setenv boot; " \
		"for BOOT_SLOT in ${BOOT_ORDER}; do " \
			"if test ! -n ${boot} && test x${BOOT_SLOT} = x1; then " \
				"if test ${BOOT_1_LEFT} -gt 0; then " \
					"setexpr BOOT_1_LEFT ${BOOT_1_LEFT} - 1; " \
					"echo Found valid slot 1, ${BOOT_1_LEFT} attempts remaining; " \
					"test ${mmcpart} = 2 || setenv mmcpart 2; " \
					"test ${raucslot} = 1 || setenv raucslot 1; " \
					"setenv boot 1; " \
				"fi; " \
			"fi; " \
			"if test ! -n ${boot} && test x${BOOT_SLOT} = x2; then " \
				"if test ${BOOT_2_LEFT} -gt 0 ; then " \
					"setexpr BOOT_2_LEFT ${BOOT_2_LEFT} - 1; " \
					"echo Found valid slot 2, ${BOOT_2_LEFT} attempts remaining; " \
					"test ${mmcpart} = 3 || setenv mmcpart 3; " \
					"test ${raucslot} = 2 || setenv raucslot 2; " \
					"setenv boot 1; " \
				"fi; " \
			"fi; " \
		"done; " \
		"setenv boot; " \
		"saveenv; " \
		"if test ${BOOT_1_LEFT} -eq 0 && test ${BOOT_2_LEFT} -eq 0; then " \
			"echo No boot tries left, resetting tries to 3; " \
			"setenv BOOT_1_LEFT 3; setenv BOOT_2_LEFT 3; " \
			"saveenv; reset; " \
		"fi\0"

#define CONFIG_BOOTCOMMAND \
	"run set_bootsys; " \
	"mmc dev ${mmcdev} ${mmcpart}; " \
	"if mmc rescan; then " \
		"echo Found mmc device; " \
		"run boot_mmc; " \
	"else " \
		"echo No mmc device found, run netconsole; " \
		"run args_nc; " \
	"fi"

#endif /* __CONFIGS_EM3X0_H__ */
