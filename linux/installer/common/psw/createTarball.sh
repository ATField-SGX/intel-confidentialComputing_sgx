#!/usr/bin/env bash
#
# Copyright(c) 2011-2026 Intel Corporation
# SPDX-License-Identifier: BSD-3-Clause
#

set -e

SCRIPT_DIR=$(dirname "$0")
ROOT_DIR="${SCRIPT_DIR}/../../../../"
BUILD_DIR="${ROOT_DIR}/build/linux"
LINUX_INSTALLER_DIR="${ROOT_DIR}/linux/installer"
LINUX_INSTALLER_COMMON_DIR="${LINUX_INSTALLER_DIR}/common"

INSTALL_PATH=${SCRIPT_DIR}/output

# Cleanup
rm -fr ${INSTALL_PATH}

# Get the architecture of the build from generated binary
get_arch()
{
    local a=$(readelf -h $BUILD_DIR/aesm_service | sed -n '2p' | awk '{print $6}')
    test $a = 01 && echo 'x86' || echo 'x64'
}

ARCH=$(get_arch)

# Get the configuration for this package
source ${SCRIPT_DIR}/installConfig.${ARCH}

# Fetch the gen_source script
cp ${LINUX_INSTALLER_COMMON_DIR}/gen_source/gen_source.py ${SCRIPT_DIR}

# Copy the files according to the BOM
python3 ${SCRIPT_DIR}/gen_source.py --bom=BOMs/psw_base.txt --deliverydir=${ROOT_DIR}
python3 ${SCRIPT_DIR}/gen_source.py --bom=BOMs/psw_${ARCH}.txt --deliverydir=${ROOT_DIR} --cleanup=false
python3 ${SCRIPT_DIR}/gen_source.py --bom=../licenses/BOM_license.txt --deliverydir=${ROOT_DIR} --cleanup=false

# Create the tarball
ECL_VER=$(awk '/ENCLAVE_COMMON_VERSION/ {print $3}' ${ROOT_DIR}/common/inc/internal/se_version.h|sed 's/^\"\(.*\)\"$/\1/')
LCH_VER=$(awk '/LAUNCH_VERSION/ {print $3}' ${ROOT_DIR}/common/inc/internal/se_version.h|sed 's/^\"\(.*\)\"$/\1/')
QEX_VER=$(awk '/QUOTE_EX_VERSION/ {print $3}' ${ROOT_DIR}/common/inc/internal/se_version.h|sed 's/^\"\(.*\)\"$/\1/')
URTS_VERSION=$(awk '/URTS_VERSION/ {print $3}' ${ROOT_DIR}/common/inc/internal/se_version.h|sed 's/^\"\(.*\)\"$/\1/')
pushd ${INSTALL_PATH} &> /dev/null
sed -i "s/ECL_VER=.*/ECL_VER=${ECL_VER}/" Makefile
sed -i "s/LCH_VER=.*/LCH_VER=${LCH_VER}/" Makefile
sed -i "s/QEX_VER=.*/QEX_VER=${QEX_VER}/" Makefile
sed -i "s/URTS_VER=.*/URTS_VER=${URTS_VERSION}/" Makefile
tar -zcvf ${TARBALL_NAME} *
popd &> /dev/null
