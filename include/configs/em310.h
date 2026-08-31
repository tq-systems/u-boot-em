/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * EM310 (Energymanager 310)
 *
 * Based on mx28evk.h
 * (C) Copyright 2011 Freescale Semiconductor, Inc.
 * Author: Fabio Estevam <fabio.estevam@freescale.com>
 *
 * Copyright (c) 2023 TQ-Systems GmbH <u-boot@ew.tq-group.com>, D-82229 Seefeld, Germany.
 * Author: Michael Krummsdorf
 */

#ifndef __CONFIGS_EM310_H__
#define __CONFIGS_EM310_H__

#include <configs/em_set_bootsys.h>

/* Memory configuration */
#define PHYS_SDRAM_1			0x40000000	/* Base address */
#define PHYS_SDRAM_1_SIZE		0x40000000	/* Max 1 GB RAM */
#define CFG_SYS_SDRAM_BASE		PHYS_SDRAM_1

/* Extra Environment */
#define CFG_EXTRA_ENV_SETTINGS_COMMON \
	"console=ttyAMA0\0" \
	"ethaddr=00:d0:93:00:00:00\0" \
	"fdtaddr=0x41000000\0" \
	"mmcdev=0\0" \
	"mmcpart=2\0" \
	"args_misc=setenv bootargs ${bootargs} rauc.slot=${raucslot} panic=1\0" \
	"args_mmc=setenv bootargs ${bootargs} root=/dev/mmcblk${mmcdev}p${mmcpart} " \
		"rootfstype=ext4 ro rootwait\0" \
	"args_nc=run netconsole; setenv ncip ${serverip}\0" \
	"args_nc_unset=run serialconsole; setenv ncip; saveenv\0" \
	"args_tty=setenv bootargs ${bootargs} console=${console},${baudrate}\0" \
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
	"netconsole=echo Starting netconsole...; setenv stderr nc; setenv stdout nc; setenv stdin nc\0" \
	"serialconsole=setenv stderr serial; setenv stdout serial; setenv stdin serial\0" \
	""

/* Extra Environment */
#define CFG_EXTRA_ENV_SETTINGS \
	CFG_EXTRA_ENV_SETTINGS_COMMON \
	EM_SET_BOOTSYS		\
	\
	"fdtfile=imx28-em310.dtb\0" \
	"hwtype=em310\0" \

/* The rest of the configuration is shared */
#include <configs/mxs.h>

#endif /* __CONFIGS_EM310_H__ */
