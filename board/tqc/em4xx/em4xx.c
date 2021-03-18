// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2020 TQ Systems GmbH
 */

#include <common.h>
#include <dm.h>
#include <dm/device-internal.h>
#include <dm/lists.h>
#include <dm/uclass-internal.h>
#include <asm/arch/imx8mn_pins.h>
#include <malloc.h>
#include <errno.h>
#include <asm/io.h>
#include <asm/mach-imx/iomux-v3.h>
#include <asm-generic/gpio.h>
#include <fsl_esdhc.h>
#include <mmc.h>
#include <netdev.h>
#include <asm/arch/sys_proto.h>
#include <asm/mach-imx/boot_mode.h>
#include <asm/mach-imx/gpio.h>
#include <asm/mach-imx/mxc_i2c.h>
#include <asm/arch/clock.h>
#include <spl.h>

#include "../common/tqc_bb.h"
#include "../common/tqc_board_gpio.h"


char *hw_rev_tbl[] = {
	"REV0100",
	"REV0101",
	"REV0200",
	"REV0201",
	"REV0202",
	"REV0203",
	"REV0204",
	"REV0205",
};

char *hw_ver_tbl[] = {
	"EM4XX-CB-LLRR",
	"EM4XX-L-CB-LRR",
	"EM4XX-L-CB-LL",
	"UNKNOWN",
	"UNKNOWN.",
	"UNKNOWN",
	"UNKNOWN",
	"UNKNOWN",
	"EM4XX-CB-ULRR",
	"EM4XX-CB-URR",
	"EM4XX-CB-LRR",
	"EM4XX-CB-UL",
	"EM4XX-CB-U",
	"EM4XX-CB-L",
	"UNKNOWN",
	"UNKNOWN"
};

#if !defined(CONFIG_SPL_BUILD)
enum {
	HW_REV0,
	HW_REV1,
	HW_REV2,
	HW_VER0,
	HW_VER1,
	HW_VER2,
	HW_VER3,
	LED1_GN,
	LED1_RD,
	LED2_GN,
	LED2_RD,
	LED3_GN,
	LED3_RD,
	INT_5V_N,
	FACTORY_DFLT_N,
	USB_EN,
};

static struct tqc_gpio_init_data em4xx_gid[] = {

	GPIO_INIT_DATA_ENTRY(HW_REV2, "GPIO4_30", GPIOD_IS_IN),
	GPIO_INIT_DATA_ENTRY(HW_REV1, "GPIO4_28", GPIOD_IS_IN),
	GPIO_INIT_DATA_ENTRY(HW_REV0, "GPIO4_27", GPIOD_IS_IN),

	GPIO_INIT_DATA_ENTRY(HW_VER3, "GPIO4_25", GPIOD_IS_IN),
	GPIO_INIT_DATA_ENTRY(HW_VER2, "GPIO4_24", GPIOD_IS_IN),
	GPIO_INIT_DATA_ENTRY(HW_VER1, "GPIO4_23", GPIOD_IS_IN),
	GPIO_INIT_DATA_ENTRY(HW_VER0, "GPIO4_22", GPIOD_IS_IN),

	GPIO_INIT_DATA_ENTRY(LED1_GN, "GPIO5_2", GPIOD_IS_OUT),
	GPIO_INIT_DATA_ENTRY(LED1_RD, "GPIO5_3", GPIOD_IS_OUT),
	GPIO_INIT_DATA_ENTRY(LED2_GN, "GPIO5_4", GPIOD_IS_OUT),
	GPIO_INIT_DATA_ENTRY(LED2_RD, "GPIO5_5", GPIOD_IS_OUT),
	GPIO_INIT_DATA_ENTRY(LED3_GN, "GPIO5_28", GPIOD_IS_OUT),
	GPIO_INIT_DATA_ENTRY(LED3_RD, "GPIO5_29", GPIOD_IS_OUT),

	GPIO_INIT_DATA_ENTRY(INT_5V_N, "GPIO5_0", GPIOD_IS_IN),
	GPIO_INIT_DATA_ENTRY(FACTORY_DFLT_N, "GPIO4_31", GPIOD_IS_IN),

	GPIO_INIT_DATA_ENTRY(USB_EN, "GPIO1_12", GPIOD_IS_OUT),
};

void print_hw_info(void)
{
	int idx;
	unsigned int hw_rev = 0x0;
	unsigned int hw_ver = 0x0;

	for (idx = HW_REV0; idx <= HW_REV2; ++idx)
		hw_rev |= (dm_gpio_get_value(&em4xx_gid[idx].desc) ? 1 : 0) <<
			(idx - HW_REV0);

	for (idx = HW_VER0; idx <= HW_VER3; ++idx)
		hw_ver |= (dm_gpio_get_value(&em4xx_gid[idx].desc) ? 1 : 0) <<
			(idx - HW_VER0);

	printf("HW:    %s | %s\n", hw_ver_tbl[hw_ver], hw_rev_tbl[hw_rev]);
};
#endif

#define HW_VER3_GPIO	IMX_GPIO_NR(4, 25)

#define WDOG_PAD_CTRL	(PAD_CTL_DSE6 | PAD_CTL_HYS)


static iomux_v3_cfg_t const wdog_pads[] = {
	IMX8MN_PAD_GPIO1_IO02__WDOG1_WDOG_B | MUX_PAD_CTRL(WDOG_PAD_CTRL),
};

DECLARE_GLOBAL_DATA_PTR;


#define UART_PAD_CTRL	(PAD_CTL_DSE6 | PAD_CTL_FSEL1)

static iomux_v3_cfg_t const uart_pads[] = {
	IMX8MN_PAD_UART1_RXD__UART1_DCE_RX | MUX_PAD_CTRL(UART_PAD_CTRL),
	IMX8MN_PAD_UART1_TXD__UART1_DCE_TX | MUX_PAD_CTRL(UART_PAD_CTRL),
};

int board_early_init_f(void)
{
	struct wdog_regs *wdog = (struct wdog_regs *)WDOG1_BASE_ADDR;

	imx_iomux_v3_setup_multiple_pads(wdog_pads, ARRAY_SIZE(wdog_pads));
	set_wdog_reset(wdog);

	init_uart_clk(0);

	imx_iomux_v3_setup_multiple_pads(uart_pads, ARRAY_SIZE(uart_pads));

	return 0;
}

int dram_init(void)
{
	/* rom_pointer[1] contains the size of TEE occupies */
	if (rom_pointer[1])
		gd->ram_size = PHYS_SDRAM_SIZE - rom_pointer[1];
	else
		gd->ram_size = PHYS_SDRAM_SIZE;

	return 0;
}

#ifdef CONFIG_OF_BOARD_SETUP
int ft_board_setup(void *blob, bd_t *bd)
{
	return 0;
}
#endif

int board_init(void)
{
#if !defined(CONFIG_SPL_BUILD)
	tqc_board_gpio_init(em4xx_gid, ARRAY_SIZE(em4xx_gid));
	print_hw_info();
#endif
	return 0;
}

int print_bootinfo(void)
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

static bool has_usb(void)
{
	static int ret = -1;

	if (ret == -1)
		ret = gpio_get_value(HW_VER3_GPIO);

	return ret;
}

void adjust_env(void)
{
	enum boot_device bt_dev = get_boot_device();

	if (!has_usb())
		env_set("fdt_file", "imx8mn-em4xx-l.dtb");
	else
		env_set("fdt_file", "imx8mn-em4xx-u.dtb");

	/* disable autoboot in serial download mode*/
	if (bt_dev == USB_BOOT)
		env_set("bootdelay", "-1");
}

int board_phy_config(struct phy_device *phydev)
{
	int ret;

	/* No switch in USB variant */
	if (has_usb())
		return 0;

	/* Set KSZ8863 driver strength to 8mA */
	ret = fec_smi_write(phydev, 0x0E, 0x07);
	if (ret) {
		printf("FEC MXS: Unable to set KSZ8863 driver strength\n");
		return ret;
	}

	/* Change KSZ8863 to use internal source for RMII clock */
	ret = fec_smi_write(phydev, 0xC6, 0x0B);
	if (ret) {
		printf("FEC MXS: Unable to change KSZ8863 RMII clock settings\n");
		return ret;
	}

	return 0;
}

int board_late_init(void)
{

#ifdef CONFIG_ENV_IS_IN_MMC
	board_late_mmc_env_init();
#endif

#if !defined(CONFIG_SPL_BUILD)

	adjust_env();

	/* set quartz load to 12500 femtofarads */
	tqc_pcf85063_adjust_capacity(0, 0x51, 12500);
	tqc_pcf85063_set_clkout(0, 0x51, TQC_PCF85063_CLKOUT_OFF);
	tqc_pcf85063_set_offset(0, 0x51, true, 5);
#endif


	return 0;
}

int checkboard(void)
{
	print_bootinfo();
	return 0;
}

int mmc_map_to_kernel_blk(int dev_no)
{
	return dev_no;
}

int board_mmc_get_env_dev(int devno)
{
	return 0;
}

u32 fsl_esdhc_clk_index(struct udevice *dev)
{
	/* We only have a single SDHC controller */
	return 2;
}
