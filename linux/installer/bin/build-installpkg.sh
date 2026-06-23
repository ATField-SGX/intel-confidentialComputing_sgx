#!/usr/bin/env bash
#
# Copyright(c) 2011-2026 Intel Corporation
#
# SPDX-License-Identifier: BSD-3-Clause
#

set -e

usage()
{
    echo "Usage : ./build-installpkg.sh [sdk|psw]"
    exit 1
}

[[ $# -le 1 ]] || usage

case "${1:-psw}" in
    sdk)
        echo "ERROR: The SDK installer is no longer supported for this SGX script."
        echo "       Please refer to https://github.com/intel-innersource/frameworks.security.confidential-computing.sgx.sdk/blob/main-internal/build_infrastructure/linux/installer/bin/build-installpkg.sh for the SDK installer script."
        exit 1
        ;;
    psw)
        ;;
    *)
        usage
        ;;
esac

SCRIPT_DIR=$(dirname "$0")
ROOT_DIR="${SCRIPT_DIR}/../../.."
LINUX_INSTALLER_COMMON_PSW_DIR="${ROOT_DIR}/linux/installer/common/psw"

# The result dir of the build
BUILD_DIR=${ROOT_DIR}/build/linux

# Get the architecture of the build from generated binary
get_arch()
{
    local a=$(readelf -h $BUILD_DIR/aesm_service | sed -n '2p' | awk '{print $6}')
    test $a = 01 && echo 'x86' || echo 'x64'
}

ARCH=$(get_arch)
PACKAGE_SUFFIX="$ARCH"

source ${LINUX_INSTALLER_COMMON_PSW_DIR}/installConfig.${PACKAGE_SUFFIX}
${LINUX_INSTALLER_COMMON_PSW_DIR}/createTarball.sh
cp  ${LINUX_INSTALLER_COMMON_PSW_DIR}/output/${TARBALL_NAME} ${SCRIPT_DIR}

trap "rm -f ${SCRIPT_DIR}/$TARBALL_NAME 2>/dev/null" 0

# Create the tarball and compute its MD5 check sum.
m=$(md5sum ${SCRIPT_DIR}/$TARBALL_NAME | awk '{print $1}')
v=$(awk '/STRFILEVER/ {print $3}' ${ROOT_DIR}/common/inc/internal/se_version.h|sed 's/^\"\(.*\)\"$/\1/')
TEMPLATE_FILE=$SCRIPT_DIR/install-sgx-psw.bin.tmpl
INSTALLER_NAME=$SCRIPT_DIR/sgx_linux_"${PACKAGE_SUFFIX}"_psw_"$v".bin
l=$(wc -l $TEMPLATE_FILE | awk '{print $1}')
l=$(($l+1))

sed -e "s:@linenum@:$l:" \
    -e "s:@md5sum@:$m:"  \
    -e "s:@arch@:$ARCH:"  \
    $TEMPLATE_FILE > $INSTALLER_NAME

cat ${SCRIPT_DIR}/${TARBALL_NAME} >> $INSTALLER_NAME
chmod +x $INSTALLER_NAME
echo "Generated psw installer: $INSTALLER_NAME"
exit 0
