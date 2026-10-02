Name:       harbour-sailvault
Summary:    Native multi-chain wallet for Sailfish OS
Version:    0.52.0
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
SailVault is a native multi-chain wallet under staged development for Sailfish OS.
It provides BTC/ETH/SOL public portfolio monitoring, activity, tokens, offline
receive QR codes, Watch-only and Address book support. M52 adds a deliberately
narrow development-only Ethereum sign/verify boundary for the published BIP39
test wallet. The raw signature is verified inside C++ and discarded before QML
can receive it. Production signing, signed-transaction assembly and broadcasting
remain intentionally unavailable.

%prep
%setup -q -n %{name}-%{version}

%build
# Preserve the proven Sailfish Wallet Core build path and staged security architecture.
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
%doc LICENSE README.md CHANGELOG.md docs/READ_ONLY_BETA_CHECKLIST.md docs/WALLET_CORE_MIGRATION_CHECKLIST.md docs/WALLET_CORE_SECURITY_REVIEW.md docs/M41_DEVICE_CHECKLIST.md docs/NETWORK_BOUNDARY_REVIEW.md docs/M42_DEVICE_CHECKLIST.md docs/PROVIDER_PRIVACY_REVIEW.md docs/M43_DEVICE_CHECKLIST.md docs/PROVIDER_RESPONSE_REVIEW.md docs/M44_DEVICE_CHECKLIST.md docs/WALLET_CORE_4_8_4_REVIEW.md docs/M45_DEVICE_CHECKLIST.md docs/UI_CONSISTENCY_REVIEW.md docs/M46_DEVICE_CHECKLIST.md docs/STORAGE_LIFECYCLE_REVIEW.md docs/M47_DEVICE_CHECKLIST.md docs/UNSIGNED_TRANSACTION_INTENT_REVIEW.md docs/M48_DEVICE_CHECKLIST.md docs/ETHEREUM_UNSIGNED_CONSTRUCTION_REVIEW.md docs/M49_DEVICE_CHECKLIST.md docs/BITCOIN_UNSIGNED_CONSTRUCTION_REVIEW.md docs/M50_DEVICE_CHECKLIST.md docs/SOLANA_UNSIGNED_CONSTRUCTION_REVIEW.md docs/M51_DEVICE_CHECKLIST.md docs/DEVELOPMENT_SIGNING_BOUNDARY_REVIEW.md docs/M52_DEVICE_CHECKLIST.md docs/RELEASE_NOTES.md
