#
# neoglogg builds with qmake, and the .pro file already installs the binary,
# the .desktop file, icons, docs and the AppStream metainfo into PREFIX.
# %%make_install just redirects those into the buildroot, exactly as the
# Debian packaging does with INSTALL_ROOT.
#
# The version is not hardcoded: the release workflow passes it in with
#     rpmbuild --define "version 1.2.3"
# and it defaults to 0.0.0 for local unversioned builds.
#
%global _version %{?version}%{!?version:0.0.0}

Name:           neoglogg
Version:        %{_version}
Release:        1%{?dist}
Summary:        The fast, smart log explorer

License:        GPL-3.0-or-later
URL:            https://github.com/nekromoff/neoglogg
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  gcc-c++
BuildRequires:  make
BuildRequires:  qt6-qtbase-devel
BuildRequires:  qt6-qt5compat-devel
BuildRequires:  boost-devel
BuildRequires:  desktop-file-utils
BuildRequires:  libappstream-glib

%description
neoglogg is a multi-platform GUI application to browse and search through
long or complex log files. It is designed with programmers and system
administrators in mind, and treats the log file as read-only.

Files are memory-mapped rather than loaded, so very large logs open
immediately, and searches run on a background thread with the results shown
in a second pane that stays in sync with the main view.

This is a maintained fork of glogg, updated and upgraded: ported to Qt 6,
with a dark theme, multi-threaded search and line-range searching.

%prep
%autosetup

%build
# qmake6 on Fedora/RHEL; PREFIX matches the paths listed in %%files below
qmake6 %{name}.pro PREFIX=%{_prefix} VERSION="%{version}"
%make_build

%install
%make_install INSTALL_ROOT=%{buildroot}

%check
desktop-file-validate %{buildroot}%{_datadir}/applications/%{name}.desktop
appstream-util validate-relax --nonet \
    %{buildroot}%{_datadir}/metainfo/%{name}.metainfo.xml

%files
%license %{_datadir}/doc/%{name}/LICENSE
%doc %{_datadir}/doc/%{name}/README.md
%{_bindir}/%{name}
%{_datadir}/applications/%{name}.desktop
%{_datadir}/metainfo/%{name}.metainfo.xml
%{_datadir}/icons/hicolor/64x64/apps/%{name}.png
%{_datadir}/icons/hicolor/128x128/apps/%{name}.png
%{_datadir}/icons/hicolor/256x256/apps/%{name}.png
%{_datadir}/icons/hicolor/scalable/apps/%{name}.svg

%changelog
* Sun Aug 16 2026 Daniel Duris <dusoft@staznosti.sk> - 1.1-1
- See the GitHub release notes for the changes in each version.
