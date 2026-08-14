/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2026 TQ-Systems GmbH <license@tq-group.com>, D-82229 Seefeld, Germany.
 * Author: Michael Krummsdorf
 */

#ifndef __EM_SET_BOOTSYS_H
#define __EM_SET_BOOTSYS_H

#define EM_SET_BOOTSYS \
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
					"setenv mmcpart 2; " \
					"setenv raucslot 1; " \
					"setenv boot 1; " \
				"fi; " \
			"fi; " \
			"if test ! -n ${boot} && test x${BOOT_SLOT} = x2; then " \
				"if test ${BOOT_2_LEFT} -gt 0 ; then " \
					"setexpr BOOT_2_LEFT ${BOOT_2_LEFT} - 1; " \
					"echo Found valid slot 2, ${BOOT_2_LEFT} attempts remaining; " \
					"setenv mmcpart 3; " \
					"setenv raucslot 2; " \
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
		"fi\0"

#endif /* __EM_SET_BOOTSYS_H */
