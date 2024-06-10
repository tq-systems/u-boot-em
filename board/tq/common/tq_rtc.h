/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2013-2024 TQ-Systems GmbH <u-boot@ew.tq-group.com>, D-82229 Seefeld, Germany.
 * Authors: Markus Niebel, Matthias Schiffer
 */

#ifndef __TQ_RTC__
#define __TQ_RTC__

#define TQ_PCF85063_CLKOUT_OFF 0x07

int tq_pcf85063_init_capacity(int bus, int address, int quartz_load);
int tq_pcf85063_init_clkout(int bus, int address, uint8_t clkout);
int tq_pcf85063_init_offset(int bus, int address, bool mode, int offset);

#endif /* __TQ_RTC_H */
