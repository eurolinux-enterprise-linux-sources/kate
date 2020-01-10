Name:    kate
Summary: Advanced Text Editor
Version: 4.10.5
Release: 6%{?dist}

# kwrite LGPLv2+
# kate: app LGPLv2, plugins, LGPLv2 and LGPLv2+ and GPLv2+
# ktexteditor: LGPLv2
# katepart: LGPLv2
License: LGPLv2 and LGPLv2+ and GPLv2+
URL:     https://projects.kde.org/projects/kde/kdebase/kate
%global revision %(echo %{version} | cut -d. -f3)
%if %{revision} >= 50
%global stable unstable
%else
%global stable stable
%endif
Source0: http://download.kde.org/%{stable}/%{version}/src/%{name}-%{version}.tar.xz

# upstream patches
Patch0:  kate-4.10.5-properly-remove-composed-characters.patch
Patch1:  kate-allow-to-save-files-in-readonly-directory.patch

BuildRequires: desktop-file-utils
BuildRequires: kactivities-devel >= %{version}
BuildRequires: kdelibs4-devel >= %{version}
# pate (kate python plugin support), not enabled yet...
# largely due to circular deps (pykde4 Requires: kate-part)
#BuildRequires: pykde4-devel >= %{version}
BuildRequires: pkgconfig(QJson)

Requires: kde-runtime >= 4.10.5
Requires: %{name}-part%{?_isa} = %{version}-%{release}
Requires: %{name}-libs%{?_isa} = %{version}-%{release}

%description
%{summary}.

%package devel
Summary:  Development files for %{name}
Requires: %{name}-libs%{?_isa} = %{version}-%{release}
Requires: kdelibs4-devel
%description devel
%{summary}.

%package libs
Summary:  Runtime files for %{name}
# when split occurred
Obsoletes: kdesdk-libs < 4.6.95-10
Requires: %{name} = %{version}-%{release}
%description libs
%{summary}.

%package part
Summary: Kate kpart plugin
License: LGPLv2
# when split occurred
Conflicts: kdelibs < 6:4.6.95-10
# katesyntaxhighlightingrc conflicts with kdelibs3, see http://bugzilla.redhat.com/883529
Conflicts: kdelibs3 < 3.5.10-40
%description part
%{summary}.

%package -n kwrite
Summary: Text Editor
License: LGPLv2+
# when split occurred
Conflicts: kdebase < 6:4.6.95-10
Requires: %{name}-part%{?_isa} = %{version}-%{release}
Requires: kde-runtime >= 4.10.5
%description -n kwrite
%{summary}.


%prep
%setup -q

%patch0 -p1 -b .properly-remove-composed-characters
%patch1 -p1 -b .allow-to-save-files-in-readonly-directory

%build
mkdir -p %{_target_platform}
pushd %{_target_platform}
%{cmake_kde4} ..
popd

make %{?_smp_mflags} -C %{_target_platform}


%install
make install/fast DESTDIR=%{buildroot} -C %{_target_platform}

%find_lang kate --with-kde --without-mo
%find_lang kwrite --with-kde --without-mo

# move devel symlinks (that would otherwise conflict with kdelibs3-devel)
mkdir -p %{buildroot}%{_kde4_libdir}/kde4/devel
pushd %{buildroot}%{_kde4_libdir}
for i in lib*.so
do
  case "$i" in
    libkate*interfaces.so)
      linktarget=`readlink "$i"`
      rm -f "$i"
      ln -sf "../../$linktarget" "kde4/devel/$i"
      ;;
  esac
done
popd

# fix documentation multilib conflict in index.cache
for f in kate kwrite ; do
   bunzip2 %{buildroot}%{_kde4_docdir}/HTML/en/$f/index.cache.bz2
   sed -i -e 's!name="id[a-z]*[0-9]*"!!g' %{buildroot}%{_kde4_docdir}/HTML/en/$f/index.cache
   sed -i -e 's!#id[a-z]*[0-9]*"!!g' %{buildroot}%{_kde4_docdir}/HTML/en/$f/index.cache
   bzip2 -9 %{buildroot}%{_kde4_docdir}/HTML/en/$f/index.cache
done

%check
for f in %{buildroot}%{_kde4_datadir}/applications/kde4/*.desktop ; do
  desktop-file-validate $f
done


%post
touch --no-create %{_kde4_iconsdir}/hicolor &> /dev/null || :

%posttrans
gtk-update-icon-cache %{_kde4_iconsdir}/hicolor &> /dev/null || :
update-mime-database %{_kde4_datadir}/mime >& /dev/null

%postun
if [ $1 -eq 0 ] ; then
touch --no-create %{_kde4_iconsdir}/hicolor &> /dev/null || :
gtk-update-icon-cache %{_kde4_iconsdir}/hicolor &> /dev/null || :
update-mime-database %{_kde4_datadir}/mime >& /dev/null
fi

%files -f kate.lang
%doc kate/AUTHORS kate/ChangeLog kate/COPYING.LIB kate/README
%{_kde4_bindir}/kate
%{_kde4_libdir}/libkdeinit4_kate.so
%{_kde4_datadir}/applications/kde4/kate.desktop
%{_kde4_appsdir}/kate/
%{_kde4_appsdir}/katexmltools/
%{_kde4_appsdir}/kconf_update/kate*.upd
%{_kde4_iconsdir}/hicolor/*/*/*
%{_mandir}/man1/kate.1.gz
%{_kde4_configdir}/katerc
%{_kde4_datadir}/kde4/services/kate*.desktop
%{_kde4_libdir}/kde4/kate*plugin.so
%{_kde4_libdir}/kde4/katefiletemplates.so
%{_kde4_appsdir}/ktexteditor_*/
%{_kde4_datadir}/kde4/services/ktexteditor_*.desktop
%{_kde4_libdir}/kde4/ktexteditor_*.so
%{_kde4_datadir}/kde4/services/plasma-applet-katesession.desktop
%{_kde4_datadir}/kde4/servicetypes/kateplugin.desktop
%{_kde4_libdir}/kde4/plasma_applet_katesession.so
%{_kde4_libdir}/kde4/kate_kttsd.so
%{_kde4_iconsdir}/oxygen/*/actions/*

%post libs -p /sbin/ldconfig
%postun libs -p /sbin/ldconfig

%files libs
%{_kde4_libdir}/libkateinterfaces.so.4*

%files devel
%{_kde4_libdir}/kde4/devel/libkateinterfaces.so
%{_kde4_libdir}/kde4/devel/libkatepartinterfaces.so
%{_kde4_includedir}/kate_export.h
%{_kde4_includedir}/kate/

%post part -p /sbin/ldconfig
%postun part -p /sbin/ldconfig

%files part
%doc part/AUTHORS part/ChangeLog part/COPYING.LIB
%doc part/INDENTATION part/NEWS part/README* part/TODO*
%{_kde4_libdir}/kde4/katepart.so
%{_kde4_libdir}/libkatepartinterfaces.so.4*
%{_kde4_appsdir}/katepart/
%{_kde4_configdir}/katemoderc
%{_kde4_configdir}/kateschemarc
%{_kde4_configdir}/katesyntaxhighlightingrc
%{_kde4_configdir}/ktexteditor_codesnippets_core.knsrc
%{_kde4_datadir}/kde4/services/katepart.desktop

%files -n kwrite -f kwrite.lang
%{_kde4_bindir}/kwrite
%{_kde4_appsdir}/kwrite
%{_kde4_libdir}/libkdeinit4_kwrite.so
%{_kde4_datadir}/applications/kde4/kwrite.desktop

#files pate
#{python_sitearch}/PyKate4/
#{_kde4_libdir}/kde4/pateplugin.so
#%{_kde4_datadir}/kde4/services/pate.desktop


%changelog
* Wed Feb 07 2018 Jan Grulich <jgrulich@redhat.com> - 4.10.5-6
- Do not forget to truncate files in write protected folders
  Resolves: bz#1382541

* Mon Oct 16 2017 Jan Grulich <jgrulich@redhat.com> - 4.10.5-5
- Allow to save files in readonly directories
  Resolves: bz#1382541

* Tue May 17 2016 Jan Grulich <jgrulich@redhat.com> - 4.10.5-4
- Properly remove composed characters
  Resolves: bz#1037962

* Fri Jan 24 2014 Daniel Mach <dmach@redhat.com> - 4.10.5-3
- Mass rebuild 2014-01-24

* Fri Dec 27 2013 Daniel Mach <dmach@redhat.com> - 4.10.5-2
- Mass rebuild 2013-12-27

* Sun Jun 30 2013 Than Ngo <than@redhat.com> - 4.10.5-1
- 4.10.5

* Sat Jun 01 2013 Rex Dieter <rdieter@fedoraproject.org> - 4.10.4-1
- 4.10.4

* Mon May 06 2013 Than Ngo <than@redhat.com> - 4.10.3-1
- 4.10.3

* Thu Apr 04 2013 Rex Dieter <rdieter@fedoraproject.org> 4.10.2-2
- -libs: Obsoletes: kdesdk-libs (instead of Conflicts)

* Sun Mar 31 2013 Rex Dieter <rdieter@fedoraproject.org> - 4.10.2-1
- 4.10.2

* Tue Mar 19 2013 Than Ngo <than@redhat.com> - 4.10.1-3
- backport to fix python indentation mode

* Tue Mar 19 2013 Than Ngo <than@redhat.com> - 4.10.1-2
- Fix documentation multilib conflict in index.cache

* Sat Mar 02 2013 Rex Dieter <rdieter@fedoraproject.org> - 4.10.1-1
- 4.10.1

* Fri Feb 01 2013 Rex Dieter <rdieter@fedoraproject.org> - 4.10.0-1
- 4.10.0

* Sun Jan 20 2013 Rex Dieter <rdieter@fedoraproject.org> - 4.9.98-1
- 4.9.98

* Fri Jan 04 2013 Rex Dieter <rdieter@fedoraproject.org> - 4.9.97-1
- 4.9.97

* Thu Dec 20 2012 Rex Dieter <rdieter@fedoraproject.org> - 4.9.95-1
- 4.9.95

* Tue Dec 04 2012 Rex Dieter <rdieter@fedoraproject.org> 4.9.90-2
- kate has a file conflict with kdelibs3 (#883529)

* Mon Dec 03 2012 Rex Dieter <rdieter@fedoraproject.org> 4.9.90-1
- 4.9.90 (4.10 beta2)

* Mon Dec 03 2012 Than Ngo <than@redhat.com> - 4.9.4-1
- 4.9.4

* Sat Nov 03 2012 Rex Dieter <rdieter@fedoraproject.org> - 4.9.3-1
- 4.9.3

* Fri Sep 28 2012 Rex Dieter <rdieter@fedoraproject.org> - 4.9.2-1
- 4.9.2

* Mon Sep 03 2012 Than Ngo <than@redhat.com> - 4.9.1-1
- 4.9.1

* Thu Jul 26 2012 Lukas Tinkl <ltinkl@redhat.com> - 4.9.0-1
- 4.9.0

* Thu Jul 19 2012 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 4.8.97-2
- Rebuilt for https://fedoraproject.org/wiki/Fedora_18_Mass_Rebuild

* Wed Jul 11 2012 Rex Dieter <rdieter@fedoraproject.org> - 4.8.97-1
- 4.8.97

* Wed Jun 27 2012 Jaroslav Reznik <jreznik@redhat.com> - 4.8.95-1
- 4.8.95

* Sat Jun 09 2012 Rex Dieter <rdieter@fedoraproject.org> - 4.8.90-1
- 4.8.90

* Fri Jun 01 2012 Jaroslav Reznik <jreznik@redhat.com> - 4.8.80-2
- respin

* Sat May 26 2012 Jaroslav Reznik <jreznik@redhat.com> - 4.8.80-1
- 4.8.80

* Mon Apr 30 2012 Rex Dieter <rdieter@fedoraproject.org> 4.8.3-2
- s/kdebase-runtime/kde-runtime/

* Mon Apr 30 2012 Jaroslav Reznik <jreznik@redhat.com> - 4.8.3-1
- 4.8.3

* Fri Mar 30 2012 Rex Dieter <rdieter@fedoraproject.org> - 4.8.2-1
- 4.8.2

* Mon Mar 05 2012 Radek Novacek <rnovacek@redhat.com> - 4.8.1-1
- 4.8.1

* Tue Feb 28 2012 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 4.8.0-2
- Rebuilt for c++ ABI breakage

* Fri Jan 20 2012 Jaroslav Reznik <jreznik@redhat.com> - 4.8.0-1
- 4.8.0

* Wed Jan 04 2012 Radek Novacek <rnovacek@redhat.com> - 4.7.97-1
- 4.7.97

* Wed Dec 21 2011 Radek Novacek <rnovacek@redhat.com> - 4.7.95-1
- 4.7.95

* Sun Dec 04 2011 Rex Dieter <rdieter@fedoraproject.org> 4.7.90-1
- 4.7.90

* Thu Nov 24 2011 Jaroslav Reznik <jreznik@redhat.com> 4.7.80-1
- 4.7.80 (beta 1)

* Sat Oct 29 2011 Rex Dieter <rdieter@fedoraproject.org> 4.7.3-1
- 4.7.3

* Tue Oct 04 2011 Rex Dieter <rdieter@fedoraproject.org> 4.7.2-1
- 4.7.2

* Wed Sep 07 2011 Than Ngo <than@redhat.com> - 4.7.1-1
- 4.7.1

* Thu Jul 28 2011 Rex Dieter <rdieter@fedoraproject.org> 4.7.0-2
- -part: move %%_kde4_appsdir/katepart/ here

* Tue Jul 26 2011 Jaroslav Reznik <jreznik@redhat.com> 4.7.0-1
- 4.7.0

* Mon Jul 18 2011 Rex Dieter <rdieter@fedoraproject.org> 4.6.95-1
- first try

