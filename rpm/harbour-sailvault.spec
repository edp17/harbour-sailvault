Name:       harbour-sailvault
Summary:    Native read-only multi-chain wallet for Sailfish OS
Version:    0.39.0
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
BuildRequires:  pkgconfig(Qt5Network)
BuildRequires:  pkgconfig(sailfishsecrets)
BuildRequires:  boost-devel
BuildRequires:  desktop-file-utils
BuildRequires:  cmake >= 3.19
BuildRequires:  curl
BuildRequires:  rust
BuildRequires:  cargo
BuildRequires:  unzip

%description
SailVault is a native read-only multi-chain wallet for Sailfish OS.
This read-only beta release provides BTC/ETH/SOL public portfolio monitoring,
activity, tokens, offline receive QR codes, Watch-only and Address book support.
Transaction construction, signing and broadcasting are intentionally unavailable.

%prep
%setup -q -n %{name}-%{version}

%build
# Preserve the proven Sailfish Wallet Core build path and read-only architecture.
./scripts/build-wallet-core-rust-sb2.sh

%cmake \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr
%make_build

%check
./scripts/preflight.sh

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
%doc LICENSE README.md docs/READ_ONLY_BETA_CHECKLIST.md docs/RELEASE_NOTES.md CHANGELOG.md
