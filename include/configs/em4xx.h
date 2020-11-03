/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright 2020 TQ Systems GmbH
 */

#ifndef __EM4XX_H
#define __EM4XX_H

#include <linux/sizes.h>
#include <asm/arch/imx-regs.h>
#include "imx_env.h"


/* ENET Config */
/* ENET1 */
#if defined(CONFIG_FEC_MXC)
	#define CONFIG_ETHPRIME                 "FEC"
	#define FEC_QUIRK_ENET_MAC
#endif

#if !defined(CONFIG_SPL_BUILD)
#define CONFIG_DM_PCA953X
#endif

#define CONFIG_MXC_UART_BASE		UART1_BASE_ADDR
#define CONFIG_BAUDRATE			115200

#define CONSOLE_DEV			"ttymxc0"

#define BB_ENV_SETTINGS \
	"console=" CONSOLE_DEV "," __stringify(CONFIG_BAUDRATE) \
		" earlycon=ec_imx6q," __stringify(CONFIG_MXC_UART_BASE) "," \
		__stringify(CONFIG_BAUDRATE) "\0" \
	"fdt_file=" CONFIG_DEFAULT_FDT_FILE "\0"

#ifdef CONFIG_SECURE_BOOT
#define CONFIG_CSF_SIZE			0x2000 /* 8K region */
#endif

#define CONFIG_SPL_MAX_SIZE		(148 * 1024)
#define CONFIG_SYS_MONITOR_LEN		(512 * 1024)
#define CONFIG_SYS_MMCSD_RAW_MODE_U_BOOT_USE_SECTOR
#define CONFIG_SYS_MMCSD_RAW_MODE_U_BOOT_SECTOR	0x300
#define CONFIG_SYS_MMCSD_FS_BOOT_PARTITION	1
#define CONFIG_SYS_UBOOT_BASE		(QSPI0_AMBA_BASE + CONFIG_SYS_MMCSD_RAW_MODE_U_BOOT_SECTOR * 512)

#define CONFIG_IPADDR           192.168.9.100
#define CONFIG_SERVERIP         192.168.9.133
#define CONFIG_NETMASK          255.255.255.0

#ifdef CONFIG_SPL_BUILD
/*#define CONFIG_ENABLE_DDR_TRAINING_DEBUG*/
#define CONFIG_SPL_WATCHDOG_SUPPORT
#define CONFIG_SPL_LDSCRIPT		"arch/arm/cpu/armv8/u-boot-spl.lds"
/*
 * The memory layout on stack:  DATA section save + gd + early malloc
 * the idea is re-use the early malloc (CONFIG_SYS_MALLOC_F_LEN) with
 * CONFIG_SYS_SPL_MALLOC_START
 */
#define CONFIG_SPL_STACK		0x95fff0
#define CONFIG_SPL_BSS_START_ADDR	0x950000
#define CONFIG_SPL_BSS_MAX_SIZE		0x2000	/* 8 KB */
#define CONFIG_SYS_SPL_MALLOC_START	0x940000
#define CONFIG_SYS_SPL_MALLOC_SIZE	0x10000	/* 64 KB */
#define CONFIG_SYS_ICACHE_OFF
#define CONFIG_SYS_DCACHE_OFF
#define CONFIG_MALLOC_F_ADDR		0x940000 /* malloc f used before GD_FLG_FULL_MALLOC_INIT set */

#define CONFIG_SPL_ABORT_ON_RAW_IMAGE

#undef CONFIG_DM_MMC
#undef CONFIG_DM_PMIC
#undef CONFIG_DM_PMIC_PCA9450

#define CONFIG_SYS_I2C
#define CONFIG_SYS_I2C_SPEED		100000
#define CONFIG_SYS_I2C_MXC_I2C1		/* enable I2C bus 1 */
#define CONFIG_SYS_I2C_MXC_I2C2		/* enable I2C bus 2 */
#define CONFIG_SYS_I2C_MXC_I2C3		/* enable I2C bus 3 */

#define CONFIG_ENV_VARS_UBOOT_RUNTIME_CONFIG

#define CONFIG_POWER
#define CONFIG_POWER_I2C
#define CONFIG_POWER_PCA9450

#endif

#define CONFIG_REMAKE_ELF

#undef CONFIG_CMD_EXPORTENV
#undef CONFIG_CMD_IMPORTENV
#undef CONFIG_CMD_IMLS

#undef CONFIG_BOOTM_NETBSD

/* Initial environment variables */
#define EM4XX_ENV_SETTINGS \
	"hwtype=em4xx\0" \
	"image=Image.gz\0" \
	"fdt_addr=0x43000000\0" \
	"fdt_high=0xffffffffffffffff\0" \
	"BOOT_1_LEFT=3\0" \
	"BOOT_2_LEFT=3\0" \
	"BOOT_ORDER=1 2\0" \
	"raucslot=1\0" \
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
		"fi\0" \
	"mmcdev="__stringify(CONFIG_SYS_MMC_ENV_DEV)"\0" \
	"mmcpart=" __stringify(CONFIG_SYS_MMC_IMG_LOAD_PART) "\0" \
	"mmcautodetect=yes\0" \
	"unzipimage=unzip ${fdt_addr} ${loadaddr}\0" \
	"loadimage=load mmc ${mmcdev}:${mmcpart} ${fdt_addr} boot/${image}\0" \
	"loadfdt=load mmc ${mmcdev}:${mmcpart} ${fdt_addr} boot/${fdt_file}\0" \
	"mmcboot=echo Booting from mmc ...; " \
		"setenv bootargs; " \
		"run mmcargs; " \
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
	"update_kernel=run set_getcmd; "                                       \
		"if ${get_cmd} ${image}; then "                                \
			"if itest ${filesize} > 0; then "                      \
				"echo Write kernel image to mmc ${mmcdev}:${mmcpart}...; " \
				"save mmc ${mmcdev}:${mmcpart} ${loadaddr} "   \
					"boot/${image} ${filesize}; "               \
			"fi; "                                                 \
		"fi; "                                                         \
		"setenv filesize; setenv get_cmd \0"                           \
	"update_fdt=run set_getcmd; "                                          \
		"if ${get_cmd} ${fdt_file}; then "                             \
			"if itest ${filesize} > 0; then "                      \
				"echo Write fdt image to mmc ${mmcdev}:${mmcpart}...; " \
				"save mmc ${mmcdev}:${mmcpart} ${loadaddr} "   \
					"boot/${fdt_file} ${filesize}; "            \
			"fi; "                                                 \
		"fi; "                                                         \
		"setenv filesize; setenv get_cmd \0"                           \
	"uboot_start=0x40\0"                                                   \
	"uboot_size=0xfbe\0"                                                   \
	"uboot=bootstream.bin\0"                                               \
	"update_uboot=run set_getcmd; if ${get_cmd} ${uboot}; then "           \
		"if itest ${filesize} > 0; then "                              \
			"echo Write u-boot image to mmc ${mmcdev} ...; "       \
			"mmc dev ${mmcdev}; mmc rescan; "                      \
			"setexpr blkc ${filesize} + 0x1ff; "                   \
			"setexpr blkc ${blkc} / 0x200; "                       \
			"if itest ${blkc} <= ${uboot_size}; then "             \
				"mmc write ${loadaddr} ${uboot_start} "        \
					"${blkc}; "                            \
			"fi; "                                                 \
		"fi; fi; "                                                     \
		"setenv filesize; setenv blkc \0"                              \
	"set_getcmd=if test \"${ip_dyn}\" = yes; then "                        \
			"setenv get_cmd dhcp; "                                \
		"else "                                                        \
			"setenv get_cmd tftp; "                                \
		"fi; \0"                                                       \
	"rootfsmode=ro\0"                                                      \
	"addtty=setenv bootargs ${bootargs} console=${console}\0"              \
	"addmmc=setenv bootargs ${bootargs} "                                  \
		"root=/dev/mmcblk${mmcblkdev}p${mmcpart} ${rootfsmode} "       \
		"rootwait rauc.slot=${raucslot}\0"                             \
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
#define CONFIG_LOADADDR			0x40480000

#define CONFIG_SYS_LOAD_ADDR		CONFIG_LOADADDR

#define CONFIG_SYS_INIT_RAM_ADDR	0x40000000
#define CONFIG_SYS_INIT_RAM_SIZE	0x80000
#define CONFIG_SYS_INIT_SP_OFFSET \
	(CONFIG_SYS_INIT_RAM_SIZE - GENERATED_GBL_DATA_SIZE)
#define CONFIG_SYS_INIT_SP_ADDR \
	(CONFIG_SYS_INIT_RAM_ADDR + CONFIG_SYS_INIT_SP_OFFSET)

#define CONFIG_ENV_OVERWRITE

#define CONFIG_SYS_REDUNDAND_ENVIRONMENT


#define CONFIG_ENV_OFFSET		(4 * SZ_1M)
#define CONFIG_ENV_SIZE			(SZ_16K)
#define CONFIG_ENV_OFFSET_REDUND	(CONFIG_ENV_OFFSET + CONFIG_ENV_SIZE)
#define CONFIG_ENV_SIZE_REDUND		(CONFIG_ENV_SIZE)
#define CONFIG_SYS_MMC_ENV_DEV		0

/* Size of malloc() pool */
#define CONFIG_SYS_MALLOC_LEN		((CONFIG_ENV_SIZE + (2*1024) + \
					 (16*1024)) * 1024)

#define CONFIG_SYS_SDRAM_BASE		0x40000000
#define PHYS_SDRAM			0x40000000

#define PHYS_SDRAM_SIZE			0x20000000 /* 512MB LPDDR4 */

#if defined(CONFIG_CMD_MEMTEST)
/*
 * Use alternative / extended memtest,
 * start at CONFIG_LOADADDR and use 3/4 of RAM
 * U-Boot is loaded to 0x40200000 (offset 2 MiB)
 * and relocated at end of configured RAM
 */
#if defined(CONFIG_SYS_ALT_MEMTEST)
#define CONFIG_SYS_MEMTEST_START	(CONFIG_LOADADDR)
#define CONFIG_SYS_MEMTEST_END		(CONFIG_SYS_MEMTEST_START + \
					((PHYS_SDRAM_SIZE / 4) * 3))
#define CONFIG_SYS_MEMTEST_SCRATCH	CONFIG_SYS_MEMTEST_END

#endif /* CONFIG_SYS_ALT_MEMTEST */

#endif /* CONFIG_CMD_MEMTEST */

#define CONFIG_BAUDRATE			115200

/* Monitor Command Prompt */
#undef CONFIG_SYS_PROMPT
#define CONFIG_SYS_PROMPT		"u-boot=> "
#define CONFIG_SYS_PROMPT_HUSH_PS2	"> "
#define CONFIG_SYS_CBSIZE		2048
#define CONFIG_SYS_MAXARGS		64
#define CONFIG_SYS_BARGSIZE CONFIG_SYS_CBSIZE
#define CONFIG_SYS_PBSIZE		(CONFIG_SYS_CBSIZE + \
					sizeof(CONFIG_SYS_PROMPT) + 16)


#if defined(CONFIG_FSL_ESDHC)

#define CONFIG_FSL_USDHC

#define CONFIG_SYS_FSL_ESDHC_ADDR	0

#define CONFIG_SUPPORT_EMMC_BOOT	/* eMMC specific */
#define CONFIG_SYS_MMC_IMG_LOAD_PART	1

#endif

#ifdef CONFIG_FSL_FSPI
#define FSL_FSPI_FLASH_SIZE		SZ_64M
#define FSL_FSPI_FLASH_NUM		1
#define FSPI0_BASE_ADDR			0x30bb0000
#define FSPI0_AMBA_BASE			0x0

#define CONFIG_SYS_FSL_FSPI_AHB
#endif

#define CONFIG_MXC_OCOTP

#if defined(CONFIG_USB)

#define CONFIG_USB_MAX_CONTROLLER_COUNT		2

#define CONFIG_USBD_HS

#define CONFIG_MXC_USB_PORTSC			(PORT_PTS_UTMI | PORT_PTS_PTW)

#endif

#define CONFIG_EXTRA_ENV_SETTINGS		\
	EM4XX_ENV_SETTINGS		\
	BB_ENV_SETTINGS

#ifndef CONFIG_SPL_BUILD
#define BOOT_TARGET_DEVICES(func) \
	func(MMC, mmc, 0) \
	func(USB, usb, 0)
#include <config_distro_bootcmd.h>
#endif

#endif /* __EM4XX_H */
