# SPDX-License-Identifier: GPL-2.0-or-later
#
# Copyright (c) 2016 - 2022 TQ-Systems GmbH <u-boot@ew.tq-group.com>,
# D-82229 Seefeld, Germany.
# Author: Markus Niebel
#

# necessary to create built-in.o
obj- := __dummy__.o
ifndef CONFIG_SPL_BUILD
obj-$(CONFIG_$(SPL_)TQ_BDI) += tq_bdi.o
obj-$(CONFIG_$(SPL_)TQ_ETH) += tq_eth.o
obj-$(CONFIG_$(SPL_)TQ_RTC) += tq_rtc.o
endif
