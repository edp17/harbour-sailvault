#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
WC="${ROOT}/vendor/wallet-core"
SPEC="${ROOT}/rpm/harbour-sailvault.spec"
CMAKE_FILE="${ROOT}/CMakeLists.txt"
DESKTOP="${ROOT}/harbour-sailvault.desktop"
README="${ROOT}/README.md"

spec_value() {
    awk -v key="$1" '$1 == key ":" { print $2; exit }' "${SPEC}"
}

cmake_define() {
    sed -n "s/^[[:space:]]*$1=\"\([^\"]*\)\".*/\1/p" "${CMAKE_FILE}" | head -n1
}

SPEC_VERSION="$(spec_value Version)"
SPEC_RELEASE="$(spec_value Release)"
CMAKE_VERSION="$(sed -n 's/^project(harbour-sailvault VERSION \([^ ]*\).*/\1/p' "${CMAKE_FILE}" | head -n1)"
CMAKE_RELEASE="$(cmake_define SAILVAULT_PACKAGE_RELEASE)"
CMAKE_MILESTONE="$(cmake_define SAILVAULT_MILESTONE)"
CMAKE_BUILD_LABEL="$(cmake_define SAILVAULT_BUILD_LABEL)"
EXPECTED_MILESTONE="$(printf '%s' "${SPEC_VERSION}" | awk -F. '{print $2}')"
PACKAGE_VERSION="${SPEC_VERSION}-${SPEC_RELEASE}"

printf 'SailVault %s provider-privacy preflight\n' "${PACKAGE_VERSION}"
printf '%*s\n' 47 '' | tr ' ' '='

fail() {
    echo "FAIL  $*"
    exit 1
}

pass() {
    echo "PASS  $*"
}

[[ -n "${SPEC_VERSION}" && -n "${SPEC_RELEASE}" ]] \
    || fail "RPM version metadata is incomplete"
[[ "${CMAKE_VERSION}" == "${SPEC_VERSION}" ]] \
    || fail "CMake version ${CMAKE_VERSION:-missing} does not match RPM ${SPEC_VERSION}"
[[ "${CMAKE_RELEASE}" == "${SPEC_RELEASE}" ]] \
    || fail "CMake package release ${CMAKE_RELEASE:-missing} does not match RPM ${SPEC_RELEASE}"
[[ "${CMAKE_MILESTONE}" == "${EXPECTED_MILESTONE}" ]] \
    || fail "CMake milestone ${CMAKE_MILESTONE:-missing} does not match version ${SPEC_VERSION}"
[[ -n "${CMAKE_BUILD_LABEL}" ]] || fail "CMake build label is empty"
pass "Build identity: ${PACKAGE_VERSION} · M${CMAKE_MILESTONE} · ${CMAKE_BUILD_LABEL}"

for required in \
    "${ROOT}/LICENSE" \
    "${README}" \
    "${ROOT}/CHANGELOG.md" \
    "${ROOT}/docs/READ_ONLY_BETA_CHECKLIST.md" \
    "${ROOT}/docs/WALLET_CORE_MIGRATION_CHECKLIST.md" \
    "${ROOT}/docs/WALLET_CORE_SECURITY_REVIEW.md" \
    "${ROOT}/docs/M41_DEVICE_CHECKLIST.md" \
    "${ROOT}/docs/NETWORK_BOUNDARY_REVIEW.md" \
    "${ROOT}/docs/M42_DEVICE_CHECKLIST.md" \
    "${ROOT}/docs/PROVIDER_PRIVACY_REVIEW.md" \
    "${ROOT}/docs/M43_DEVICE_CHECKLIST.md" \
    "${ROOT}/docs/RELEASE_NOTES.md" \
    "${ROOT}/harbour-sailvault.png" \
    "${DESKTOP}" \
    "${ROOT}/qml/pages/ReleaseReadinessPage.qml"; do
    [[ -s "${required}" ]] || fail "Required release file missing or empty: ${required#${ROOT}/}"
done
pass "Required release files present"

grep -Fq "**Version:** \`${PACKAGE_VERSION}\`" "${README}" \
    || fail "README package version does not match ${PACKAGE_VERSION}"
grep -Fq "**Build:** ${CMAKE_BUILD_LABEL}" "${README}" \
    || fail "README build label does not match '${CMAKE_BUILD_LABEL}'"
grep -Fq "SailVault ${SPEC_VERSION}-${SPEC_RELEASE}" "${ROOT}/docs/RELEASE_NOTES.md" \
    || fail "Release notes do not identify ${PACKAGE_VERSION}"
grep -Fq "# SailVault — Milestone ${CMAKE_MILESTONE}" "${README}" \
    || fail "README milestone heading does not identify M${CMAKE_MILESTONE}"
grep -Fq "# SailVault ${PACKAGE_VERSION} — ${CMAKE_BUILD_LABEL}" \
    "${ROOT}/docs/RELEASE_NOTES.md" \
    || fail "Release-note heading does not match current beta identity"
grep -Fq "# SailVault read-only beta 1 device checklist" \
    "${ROOT}/docs/READ_ONLY_BETA_CHECKLIST.md" \
    || fail "Device checklist is not the beta 1 regression checklist"
grep -Fq "# SailVault M40 — Wallet Core 4.8.3 migration checklist" \
    "${ROOT}/docs/WALLET_CORE_MIGRATION_CHECKLIST.md" \
    || fail "Wallet Core migration checklist is missing or stale"
grep -Fq "# SailVault M41 — Wallet Core 4.8.3 security review" \
    "${ROOT}/docs/WALLET_CORE_SECURITY_REVIEW.md" \
    || fail "M41 Wallet Core security review is missing or stale"
grep -Fq "# SailVault M41 device checklist" \
    "${ROOT}/docs/M41_DEVICE_CHECKLIST.md" \
    || fail "M41 device checklist is missing or stale"
grep -Fq "# SailVault M42 — network boundary review" \
    "${ROOT}/docs/NETWORK_BOUNDARY_REVIEW.md" \
    || fail "M42 network-boundary review is missing or stale"
grep -Fq "# SailVault M42 device checklist" \
    "${ROOT}/docs/M42_DEVICE_CHECKLIST.md" \
    || fail "M42 device checklist is missing or stale"
grep -Fq "# SailVault M43 — provider privacy and request-state review" \
    "${ROOT}/docs/PROVIDER_PRIVACY_REVIEW.md" \
    || fail "M43 provider-privacy review is missing or stale"
grep -Fq "# SailVault M43 device checklist" \
    "${ROOT}/docs/M43_DEVICE_CHECKLIST.md" \
    || fail "M43 device checklist is missing or stale"
[[ "${CMAKE_BUILD_LABEL}" == "stateless provider privacy" ]] \
    || fail "Unexpected M43 build label: ${CMAKE_BUILD_LABEL}"
pass "M43 build/document identity"

for tool in rustc cargo cc c++; do
    if command -v "${tool}" >/dev/null 2>&1; then
        echo "PASS  ${tool}: $(command -v "${tool}")"
    else
        echo "INFO  ${tool} is not visible here; RPM BuildRequires/build shell may provide it."
    fi
done

if command -v rustc >/dev/null 2>&1; then
    rustc -vV || true
fi

grep -q '^Exec=harbour-sailvault$' "${DESKTOP}" || fail "Desktop Exec entry is unexpected"
grep -q '^Icon=harbour-sailvault$' "${DESKTOP}" || fail "Desktop Icon entry is unexpected"
grep -q '^Permissions=Secrets;Internet$' "${DESKTOP}" || fail "Sailjail permissions must remain Secrets;Internet"
pass "Desktop/Sailjail metadata"

if grep -RIn --exclude-dir=vendor --exclude='*.md' --exclude='*.spec' --exclude='preflight.sh' 'http://' \
        "${ROOT}/src" "${ROOT}/qml" "${DESKTOP}" >/dev/null 2>&1; then
    fail "Plaintext http:// URL found in runtime source"
fi
pass "Runtime source contains no plaintext HTTP endpoint"

NETWORK_HELPER="${ROOT}/src/networkrequestutils.h"
for network_contract in \
    'DefaultMaxResponseBytes = 16LL * 1024LL * 1024LL' \
    'url.userInfo().isEmpty()' \
    'QNetworkRequest::FollowRedirectsAttribute' \
    'request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, false)' \
    'request.setMaximumRedirectsAllowed(0)' \
    'QNetworkRequest::CookieLoadControlAttribute' \
    'QNetworkRequest::CookieSaveControlAttribute' \
    'QNetworkRequest::AuthenticationReuseAttribute' \
    'QNetworkRequest::CacheLoadControlAttribute' \
    'QNetworkRequest::AlwaysNetwork' \
    'QNetworkRequest::CacheSaveControlAttribute' \
    'request.setRawHeader("Cookie", QByteArray())' \
    'request.setRawHeader("Authorization", QByteArray())' \
    'request.setRawHeader("Referer", QByteArray())' \
    'request.setRawHeader("Origin", QByteArray())' \
    'request.setRawHeader("Cache-Control", "no-store, no-cache")' \
    'QNetworkRequest::RedirectionTargetAttribute' \
    'QNetworkRequest::ContentLengthHeader' \
    '&QNetworkReply::downloadProgress' \
    '&QNetworkReply::readyRead' \
    'reply->setReadBufferSize(maxResponseBytes + 1)' \
    'reply->bytesAvailable() > maxResponseBytes' \
    'Blocked non-HTTPS provider endpoint' \
    'Provider redirect blocked' \
    'Provider response exceeded %1 safety limit'; do
    grep -Fq "${network_contract}" "${NETWORK_HELPER}" \
        || fail "M43 network/privacy contract missing: ${network_contract}"
done
for provider_source in \
    portfolioservice.cpp \
    activityservice.cpp \
    tokenservice.cpp \
    priceservice.cpp \
    transactiondetailservice.cpp \
    networkhealthservice.cpp \
    networkfeeservice.cpp; do
    grep -Fq 'SailVaultNetwork::' "${ROOT}/src/${provider_source}" \
        || fail "Provider source is not using shared network policy: ${provider_source}"
    send_count="$(grep -Ec 'm_network\.(get|post)' "${ROOT}/src/${provider_source}" || true)"
    guard_count="$(grep -Fc 'SailVaultNetwork::hardenRequest(request)' "${ROOT}/src/${provider_source}" || true)"
    [[ "${send_count}" -gt 0 ]] \
        || fail "Provider source has no recognized network sends: ${provider_source}"
    [[ "${send_count}" -eq "${guard_count}" ]] \
        || fail "Every provider request must be hardened before dispatch: ${provider_source} (${guard_count}/${send_count})"
done
grep -Fq 'SailVaultNetwork::safeHttpsEndpoint' "${ROOT}/src/portfolioservice.cpp" \
    || fail "Portfolio endpoint reads do not use checked HTTPS fallback"
for ordinary_source in activityservice.cpp tokenservice.cpp transactiondetailservice.cpp; do
    grep -Fq 'SailVaultNetwork::safeHttpsEndpoint' "${ROOT}/src/${ordinary_source}" \
        || fail "Ordinary provider endpoint fallback missing: ${ordinary_source}"
done
grep -Fq 'url.userInfo().isEmpty()' "${ROOT}/src/settingsstore.cpp" \
    || fail "Persistent endpoint sanitizer does not reject URL credentials"
grep -Fq 'QVariantMap AppTools::networkSecurityDiagnostics() const' "${ROOT}/src/apptools.cpp" \
    || fail "Local network security diagnostics are missing"
grep -Fq 'redirectPolicySelfTestPassed' "${ROOT}/src/apptools.cpp" \
    || fail "Local redirect-policy self-test is missing"
for privacy_probe in \
    'requestStatePolicySelfTestPassed' \
    'cookieIsolationPassed' \
    'authIsolationPassed' \
    'cacheIsolationPassed' \
    'contextHeaderIsolationPassed'; do
    grep -Fq "${privacy_probe}" "${ROOT}/src/apptools.cpp" \
        || fail "Local M43 request-state self-test is missing: ${privacy_probe}"
done
grep -Fq 'networkInfo.cookieIsolationPassed' "${ROOT}/qml/pages/DiagnosticsPage.qml" \
    || fail "Developer diagnostics does not expose M43 cookie isolation"
grep -Fq 'networkInfo.authIsolationPassed' "${ROOT}/qml/pages/DiagnosticsPage.qml" \
    || fail "Developer diagnostics does not expose M43 auth isolation"
grep -Fq 'networkInfo.cacheIsolationPassed' "${ROOT}/qml/pages/DiagnosticsPage.qml" \
    || fail "Developer diagnostics does not expose M43 cache isolation"
grep -Fq 'networkInfo.contextHeaderIsolationPassed' "${ROOT}/qml/pages/DiagnosticsPage.qml" \
    || fail "Developer diagnostics does not expose M43 context-header isolation"
grep -Fq 'networkInfo.networkBoundaryPassed' "${ROOT}/qml/pages/DiagnosticsPage.qml" \
    || fail "Developer diagnostics does not expose combined network/privacy status"
grep -Fq '&& networkInfo.networkBoundaryPassed' "${ROOT}/qml/pages/ReleaseReadinessPage.qml" \
    || fail "Release readiness does not gate on M43 network/privacy status"
pass "M43 HTTPS/stateless/redirect/response-boundary contract"

while IFS= read -r qml; do
    [[ -f "${ROOT}/qml/pages/${qml}" ]] || fail "Referenced QML page is missing: ${qml}"
done < <(grep -Rho 'Qt\.resolvedUrl("[^"]*\.qml")' "${ROOT}/qml/pages" \
         | sed 's/.*("//; s/")//' | sort -u)
pass "All resolved QML page references exist"

grep -q 'text: tools.buildLabel' "${ROOT}/qml/cover/CoverPage.qml" \
    || fail "Cover does not use centralized build label"
grep -q 'text: "This build is " + tools.buildLabel' "${ROOT}/qml/pages/AboutPage.qml" \
    || fail "About page does not use centralized build label"
grep -q 'description: tools.buildLabel + " · Milestone " + tools.milestone' \
    "${ROOT}/qml/pages/ReleaseReadinessPage.qml" \
    || fail "Release readiness does not use centralized build label"
if grep -q 'M24r1' "${ROOT}/scripts/build-wallet-core-rust-sb2.sh"; then
    fail "Rust build script still contains stale M24r1 identity"
fi
grep -q 'APP_VERSION=.*Version:' "${ROOT}/scripts/build-wallet-core-rust-sb2.sh" \
    || fail "Rust build script does not derive app version from RPM spec"
pass "Centralized build identity"

grep -q '^VERSION="4.8.3"$' "${ROOT}/scripts/prepare-wallet-core.sh" \
    || fail "Wallet Core prepare script is not pinned to 4.8.3"
grep -q '^PREPARED_ID="${VERSION}-sailvault-m41r1"$' "${ROOT}/scripts/prepare-wallet-core.sh" \
    || fail "Wallet Core preparation marker is not the proven M41r1 baseline"
grep -q '^WC_VERSION="4.8.3"$' "${ROOT}/scripts/generate-wallet-core-sources.sh" \
    || fail "Wallet Core generator version is not 4.8.3"
grep -q '^PROTOBUF_VERSION="3.20.3"$' "${ROOT}/scripts/generate-wallet-core-sources.sh" \
    || fail "Wallet Core protobuf generator is not 3.20.3"
grep -q 'SAILVAULT_WALLET_CORE_VERSION="4.8.3"' "${CMAKE_FILE}" \
    || fail "Application build identity is not Wallet Core 4.8.3"
grep -q 'RUST_FEATURES="any-coin,bitcoin,ethereum,evm,keypair,solana,utils"' \
    "${ROOT}/scripts/build-wallet-core-rust-sb2.sh" \
    || fail "Rust build feature scope is not the SailVault BTC/ETH/SOL set"
grep -q 'version = 3' "${ROOT}/scripts/patch-wallet-core.sh" \
    || fail "Cargo.lock v4-to-v3 compatibility patch is missing"
grep -q 'tw_zcash remains in SailVault Rust registry dependencies' "${ROOT}/scripts/patch-wallet-core.sh" \
    || fail "Rust registry scope guard is missing"
grep -Fq 'hex_string.len() % 2 == 0' "${ROOT}/scripts/patch-wallet-core.sh" \
    || fail "Rust 1.75 tw_encoding compatibility patch is missing"
grep -Fq 'bits % 8 != 0 || bits == 0 || bits > 256' "${ROOT}/scripts/patch-wallet-core.sh" \
    || fail "Rust 1.75 tw_evm compatibility patch is missing"
grep -Fq 'bit_len % 32 == 0' "${ROOT}/scripts/patch-wallet-core.sh" \
    || fail "Rust 1.75 TON compatibility patch is missing"
grep -Fq '__builtin_clzll(lo) + 64' "${ROOT}/scripts/patch-wallet-core.sh" \
    || fail "C++17 countl_zero compatibility patch is missing"
grep -Fq 'DefaultFeeCalculator|DecredFeeCalculator|SegwitFeeCalculator|Zip0317FeeCalculator' "${ROOT}/scripts/patch-wallet-core.sh" \
    || fail "Bitcoin fee calculator constexpr compatibility patch is incomplete"
grep -Fq "replace_exact_line \"\$src\" '#include <bit>' '#include <cstdint>'" "${ROOT}/scripts/patch-wallet-core.sh" \
    || fail "C++17 <bit> compatibility patch is missing"
for wc_stdexcept_path in \
    'src/Everscale/CommonTON/Cell.cpp' \
    'src/Everscale/CommonTON/CellBuilder.cpp' \
    'src/Everscale/CommonTON/CellSlice.cpp' \
    'src/Ontology/ParamsBuilder.cpp' \
    'src/Nano/Signer.cpp' \
    'src/Tron/Deserialization.cpp'; do
    grep -Fq "${wc_stdexcept_path}" "${ROOT}/scripts/patch-wallet-core.sh" \
        || fail "C++17 <stdexcept> hardening missing for ${wc_stdexcept_path}"
done
grep -Fq '#include <stdexcept>' "${ROOT}/scripts/patch-wallet-core.sh" \
    || fail "C++17 standard-exception include hardening is missing"
for wc_chrono_path in \
    'src/MultiversX/TransactionFactoryConfig.cpp' \
    'src/Tron/Signer.cpp'; do
    grep -Fq "${wc_chrono_path}" "${ROOT}/scripts/patch-wallet-core.sh" \
        || fail "chrono duration_cast qualification patch missing for ${wc_chrono_path}"
done
grep -Fq 'std::chrono::duration_cast<std::chrono::seconds>' "${ROOT}/scripts/patch-wallet-core.sh" \
    || fail "MultiversX chrono duration_cast qualification is missing"
grep -Fq 'std::chrono::duration_cast<std::chrono::milliseconds>' "${ROOT}/scripts/patch-wallet-core.sh" \
    || fail "Tron chrono duration_cast qualification is missing"
grep -Fq 'patch_codegen_v2_for_rust_175' "${ROOT}/scripts/patch-wallet-core.sh" \
    || fail "codegen-v2 Cargo 1.75 compatibility patch is missing"
grep -Fq 'CARGO_WORKSPACE_DIR="${WC}/rust"' "${ROOT}/scripts/build-wallet-core-rust-sb2.sh" \
    || fail "Rust FFI manifest output directory is not pinned"
grep -Fq 'cargo build --locked --release -j1' "${ROOT}/scripts/build-wallet-core-rust-sb2.sh" \
    || fail "codegen-v2 build step is missing"
grep -Fq '"${CODEGEN_BIN}" cpp' "${ROOT}/scripts/build-wallet-core-rust-sb2.sh" \
    || fail "codegen-v2 C++ generation step is missing"
grep -Fq 'validate-wallet-core-generated.sh' "${ROOT}/scripts/build-wallet-core-rust-sb2.sh" \
    || fail "generated-binding validation step is missing"
[[ -x "${ROOT}/scripts/validate-wallet-core-generated.sh" ]] \
    || fail "generated-binding validator is missing or not executable"
grep -q 'nativeTokenName' "${ROOT}/scripts/generate-wallet-core-registry.cmake" \
    || fail "Wallet Core 4.8.3 nativeTokenName registry generation is missing"
for derivation_abi in \
    'TWDerivationBitcoinSegwit|2' \
    'TWDerivationBitcoinLegacy|3' \
    'TWDerivationBitcoinTestnet|4' \
    'TWDerivationLitecoinLegacy|5' \
    'TWDerivationSolanaSolana|6' \
    'TWDerivationStratisSegwit|7' \
    'TWDerivationBitcoinTaproot|8' \
    'TWDerivationPactusMainnet|9' \
    'TWDerivationPactusTestnet|10' \
    'TWDerivationSmartChainStableAccount|11'; do
    grep -Fq "\"${derivation_abi}\"" "${ROOT}/scripts/generate-wallet-core-registry.cmake" \
        || fail "Canonical Wallet Core 4.8.3 derivation ABI entry missing: ${derivation_abi}"
done
grep -Fq 'list(FIND CANONICAL_DERIVATION_NAMES "${DERIV_ENUM}" DERIV_ENUM_INDEX)' \
    "${ROOT}/scripts/generate-wallet-core-registry.cmake" \
    || fail "CoinInfoData derivation ABI generation guard is missing"
grep -Fq 'string(REGEX REPLACE "[^A-Za-z0-9]+" ";" _parts' \
    "${ROOT}/scripts/generate-wallet-core-registry.cmake" \
    || fail "Delimiter-aware derivation CamelCase conversion is missing"
pass "Wallet Core 4.8.3 Sailfish build contract"

PROBE="${ROOT}/src/walletcoreprobe.cpp"
for probe_contract in \
    'static_assert(TWDerivationBitcoinSegwit == 2' \
    'static_assert(TWDerivationSmartChainStableAccount == 11' \
    'static_assert(TWCoinTypeBitcoin == 0' \
    'static_assert(TWCoinTypeEthereum == 60' \
    'static_assert(TWCoinTypeSolana == 501' \
    'TWMnemonicIsValid' \
    'TWAnyAddressIsValid' \
    'TWAnyAddressCreateWithPublicKey' \
    '2147483648' \
    'shortSignature' \
    'TWCryptoBoxPublicKeyIsValid' \
    'TWCryptoBoxEncryptEasy' \
    'TWCryptoBoxDecryptEasy'; do
    grep -Fq "${probe_contract}" "${PROBE}" \
        || fail "M41 Wallet Core probe contract missing: ${probe_contract}"
done
if grep -q 'TWPrivateKeyData' "${PROBE}"; then
    fail "M41 probe must not export Wallet Core private-key bytes"
fi
pass "M41 Wallet Core security/ABI probe contract"

grep -q 'Q_PROPERTY(bool storageProtected READ storageProtected' \
    "${ROOT}/src/walletvault.h" \
    || fail "WalletVault does not expose the protected/relocked storage state"
grep -q 'return vault && vault.backendReady' \
    "${ROOT}/qml/pages/ReleaseReadinessPage.qml" \
    || fail "Release readiness does not use the Secrets backend as its storage gate"
if grep -q 'vault.refreshStatus()' "${ROOT}/qml/pages/ReleaseReadinessPage.qml"; then
    fail "Release readiness must not interrogate the secure wallet collection"
fi
grep -q 'DeviceLockRelock' "${ROOT}/src/walletvault.cpp" \
    || fail "Wallet collection must retain DeviceLockRelock protection"
pass "Release-readiness no-wallet-probe contract"

grep -q 'const QString expected = QString::fromLatin1(kPublicTestMnemonic)' \
    "${ROOT}/src/walletvault.cpp" \
    || fail "Development restore no longer binds to the public test mnemonic"
grep -q 'if (normalized != expected)' "${ROOT}/src/walletvault.cpp" \
    || fail "Development restore no longer rejects other recovery phrases"
grep -q 'Development build accepts only the public test phrase' "${ROOT}/src/walletvault.cpp" \
    || fail "Development restore refusal path is missing"
pass "Development wallet remains restricted to the published test mnemonic"

if [[ -f "${WC}/.sailvault-wallet-core-version" ]]; then
    MARKER="$(cat "${WC}/.sailvault-wallet-core-version")"
    echo "PASS  Wallet Core source already prepared: ${MARKER}"
    [[ "${MARKER}" == "4.8.3-sailvault-m41r1" ]] \
        || fail "Unexpected Wallet Core preparation marker: ${MARKER}"
    if [[ -s "${WC}/include/TrustWalletCore/TWDerivation.h" ]] &&
       [[ -s "${WC}/src/proto/Algorand.pb.h" ]]; then
        echo "PASS  Wallet Core generated C/C++ sources"
    else
        fail "Wallet Core preparation marker exists but generated C/C++ sources are incomplete"
    fi
else
    echo "INFO  Wallet Core is not prepared yet; normal RPM build will prepare it."
fi

for script in "${ROOT}"/scripts/*.sh; do
    bash -n "${script}" || fail "Shell syntax failed: ${script#${ROOT}/}"
done
pass "Shell script syntax"

echo
echo "No manual bootstrap is required."
echo "M43 remains read-only: no transaction construction, user signing UI or broadcasting."
