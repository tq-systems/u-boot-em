# SPDX-License-Identifier: GPL-2.0-or-later
#
# Copyright 2023 TQ-Systems GmbH
#


ifndef CONFIG_SPL_BUILD
obj-y += em4xx.o
else
obj-y += spl.o
obj-$(CONFIG_IMX8MN_EM4XX_MEMORY_512M) += em4xx-512mb-lpddr4_timing.o
obj-$(CONFIG_IMX8MN_EM4XX_MEMORY_1G) += em4xx-1gb-lpddr4_timing.o
endif
