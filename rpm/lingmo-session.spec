%global majver 1
Name:          lingmo-session
Version:       1.0.0
Release:       1%{?dist}
Summary:       Lingmo Desktop - Session manager
License:       GPL-2.0-or-later
URL:           https://lingmo.org
Source0:       %{name}-%{version}.tar.gz
BuildRequires: cmake >= 3.21 ninja-build qt6-qtbase-devel
BuildRequires: lingmo-ipc-devel lingmo-utils-devel lingmo-config-devel
%description
Session manager that orchestrates the startup of all Lingmo Desktop components.
%prep
%setup -q
%build
%cmake -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_TESTING=OFF
%cmake_build
%install
%cmake_install
%files
%license LICENSE
%{_libexecdir}/lingmo-session
%{_libdir}/libLingmoSession.so.%{majver}*
%{_libdir}/systemd/user/lingmo-session.service
%{_libdir}/systemd/user/lingmo-session.target
%changelog
* Wed Jul 09 2026 Lingmo OS Team <team@lingmo.org> - 1.0.0-1
- Initial release
