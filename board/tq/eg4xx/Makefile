## SPDX-License-Identifier: GPL-2.0-or-later
##
## Copyright (c) 2026 TQ-Systems GmbH <u-boot@ew.tq-group.com>,
## D-82229 Seefeld, Germany.
## Author: Paul Gerber
##

ifndef CONFIG_SPL_BUILD
obj-y += eg4xx.o
else
obj-y += spl.o
obj-$(CONFIG_IMX8MN_EG4XX_MEMORY_512M) += eg4xx-512mb-lpddr4_timing.o
endif
