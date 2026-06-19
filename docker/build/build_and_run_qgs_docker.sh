#!/bin/sh
#
# Copyright(c) 2020-2026 Intel Corporation
#
# SPDX-License-Identifier: BSD-3-Clause
#

set -e
docker build --target qgs --build-arg https_proxy=$https_proxy \
             --build-arg http_proxy=$http_proxy -t tdx_qgs -f ./tdx-qgs.dockerfile ../../

docker volume create --driver local --opt type=tmpfs --opt device=tmpfs --opt o=rw qgs-socket

# In-kernel driver (5.11+) uses /dev/sgx_enclave and /dev/sgx_provision.

# Add host SGX groups when present: "sgx" (systemd >= 248) and "sgx_prv"
# (Intel libsgx-ae-pce); each is skipped if absent (0666 nodes, no group).
GROUP_ARGS=""
for g in sgx sgx_prv; do
  gid=$(getent group "$g" 2>/dev/null | cut -d: -f3)
  [ -n "$gid" ] && GROUP_ARGS="$GROUP_ARGS --group-add $gid"
done

docker run --init --rm --name tdx_qgs --device=/dev/sgx_enclave --device=/dev/sgx_provision \
  $GROUP_ARGS \
  -v /dev/log:/dev/log -v qgs-socket:/var/run/tdx-qgs/ -it \
  --add-host=host.docker.internal:host-gateway tdx_qgs /opt/intel/tdx-qgs/qgs --no-daemon
