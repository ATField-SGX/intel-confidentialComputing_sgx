#
# Copyright(c) 2011-2026 Intel Corporation
#
# SPDX-License-Identifier: BSD-3-Clause
#

# This Makefile will compile PSW to generate various components
# including:
#
#  - uae_service dynamice library
#  - AESM and LE
# 
include ../buildenv.mk

.PHONY: all uae_service AE clean rebuild

all: uae_service AE

uae_service:
	$(MAKE) -C uae_service/linux/

#Note: AE's have a *runtime* dep on enclave_runtime (uRTS), i.e. 'make -C ../ enclave_runtime'; Not needed for build
AE:
	$(MAKE) -C ae/

clean:
	$(MAKE) -C ae/                 clean
	$(MAKE) -C uae_service/linux/  clean

rebuild:
	$(MAKE) clean
	$(MAKE) all

	
