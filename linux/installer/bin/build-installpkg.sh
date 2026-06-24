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
        # TOMBSTONE: the legacy PSW binary (.bin) installer has been removed.
        # The Linux PSW has been delivered exclusively via DEB/RPM packages since
        # 2019 (the last .bin installer published was 2.7.1 for RHEL/Fedora/SUSE
        # and 2.2 for Ubuntu). The .bin shipped the AESM out-of-process quoting
        # service; its direct replacement is the full PSW AESM runtime aggregate.
        echo "ERROR: The PSW binary (.bin) installer is no longer supported."
        echo "       The Linux PSW is delivered via DEB/RPM packages; build the AESM"
        echo "       runtime aggregate instead (e.g. 'make deb_psw_aesm_pkg' or"
        echo "       'make rpm_psw_aesm_pkg')."
        exit 1
        ;;
    *)
        usage
        ;;
esac
