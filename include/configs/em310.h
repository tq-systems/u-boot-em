/*
 * EM300 (Energymanager 300)
 * Copyright (C) 2017, TQ Systems
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

#ifndef __CONFIGS_EM310_H__
#define __CONFIGS_EM310_H__

/* Extra Environment */
#define CONFIG_EXTRA_ENV_SETTINGS \
	CONFIG_EXTRA_ENV_SETTINGS_COMMON \
	\
	"fdtfile=imx28-em310.dtb\0" \
	"hwtype=em310\0" \

/* The rest of the configuration is shared */
#include <configs/em3x0.h>
#include <configs/mxs.h>

#endif /* __CONFIGS_EM310_H__ */
