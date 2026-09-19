Name:       harbour-sailvault
Summary:    Native multi-chain wallet for Sailfish OS
Version:    0.3.0
Release:    1
Group:      Qt/Qt
License:    BSD-3-Clause
URL:        https://github.com/edp17/harbour-sailvault
Source0:    %{name}-%{version}.tar.bz2

Requires:   sailfishsilica-qt5 >= 0.10.9
Requires:   libsailfishsecrets

BuildRequires:  pkgconfig(sailfishapp) >= 1.0.2
BuildRequires:  pkgconfig(Qt5Core)
BuildRequires:  pkgconfig(Qt5Gui)
BuildRequires:  pkgconfig(Qt5Qml)
BuildRequires:  pkgconfig(Qt5Quick)
BuildRequires:  pkgconfig(sailfishsecrets)
BuildRequires:  boost-devel
BuildRequires:  desktop-file-utils
BuildRequires:  cmake >= 3.19
BuildRequires:  curl
BuildRequires:  rust
BuildRequires:  cargo
BuildRequires:  unzip

%description
SailVault is a native multi-chain wallet project for Sailfish OS.
Milestone 3 integrates Sailfish Secrets with Wallet Core to prove a secure
wallet create/load/derive/sign/delete lifecycle. It is not yet a usable wallet.

%prep
%setup -q -n %{name}-%{version}

%build
# M3r1 keeps the proven Wallet Core build recipe and integrates the
# application wallet lifecycle with a dedicated relocking Sailfish Secrets collection.
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
