// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2020 TQ Systems GmbH
 */
#include <common.h>
#include <errno.h>
#include <asm/io.h>
#include <asm/gpio.h>
#include <asm/arch/ddr.h>
#include <asm/arch/imx8mn_pins.h>
#include <asm/arch/sys_proto.h>
#include <asm/arch/clock.h>
#include <asm/mach-imx/iomux-v3.h>
#include <asm/mach-imx/boot_mode.h>
#include <asm/mach-imx/gpio.h>
#include <asm/mach-imx/mxc_i2c.h>
#include <fsl_esdhc.h>
#include <mmc.h>
#include <power/pmic.h>
#ifdef CONFIG_POWER_PCA9450
#include <power/pca9450.h>
#endif
#include <spl.h>
#include <usb.h>
#include <usb/ehci-ci.h>

DECLARE_GLOBAL_DATA_PTR;

#define USDHC_PAD_CTRL	(PAD_CTL_DSE6 | PAD_CTL_HYS | PAD_CTL_PUE | \
				PAD_CTL_PE | PAD_CTL_FSEL2)

#define USDHC_GPIO_PAD_CTRL (PAD_CTL_HYS | PAD_CTL_DSE1)

#define HW_VER3_GPIO	IMX_GPIO_NR(4, 25)

static iomux_v3_cfg_t const usdhc3_pads[] = {
	IMX8MN_PAD_NAND_WE_B__USDHC3_CLK | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	IMX8MN_PAD_NAND_WP_B__USDHC3_CMD | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	IMX8MN_PAD_NAND_DATA04__USDHC3_DATA0 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	IMX8MN_PAD_NAND_DATA05__USDHC3_DATA1 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	IMX8MN_PAD_NAND_DATA06__USDHC3_DATA2 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	IMX8MN_PAD_NAND_DATA07__USDHC3_DATA3 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	IMX8MN_PAD_NAND_RE_B__USDHC3_DATA4 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	IMX8MN_PAD_NAND_CE2_B__USDHC3_DATA5 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	IMX8MN_PAD_NAND_CE3_B__USDHC3_DATA6 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	IMX8MN_PAD_NAND_CLE__USDHC3_DATA7 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	IMX8MN_PAD_NAND_READY_B__USDHC3_RESET_B | MUX_PAD_CTRL(USDHC_GPIO_PAD_CTRL),
	IMX8MN_PAD_NAND_CE1_B__USDHC3_STROBE | MUX_PAD_CTRL(USDHC_GPIO_PAD_CTRL),
};

#define I2C_PAD_CTRL	(PAD_CTL_DSE6 | PAD_CTL_HYS | PAD_CTL_PUE | PAD_CTL_PE)

static struct i2c_pads_info i2c_pad_info = {
	.scl = {
		.i2c_mode = IMX8MN_PAD_I2C1_SCL__I2C1_SCL | MUX_PAD_CTRL(I2C_PAD_CTRL),
		.gpio_mode = IMX8MN_PAD_I2C1_SCL__GPIO5_IO14 | MUX_PAD_CTRL(I2C_PAD_CTRL),
		.gp = IMX_GPIO_NR(5, 14),
	},
	.sda = {
		.i2c_mode = IMX8MN_PAD_I2C1_SDA__I2C1_SDA | MUX_PAD_CTRL(I2C_PAD_CTRL),
		.gpio_mode = IMX8MN_PAD_I2C1_SDA__GPIO5_IO15 | MUX_PAD_CTRL(I2C_PAD_CTRL),
		.gp = IMX_GPIO_NR(5, 15),
	},
};

static void spl_dram_init(void)
{
	extern struct dram_timing_info em4xx_512mb_lpddr4_timing;
	extern struct dram_timing_info em4xx_1gb_lpddr4_timing;
	extern struct dram_timing_info em4xx_2gb_lpddr4_timing;

	const struct {
		long size;
		struct dram_timing_info *timing;
	} timings[] = {
		{ SZ_2G, &em4xx_2gb_lpddr4_timing },
		{ SZ_1G, &em4xx_1gb_lpddr4_timing },
		{ SZ_512M, &em4xx_512mb_lpddr4_timing },
	};
	int i;

	for (i = 0; i < ARRAY_SIZE(timings); i++) {
		long ram_size = timings[i].size;
		printf("Probing for %ld MiB RAM\n", (long)(ram_size / SZ_1M));
		if (ddr_init(timings[i].timing))
			continue;

		if (get_ram_size((void *)PHYS_SDRAM, ram_size) == ram_size) {
			printf("Detected %ld MiB RAM\n", (long)(ram_size / SZ_1M));
			return;
		}
	}

	puts("RAM detection failed\n");
	hang();
};

static struct fsl_esdhc_cfg usdhc3_cfg = {
	.esdhc_base = USDHC3_BASE_ADDR,
	.max_bus_width = 8,
};

int board_mmc_getcd(struct mmc *mmc)
{
	return 1;
}

int board_mmc_init(bd_t *bis)
{
	int ret;

	debug("%s\n", __func__);
	/*
	 * According to the board_mmc_init() the following map is done:
	 * (U-Boot device node)    (Physical Port)
	 * mmc2                    USDHC3
	 */
	init_clk_usdhc(2);
	usdhc3_cfg.sdhc_clk = mxc_get_clock(MXC_ESDHC3_CLK);
	imx_iomux_v3_setup_multiple_pads(
		usdhc3_pads, ARRAY_SIZE(usdhc3_pads));

	ret = fsl_esdhc_initialize(bis, &usdhc3_cfg);
	if (ret)
		return ret;

	return 0;
}

#if defined(CONFIG_POWER)

#define I2C_PMIC	0

#if defined(CONFIG_POWER_PCA9450)
int power_init_board(void)
{
	struct pmic *p;
	int ret;
	u32 regval;

	ret = power_pca9450b_init(I2C_PMIC);
	if (ret)
		printf("power init failed");
	p = pmic_get("PCA9450");
	pmic_probe(p);

	pmic_reg_read(p, PCA9450_REG_DEV_ID, &regval);
	printf("PMIC:  PCA9450 ID=0x%02x\n", regval);

	/*
	 * TODO:
	 * check DVS for BUCK (power save with PMIC_STBY_REQ)
	 * check VDD_SOC/dRAM -> 0.95 Volt
	 * check VDD_SNVS_0V8 -> 0.85V
	 * see imx8m[m,n]_evk
	 */

	/* set WDOG_B_CFG to cold reset w/o LDO1/2*/
	pmic_reg_read(p, PCA9450_RESET_CTRL, &regval);
	regval &= 0x3f;
	regval |= 0x80;
	pmic_reg_write(p, PCA9450_RESET_CTRL, regval);

	return 0;
}

#endif /* CONFIG_POWER_PCA9450 */

#endif /* CONFIG_POWER */

void spl_board_init(void)
{
#ifndef CONFIG_SPL_USB_SDP_SUPPORT
	/* Serial download mode */
	if (is_usb_boot()) {
		puts("Back to ROM, SDP\n");
		restore_boot_params();
	}
#endif

	puts("Normal Boot\n");
}

#ifdef CONFIG_SPL_LOAD_FIT
int board_fit_config_name_match(const char *name)
{
	bool has_usb = gpio_get_value(HW_VER3_GPIO);
	if (has_usb && !strcmp(name, "fsl-imx8mn-em4xx-u")) {
		debug("USB Version detected: load USB devicetree\n");
		return 0;
	} else if (!has_usb && !strcmp(name, "fsl-imx8mn-em4xx-l")) {
		debug("LAN Version detected: load LAN devicetree\n");
		return 0;
	} else
		return -1;
}
#endif

void board_init_f(ulong dummy)
{
	int ret;

	/* Clear the BSS. */
	memset(__bss_start, 0, __bss_end - __bss_start);

	arch_cpu_init();

	board_early_init_f();

	timer_init();

	init_uart_clk(0);

	preloader_console_init();

	ret = spl_init();
	if (ret) {
		debug("spl_init() failed: %d\n", ret);
		hang();
	}

	enable_tzc380();

	setup_i2c(0, CONFIG_SYS_I2C_SPEED, 0x7f, &i2c_pad_info);
	power_init_board();

	/* DDR initialization */
	spl_dram_init();

	board_init_r(NULL, 0);
}

/*
 * To enable boot from MMC3 (e-MMC) we must correct the boot
 * device settings, since the i.MX8MN CPU code will return BOOT_DEVICE_MMC2
 * which is wrong for our board.
 * Only one MMC is intialised by spl and BOOT_DEVICE_MMC1 is used in spl_mmc
 * to query the index of the MMC device.
 */
void board_boot_order(u32 *spl_boot_list)
{
	enum boot_device boot_device_spl = get_boot_device();

	switch (boot_device_spl) {
	case MMC3_BOOT:
		spl_boot_list[0] = BOOT_DEVICE_MMC1;
		break;
	default:
		spl_boot_list[0] = spl_boot_device();
	}
}

#ifdef CONFIG_SPL_MMC_SUPPORT

#define UBOOT_RAW_SECTOR_OFFSET 0x40
unsigned long spl_mmc_get_uboot_raw_sector(struct mmc *mmc)
{
	int part = (mmc->part_config >> 3) & PART_ACCESS_MASK;

	if (part == 1 || part == 2)
		return CONFIG_SYS_MMCSD_RAW_MODE_U_BOOT_SECTOR -
			UBOOT_RAW_SECTOR_OFFSET;

	return CONFIG_SYS_MMCSD_RAW_MODE_U_BOOT_SECTOR;
}

#endif
