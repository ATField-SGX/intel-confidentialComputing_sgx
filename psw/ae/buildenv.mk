#
# Copyright(c) 2011-2026 Intel Corporation
#
# SPDX-License-Identifier: BSD-3-Clause
#

ENV := $(strip $(wildcard $(TOP_DIR)/buildenv.mk))

SGX_MODE ?= HW

ifeq ($(ENV),)
    $(error "Can't find $(TOP_DIR)/buildenv.mk")
endif

include $(TOP_DIR)/buildenv.mk

WORK_DIR := $(shell pwd)
AENAME   := $(notdir $(WORK_DIR))
SONAME  := $(AENAME).so
ifdef DEBUG
CONFIG   := config_debug.xml
else
CONFIG   := config.xml
endif
EDLFILE  := $(wildcard *.edl)

ifneq ($(SGX_MODE), HW)
	URTSLIB := -lsgx_urts_sim
	TRTSLIB := -lsgx_trts_sim
else
	URTSLIB := -lsgx_urts
	TRTSLIB := -lsgx_trts
endif
EXTERNAL_LIB := -lsgx_tservice

EXTERNAL_LIB += -lsgx_tstdc -lsgx_tcrypto -lsgx_tcxx

# SDK_COMMON_DIR paths are needed for headers (e.g. sgx_random_buffers.h, se_types.h)
# that moved from common/inc/ to sdk/common/inc/ after SDK submodule extraction.
INCLUDE := -I$(LINUX_PSW_DIR)/ae/inc                   \
           -I$(SGX_HEADER_DIR)                         \
           -I$(SGX_HEADER_DIR)/tlibc                   \
           -I$(COMMON_DIR)/inc                         \
           -I$(COMMON_DIR)/inc/internal                \
           -I$(SDK_COMMON_DIR)/inc                     \
           -I$(SDK_COMMON_DIR)/inc/internal

SGXSIGN   := $(SGX_BIN_DIR)/sgx_sign
EDGER8R   := $(SGX_BIN_DIR)/sgx_edger8r

CXXFLAGS  += $(ENCLAVE_CXXFLAGS)
CFLAGS    += $(ENCLAVE_CFLAGS)

LDTFLAGS  = -L$(SGX_LIB_DIR) -Wl,--whole-archive $(TRTSLIB) -Wl,--no-whole-archive \
            -Wl,--start-group $(EXTERNAL_LIB) -Wl,--end-group -Wl,--build-id       \
            -Wl,--version-script=$(ROOT_DIR)/build-scripts/enclave.lds $(ENCLAVE_LDFLAGS)

LDTFLAGS += -Wl,-Map=out.map -Wl,--undefined=version -Wl,--gc-sections

DEFINES := -D__linux__

vpath %.cpp $(COMMON_DIR)/src:$(LINUX_PSW_DIR)/ae/common

.PHONY : version

version.o: $(LINUX_PSW_DIR)/ae/common/version.cpp
	$(CXX) $(CXXFLAGS) -fno-exceptions -fno-rtti $(INCLUDE) $(DEFINES) -c $(LINUX_PSW_DIR)/ae/common/version.cpp -o $@
