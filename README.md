

Intel&reg; Software Guard Extensions for Linux\* OS
================================================

- [Intel® Software Guard Extensions for Linux\* OS](#intel-software-guard-extensions-for-linux-os)
  - [Introduction](#introduction)
  - [License](#license)
  - [Contributing](#contributing)
  - [Documentation](#documentation)
  - [Downloads](#downloads)
  - [Quick Start](#quick-start)
  - [Supported Operating Systems](#supported-operating-systems)
  - [Building](#building)
    - [Prerequisites](#prerequisites)
    - [Build the Intel® SGX SDK](#build-the-intel-sgx-sdk)
    - [Build the Intel® SGX PSW](#build-the-intel-sgx-psw)
  - [Installation](#installation)
    - [Install the Intel® SGX SDK](#install-the-intel-sgx-sdk)
      - [Sample applications](#sample-applications)
    - [Install the Intel® SGX PSW](#install-the-intel-sgx-psw)
      - [Intel SGX installation guide](#intel-sgx-installation-guide)
      - [Package manager (recommended)](#package-manager-recommended)
      - [Upgrading from legacy packages across releases](#upgrading-from-legacy-packages-across-releases)
      - [Configure the installation: optional dependencies](#configure-the-installation-optional-dependencies)
      - [ECDSA attestation](#ecdsa-attestation)
      - [Manage the aesmd service](#manage-the-aesmd-service)
  - [Reproducibility](#reproducibility)


Introduction
------------
Intel&reg; Software Guard Extensions (Intel&reg; SGX) is an Intel technology for application developers seeking to protect select code and data from disclosure or modification.

The Linux\* Intel&reg; SGX software stack is comprised of the Intel&reg; SGX driver<sup> [_see note_](#note-driver)</sup>, the Intel&reg; SGX SDK and the Intel&reg; SGX Platform Software (PSW).

<a id="note-sdk-split"></a>

> [!NOTE]
> ### :jigsaw: The Intel<sup>&reg;</sup> SGX SDK now lives in its own repository
> 
> Starting with [release 2.30](https://github.com/intel/confidential-computing.sgx/releases/tag/sgx_2.30), the _SDK_ (including _samples_) as well as the SGX _enclave runtime_ libraries are maintained in [:octocat: confidential-computing.sgx.sdk](https://github.com/intel/confidential-computing.sgx.sdk) and vendored here as the [`sdk` submodule](sdk).[^sdk-split]
>  - The `sdk-extraction-2.30` *tag* captures the same source state ([ :label:  SGX](https://github.com/intel/confidential-computing.sgx/tree/sdk-extraction-2.30) ↔ [:label:  SGX SDK](https://github.com/intel/confidential-computing.sgx.sdk/tree/sdk-extraction-2.30)) at split.

[^sdk-split]: This is a source-layout change only: the consolidated build is unchanged (`make` targets from _this_ repository continue to build all the components), and the released packages, installers and download locations are not affected.

<a id="note-driver"></a>

> [!NOTE]
> The Linux\* kernel [includes the SGX driver](https://docs.kernel.org/arch/x86/sgx.html) since mainline release [`5.11`](https://git.kernel.org/torvalds/c/5583ff677b3108cde989b6d4fd1958e091420c0c) — no separate driver installation is needed on modern kernels. Device nodes are at `/dev/{sgx_enclave, sgx_provision}`. The platform must support and have [Flexible Launch Control](https://cc-enabling.trustedservices.intel.com/intel-sgx-sw-installation-guide-linux/02/installation_instructions/#driver-installation) configured.

<a id="related-projects"></a>

**Related Projects**

The [intel-device-plugins-for-kubernetes](https://github.com/intel/intel-device-plugins-for-kubernetes) project enables users to run container applications running Intel&reg; SGX enclaves in Kubernetes clusters. It also gives instructions how to set up ECDSA based attestation in a cluster.


The [intel-sgx-ssl](https://github.com/intel/intel-sgx-ssl) project provides a full-strength general purpose cryptography library for Intel&reg; SGX enclave applications. It is based on the underlying OpenSSL* Open Source project.

- Intel&reg; SGX SDK provides a [build combination](https://github.com/intel/confidential-computing.sgx.sdk#build-the-intel-sgx-sdk) to build out a SGXSSL-based SDK.
- Users can also use this cryptography library in SGX enclave applications separately.

The [Intel&reg; SGX Data Center Attestation Primitives (DCAP)](https://github.com/intel/confidential-computing.tee.dcap) project provides libraries and tools for ECDSA-based remote attestation targeted for data centers, cloud service providers, and enterprises, including the Quoting Library, PCK Certificate Caching Service (PCCS), Quote Verification Library, and more.


License
-------
See [License.txt](License.txt) for details.

Contributing
-------
See [CONTRIBUTING.md](CONTRIBUTING.md) for details.

For questions and general discussion, the [Intel&reg; SGX community forum](https://community.intel.com/t5/Intel-Software-Guard-Extensions/bd-p/software-guard-extensions) is a good place to start.

Documentation
-------------
- [Intel&reg; SGX for Linux\* OS](https://www.intel.com/content/www/us/en/developer/tools/software-guard-extensions/linux-overview.html) project home page on [Intel Developer Zone](https://www.intel.com/content/www/us/en/developer/overview.html)
- [Intel&reg; Confidential Computing Enabling documentation](https://cc-enabling.trustedservices.intel.com/)
- [Intel&reg; SGX for Linux\* OS — release documents](https://download.01.org/intel-sgx/latest/linux-latest/docs/)
  - [Developer Guide](https://download.01.org/intel-sgx/latest/linux-latest/docs/Intel_SGX_Developer_Guide_for_Linux.pdf) — enclave design patterns, sealing, attestation concepts
  - [Developer Reference](https://download.01.org/intel-sgx/latest/linux-latest/docs/Intel_SGX_Developer_Reference_for_Linux_OS.pdf) — SDK/EDL API reference (edger8r, trts/urts)
- Intel&reg; SGX Programming Reference[^sgx-progref]
  - [SDM Vol 3D: System Programming Guide, Part 4](https://www.intel.com/content/www/us/en/content-details/916745/intel-64-and-ia-32-architectures-software-developer-s-manual-volume-3d-system-programming-guide-part-4.html) — SGX architecture, data structures, ENCLS/ENCLU leaf function reference
  - [SDM Vol 2: Instruction Set Reference](https://www.intel.com/content/www/us/en/content-details/916605/intel-64-and-ia-32-architectures-software-developer-s-manual-combined-volumes-2a-2b-2c-and-2d-instruction-set-reference-a-z.html) — ENCLS/ENCLU instruction opcode encoding

[^sgx-progref]: The [original standalone doc (329298)](https://www.intel.com/content/dam/develop/external/us/en/documents/329298-002-629101.pdf) has been absorbed into the Intel&reg; Software Developer's Manual (SDM); the two SDM volumes above are the current authoritative references.

Downloads
---------
Pre-built packages for Linux are published on [01.org](https://download.01.org/intel-sgx/latest/):
- [Intel&reg; SGX PSW and SDK](https://download.01.org/intel-sgx/latest/linux-latest/) — distro packages and the SDK installer binary
- [Intel&reg; SGX DCAP](https://download.01.org/intel-sgx/latest/dcap-latest/linux/) — quoting and verification libraries

Packages can be installed directly, or via a **live repository** (Intel-hosted) or a **local repository** assembled from the packages above. Both approaches and the full installation procedure are covered in the [Intel&reg; SGX Software Installation Guide for Linux\*](https://cc-enabling.trustedservices.intel.com/intel-sgx-sw-installation-guide-linux/).

**Note:** Packages and repository metadata published by Intel are signed with the [GPG package signing key](https://download.01.org/intel-sgx/sgx_repo/ubuntu/intel-sgx-deb.key) (fingerprint: <!--sgx-key-fingerprint-->`1504 34D1 488B F803 08B6 9398 E5C7 F0FA 1C6C 6C3C`<!--/sgx-key-fingerprint-->, "Intel(R) Software Development Products").

> [!NOTE]
> **Windows\*:** The Intel&reg; SGX SDK, DCAP, and Platform Software for Windows\* are distributed via the [Intel&reg; License &amp; Registration Center](https://lemcenter.intel.com/forms/?productId=ps_3074) (login required; the product suite covers the SDK, DCAP, and PSW). 
> See the [Getting Started with Intel&reg; SGX SDK for Windows\*](https://www.intel.com/content/www/us/en/developer/articles/guide/getting-started-with-sgx-sdk-for-windows.html) guide for setup instructions.
> Individual packages (SDK, headers, enclave common API, DCAP components) are also available on [NuGet](https://www.nuget.org/packages?q=sgx+owner%3AIntel).

Quick Start
-----------

Clone this repository, then use Docker and Docker Compose to get up and running quickly.
Two modes are available — pick the one that fits your goal.

- **Build from source** — compiles the Intel® SGX PSW and SDK inside Docker, then runs a sample enclave.
  See [docker/build/README.md](docker/build/README.md) for details.
  ```bash
  # 'make preparation': apply patches and download prebuilt dependencies
  make preparation \
    && cd docker/build && ./build_compose_run.sh
  ```

- **Use pre-built packages** — downloads the Intel® SGX PSW and SDK from 01.org (no local build required) and deploys a sample SGX application.
  See [linux/installer/docker/README.md](linux/installer/docker/README.md) for details.
  ```bash
  cd linux/installer/docker && ./build_compose_run.sh
  ```


Supported Operating Systems
---------------------------
The following Linux\* distributions are supported for both building and installing the Intel&reg; SGX software stack:

- Ubuntu\* Server (64-bit): 22.04, 24.04, 26.04 LTS
- Red Hat Enterprise Linux\* Server (64-bit): 9.6, 9.8, 10.0, 10.2
- CentOS\* Stream (64-bit): 9, 10
- SUSE Linux Enterprise Server\* (64-bit): 15 SP7, 16
- Anolis\* OS (64-bit): 8.10
- Azure\* Linux (64-bit): 3.0
- Debian\* (64-bit): 10, 12, 13


Building
--------
### Prerequisites
- Ensure that you have one of the [supported operating systems](#supported-operating-systems).

- To build the Intel&reg; SGX SDK, install its build toolchain (gcc 7.3+ and glibc 2.27+ are required). On Ubuntu, for example:
  ```bash
  sudo apt-get install build-essential ocaml ocamlbuild automake autoconf \
      libtool wget python-is-python3 libssl-dev git cmake perl
  ```
  The full per-distribution list is in the [SDK repository](https://github.com/intel/confidential-computing.sgx.sdk#prerequisites).

- Install the additional tools required to build the Intel&reg; SGX PSW:
  1)  System libraries and build tools — install the packages for your distribution:
      * On Debian 10, Debian 12, Debian 13, Ubuntu 22.04, Ubuntu 24.04, and Ubuntu 26.04:
        ```bash
        sudo apt-get install libssl-dev libcurl4-openssl-dev protobuf-compiler \
            libprotobuf-dev debhelper cmake reprepro unzip pkgconf libboost-dev \
            libboost-system-dev libboost-thread-dev lsb-release libsystemd0
        ```
      * On Red Hat Enterprise Linux 9.6, 9.8, 10.0, and 10.2:
        ```bash
        sudo yum install openssl-devel libcurl-devel protobuf-devel cmake \
            rpm-build createrepo yum-utils pkgconf boost-devel \
            protobuf-lite-devel systemd-libs
        ```
      * On CentOS Stream 9 and 10:
        ```bash
        sudo dnf install openssl-devel libcurl-devel protobuf-devel cmake \
            rpm-build createrepo yum-utils pkgconf boost-devel \
            protobuf-lite-devel systemd-libs
        ```
      * On Anolis 8.10:
        ```bash
        sudo dnf --enablerepo=PowerTools install openssl-devel libcurl-devel \
            protobuf-devel cmake rpm-build createrepo yum-utils pkgconf \
            boost-devel protobuf-lite-devel systemd-libs
        ```
      * On Azure Linux 3.0:
        ```bash
        sudo dnf --enablerepo=powertools install openssl-devel libcurl-devel \
            protobuf-devel cmake rpm-build createrepo yum-utils pkgconf \
            boost-devel protobuf-lite-devel systemd-libs
        ```
      * On SUSE Linux Enterprise Server 15 SP7 and 16:
        ```bash
        sudo zypper install libopenssl-devel libcurl-devel protobuf-devel cmake \
            rpm-build createrepo_c libsystemd0 libboost_system1_66_0-devel \
            libboost_thread1_66_0-devel
        ```
  2) The Intel&reg; SGX SDK installer — required before building the PSW. Either:
     - **Build it** from this repo: `make sdk_install_pkg` (see [Build the Intel® SGX SDK](#build-the-intel-sgx-sdk) below), or
     - **Download a pre-built release** from [01.org](https://download.01.org/intel-sgx/latest/linux-latest/distro/):
       ```bash
       wget -r -l1 -np -nd --accept 'sgx_linux_x64_sdk_*.bin' \
           https://download.01.org/intel-sgx/latest/linux-latest/distro/<distro>/
       ```

     Then install it — same for both methods (the installer will prompt you to accept the license; adjust `--prefix` to your preferred location):
     ```bash
     ./sgx_linux_x64_sdk_*.bin --prefix=$(pwd)
     source $(pwd)/sgxsdk/environment
     ```

- If you haven't already cloned this repository, do so now and prepare the submodules and prebuilt binaries:
  ```bash
  git clone --recurse-submodules \
      https://github.com/intel/confidential-computing.sgx.git sgx-source
  cd sgx-source && make preparation
  ```
  `make preparation` initialises git submodules, applies patches, and runs `download_prebuilt.sh` to fetch prebuilt binaries.
  If you prefer to initialise submodules manually (e.g. to control depth or mirror URLs), do so before running `make preparation`:
  ```bash
  git submodule update --init --recursive
  ```

  > 💡 **Tip:** Both `git submodule update` and `download_prebuilt.sh` (`wget`) fetch over the network. If you are behind a proxy, set the relevant proxy variables first, e.g.:
  > ```bash
  > export https_proxy=http://proxy:port
  > export no_proxy=localhost,127.0.0.1
  > ```

  > ℹ️ **Note:** When pulling a new release, run `make distclean` first if patched submodules changed — this avoids patch conflicts (`error: Your local changes to the following files would be overwritten by checkout`).
  > **Caution:** this deletes all submodule working trees; commit any local changes first.
  > ```bash
  > make distclean
  > ```

- *(Debian 10 only)* The system `binutils` on Debian 10 lacks mitigation options support. Copy the patched tools downloaded by `make preparation` to `/usr/local/bin`:
  ```bash
  sudo cp external/toolset/debian10/* /usr/local/bin
  ```
  Repeat after any update to ensure the latest versions are used.


### Build the Intel&reg; SGX SDK
The Intel&reg; SGX SDK is maintained in the [confidential-computing.sgx.sdk](https://github.com/intel/confidential-computing.sgx.sdk) repository and vendored here as the `sdk` submodule. If you prefer to build everything in one place, it can be built from this repository:
```bash
make sdk              # LOAD + CF + no-mitigation passes (sdk_install_pkg runs this automatically)
make sdk_install_pkg  # bundles all three variants into one .bin installer
```
For build flavors (`USE_OPT_LIBS`, `DEBUG`, `make sdk_install_pkg_no_mitigation`), see the [SDK repository README](sdk/README.md#build-the-intel-sgx-sdk).

### Build the Intel&reg; SGX PSW
The Intel&reg; SGX Platform Software (PSW) covers the following components built from this repository:

- **SGX Runtime System (RTS, enclave runtime)** (`libsgx-urts`, `libsgx-enclave-common`) — the SGX loader and enclave-common API. Sourced from the [confidential-computing.sgx.sdk](https://github.com/intel/confidential-computing.sgx.sdk) repository (`sdk/enclave_runtime/`) via the `sdk` submodule.
- **Architectural Enclave Service Manager (AESM)** — the `aesmd` background daemon hosting the attestation enclaves (PCE, QE3, IDE), plus `libsgx-quote-ex`, the untrusted client library applications use to request quotes from the daemon, and `libsgx-uae-service`, a binary-compatibility shim over it for applications linked against the legacy Untrusted AE (UAE) API.

  > ℹ️ **Note:** The [DCAP](https://github.com/intel/confidential-computing.tee.dcap) quoting library (`libsgx-dcap-ql`) from `external/dcap_source/` can operate either **in-process** (loading QE3 directly into the app, no daemon required) or **out-of-process** (delegating to AESM via `libsgx-quote-ex`). In both modes, QPL (`libdcap_quoteprov`) is loaded at runtime to fetch PCK certificate collateral from PCCS/Intel PCS.

`make psw` builds both the enclave runtime (via `sdk_urts` from the SDK submodule) and the AESM service together.

- To build the Intel&reg; SGX PSW (both the runtime libraries as well as the AESM) with default configuration:
  ```bash
  make psw
  ```
  Tools and libraries are generated in the `build/linux` directory.

  > 💡 **Tip:** To build each part standalone:
  > - **Enclave runtime only**: [`make sdk_urts`](sdk/README.md#build-the-intel-sgx-enclave-runtime-packages) from the root (or `make urts` directly in `sdk/`).
  > - **AESM side only**: `make` directly in `psw/` — provided the enclave runtime has already been built.

- To build with debug information:
  ```bash
  make psw DEBUG=1
  ```
- To clean files generated by a previous `make psw`:
  ```bash
  make clean
  ```
- To build the Intel&reg; SGX PSW installer packages (`DEB` / `RPM`):

  > ℹ️ **Note:** The packaging bundles prebuilt Intel® Architecture Enclaves (**PCE**[^pce-psw], **QE3**, **IDE**), signed by Intel, downloaded by `make preparation`. To build any of these yourself (unsigned), e.g. PCE:
  > ```bash
  > cd psw/ae/pce && make
  > ```
  > To verify or reproduce them, see [Reproducibility](#reproducibility).
  >
  > [^pce-psw]: PCE is also copied into the PSW build output (`build/linux`) during `make psw`.
  >
  > [^dcap-ae]: QE3 (Quoting Enclave 3) and IDE (Identity Enclave) are part of the DCAP component and sourced from `external/dcap_source/QuoteGeneration/psw/ae/data/prebuilt/`.

  For packaging, run the command matching your package format:

  * **DEB-based systems** (Ubuntu 22.04/24.04/26.04, Debian 10/12):
    ```bash
    make deb_psw_pkg
    ```
    Packages are generated under `linux/installer/deb/` and cover the three components required to run SGX workloads and generate SGX ECDSA quotes:
    - **Enclave runtime** — `libsgx-urts`, `libsgx-enclave-common`
    - **AESM daemon and client libraries** — `sgx-aesm-service` (with PCE/ECDSA/quote-ex plugins), `libsgx-quote-ex`, `libsgx-uae-service`
    - **SGX DCAP quoting stack** — `libsgx-dcap-ql`, `libsgx-pce-logic`, `libsgx-qe3-logic`, QPL (`libsgx-dcap-default-qpl`), and the signed enclaves they drive (`libsgx-ae-pce`, `libsgx-ae-qe3`, `libsgx-ae-id-enclave`)

    Use `make deb_local_repo` to assemble a complete local repository that additionally includes TDX quoting, quote verification, appraisal, PCCS, and platform registration tools. The repository is generated under `linux/installer/deb/sgx_debian_local_repo` relative to the repo root. Register it and refresh apt — adjust the path if you moved or copied the repo elsewhere:

    ```bash
    echo "deb [trusted=yes arch=amd64] file:$PWD/linux/installer/deb/sgx_debian_local_repo $(. /etc/os-release && echo $VERSION_CODENAME) main" \
      | sudo tee /etc/apt/sources.list.d/sgx-local.list
    sudo apt update
    ```

    > 💡 **Tip:** To ensure locally built packages take precedence over any same-named packages from official repositories, pin the local repo to a higher priority:
    > ```bash
    > printf 'Package: *\nPin: origin ""\nPin-Priority: 1001\n' \
    >   | sudo tee /etc/apt/preferences.d/sgx-local
    > ```

    > ℹ️ **Note:** A `*-dbgsym_*.ddeb` package is always produced alongside each `.deb` by `dh_strip`. 
    > 
    > In a default (release) build it contains only minimal symbol information; use `DEBUG=1` to rebuild with `-O0 -ggdb` and get fully populated symbol packages:
    > ```bash
    > make deb_psw_pkg DEBUG=1
    > ```

    > **Debian 10 only**: the default `PATH` may not include `/sbin` — add it before building:
    > ```bash
    > export PATH=$PATH:/sbin
    > ```
  * **RPM-based systems** (RHEL 9.6/9.8/10.0/10.2, CentOS Stream 9/10, Anolis OS 8.10, SLES 15 SP7/16):
    ```bash
    make rpm_psw_pkg
    ```
    Use `make rpm_local_repo` to assemble a complete local repository that additionally includes TDX quoting, quote verification, appraisal, PCCS, and platform registration tools. The repository is generated under `linux/installer/rpm/sgx_rpm_local_repo` relative to the repo root. Register it with highest priority so locally built packages take precedence — adjust the path if you moved or copied the repo elsewhere:

    * On RHEL/CentOS Stream/Anolis:
      ```bash
      sudo dnf config-manager --add-repo file://$PWD/linux/installer/rpm/sgx_rpm_local_repo
      sudo dnf config-manager --save --setopt="*sgx_rpm_local_repo*.priority=1" \
                                     --setopt="*sgx_rpm_local_repo*.gpgcheck=0"
      ```
    * On SLES 15 SP7/16:
      ```bash
      sudo zypper addrepo --priority 1 --no-gpgcheck \
        $PWD/linux/installer/rpm/sgx_rpm_local_repo sgx-local
      ```

    > ℹ️ **Note:** To build with debug information:
    > ```bash
    > make rpm_psw_pkg DEBUG=1
    > ```

Installation
------------

### Install the Intel&reg; SGX SDK
The ``make sdk_install_pkg`` build above produces the installer at `linux/installer/bin/sgx_linux_x64_sdk_${version}.bin`. A typical install is:
```bash
cd linux/installer/bin
# omit --prefix to choose the install path interactively:
sudo ./sgx_linux_x64_sdk_${version}.bin --prefix /opt/intel
source /opt/intel/sgxsdk/environment   # set up the build environment
```
That covers the common case. The full installation manual — non-interactive options, custom prefixes, environment setup and the code samples — is maintained in the [SDK repository](https://github.com/intel/confidential-computing.sgx.sdk). To run the samples in hardware mode you also need the Intel&reg; SGX PSW installed (see [Install the Intel&reg; SGX PSW](#install-the-intel-sgx-psw) below).

#### Sample applications

The Intel&reg; SGX SDK sample enclaves and host applications live in the SDK source tree under [SampleCode/](sdk/SampleCode); they are also packaged by the SDK installer, so the same samples are available after installation. See the [SDK README](sdk/README.md#test-the-intel-sgx-sdk-package-with-the-code-samples) for simulation-mode and [hardware-mode](sdk/README.md#compile-and-run-the-code-samples-in-the-hardware-mode) walkthroughs.

For remote attestation and quoting flows, start with the DCAP samples, especially `QuoteGenerationSample` and `QuoteVerificationSample` in the repository's [`SampleCode/`](https://github.com/intel/confidential-computing.tee.dcap/tree/main/SampleCode) directory.

[^in-kernel-driver-info-note]: The Linux\* kernel contains the necessary driver since the mainline kernel release `5.11`.

### Install the Intel&reg; SGX PSW

Before installing, ensure that:
- Your system runs one of the [supported operating systems](#supported-operating-systems).
- Your hardware supports Intel&reg; SGX with Flexible Launch Control (FLC). This includes select Intel® Xeon® Scalable, Xeon® D, Xeon® E, Core™, and Atom® processors — see [Which Platforms Support Intel® SGX DCAP?](https://www.intel.com/content/www/us/en/support/articles/000057420/software/intel-security-products.html) for the full list.
- Intel SGX hardware mode is enabled in the firmware and a Linux\* kernel with SGX driver support is running[^in-kernel-driver-info-note].

Starting with release `2.8`, the PSW is split into smaller packages — install only the features you need. Use the local repo method where possible: the package manager resolves dependencies automatically.

> [!NOTE]
> **Removed in release 2.28.** All [legacy](https://community.intel.com/t5/Intel-Software-Guard-Extensions/IAS-End-of-Life-Announcement/td-p/1545831) EPID-based functionality has been removed, including EPID provisioning and attestation (QE/PVE) and platform services (PSE). Whitelist-based launch control and support for the deprecated <sup>[[ref1]](https://github.com/intel/confidential-computing.tee.dcap/blob/DCAP_1.23/driver/linux/README.md#important), [[ref2]](https://github.com/intel/linux-sgx-driver?tab=readme-ov-file#project-not-under-active-management)</sup> out-of-tree Linux kernel drivers have also been removed.

#### Intel SGX installation guide

For a full step-by-step walkthrough — including how to set up the Intel SGX package repository, alternative installation methods, and platform-specific notes — see the [Intel® SGX Software Installation Guide for Linux](https://download.01.org/intel-sgx/latest/linux-latest/docs/).

#### Package manager (recommended)

Install the top-level package for your use case — the package manager resolves all dependencies automatically:

| Package | Purpose | Depends | Recommends |
| --- | --- | --- | --- |
| `libsgx-urts` | SGX enclave loader and runtime | `libsgx-enclave-common` | — |
| `libsgx-dcap-ql` | DCAP ECDSA quoting — in-process by default; set `SGX_AESM_ADDR` to delegate to AESM | • **quoting orchestration** (host-side, untrusted wrappers): `libsgx-qe3-logic`, `libsgx-pce-logic`<br>• **architectural enclaves** (signed, runs in SGX): `libsgx-ae-qe3`, `libsgx-ae-id-enclave`, `libsgx-ae-pce`<br>• **enclave loader:** `libsgx-urts` | • `libsgx-dcap-quote-verify`<br>• `libsgx-quote-ex` (for AESM-delegated mode) |
| `libsgx-quote-ex` | Algorithm-agnostic quote client (`sgx_get_quote_ex` API, delegates to AESM via Unix socket);<br>**Note:** A separate package: `libsgx-uae-service` is a compat shim over it for apps linked against the older API| *(shlibs only)* | • `libsgx-aesm-quote-ex-plugin` |
| `libsgx-dcap-default-qpl` | Quote collateral provider — fetches collateral _(PCK certificates for quote generation, and/or reference values for quote verification)_ from [PCCS](https://github.com/intel/confidential-computing.tee.dcap.pccs) (self-hosted cache) or directly from Intel [PCS](https://api.portal.trustedservices.intel.com/provisioning-certification) | `libcurl4` | — |
| ***Notable supporting packages*** | | | |
| `libsgx-aesm-quote-ex-plugin` | AESM plugin: algorithm-agnostic quoting via QE3 | • `sgx-aesm-service`<br>• `libsgx-aesm-ecdsa-plugin` (pulls the same logic/AE/loader deps as `libsgx-dcap-ql`, routed via `libsgx-aesm-pce-plugin`) | — |
| `libsgx-dcap-quote-verify` | Quote verification (QVL) and appraisal (QAL) — both APIs exported from the same `.so`; for each, the caller selects either the hardware-attested mode _(running inside QVE/QAE respectively - both in-process)_ or software-only mode | *(shlibs only)* | • `libsgx-ae-qve` (QVE enclave image)<br>• `libsgx-urts` (required to load QVE or QAE)<br>• **Note:** ~~`libsgx-ae-qae`~~ — _not listed here_; must be installed manually for hardware-attested appraisal |

> [!IMPORTANT]
> `libsgx-dcap-default-qpl` is **not pulled in automatically by any package** — it is loaded at runtime via `dlopen("libdcap_quoteprov.so.1")` by `libsgx-qe3-logic` (in-process quoting), `libsgx-dcap-quote-verify` (in-process verification), and AESM (delegated path). 
> 
> Install it explicitly when live collateral fetching is needed.

> [!NOTE]
> `sgx-aesm-service` arrives automatically on a default installation via the _Recommends_ chain (`libsgx-aesm-quote-ex-plugin` → `sgx-aesm-service`).
> 
> `libsgx-quote-ex` always requires a running AESM daemon at runtime — it connects to the AESM Unix socket unconditionally; the _Recommends_ rather than _Depends_ is a packaging flexibility that allows the daemon to be provided externally _(e.g. a host socket bind-mounted into a container)_. To suppress it — for example when using only `libsgx-dcap-ql` in in-process mode — pass `--no-install-recommends` (see [Configure the installation](#configure-the-installation-optional-dependencies)).

> [!TIP]
> The package manager command depends on the distribution:
> - `sudo apt-get install` (Ubuntu, Debian),
> - `sudo dnf install` / `sudo yum install` (RHEL, CentOS Stream, Anolis OS, Azure Linux)
> - `sudo zypper install` (SLES).

> [!NOTE]
> Optionally install `*-dbgsym` (DEB) / `*-debuginfo` (RPM) packages for debug symbols, or `*-dev` (DEB) / `*-devel` (RPM) packages for development headers and static link libraries.

#### Upgrading from legacy packages across releases

When upgrading from a legacy installation to a newer release where packages were reorganised (files moved between packages, or a single package split into smaller ones), or when mixing release packages with locally built ones, the package manager may report a file conflict:

```
# DEB: dpkg: error processing archive ... (--unpack): trying to overwrite ...
# RPM: file ... from install of ... conflicts with file from package ...
```

To resolve this, choose one of:
* **Clean upgrade** (DEB and RPM) — uninstall the legacy packages first, then install the new ones. This is the recommended path for RPM-based systems.
* **In-place upgrade (DEB only)** — use `dist-upgrade` instead of `upgrade`, and pass `--force-overwrite` through to dpkg:
  ```bash
  apt-get dist-upgrade -o Dpkg::Options::="--force-overwrite"
  ```
  For locally built `.rpm` packages, `rpm -Uvh --replacefiles` achieves the equivalent.

#### Configure the installation: optional dependencies

Some packages list optional Recommends that are not required for all use cases — for example, the AESM daemon is not needed for fully in-process deployments or container workloads where the daemon runs on the host and its Unix socket is bind-mounted in. Recommends are installed by default; to suppress them, pass the appropriate flag to the install command:

| Distribution | Flag |
| --- | --- |
| Ubuntu, Debian | `apt-get install --no-install-recommends <package>` |
| RHEL, CentOS Stream, Anolis OS, Azure Linux | `dnf install --setopt=install_weak_deps=False <package>` |
| SLES | `zypper install --no-recommends <package>` |

#### ECDSA attestation

In addition to the [prerequisites](#install-the-intel-sgx-psw) and the packages in the [table above](#package-manager-recommended):

1. **Hardware** — requires FLC-capable hardware (8th Gen Intel® Core™ or newer, or Intel® Atom® with FLC). See [Which Platforms Support Intel® SGX DCAP?](https://www.intel.com/content/www/us/en/support/articles/000057420/software/intel-security-products.html).

2. **QPL** — install `libsgx-dcap-default-qpl` (see the [`[!IMPORTANT]` callout in the package table](#package-manager-recommended)) or provide a custom `libdcap_quoteprov.so.1`.

3. _\<recommended\>_ **PCCS** — deploy a [Provisioning Certificate Caching Service](https://cc-enabling.trustedservices.intel.com/intel-tdx-enabling-guide/02/infrastructure_setup/#provisioning-certificate-caching-service-pccs) and configure `/etc/sgx_default_qcnl.conf` (JSON; overridable per-process via `QCNL_CONF_PATH`).  
   For all available keys and their semantics, see the [annotated default config](https://github.com/intel/confidential-computing.tee.dcap/blob/main/QuoteGeneration/qcnl/linux/sgx_default_qcnl.conf).

   Example (on-premises PCCS):
   ```json
   { "pccs_url": "https://your_pccs_server:8081/sgx/certification/v4/" }
   ```

   > 💡 **Tip:** For local testing or single-machine deployments, PCCS caching layer can be skipped — point the QPL directly at the [Intel PCS](https://api.portal.trustedservices.intel.com/provisioning-certification) by setting `pccs_url` in the config file:
   > ```json
   > { "pccs_url": "https://api.trustedservices.intel.com/sgx/certification/v4/" }
   > ```
   > For production scale deployments a local PCCS is strongly recommended: it caches collateral, reduces round-trip latency on every quote operation, and removes a dependency on a persistent internet connection.

   > ℹ️ **Note:** A [PCS subscription key](https://api.portal.trustedservices.intel.com/provisioning-certification) (`Ocp-Apim-Subscription-Key`) is optional for most PCS APIs, but still recommended for production: without one, rate limits are shared across all anonymous callers; with one, your account receives a dedicated quota. When using PCCS, configure the key in PCCS itself — see the [PCCS documentation](https://github.com/intel/confidential-computing.tee.dcap.pccs). When pointing QCNL directly at Intel PCS, pass it as a custom request header:
   > ```json
   > {
   >   "pccs_url": "https://api.trustedservices.intel.com/sgx/certification/v4/",
   >   "custom_request_options": {
   >     "get_cert": {
   >       "headers": { "Ocp-Apim-Subscription-Key": "<your-subscription-key>" }
   >     }
   >   }
   > }
   > ```

#### Manage the aesmd service

The PSW installer registers `aesmd` as a systemd service running under the `aesmd` account:

```bash
sudo systemctl start aesmd
sudo systemctl stop aesmd
sudo systemctl restart aesmd
```

> [!NOTE]
> If using AESM-based out-of-process quoting, the QPL runs inside `aesmd` and makes outbound HTTP requests to PCCS or Intel PCS. If a proxy is required, configure it in [`/etc/aesmd.conf`](psw/ae/aesm_service/config/network/aesmd.conf) and restart the service. The same file also controls the QPL log level.

Reproducibility
-----------------------------------------
Intel® SGX ships several prebuilt binaries. All are produced inside a reproducible SGX Docker container to allow independent verification. To reproduce the build environment and verify the binaries, follow the [reproducibility README](linux/reproducibility/README.md).

Most binaries can be compared against a locally rebuilt output using the standard `diff` command. Architectural Enclaves (AEs) are an exception — their verification requires the dedicated [AE reproducibility verifier](linux/reproducibility/ae_reproducibility_verifier/README.md). AE reproducibility also depends on a reproducibly built Intel® SGX SDK; the SDK's own reproducible build process is documented in the [SDK repository](https://github.com/intel/confidential-computing.sgx.sdk).
