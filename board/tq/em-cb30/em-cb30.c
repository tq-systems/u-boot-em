// SPDX-License-Identifier: GPL-2.0+
/*
 * Board specific initialization for AM62x platforms
 *
 * Copyright (C) 2020-2022 Texas Instruments Incorporated - https://www.ti.com/
 *	Suman Anna <s-anna@ti.com>
 *
 */

#include <cpu_func.h>
#include <env.h>
#include <spl.h>
#include <init.h>
#include <k3-ddrss.h>
#include <fdt_support.h>
#include <sysinfo.h>
#include <asm/io.h>
#include <asm/arch/hardware.h>
#include <dm/uclass.h>
#include <power/pmic.h>
#include <power/tps65219.h>

DECLARE_GLOBAL_DATA_PTR;

void spl_board_init(void)
{
	struct udevice *dev;
	int ret;

	/* PMIC configuration is done as early as possible, in R5 SPL */
	if (!IS_ENABLED(CONFIG_TARGET_AM625_R5_EM_CB30))
		return;

	ret = pmic_get("pmic@30", &dev);
	if (ret) {
		printf("Failed to get PMIC: %d\n", ret);
		return;
	}

	/*
	 * Clear WARM_COLD_RESET_CONFIG flag to repeat the whole power
	 * sequencing on reset via PMIC RESET pin
	 */
	ret = pmic_clrsetbits(dev, TPS65219_MFP_2_CONFIG_REG, 0x40, 0);
	if (ret) {
		printf("Failed to apply PMIC configuration: %d\n", ret);
		return;
	}
}

int board_init(void)
{
	return 0;
}

static int em_cb30_board_model(const char *path, size_t size, char *val)
{
	struct udevice *dev;
	int ret;

	ret = uclass_get_device_by_of_path(UCLASS_SYSINFO, path, &dev);
	if (ret)
		return ret;

	ret = sysinfo_detect(dev);
	if (ret)
		return ret;

	return sysinfo_get_str(dev, SYSINFO_ID_BOARD_MODEL, size, val);
}

int board_late_init(void)
{
	char verbuf[30] = "unknown", revbuf[8] = "unknown";

	em_cb30_board_model("/sysinfo-ver", sizeof(verbuf), verbuf);
	em_cb30_board_model("/sysinfo-rev", sizeof(revbuf), revbuf);

	printf("Board version: %s REV.%s\n", verbuf, revbuf);

	return 0;
}

int dram_init(void)
{
	return fdtdec_setup_mem_size_base();
}

int dram_init_banksize(void)
{
	return fdtdec_setup_memory_banksize();
}

phys_size_t get_effective_memsize(void)
{
	/*
	 * Just below 512MiB are TF-A and OPTEE reserved regions, thus
	 * SPL/U-Boot RAM has to start below that. Leave 256MiB space for
	 * all reserved memories.
	 */
	return gd->ram_size == SZ_512M ? SZ_256M : gd->ram_size;
}

#if defined(CONFIG_SPL_BUILD)

#if defined(CONFIG_K3_AM64_DDRSS)
static void fixup_ddr_driver_for_ecc(struct spl_image_info *spl_image)
{
	struct udevice *dev;
	int ret;

	dram_init_banksize();

	ret = uclass_get_device(UCLASS_RAM, 0, &dev);
	if (ret)
		panic("Cannot get RAM device for ddr size fixup: %d\n", ret);

	ret = k3_ddrss_ddr_fdt_fixup(dev, spl_image->fdt_addr, gd->bd);
	if (ret)
		printf("Error fixing up ddr node for ECC use! %d\n", ret);
}
#else
static void fixup_memory_node(struct spl_image_info *spl_image)
{
	u64 start[CONFIG_NR_DRAM_BANKS];
	u64 size[CONFIG_NR_DRAM_BANKS];
	int bank;
	int ret;

	dram_init();
	dram_init_banksize();

	for (bank = 0; bank < CONFIG_NR_DRAM_BANKS; bank++) {
		start[bank] =  gd->bd->bi_dram[bank].start;
		size[bank] = gd->bd->bi_dram[bank].size;
	}

	/* dram_init functions use SPL fdt, and we must fixup u-boot fdt */
	ret = fdt_fixup_memory_banks(spl_image->fdt_addr, start, size,
				     CONFIG_NR_DRAM_BANKS);
	if (ret)
		printf("Error fixing up memory node! %d\n", ret);
}
#endif

void spl_perform_fixups(struct spl_image_info *spl_image)
{
#if defined(CONFIG_K3_AM64_DDRSS)
	fixup_ddr_driver_for_ecc(spl_image);
#else
	fixup_memory_node(spl_image);
#endif
}
#endif

#ifdef CONFIG_OF_BOARD_SETUP
static int em_cb30_set_revision(void *blob)
{
	char revbuf[8] = {};
	char *endp;
	ulong rev;
	int ret;

	ret = em_cb30_board_model("/sysinfo-rev", sizeof(revbuf), revbuf);
	if (ret)
		return ret;

	rev = hextoul(revbuf, &endp);
	if (*endp) /* Revision string in Device Tree is not a valid hex number */
		return -EINVAL;

	do_fixup_by_path_u32(blob, "/", "tq,revision", rev, 1);

	return 0;
}

static int em_cb30_set_serial(void *blob)
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

	ret = em_cb30_set_revision(blob);
	if (ret)
		printf("Failed to set board revision in FDT: %d\n", ret);
	ret = em_cb30_set_serial(blob);
	if (ret)
		printf("Failed to set serial number in FDT: %d\n", ret);

	return 0;
}
#endif
