// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2026 TQ-Systems GmbH <license@tq-group.com>, D-82229 Seefeld, Germany. All rights reserved.
 * Author: Michael Krummsdorf
 */

#ifndef __TQ_BDI_H__
#define __TQ_BDI_H__

#include <stddef.h>

struct bd_info;

#if defined(CONFIG_OF_BOARD_SETUP)
int ft_board_setup(void *blob, struct bd_info *bd);
#endif

void tq_bdi_print_hw_info(void);
int tq_bdi_print_bootinfo(void);

#endif /* __TQ_BDI_H__ */
