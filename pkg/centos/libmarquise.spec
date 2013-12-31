Name:	        libmarquise	
Version:	1.0.2
Release:	1.0anchor1%{?dist}
Summary:	libmarquise is a library for writing data frames to vaultaire

Group:		Development/Libraries
License:	BSD
URL:		https://github.com/anchor/libmarquise
Source0:	%{name}-%{version}.tar.gz
BuildRoot:	%(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)

BuildRequires:	glib2-devel, protobuf-devel, protobuf-compiler, protobuf-c-devel, zeromq-devel >= 4.0, libtool
Requires:	protobuf, protobuf-c, zeromq >= 4.0

%description
libmarquise is a client library for chateau, vaultaire's broker.

%package devel
Summary:  Development files for libmarquise
Group:    Development/Libraries
Requires: %{name} = %{version}-%{release}, pkgconfig

%description devel
libmarquise is a client library for chateau, vaultaire's broker.

This package contains libmarquise development libraries and header 
files.

%prep
%setup -q


%build
mkdir m4
autoreconf -i
%configure
make %{?_smp_mflags}


%install
rm -rf %{buildroot}
make install DESTDIR=%{buildroot}


%clean
rm -rf %{buildroot}


%files
%defattr(-,root,root,-)
%doc
%{_bindir}/send_anchor_stat
%{_libdir}/libmarquise.so
%{_libdir}/libmarquise.so.1
%{_libdir}/libmarquise.so.1.0.0
%{_docdir}/libmarquise/README

%files devel
%{_includedir}/marquise.h
%{_libdir}/libmarquise.a
%{_libdir}/libmarquise.la

%changelog
* Tue Dec 31 2013 Sharif Olorin <sio@tesser.org> - 1.0.2-0anchor1
- Bump to 1.0.2
* Tue Dec 31 2013 Sharif Olorin <sio@tesser.org> - 1.0.1-0anchor1
- Bump to 1.0.1
* Tue Dec 31 2013 Sharif Olorin <sio@tesser.org> - 1.0-0anchor1
- Initial build
