// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2023 - 2024 TQ-Systems GmbH <u-boot@ew.tq-group.com>,
 * D-82229 Seefeld, Germany.
 * Author: Michael Krummsdorf
 */

#include <common.h>
#include <errno.h>
#include <asm/io.h>
#include <asm/gpio.h>
#include <asm/sections.h>
#include <asm/arch/ddr.h>
#include <asm/arch/imx-regs.h>
#include <asm/arch/imx8mn_pins.h>
#include <asm/arch/sys_proto.h>
#include <asm/arch/clock.h>
#include <asm/mach-imx/iomux-v3.h>
#include <asm/mach-imx/boot_mode.h>
#include <asm/mach-imx/gpio.h>
#include <asm/mach-imx/mxc_i2c.h>
#include <env.h>
#include <fsl_esdhc_imx.h>
#include <init.h>
#include <hang.h>
#include <mmc.h>
#include <power/pmic.h>
#include <power/pca9450.h>
#include <dm/uclass.h>
#include <dm/device.h>
#include <spl.h>
#include <usb.h>
#include <usb/ehci-ci.h>

#include "../common/tq_board_gpio.h"

DECLARE_GLOBAL_DATA_PTR;

#define HW_VER3_GPIO	IMX_GPIO_NR(4, 25)

static void spl_dram_init(void)
{
	struct dram_timing_info *timing;
	long size = PHYS_SDRAM_SIZE;

#if IS_ENABLED(CONFIG_IMX8MN_EM4XX_MEMORY_1G)
	extern struct dram_timing_info em4xx_1gb_lpddr4_timing;
	timing = &em4xx_1gb_lpddr4_timing;
#elif IS_ENABLED(CONFIG_IMX8MN_EM4XX_MEMORY_512M)
	extern struct dram_timing_info em4xx_512mb_lpddr4_timing;
	timing = &em4xx_512mb_lpddr4_timing;
#else
# error "No RAM timing configured"
#endif

	printf("Probing for %ld MiB RAM\n", (long)(size / SZ_1M));
	if ((ddr_init(timing) == 0) &&
	    (get_ram_size((void *)PHYS_SDRAM, size) == size)) {
		printf("Detected %ld MiB RAM\n", (long)(size / SZ_1M));
		return;
	}

	puts("RAM detection failed\n");
	hang();
};

int board_mmc_getcd(struct mmc *mmc)
{
	return 1;
}

static int power_init_board(void)
{
#if CONFIG_IS_ENABLED(DM_PMIC_PCA9450)
	struct udevice *dev;
	int ret;

	ret = pmic_get("pmic@25", &dev);
	if (ret == -ENODEV) {
		puts("No pmic@25\n");
		return 0;
	}

	if (ret != 0)
		return ret;

	/*
	 * LDO1		1.8V	(default)
	 * LDO2		0.85V	(default)
	 * BUCK1	0.85V	(default)
	 * BUCK2	0.85V	(default)
	 * LDO3		1.8V	(default)
	 * BUCK5	1.8V	(default)
	 * BUCK6	1.1V	(default)
	 * BUCK4	3.3V	(default)
	 * LDO5		3.3V	(default)
	 * BUCK3 & LDO4 disabled
	 */

	/* Disable PRESET_EN */
	pmic_reg_write(dev, PCA9450_BUCK123_DVS, 0x29);

	/* BUCK3: off */
	pmic_reg_write(dev, PCA9450_BUCK3CTRL, 0x48);
	/* LDO4: off */
	pmic_reg_write(dev, PCA9450_LDO4CTRL, 0x01);

	/* set WDOG_B_CFG to cold reset */
	pmic_reg_write(dev, PCA9450_RESET_CTRL, 0xA1);
#endif

	return 0;
}

void spl_board_init(void)
{
}

#ifdef CONFIG_SPL_LOAD_FIT
int board_fit_config_name_match(const char *name)
{
	bool has_usb;

	gpio_request(HW_VER3_GPIO, "");
	gpio_direction_input(HW_VER3_GPIO);
	has_usb = gpio_get_value(HW_VER3_GPIO);
	gpio_free(HW_VER3_GPIO);

	if (has_usb && !strcmp(name, "imx8mn-em4xx-u")) {
		debug("USB Version detected: load USB devicetree\n");
		return 0;
	} else if (!has_usb && !strcmp(name, "imx8mn-em4xx-l")) {
		debug("LAN Version detected: load LAN devicetree\n");
		return 0;
	} else
		return -1;
}
#endif

void board_init_f(ulong dummy)
{
	int ret;

	arch_cpu_init();

	init_uart_clk(0);

	/* Clear the BSS. */
	memset(__bss_start, 0, __bss_end - __bss_start);

	ret = spl_init();
	if (ret) {
		debug("spl_init() failed: %d\n", ret);
		hang();
	}

	preloader_console_init();

	enable_tzc380();

	power_init_board();

	/* DDR initialization */
	spl_dram_init();

	board_init_r(NULL, 0);
}

int spl_board_boot_device(enum boot_device boot_dev_spl)
{
	return BOOT_DEVICE_BOOTROM;
}
