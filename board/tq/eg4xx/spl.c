// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2026 TQ-Systems GmbH <u-boot@ew.tq-group.com>,
 * D-82229 Seefeld, Germany.
 * Author: Paul Gerber
 */

#include <asm/sections.h>
#include <asm/arch/clock.h>
#include <asm/arch/ddr.h>
#include <asm/arch/sys_proto.h>
#include <asm/mach-imx/boot_mode.h>
#include <asm/global_data.h>
#include <init.h>
#include <hang.h>
#include <mmc.h>
#include <power/pmic.h>
#include <power/pca9450.h>
#include <spl.h>

DECLARE_GLOBAL_DATA_PTR;

static void spl_dram_init(void)
{
	struct dram_timing_info *timing;
	long size = PHYS_SDRAM_SIZE;

#if IS_ENABLED(CONFIG_IMX8MN_EG4XX_MEMORY_512M)
	extern struct dram_timing_info eg4xx_512mb_lpddr4_timing;

	timing = &eg4xx_512mb_lpddr4_timing;
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
	if (IS_ENABLED(CONFIG_DM_PMIC_PCA9450)) {
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
	}

	return 0;
}

void spl_board_init(void)
{
}

#ifdef CONFIG_SPL_LOAD_FIT
int board_fit_config_name_match(const char *name)
{
	return 0;
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
}

int spl_board_boot_device(enum boot_device boot_dev_spl)
{
	return BOOT_DEVICE_BOOTROM;
}
