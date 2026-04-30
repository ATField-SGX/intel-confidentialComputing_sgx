#
# Copyright(c) 2011-2026 Intel Corporation 
#
# SPDX-License-Identifier: BSD-3-Clause
#

%define _install_path @install_path@
%define _license_file COPYING

Name:           libsgx-aesm-pce-plugin
Version:        @version@
Release:        1%{?dist}
Summary:        PCE Plugin for Intel(R) Software Guard Extensions AESM Service
Group:          Development/System
Requires:       sgx-aesm-service >= %{version}-%{release} libsgx-pce-logic >= 1.26 libsgx-ae-pce >= %{version}-%{release}

License:        BSD License
URL:            https://github.com/intel/linux-sgx
Source0:        %{name}-%{version}.tar.gz

%description
PCE Plugin for Intel(R) Software Guard Extensions AESM Service

%prep
%setup -qc

%install
make DESTDIR=%{?buildroot} install
OLDDIR=$(pwd)
cd %{?buildroot}
rm -fr $(ls | grep -xv "%{name}")
install -d %{name}%{_docdir}/%{name}
find %{?_sourcedir}/package/licenses/ -type f -print0 | xargs -0 -n1 cat >> %{name}%{_docdir}/%{name}/%{_license_file}
cd "$OLDDIR"
echo "%{_install_path}" > %{_specdir}/list-%{name}
find %{?buildroot}/%{name} | sort | \
awk '$0 !~ last "/" {print last} {last=$0} END {print last}' | \
sed -e "s#^%{?buildroot}/%{name}##" | \
grep -v "^%{_install_path}" >> %{_specdir}/list-%{name} || :
cp -r %{?buildroot}/%{name}/* %{?buildroot}/
rm -fr %{?buildroot}/%{name}

%files -f %{_specdir}/list-%{name}

# Detect whether rpmbuild has modern auto-debuginfo support (rpm >= 4.14).
# We use this to keep one spec compatible across old/new RPM and only enable
# legacy debug_package handling when auto-debuginfo is not available.
%global __auto_debuginfo %{lua:
  local v = rpm.expand("%{rpmversion}")
  local maj, min = v:match("^(%d+)%.(%d+)")
  maj, min = tonumber(maj), tonumber(min)
  -- Unparseable version: assume modern RPM, skip legacy debug_package
  if not (maj and min) then print("1")
  elseif maj > 4 or (maj == 4 and min >= 14) then print("1")
  else print("0")
  end
}

%if 0%{?__auto_debuginfo} == 0
%debug_package
%endif

%changelog
* Mon Jul 29 2019 SGX Team
- Initial Release
