# Note that this is NOT a relocatable package
%define ver      1.7
%define RELEASE  1
%define rel      %{?CUSTOM_RELEASE} %{!?CUSTOM_RELEASE:%RELEASE}
%define prefix   /usr

Summary: Bluetooth protocol analyzer - HCIdump
Name: bluez-hcidump
Version: %ver
Release: %rel
Copyright: GPL
Group: Applications/System
Vendor: Official Linux Bluetooth protocol stack
Packager: Sebastian Frankfurt <sf@infesto.de>
Source: http://bluez.sourceforge.net/%{name}-%{ver}.tar.gz
BuildRoot: /var/tmp/%{name}-%{PACKAGE_VERSION}-root
URL: http://bluez.sourceforge.net
Docdir: %{prefix}/share/doc
Requires: glibc >= 2.2.4
Requires: bluez-libs >= 2.0
BuildRequires: glibc >= 2.2.4
BuildRequires: bluez-libs >= 2.0

%description
Bluetooth protocol analyzer.

%changelog
* Tue Aug 14 2002 Sebastian Frankfurt <sf@infesto.de>
- Initial RPM

%prep
rm -rf $RPM_BUILD_ROOT

%setup -q

%build
automake
CFLAGS="$RPM_OPT_FLAGS" ./configure --enable-test --prefix=%{prefix} --mandir=%{_mandir} --sysconfdir=%{_sysconfdir}
make

%install
rm -rf $RPM_BUILD_ROOT
make DESTDIR=$RPM_BUILD_ROOT prefix=%{prefix} confdir=%{_sysconfdir}/bluetooth mandir=%{_mandir} sysconfdir=%{_sysconfdir} install

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-, root, root)

%{_sbindir}/hcidump

%doc AUTHORS COPYING INSTALL ChangeLog NEWS README

