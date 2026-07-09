#!/usr/bin/env bash
#
# Copyright(c) 2011-2026 Intel Corporation
#
# SPDX-License-Identifier: BSD-3-Clause
#

# The script is to automatically prepare the reproducible code, build docker image and launch the build
# in the docker container.
#
# Usage:
#     ./build_and_launch_docker.sh [ [ -d | --code-dir dir ] [ -t | --reproduce-type type ] | [ -i | --sdk-installer installer ] | [ -s | --sgx-src-dir src_dir ] [ -h | --help ] ]
#
# Options:
#     -d, --code-dir:
#         Specify the directory you want to download the repo. If this option is
#         not specified, will use the same directory as the script location.
#
#     -t, --reproduce-type:
#         Specify the reproducibility type. Provided options: all|sdk|ae|ipp.
#         If one type is provided, the corresponding code will be prepared. And the correponding
#         build steps will also be launched in the container automatically.
#         If no type is provided, all the code will be prepared. And the build steps will
#         be triggered in the container. Then you can choose to build what you want in the container.
#
#    -i, --sdk-installer:
#         Specify the SDK installer used for AE reproducibility. If this option is not specified,
#         script will download the default SDK installer.
#
#    -s, --sgx-src-dir:
#         Specify the local sgx source path if you have pulled the sgx source code via `$git clone`
#         or by other ways.
#         If this option is specified, script will not clone sgx source but start the build based on
#         the code base specified by this option.
#
#     -h, --help:
#         Show this usage message.
#
#

set -e

script_dir="$( cd "$( dirname "$0" )" >> /dev/null 2>&1 && pwd )"
code_dir="$script_dir/code_dir"
sgx_repo="$code_dir/sgx"
type="all"
type_flag=0
mount_dir="/linux-sgx"

sdk_installer=""
sgx_src=""

default_sdk_installer=sgx_linux_x64_sdk_reproducible_2.30.100.1.bin
default_sdk_installer_url=https://download.01.org/intel-sgx/sgx-linux/2.30/distro/nix_reproducibility/$default_sdk_installer


usage()
{
    echo "
    The script is to automatically prepare the reproducible code, build docker image and launch the build
    in the docker container.

    Usage:
        $0 [ [ -d | --code-dir dir ] [ -t | --reproduce-type type ] | [ -i | --sdk-installer installer ] | [ -s | --sgx-src-dir src_dir ] [ -h | --help ] ]

    Options:
        -d, --code-dir:
            Specify the directory you want to prepare the code and share to the reproducible container.
            If this option is not specified, will use the same directory as the script location.
        -t, --reproduce-type:
            Specify the reproducibility type. Provided options: all|sdk|ae|ipp.
            If one type is provided, the corresponding code will be prepared. And the correponding
            build steps will also be executed in the container automatically.
            If no type is provided, all the code will be prepared. And the build steps will not
            be triggered in the container. Then you can choose to build what you want in the container.
        -i, --sdk-installer:
            Specify the SDK installer used for AE reproducibility.
            If this option is not provided, script will choose the default SDK installer to build AEs.
            Only valid when the reproduce type is 'ae'.
        -s, --sgx-src-dir:
            Specify the local sgx source path if you have pulled the sgx source code via \`\$git clone\`
            or by other ways.
            If this option is specified, script will not clone sgx source but start the build based on
            the code base specified by this option.
        -h, --help:
            Show this usage message."
}

parse_cmd()
{
    while [ "$1" != "" ]; do
        case $1 in
            -d | --code-dir ) shift
                code_dir="$1"
                ;;
            -t | --reproduce-type ) shift
                type="$1"
                type_flag=1
                if [ "$type" != "all" ] && [ "$type" != "sdk" ] && [ "$type" != "ae" ] && [ "$type" != "ipp" ]; then
                    usage
                    exit 1
                fi
                ;;
            -h | --help )
                usage
                exit
                ;;
             -i | --sdk-installer ) shift
                sdk_installer="$1"
                if [ ! -f "$sdk_installer" ]; then
                    echo "The $sdk_installer doesn't exist."
                    usage
                    exit 1
                fi
                sdk_installer="$(realpath $sdk_installer)"
                ;;
            -s | --sgx-src-dir) shift
                sgx_src="$1"
                if [ ! -d "$sgx_src" ]; then
                    echo "The $sgx_src doesn't exist."
                    usage
                    exit 1
                fi
                sgx_src="$(realpath $sgx_src)"
                ;;
            * )
                usage
                exit 1
        esac
        shift
    done
    if [ "$type" != "ae" ] && [ $type_flag == 1 ] && [ "$sdk_installer" != "" ]; then
        echo -e "\n   ERROR: Option '--sdk-installer' is valid only if '--reproduce-type' is 'ae'."
        usage
        exit 1
    fi
    mkdir -p "$code_dir" | exit
    code_dir="$(realpath $code_dir)"
    sgx_repo="$code_dir/sgx"
}

prepare_sgx_src()
{
    pushd .
    if [ -d $sgx_repo ]; then
        echo "Removing existing SGX code repo in $sgx_repo"
        rm -rf $sgx_repo
    fi

    # If user prepares the sgx code repo in the host machine, copy the code to $sgx_repo
    # Otherwise, pull the sgx source code.
    if [ "$sgx_src" != "" ]; then
        mkdir -p "$sgx_repo" && cp -a "$sgx_src/." "$sgx_repo"
    else
        git clone -b sgx_2.30_reproducible https://github.com/intel/confidential-computing.sgx.git $sgx_repo
    fi

    cd "$sgx_repo" && make preparation
    popd

}

prepare_sdk_installer()
{
    # Used for 'ae' type repreducibility.
    # If user prepares the sdk installer, we copy it to the right place
    # Otherwise, we download one from 01.org
    if [ "$sdk_installer" != "" ]; then
        chmod +x "$sdk_installer" && cp "$sdk_installer" "$code_dir"
    else
        cd $code_dir && wget $default_sdk_installer_url && chmod +x $default_sdk_installer && cd -
    fi
}

# generate_cmd_script <cmd_file> <start_script_basename> <build_type>
# Writes a container entry script that sources the NIX profile and runs the
# given reproducible start script (found at $mount_dir/<basename>) for the
# given build type.
generate_cmd_script()
{
    local cmd_file="$1"
    local start_script="$2"
    local build_type="$3"
    rm -f "$cmd_file"

    cat > "$cmd_file" << EOF
#!/usr/bin/env bash

. ~/.bash_profile
nix-shell ~/shell.nix --run "$mount_dir/$start_script $build_type"

EOF

    chmod +x "$cmd_file"
}

######################################################
# Step 1: Parse command line, prepare code and scripts
######################################################
parse_cmd $@

case $type in
    "all")
        prepare_sgx_src
        ;;
    "sdk")
        prepare_sgx_src
        ;;
    "ae")
        prepare_sgx_src
        prepare_sdk_installer
        ;;
    "ipp")
        prepare_sgx_src
        ;;
    *)
        echo "Unsupported reproducibility type."
        exit 1
esac

# ---------------------------------------------------------------------------
# Reproducible-build path note (bit-for-bit equality with the SDK repo build)
# ---------------------------------------------------------------------------
# IPP and the SDK must be built with the SDK source physically rooted at
# "$mount_dir/sgx" (i.e. /linux-sgx/sgx), exactly as the standalone SDK-repo
# reproducible build does. If the SDK is instead built from the full SGX repo
# (where it lives under sgx/sdk/), the extra "sdk/" path segment gets baked into
# DWARF/debug strings, IPP generated-asm source paths and archive member names,
# breaking byte-for-byte equality with the SDK-repo artifacts even though the
# emitted code is identical.
#
# We therefore build in up to two container passes:
#   Pass 1 (SDK + IPP): bind-mount the SDK submodule subtree ($code_dir/sgx/sdk)
#       at "$mount_dir/sgx" and reuse the SDK submodule's OWN reproducible start
#       script, so the build is identical to the standalone SDK-repo build.
#   Pass 2 (AE): bind-mount the full SGX repo at "$mount_dir" (unchanged); AEs
#       are not built by the SDK repo, so there is nothing to match for them.
# Both passes write artifacts to the shared "$code_dir/out".

# AE / interactive start script (used with the full SGX repo mounted at $mount_dir).
cp "$script_dir/start_build.sh.tmpl" "$code_dir/start_build.sh"
chmod +x "$code_dir/start_build.sh"

# SDK/IPP start script: use the SDK submodule's own reproducible build script so
# the SDK + IPP build steps are byte-for-byte identical to the standalone
# SDK-repo reproducible build.
sdk_start_src="$sgx_repo/sdk/build_infrastructure/linux/reproducibility/start_build.sh.tmpl"
if [ ! -f "$sdk_start_src" ]; then
    echo "ERROR: SDK submodule reproducible start script not found:"
    echo "       $sdk_start_src"
    echo "       Ensure the sdk/ submodule is checked out ('make preparation')."
    exit 1
fi
cp "$sdk_start_src" "$code_dir/start_build_sdk.sh"
chmod +x "$code_dir/start_build_sdk.sh"

# Shared output directory (created host-side so the bind mount is host-owned).
mkdir -p "$code_dir/out"

######################################################
# Step 2: Build docker image and launch the container(s)
######################################################
# Check if the image already exists. If not, build the docker image
set +e && docker image inspect sgx.build.env:latest > /dev/null 2>&1 && set -e
if [ $? != 0 ]; then
    docker build -t sgx.build.env --build-arg https_proxy=$https_proxy \
              --build-arg http_proxy=$http_proxy -f $script_dir/Dockerfile .
fi

# WAMR cleanup: `version.h` is an auto-generated file (via CMake's `configure_file()` which is committed to the 3rd party repo and not cleaned up by its `clean` target.
# Deleting it ahead of build, so that subsequent container operations running on the bind-mounted tree, possibly with a different UID, do not encounter permission issues unlinking/replacing this file.
rm -f "${sgx_repo}/external/dcap_source/external/wasm-micro-runtime/core/version.h"

# Allow 'w' permission for other users to the code_dir in case the uid in the container
# is different from the host uid.
chmod -R o+w $code_dir

# Choose whether to allocate a pseudo-TTY for `docker run`.
# - Interactive terminal runs should use `-it` for a good UX.
# - Non-interactive environments (CI, nohup, stdout/stderr redirected to a file, cron)
#   do not have a TTY, and `docker run -t` will fail with:
#     "the input device is not a TTY"
DOCKER_RUN_ARGS=()
if [ -t 0 ] && [ -t 1 ]; then
  DOCKER_RUN_ARGS+=(-it)
fi

# Pass 1 - SDK + IPP, with the SDK subtree rooted at $mount_dir/sgx so embedded
# paths match the SDK-repo build exactly. $1 is the SDK-side build type
# (all = ipp+sdk | sdk | ipp).
run_sdk_ipp_pass()
{
    local sdk_type="$1"
    generate_cmd_script "$code_dir/cmd_sdk.sh" "start_build.sh" "$sdk_type"
    docker run "${DOCKER_RUN_ARGS[@]}" \
        -v "$code_dir/sgx/sdk:$mount_dir/sgx" \
        -v "$code_dir/out:$mount_dir/out" \
        -v "$code_dir/start_build_sdk.sh:$mount_dir/start_build.sh" \
        -v "$code_dir/cmd_sdk.sh:$mount_dir/cmd.sh" \
        --network none --rm sgx.build.env \
        /bin/bash -c "$mount_dir/cmd.sh"
}

# Pass 2 - AEs, with the full SGX repo mounted at $mount_dir (path embedding
# unchanged). Consumes the SDK installer produced by pass 1 from $code_dir/out.
run_ae_pass()
{
    generate_cmd_script "$code_dir/cmd.sh" "start_build.sh" "ae"
    docker run "${DOCKER_RUN_ARGS[@]}" \
        -v "$code_dir:$mount_dir" \
        --network none --rm sgx.build.env \
        /bin/bash -c "$mount_dir/cmd.sh"
}

# Interactive - no reproduce-type requested: drop into the container with the
# full SGX repo mounted so the user can build whatever they want by hand.
run_interactive()
{
    docker run "${DOCKER_RUN_ARGS[@]}" \
        -v "$code_dir:$mount_dir" \
        --network none --rm sgx.build.env
}

if [ $type_flag = 0 ]; then
    run_interactive
else
    case $type in
        "ipp")
            run_sdk_ipp_pass ipp
            ;;
        "sdk")
            run_sdk_ipp_pass sdk
            ;;
        "all")
            run_sdk_ipp_pass all
            run_ae_pass
            ;;
        "ae")
            run_ae_pass
            ;;
        *)
            echo "Unsupported reproducibility type."
            exit 1
            ;;
    esac
fi
