Name:       harbour-sailvault
Summary:    Native multi-chain wallet for Sailfish OS
Version:    0.1.0
Release:    17
Group:      Qt/Qt
License:    BSD-3-Clause
URL:        https://github.com/edp17/harbour-sailvault
Source0:    %{name}-%{version}.tar.bz2

Requires:   sailfishsilica-qt5 >= 0.10.9

BuildRequires:  pkgconfig(sailfishapp) >= 1.0.2
BuildRequires:  pkgconfig(Qt5Core)
BuildRequires:  pkgconfig(Qt5Gui)
BuildRequires:  pkgconfig(Qt5Qml)
BuildRequires:  pkgconfig(Qt5Quick)
BuildRequires:  boost-devel
BuildRequires:  desktop-file-utils
BuildRequires:  cmake >= 3.19
BuildRequires:  curl
BuildRequires:  rust
BuildRequires:  cargo
BuildRequires:  unzip

%description
SailVault is a native multi-chain wallet project for Sailfish OS.
Milestone 1 is an offline Trust Wallet Core cross-compilation and
deterministic cryptographic self-test probe. It is not a usable wallet.

%prep
%setup -q -n %{name}-%{version}

%build
# M1r17 keeps the proven r16 Wallet Core build unchanged and fixes only
# QML MainPage type resolution in the application loader.
./scripts/build-wallet-core-rust-sb2.sh

%cmake \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr
%make_build

%install
rm -rf %{buildroot}
%make_install

desktop-file-install --delete-original \
    --dir %{buildroot}%{_datadir}/applications \
    %{buildroot}%{_datadir}/applications/%{name}.desktop

%files
%defattr(-,root,root,-)
%{_bindir}/%{name}
%{_datadir}/%{name}
%{_datadir}/applications/%{name}.desktop
%{_datadir}/icons/hicolor/86x86/apps/%{name}.png
