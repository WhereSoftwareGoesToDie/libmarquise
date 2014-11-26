# Build with tagged release v1.1.4 from 
# https://github.com/anchor/libmarquise
Name:	        libmarquise	
Version:	1.2.0
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

It's designed for building collectors for statistical data and
operations metrics.

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
%{_bindir}/marquise
%{_bindir}/marquised
%{_bindir}/marquise_process_deferrals
%{_libdir}/libmarquise.so
%{_libdir}/libmarquise.so.1
%{_libdir}/libmarquise.so.1.0.0
%{_docdir}/libmarquise/README

%files devel
%{_includedir}/marquise.h
%{_libdir}/libmarquise.a
%{_libdir}/libmarquise.la

%changelog
* Fri Mar 21 2014 Sharif Olorin <sio@tesser.org> - 1.2.0-0anchor1
- Bump to 1.2.0 
* Wed Mar 19 2014 Sharif Olorin <sio@tesser.org> - 1.1.4-0anchor1
- Bump to 1.1.4 
* Mon Mar 17 2014 Sharif Olorin <sio@tesser.org> - 1.1.2-0anchor1
- Bump to 1.1.2 
* Wed Mar 05 2014 Sharif Olorin <sio@tesser.org> - 1.1.0-0anchor1
- Bump to 1.1.0 (marquised signal handling)
* Tue Mar 04 2014 Sharif Olorin <sio@tesser.org> - 1.0.9-0anchor1
- Bump to 1.0.9 (more deferral bugfixes)
* Tue Feb 28 2014 Sharif Olorin <sio@tesser.org> - 1.0.8-0anchor1
- Bump to 1.0.8 (deferral bugfixes)
* Tue Feb 25 2014 Sharif Olorin <sio@tesser.org> - 1.0.7-0anchor1
- Bump to 1.0.7 (marquised)
* Tue Dec 31 2013 Sharif Olorin <sio@tesser.org> - 1.0.6-0anchor1
- Bump to 1.0.6 (async)
* Tue Dec 31 2013 Sharif Olorin <sio@tesser.org> - 1.0.5-0anchor1
- Bump to 1.0.5 (new protobuf version)
* Tue Dec 31 2013 Sharif Olorin <sio@tesser.org> - 1.0.4-0anchor1
- Bump to 1.0.4
* Tue Dec 31 2013 Sharif Olorin <sio@tesser.org> - 1.0.3-0anchor1
- Bump to 1.0.3
* Tue Dec 31 2013 Sharif Olorin <sio@tesser.org> - 1.0.2-0anchor1
- Bump to 1.0.2
* Tue Dec 31 2013 Sharif Olorin <sio@tesser.org> - 1.0.1-0anchor1
- Bump to 1.0.1
* Tue Dec 31 2013 Sharif Olorin <sio@tesser.org> - 1.0-0anchor1
- Initial build
