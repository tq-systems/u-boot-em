// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2020 - 2022 TQ-Systems GmbH <u-boot@ew.tq-group.com>,
 * D-82229 Seefeld, Germany.
 * Author: Markus Niebel
 */

#include <common.h>
#include <i2c.h>

#include "tq_rtc.h"

#define PCF85063_REG_CTRL1		0x00 /* status */
#define PCF85063_REG_CTRL1_CAP_SEL	BIT(0)

#define PCF85063_REG_CTRL2		0x01
#define PCF85063_REG_CTRL2_CLKOUT_MASK	0x07

#define PCF85063_REG_OFFSET		0x02
#define PCF85063_REG_OFFSET_OFFSET_MASK	0x7f
#define PCF85063_REG_OFFSET_MODE	BIT(7)

int tq_pcf85063_init_capacity(int bus, int address, int quartz_load)
{
	struct udevice *dev;
	int ret;
	u8 val;

	ret = i2c_get_chip_for_busnum(bus, address, 1, &dev);
	if (ret)
		return ret;

	val = 0;
	/* Set Bit 0 of Register 0 of RTC to adjust to 12.5 pF */
	switch (quartz_load) {
	default:
		printf("Unknown quartz load %d. Assuming 7000", quartz_load);
		fallthrough;
	case 7000:
		break;
	case 12500:
		val |= PCF85063_REG_CTRL1_CAP_SEL;
		break;
	}

	ret = dm_i2c_reg_write(dev, PCF85063_REG_CTRL1, val);

	return ret;
}

int tq_pcf85063_init_clkout(int bus, int address, uint8_t clkout)
{
	struct udevice *dev;
	int ret;
	u8 val;

	if (clkout > 0x07)
		return -EINVAL;

	ret = i2c_get_chip_for_busnum(bus, address, 1, &dev);
	if (ret)
		return ret;

	val = clkout & PCF85063_REG_CTRL2_CLKOUT_MASK;
	ret = dm_i2c_reg_write(dev, PCF85063_REG_CTRL2, val);

	return ret;
}

int tq_pcf85063_init_offset(int bus, int address, bool mode, int offset)
{
	struct udevice *dev;
	int ret;
	u8 val;

	if (offset < -64 || offset > 63)
		return -EINVAL;

	ret = i2c_get_chip_for_busnum(bus, address, 1, &dev);
	if (ret)
		return ret;

	val = ((uint8_t)offset) & PCF85063_REG_OFFSET_OFFSET_MASK;
	if (mode)
		val |= PCF85063_REG_OFFSET_MODE;
	ret = dm_i2c_reg_write(dev, PCF85063_REG_OFFSET, val);

	return ret;
}
