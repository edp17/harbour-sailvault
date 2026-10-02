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

printf 'SailVault %s release preflight\n' "${PACKAGE_VERSION}"
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
    "${ROOT}/docs/PROVIDER_RESPONSE_REVIEW.md" \
    "${ROOT}/docs/M44_DEVICE_CHECKLIST.md" \
    "${ROOT}/docs/WALLET_CORE_4_8_4_REVIEW.md" \
    "${ROOT}/docs/M45_DEVICE_CHECKLIST.md" \
    "${ROOT}/docs/UI_CONSISTENCY_REVIEW.md" \
    "${ROOT}/docs/M46_DEVICE_CHECKLIST.md" \
    "${ROOT}/docs/STORAGE_LIFECYCLE_REVIEW.md" \
    "${ROOT}/docs/M47_DEVICE_CHECKLIST.md" \
    "${ROOT}/docs/UNSIGNED_TRANSACTION_INTENT_REVIEW.md" \
    "${ROOT}/docs/M48_DEVICE_CHECKLIST.md" \
    "${ROOT}/docs/ETHEREUM_UNSIGNED_CONSTRUCTION_REVIEW.md" \
    "${ROOT}/docs/M49_DEVICE_CHECKLIST.md" \
    "${ROOT}/docs/BITCOIN_UNSIGNED_CONSTRUCTION_REVIEW.md" \
    "${ROOT}/docs/M50_DEVICE_CHECKLIST.md" \
    "${ROOT}/docs/SOLANA_UNSIGNED_CONSTRUCTION_REVIEW.md" \
    "${ROOT}/docs/M51_DEVICE_CHECKLIST.md" \
    "${ROOT}/docs/DEVELOPMENT_SIGNING_BOUNDARY_REVIEW.md" \
    "${ROOT}/docs/M52_DEVICE_CHECKLIST.md" \
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
grep -Fq "# SailVault M44 — provider response validation review" \
    "${ROOT}/docs/PROVIDER_RESPONSE_REVIEW.md" \
    || fail "M44 provider-response review is missing or stale"
grep -Fq "# SailVault M44 device checklist" \
    "${ROOT}/docs/M44_DEVICE_CHECKLIST.md" \
    || fail "M44 device checklist is missing or stale"
grep -Fq "# SailVault M45 — Wallet Core 4.8.4 maintained-tip review" \
    "${ROOT}/docs/WALLET_CORE_4_8_4_REVIEW.md" \
    || fail "M45 Wallet Core 4.8.4 review is missing or stale"
grep -Fq "# SailVault M45 device checklist" \
    "${ROOT}/docs/M45_DEVICE_CHECKLIST.md" \
    || fail "M45 device checklist is missing or stale"
grep -Fq "# SailVault M46 — Sailfish UI consistency review" \
    "${ROOT}/docs/UI_CONSISTENCY_REVIEW.md" \
    || fail "M46 UI consistency review is missing or stale"
grep -Fq "# SailVault M46 device checklist" \
    "${ROOT}/docs/M46_DEVICE_CHECKLIST.md" \
    || fail "M46 device checklist is missing or stale"
grep -Fq "# SailVault M47 — storage lifecycle and release-gate review" \
    "${ROOT}/docs/STORAGE_LIFECYCLE_REVIEW.md" \
    || fail "M47 storage lifecycle review is missing or stale"
grep -Fq "# SailVault M47 device checklist" \
    "${ROOT}/docs/M47_DEVICE_CHECKLIST.md" \
    || fail "M47 device checklist is missing or stale"
grep -Fq "# SailVault M48 — unsigned transaction intent and review model" \
    "${ROOT}/docs/UNSIGNED_TRANSACTION_INTENT_REVIEW.md" \
    || fail "M48 unsigned transaction review is missing or stale"
grep -Fq "# SailVault M48 device checklist" \
    "${ROOT}/docs/M48_DEVICE_CHECKLIST.md" \
    || fail "M48 device checklist is missing or stale"
grep -Fq "# SailVault M49 — Ethereum unsigned construction review" \
    "${ROOT}/docs/ETHEREUM_UNSIGNED_CONSTRUCTION_REVIEW.md" \
    || fail "M49 Ethereum construction review is missing or stale"
grep -Fq "# SailVault M49 device checklist" \
    "${ROOT}/docs/M49_DEVICE_CHECKLIST.md" \
    || fail "M49 device checklist is missing or stale"
grep -Fq "# M50 Bitcoin unsigned construction review" \
    "${ROOT}/docs/BITCOIN_UNSIGNED_CONSTRUCTION_REVIEW.md" \
    || fail "M50 Bitcoin construction review is missing or stale"
grep -Fq "# M50 device checklist" \
    "${ROOT}/docs/M50_DEVICE_CHECKLIST.md" \
    || fail "M50 device checklist is missing or stale"
grep -Fq "# SailVault M51 — Solana unsigned construction review" \
    "${ROOT}/docs/SOLANA_UNSIGNED_CONSTRUCTION_REVIEW.md" \
    || fail "M51 Solana construction review is missing or stale"
grep -Fq "# SailVault M51r2 device checklist" \
    "${ROOT}/docs/M51_DEVICE_CHECKLIST.md" \
    || fail "M51 device checklist is missing or stale"
grep -Fq "# SailVault M52 — development signing boundary review" \
    "${ROOT}/docs/DEVELOPMENT_SIGNING_BOUNDARY_REVIEW.md" \
    || fail "M52 development signing-boundary review is missing or stale"
grep -Fq "# SailVault M52 device checklist" \
    "${ROOT}/docs/M52_DEVICE_CHECKLIST.md" \
    || fail "M52 device checklist is missing or stale"
[[ "${CMAKE_BUILD_LABEL}" == "development signing boundary" ]] \
    || fail "Unexpected M52 build label: ${CMAKE_BUILD_LABEL}"
pass "M52 build/document identity"

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
    'Provider response exceeded %1 safety limit' \
    'Provider response missing JSON Content-Type' \
    'Provider response Content-Type is %1 · expected JSON' \
    'JSON-RPC version is not 2.0' \
    'JSON-RPC response ID does not match request' \
    'JSON-RPC response must contain exactly one of result or error'; do
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
    networkfeeservice.cpp \
    ethereumunsignedtransactionservice.cpp; do
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
grep -Fq 'networkInfo.jsonMediaTypePolicyPassed' "${ROOT}/qml/pages/DiagnosticsPage.qml" \
    || fail "Developer diagnostics does not expose M44 media-type validation"
grep -Fq 'networkInfo.jsonRpcEnvelopePolicyPassed' "${ROOT}/qml/pages/DiagnosticsPage.qml" \
    || fail "Developer diagnostics does not expose M44 JSON-RPC validation"
grep -Fq 'networkInfo.providerResponsePolicyPassed' "${ROOT}/qml/pages/ReleaseReadinessPage.qml" \
    || fail "Release readiness does not expose M44 provider-response validation"
for response_probe in \
    'jsonMediaTypePolicyPassed' \
    'jsonRpcEnvelopePolicyPassed' \
    'providerResponsePolicyPassed'; do
    grep -Fq "${response_probe}" "${ROOT}/src/apptools.cpp" \
        || fail "Local M44 provider-response self-test is missing: ${response_probe}"
done
RPC_VALIDATION_COUNT="$(grep -h 'SailVaultNetwork::validateJsonRpcEnvelope' \
    "${ROOT}"/src/activityservice.cpp \
    "${ROOT}"/src/networkfeeservice.cpp \
    "${ROOT}"/src/networkhealthservice.cpp \
    "${ROOT}"/src/portfolioservice.cpp \
    "${ROOT}"/src/tokenservice.cpp \
    "${ROOT}"/src/transactiondetailservice.cpp | wc -l)"
[[ "${RPC_VALIDATION_COUNT}" -eq 10 ]] \
    || fail "Expected 10 runtime JSON-RPC envelope validation sites, found ${RPC_VALIDATION_COUNT}"
LOCAL_RPC_TEST_COUNT="$(grep -Fc 'SailVaultNetwork::validateJsonRpcEnvelope' \
    "${ROOT}/src/apptools.cpp" || true)"
[[ "${LOCAL_RPC_TEST_COUNT}" -eq 5 ]] \
    || fail "Expected 5 local JSON-RPC policy self-tests, found ${LOCAL_RPC_TEST_COUNT}"
grep -Fq 'SailVaultNetwork::plainTextErrorText' "${ROOT}/src/networkhealthservice.cpp" \
    || fail "Bitcoin genesis text/plain response validation is missing"
grep -Fq 'networkInfo.networkBoundaryPassed' "${ROOT}/qml/pages/DiagnosticsPage.qml" \
    || fail "Developer diagnostics does not expose combined network/privacy status"
grep -Fq '&& networkInfo.networkBoundaryPassed' "${ROOT}/qml/pages/ReleaseReadinessPage.qml" \
    || fail "Release readiness does not gate on M43 network/privacy status"
pass "M44 HTTPS/stateless/bounded/provider-response contract"

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

# M46 Sailfish-native UI contract. Runtime typography must use Theme roles,
# and Developer diagnostics uses one pulley action rather than duplicate buttons.
if grep -RInE --include='*.qml' 'font\.(pixelSize|pointSize):[[:space:]]*[0-9]' "${ROOT}/qml" >/dev/null 2>&1; then
    fail "Literal QML font size found; use Sailfish Theme.fontSize* roles"
fi
grep -Fq 'text: "Run all diagnostics again"' "${ROOT}/qml/pages/DiagnosticsPage.qml" \
    || fail "Developer diagnostics consolidated pulley action is missing"
grep -Fq 'PullDownMenu {' "${ROOT}/qml/pages/DiagnosticsPage.qml" \
    || fail "Developer diagnostics does not use a Sailfish pulley action"
if grep -Fq 'text: "Run storage probe again"' "${ROOT}/qml/pages/DiagnosticsPage.qml" \
   || grep -Fq 'text: "Run diagnostics again"' "${ROOT}/qml/pages/DiagnosticsPage.qml"; then
    fail "Legacy in-page diagnostics rerun buttons remain"
fi
if grep -Fq 'text: "Offline address QR"' "${ROOT}/qml/pages/ReceivePage.qml"; then
    fail "Receive page still contains the redundant QR subheading"
fi
pass "M46 Sailfish typography/interaction contract"

# M47 storage lifecycle/release gate. The real profile keeps a stable internal
# install identity while fresh/upgrade/import behavior is exercised in QTemporaryDir.
SETTINGS_STORE="${ROOT}/src/settingsstore.cpp"
for storage_contract in \
    'const int kCurrentSchemaVersion = 5' \
    'const char kProfileOriginKey[] = "meta/profileOrigin"' \
    'const char kFirstRecordedAppVersionKey[] = "meta/firstRecordedAppVersion"' \
    'const char kLastUpgradeFromVersionKey[] = "meta/lastUpgradeFromVersion"' \
    'QTemporaryDir tempDir' \
    'Fresh-profile simulation failed' \
    'upgrade.setValue(QString::fromLatin1(kSchemaKey), 4)' \
    'Upgrade-preservation simulation failed' \
    'Legacy-import/sanitization simulation failed' \
    'storageReleaseGatePassed' \
    'installIdentityPresent' \
    'profileOrigin' \
    'firstRecordedAppVersion' \
    'lastUpgradeFromVersion'; do
    grep -Fq "${storage_contract}" "${SETTINGS_STORE}" \
        || fail "M47 storage lifecycle contract missing: ${storage_contract}"
done
grep -Fq 'SailVaultSettings::runStorageLifecycleProbe();' "${ROOT}/src/apptools.cpp" \
    || fail "M47 diagnostics rerun does not execute the lifecycle probe"
grep -Fq 'storageInfo.storageReleaseGatePassed' "${ROOT}/qml/pages/ReleaseReadinessPage.qml" \
    || fail "Release readiness does not gate on M47 storage lifecycle validation"
grep -Fq 'storageInfo.storageLifecycleProbePassed' "${ROOT}/qml/pages/DiagnosticsPage.qml" \
    || fail "Developer diagnostics does not expose M47 lifecycle self-test"
grep -Fq 'storageInfo.installIdentityPresent' "${ROOT}/qml/pages/DiagnosticsPage.qml" \
    || fail "Developer diagnostics does not expose install-identity presence"
if grep -RInE --include='*.qml' '(^|[^A-Za-z0-9_])installId([^A-Za-z0-9_]|$)' "${ROOT}/qml" >/dev/null 2>&1; then
    fail "Internal install UUID must not be exposed directly to QML"
fi
if grep -Eqi 'WalletVault|Sailfish::Secrets|private[ -]?key|recovery phrase|mnemonic' "${SETTINGS_STORE}"; then
    fail "M47 non-sensitive settings lifecycle code must not access wallet secrets"
fi
pass "M47 storage lifecycle/release-gate contract"

# M48 unsigned transaction intent/review gate. This is deliberately public-data-only:
# no network manager, settings persistence, WalletVault/Secrets access or chain signing APIs.
UNSIGNED_HEADER="${ROOT}/src/unsignedtransactionservice.h"
UNSIGNED_SOURCE="${ROOT}/src/unsignedtransactionservice.cpp"
for unsigned_contract in \
    'class UnsignedTransactionService : public QObject' \
    'Q_PROPERTY(bool readyForReview READ readyForReview' \
    'Q_INVOKABLE QVariantMap reviewModel() const' \
    'Q_INVOKABLE QVariantMap runSelfTest() const'; do
    grep -Fq "${unsigned_contract}" "${UNSIGNED_HEADER}" \
        || fail "M48 unsigned-intent interface missing: ${unsigned_contract}"
done
for unsigned_contract in \
    'TWAnyAddressIsValid' \
    'TWCoinTypeEthereum' \
    'TWCoinTypeBitcoin' \
    'TWCoinTypeSolana' \
    'info.decimals = 18' \
    'info.decimals = 8' \
    'info.decimals = 9' \
    '^[0-9]+(?:\\.[0-9]+)?$' \
    'QCryptographicHash::Sha256' \
    'sailvault-intent-v1' \
    'Intent only · chain transaction not constructed' \
    'fingerprintValidationPassed'; do
    grep -Fq "${unsigned_contract}" "${UNSIGNED_SOURCE}" \
        || fail "M48 unsigned-intent contract missing: ${unsigned_contract}"
done
if grep -Eqi '#include .*QNetwork|QNetworkAccessManager|settingsstore|walletvault|Sailfish::Secrets|<Secrets/' \
        "${UNSIGNED_HEADER}" "${UNSIGNED_SOURCE}"; then
    fail "M48 unsigned-intent layer crossed the network/settings/Secrets boundary"
fi
if grep -Eqi 'TWAnySigner|TWTransactionCompiler|TWPrivateKey|TWHDWallet|TWDataCreateWithBytes' \
        "${UNSIGNED_HEADER}" "${UNSIGNED_SOURCE}"; then
    fail "M48 unsigned-intent layer must not construct/sign Wallet Core transactions"
fi
grep -Fq 'text: "Prepare transfer"' "${ROOT}/qml/pages/ChainPage.qml" \
    || fail "M48 development-wallet Prepare transfer entry point is missing"
grep -Fq 'Qt.resolvedUrl("TransactionDraftPage.qml")' "${ROOT}/qml/pages/ChainPage.qml" \
    || fail "M48 ChainPage does not open the draft page"
grep -Fq 'text: "Review unsigned intent"' "${ROOT}/qml/pages/TransactionDraftPage.qml" \
    || fail "M48 draft review action is missing"
grep -Fq 'enabled: false' "${ROOT}/qml/pages/TransactionReviewPage.qml" \
    || fail "M48 review page does not visibly keep signing disabled"
grep -Fq 'text: "Production signing unavailable in M52"' "${ROOT}/qml/pages/TransactionReviewPage.qml" \
    || fail "Current production-signing boundary is not visible"
grep -Fq 'transactionInfo = unsignedIntent.runSelfTest()' "${ROOT}/qml/pages/DiagnosticsPage.qml" \
    || fail "Developer diagnostics does not run the M48 intent self-test"
grep -Fq '&& transactionInfo.passed' "${ROOT}/qml/pages/ReleaseReadinessPage.qml" \
    || fail "Release readiness does not gate on M48 intent validation"
if grep -Fq 'Prepare transfer' "${ROOT}/qml/pages/PublicAddressPage.qml" \
   || grep -Fq 'Prepare transfer' "${ROOT}/qml/pages/WatchOnlyPage.qml" \
   || grep -Fq 'Prepare transfer' "${ROOT}/qml/pages/AddressBookPage.qml"; then
    fail "M48 Prepare transfer must remain development-wallet-only"
fi
pass "M48 unsigned transaction intent/review boundary"


# M49 Ethereum unsigned construction gate. This layer may use the hardened public
# network boundary and Wallet Core hashing, but it must not cross into Secrets,
# private-key handling, signing or broadcasting.
ETH_HEADER="${ROOT}/src/ethereumunsignedtransactionservice.h"
ETH_SOURCE="${ROOT}/src/ethereumunsignedtransactionservice.cpp"
for eth_contract in \
    'class EthereumUnsignedTransactionService : public QObject' \
    'Q_INVOKABLE void construct(const QVariantMap &intent' \
    'Q_INVOKABLE QVariantMap runSelfTest() const' \
    'QNetworkAccessManager m_network'; do
    grep -Fq "${eth_contract}" "${ETH_HEADER}" \
        || fail "M49 Ethereum constructor interface missing: ${eth_contract}"
done
for eth_contract in \
    'UnsignedTransactionService::validateReviewModel' \
    'eth_chainId' \
    'eth_getTransactionCount' \
    'eth_getBlockByNumber' \
    'eth_maxPriorityFeePerGas' \
    'eth_gasPrice' \
    'eth_getCode' \
    'm_gasLimit = 21000' \
    'TWHashKeccak256' \
    'sailvault-eth-unsigned-v1' \
    'ed5ffeca0540f40606b47a01aaf578365c2149d60220703eae796bc2a01da6d0' \
    'eip1559RlpVectorPassed' \
    'keccakVectorPassed' \
    'intentBindingPassed' \
    'constructionFingerprintPassed'; do
    grep -Fq "${eth_contract}" "${ETH_SOURCE}" \
        || fail "M49 Ethereum construction contract missing: ${eth_contract}"
done
grep -Fq 'SailVaultNetwork::hardenRequest(request)' "${ETH_SOURCE}" \
    || fail "M49 Ethereum constructor does not use shared request hardening"
grep -Fq 'SailVaultNetwork::validateJsonRpcEnvelope' "${ETH_SOURCE}" \
    || fail "M49 Ethereum constructor does not validate JSON-RPC envelopes"
grep -Fq 'SailVaultNetwork::jsonErrorText' "${ETH_SOURCE}" \
    || fail "M49 Ethereum constructor does not validate JSON response media/network policy"
grep -Fq 'SailVaultNetwork::offlineModeEnabled()' "${ETH_SOURCE}" \
    || fail "M49 Ethereum construction ignores Offline mode"
if grep -Eqi 'walletvault|Sailfish::Secrets|<Secrets/|TWPrivateKey|TWHDWallet|TWAnySigner|TWTransactionCompiler' \
        "${ETH_HEADER}" "${ETH_SOURCE}"; then
    fail "M49 Ethereum constructor crossed the Secrets/private-key/signing boundary"
fi
if grep -Eqi 'sendRawTransaction|eth_sendRawTransaction' "${ETH_HEADER}" "${ETH_SOURCE}"; then
    fail "M49 Ethereum constructor must not contain a broadcast API"
fi
grep -Fq '"Construct unsigned Ethereum payload"' "${ROOT}/qml/pages/TransactionReviewPage.qml" \
    || fail "M49 explicit Ethereum construction action is missing"
grep -Fq 'ethBuilder.construct(transaction, ethereumRpcUrl)' "${ROOT}/qml/pages/TransactionReviewPage.qml" \
    || fail "M49 review page does not bind the reviewed intent to construction"
grep -Fq 'text: "Production signing unavailable in M52"' "${ROOT}/qml/pages/TransactionReviewPage.qml" \
    || fail "Ethereum path no longer visibly disables production signing"
grep -Fq 'ethereumConstructionInfo = ethereumBuilder.runSelfTest()' "${ROOT}/qml/pages/DiagnosticsPage.qml" \
    || fail "Developer diagnostics does not run the M49 Ethereum construction self-test"
grep -Fq '&& ethereumConstructionInfo.passed' "${ROOT}/qml/pages/ReleaseReadinessPage.qml" \
    || fail "Release readiness does not gate on M49 Ethereum construction validation"
grep -Fq 'static bool validateReviewModel' "${ROOT}/src/unsignedtransactionservice.h" \
    || fail "M49 does not expose shared reviewed-intent validation"
pass "M49 Ethereum unsigned-construction/signing boundary"


# M50 Bitcoin unsigned construction gate. The constructor may use the hardened
# public network boundary and Wallet Core script/hash helpers, but must remain
# completely disconnected from secrets, signing and broadcast APIs.
BTC_HEADER="${ROOT}/src/bitcoinunsignedtransactionservice.h"
BTC_SOURCE="${ROOT}/src/bitcoinunsignedtransactionservice.cpp"
for btc_contract in \
    'class BitcoinUnsignedTransactionService : public QObject' \
    'Q_INVOKABLE void construct(const QVariantMap &intent' \
    'Q_INVOKABLE QVariantMap runSelfTest() const' \
    'QNetworkAccessManager m_network'; do
    grep -Fq "${btc_contract}" "${BTC_HEADER}" \
        || fail "M50 Bitcoin constructor interface missing: ${btc_contract}"
done
for btc_contract in \
    'UnsignedTransactionService::validateReviewModel' \
    '/block-height/0' \
    '/utxo' \
    '/fee-estimates' \
    '000000000019d6689c085ae165831e934ff763ae46a2a6c172b3f1b60a8ce26f' \
    'kDustThresholdSats = 546' \
    'kSequenceRbf = 0xfffffffd' \
    'TWBitcoinScriptLockScriptForAddress' \
    'TWBitcoinScriptIsPayToWitnessPublicKeyHash' \
    'TWHashSHA256SHA256' \
    'PSBT_IN_WITNESS_UTXO' \
    'PSBT_IN_SIGHASH_TYPE' \
    'SailVault M50 Bitcoin construction v1' \
    'coinSelectionPassed' \
    'serializationVectorPassed' \
    'psbtPassed' \
    'constructionFingerprintPassed'; do
    grep -Fq "${btc_contract}" "${BTC_SOURCE}" \
        || fail "M50 Bitcoin construction contract missing: ${btc_contract}"
done
grep -Fq 'SailVaultNetwork::hardenRequest(request)' "${BTC_SOURCE}" \
    || fail "M50 Bitcoin constructor does not use shared request hardening"
grep -Fq 'SailVaultNetwork::jsonErrorText' "${BTC_SOURCE}" \
    || fail "M50 Bitcoin JSON responses do not use shared response policy"
grep -Fq 'SailVaultNetwork::plainTextErrorText' "${BTC_SOURCE}" \
    || fail "M50 Bitcoin genesis response does not enforce text/plain"
grep -Fq 'SailVaultNetwork::offlineModeEnabled()' "${BTC_SOURCE}" \
    || fail "M50 Bitcoin construction ignores Offline mode"
if grep -Eqi 'walletvault|Sailfish::Secrets|<Secrets/|TWPrivateKey|TWHDWallet|TWAnySigner|TWTransactionCompiler' \
        "${BTC_HEADER}" "${BTC_SOURCE}"; then
    fail "M50 Bitcoin constructor crossed the Secrets/private-key/signing boundary"
fi
if grep -Eqi 'sendRawTransaction|broadcast|/tx[^a-zA-Z]' "${BTC_HEADER}" "${BTC_SOURCE}"; then
    fail "M50 Bitcoin constructor must not contain a broadcast API"
fi
grep -Fq '"Construct unsigned Bitcoin transaction"' "${ROOT}/qml/pages/TransactionReviewPage.qml" \
    || fail "M50 explicit Bitcoin construction action is missing"
grep -Fq 'btcBuilder.construct(transaction, bitcoinApiUrl)' "${ROOT}/qml/pages/TransactionReviewPage.qml" \
    || fail "M50 review page does not bind reviewed Bitcoin intent to construction"
grep -Fq 'bitcoinConstructionInfo = bitcoinBuilder.runSelfTest()' "${ROOT}/qml/pages/DiagnosticsPage.qml" \
    || fail "Developer diagnostics does not run the M50 Bitcoin construction self-test"
grep -Fq '&& bitcoinConstructionInfo.passed' "${ROOT}/qml/pages/ReleaseReadinessPage.qml" \
    || fail "Release readiness does not gate on M50 Bitcoin construction validation"
pass "M50 Bitcoin unsigned-construction/signing boundary"

# M51 Solana unsigned construction gate. This service may contact the configured
# public RPC after explicit user action, but it must stop at Wallet Core's external
# pre-signing message and remain entirely outside WalletVault/Sailfish Secrets.
SOL_HEADER="${ROOT}/src/solanaunsignedtransactionservice.h"
SOL_SOURCE="${ROOT}/src/solanaunsignedtransactionservice.cpp"
for sol_contract in \
    'class SolanaUnsignedTransactionService : public QObject' \
    'Q_INVOKABLE void construct(const QVariantMap &intent' \
    'Q_INVOKABLE QVariantMap runSelfTest() const' \
    'QNetworkAccessManager m_network'; do
    grep -Fq "${sol_contract}" "${SOL_HEADER}" \
        || fail "M51 Solana constructor interface missing: ${sol_contract}"
done
for sol_contract in \
    'UnsignedTransactionService::validateReviewModel' \
    '5eykt4UsFv8P8NJdTREpY1vzqKqZKvdpKuc147dw2N9d' \
    'getGenesisHash' \
    'getLatestBlockhash' \
    'getFeeForMessage' \
    'TWBase58DecodeNoCheck' \
    'TWTransactionCompilerPreImageHashes' \
    'TWCoinTypeSolana' \
    'sailvault-solana-unsigned-v1' \
    '3b97097d9c22adc9d86c1a3db8822b4b009939f58c384d059aacf1af11393136' \
    'AQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAABAAEDDQRKYtCk3+WgN6FbWfpNTQ06uBEDosEKbaCKTQWGEcAkwlWovD6EliF6LNKhiUubncrOBPzZwNWZrNqupAobYQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAABsJQEswRpZmkWzsvf4p8ZbBUf6C7ZxcNegzR7aTiyeUBAgIAAQwCAAAA6AMAAAAAAAA=' \
    'walletCorePreimageVectorPassed' \
    'transactionTemplatePassed' \
    'constructionFingerprintPassed'; do
    grep -Fq "${sol_contract}" "${SOL_SOURCE}" \
        || fail "M51 Solana construction contract missing: ${sol_contract}"
done
grep -Fq 'SailVaultNetwork::hardenRequest(request)' "${SOL_SOURCE}" \
    || fail "M51 Solana constructor does not use shared request hardening"
grep -Fq 'SailVaultNetwork::jsonErrorText' "${SOL_SOURCE}" \
    || fail "M51 Solana responses do not use shared response policy"
grep -Fq 'SailVaultNetwork::validateJsonRpcEnvelope' "${SOL_SOURCE}" \
    || fail "M51 Solana constructor does not validate JSON-RPC envelopes"
grep -Fq 'SailVaultNetwork::offlineModeEnabled()' "${SOL_SOURCE}" \
    || fail "M51 Solana construction ignores Offline mode"
if grep -Eqi 'walletvault|Sailfish::Secrets|<Secrets/|TWPrivateKey|TWHDWallet|TWAnySigner|CompileWithSignatures' \
        "${SOL_HEADER}" "${SOL_SOURCE}"; then
    fail "M51 Solana constructor crossed the Secrets/private-key/signing boundary"
fi
if grep -Eqi 'sendTransaction|sendRawTransaction|broadcast' "${SOL_HEADER}" "${SOL_SOURCE}"; then
    fail "M51 Solana constructor must not contain a broadcast API"
fi
grep -Fq '"Construct unsigned Solana message"' "${ROOT}/qml/pages/TransactionReviewPage.qml" \
    || fail "M51 explicit Solana construction action is missing"
grep -Fq 'solBuilder.construct(transaction, solanaRpcUrl)' "${ROOT}/qml/pages/TransactionReviewPage.qml" \
    || fail "M51 review page does not bind reviewed Solana intent to construction"
grep -Fq 'solanaRpcUrl: portfolio ? portfolio.solanaRpcUrl : ""' "${ROOT}/qml/pages/ChainPage.qml" \
    || fail "M51 ChainPage does not pass the configured Solana RPC to the draft/review flow"
grep -Fq 'solanaConstructionInfo = solanaBuilder.runSelfTest()' "${ROOT}/qml/pages/DiagnosticsPage.qml" \
    || fail "Developer diagnostics does not run the M51 Solana construction self-test"
grep -Fq '&& solanaConstructionInfo.passed' "${ROOT}/qml/pages/ReleaseReadinessPage.qml" \
    || fail "Release readiness does not gate on M51 Solana construction validation"
grep -Fq 'text: "Production signing unavailable in M52"' "${ROOT}/qml/pages/TransactionReviewPage.qml" \
    || fail "M51 Solana path no longer visibly keeps production signing disabled"
pass "M51 Solana unsigned-construction/signing boundary"

# M52 development signing boundary. This is the only deliberate secret/signing
# crossing in the current application. It must remain restricted to the published
# public test wallet, Ethereum-only, non-exporting, fresh-construction-only and
# incapable of assembling or broadcasting a signed transaction.
SIGN_HEADER="${ROOT}/src/developmentsigningservice.h"
SIGN_SOURCE="${ROOT}/src/developmentsigningservice.cpp"
VAULT_HEADER="${ROOT}/src/walletvault.h"
VAULT_SOURCE="${ROOT}/src/walletvault.cpp"
for sign_contract in \
    'class DevelopmentSigningService : public QObject' \
    'Q_INVOKABLE void signEthereum(QObject *vaultObject' \
    'Q_INVOKABLE QVariantMap runSelfTest() const' \
    'kMaximumConstructionAgeMs = 120000' \
    'signDigestWithDevelopmentKey(TWPrivateKey *key' \
    'signatureProofFingerprint READ signatureProofFingerprint'; do
    grep -Fq "${sign_contract}" "${SIGN_HEADER}" \
        || fail "M52 development signer interface missing: ${sign_contract}"
done
for sign_contract in \
    '0x9858EfFD232B4033E47d90003D41EC34EcaEda94' \
    'bc1qcr8te4kr609gcawutmrza0j4xv80jy8z306fyu' \
    'GjJyeC1r2RgkuoCWMyPYkCWSGSGLcz266EaAkLA27AhL' \
    'EthereumUnsignedTransactionService::validateSigningSnapshot' \
    'vault->withVerifiedDevelopmentEthereumKey' \
    'TWAnyAddressCreateWithPublicKey' \
    'TWPrivateKeySign' \
    'TWCurveSECP256k1' \
    'TWPublicKeyVerify' \
    'TWDataReset' \
    'sailvault-development-eth-signature-proof-v1' \
    'rawSignatureExported' \
    'secureErase(&signature)' \
    'no signed transaction assembled'; do
    grep -Fq "${sign_contract}" "${SIGN_SOURCE}" \
        || fail "M52 signing-boundary contract missing: ${sign_contract}"
done
# The signer must never export private key bytes, own a network channel, invoke
# generic Wallet Core signers/compilers, or contain a transaction broadcast API.
if grep -Eqi 'TWPrivateKeyData|QNetworkAccessManager|QNetworkRequest|TWAnySignerSign|TWTransactionCompilerCompileWithSignatures|eth_sendRawTransaction|sendRawTransaction|sendTransaction' \
        "${SIGN_HEADER}" "${SIGN_SOURCE}"; then
    fail "M52 development signer crossed the non-exporting/no-network/no-broadcast boundary"
fi
if grep -Eqi 'Q_PROPERTY\([^)]*(mnemonic|privateKey|signatureHex)|Q_INVOKABLE[^\n]*(mnemonic|privateKey|signatureHex)' \
        "${SIGN_HEADER}"; then
    fail "M52 development signer exposes secret/raw-signature material to QML"
fi
# WalletVault owns secret reopening. The callback is private C++ only and must
# revalidate the public session against the secret immediately before use.
grep -Fq 'friend class DevelopmentSigningService;' "${VAULT_HEADER}" \
    || fail "M52 WalletVault friend boundary is missing"
grep -Fq 'bool withVerifiedDevelopmentEthereumKey(' "${VAULT_HEADER}" \
    || fail "M52 WalletVault transient-Ethereum-key callback is missing"
if grep -Fq 'Q_INVOKABLE bool withVerifiedDevelopmentEthereumKey' "${VAULT_HEADER}"; then
    fail "M52 secret callback must not be QML-invokable"
fi
for vault_contract in \
    'const SecretFetchState fetched = fetchMnemonic(&mnemonic, &fetchError)' \
    'const DerivedWallet verified = deriveAndCheck(mnemonic)' \
    'TWHDWalletGetKeyForCoin(wallet.get(), TWCoinTypeEthereum)' \
    'operation(ethereumKey.get(), &operationError)' \
    'm_ethereumAddress != verified.ethereumAddress' \
    'm_bitcoinAddress != verified.bitcoinAddress' \
    'm_solanaAddress != verified.solanaAddress' \
    'secureErase(&mnemonic)'; do
    grep -Fq "${vault_contract}" "${VAULT_SOURCE}" \
        || fail "M52 WalletVault revalidation contract missing: ${vault_contract}"
done
# M49 now provides a private C++ signing snapshot that must rebuild the reviewed
# payload/hash/fingerprint before the secret boundary can be crossed.
for eth_sign_contract in \
    'QVariantMap signingSnapshot() const' \
    'static bool validateSigningSnapshot' \
    'signingSnapshotBindingPassed' \
    'constructedEpochMs'; do
    grep -Fq "${eth_sign_contract}" "${ETH_HEADER}" "${ETH_SOURCE}" \
        || fail "M52 Ethereum signing-snapshot gate missing: ${eth_sign_contract}"
done
grep -Fq 'Development sign / verify' "${ROOT}/qml/pages/TransactionReviewPage.qml" \
    || fail "M52 explicit development sign/verify action is missing"
grep -Fq 'devSigner.signEthereum(vault, ethBuilder, transaction)' "${ROOT}/qml/pages/TransactionReviewPage.qml" \
    || fail "M52 review page does not bind signing to WalletVault + reviewed Ethereum construction"
grep -Fq 'text: "Production signing unavailable in M52"' "${ROOT}/qml/pages/TransactionReviewPage.qml" \
    || fail "M52 review page does not visibly disable production signing"
grep -Fq 'label: "Broadcasting"' "${ROOT}/qml/pages/TransactionReviewPage.qml" \
    || fail "M52 review page no longer shows the broadcasting boundary"
grep -Fq 'value: "Disabled"' "${ROOT}/qml/pages/TransactionReviewPage.qml" \
    || fail "M52 review page no longer visibly disables broadcasting"
if grep -Eqi 'signatureHex|privateKey|developmentTestMnemonic' "${ROOT}/qml/pages/TransactionReviewPage.qml"; then
    fail "M52 review page exposes raw signing or recovery material"
fi
grep -Fq 'signingBoundaryInfo = developmentSigner.runSelfTest()' "${ROOT}/qml/pages/DiagnosticsPage.qml" \
    || fail "Developer diagnostics does not run the M52 signing-boundary self-test"
grep -Fq '&& signingBoundaryInfo.passed' "${ROOT}/qml/pages/ReleaseReadinessPage.qml" \
    || fail "Release readiness does not gate on M52 signing-boundary validation"
grep -Fq 'Raw signature to QML' "${ROOT}/qml/pages/DiagnosticsPage.qml" \
    || fail "Developer diagnostics does not surface the M52 non-exporting signature contract"
pass "M52 development-only non-exporting signing boundary"

grep -q '^VERSION="4.8.4"$' "${ROOT}/scripts/prepare-wallet-core.sh" \
    || fail "Wallet Core prepare script is not pinned to 4.8.4"
grep -q '^SOURCE_COMMIT="d40d24a63d92619167903369308bf0e2f7eb3a59"$' "${ROOT}/scripts/prepare-wallet-core.sh" \
    || fail "Wallet Core 4.8.4 immutable source commit is not pinned"
grep -q '^PREPARED_ID="${VERSION}-d40d24a63d92-sailvault-m45r1"$' "${ROOT}/scripts/prepare-wallet-core.sh" \
    || fail "Wallet Core preparation marker is not the M45 4.8.4 baseline"
grep -Fq 'archive/${SOURCE_COMMIT}.tar.gz' "${ROOT}/scripts/prepare-wallet-core.sh" \
    || fail "Wallet Core source download is not pinned to the immutable commit archive"
grep -Fq '"url": "https://robin.etherscan.io"' "${ROOT}/scripts/prepare-wallet-core.sh" \
    || fail "Wallet Core 4.8.4 release marker guard is missing"
grep -q '^WC_VERSION="4.8.4"$' "${ROOT}/scripts/generate-wallet-core-sources.sh" \
    || fail "Wallet Core generator version is not 4.8.4"
grep -q '^PROTOBUF_VERSION="3.20.3"$' "${ROOT}/scripts/generate-wallet-core-sources.sh" \
    || fail "Wallet Core protobuf generator is not 3.20.3"
grep -q 'SAILVAULT_WALLET_CORE_VERSION="4.8.4"' "${CMAKE_FILE}" \
    || fail "Application build identity is not Wallet Core 4.8.4"
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
    || fail "Wallet Core 4.8.4 nativeTokenName registry generation is missing"
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
        || fail "Canonical Wallet Core 4.8.4 derivation ABI entry missing: ${derivation_abi}"
done
grep -Fq 'list(FIND CANONICAL_DERIVATION_NAMES "${DERIV_ENUM}" DERIV_ENUM_INDEX)' \
    "${ROOT}/scripts/generate-wallet-core-registry.cmake" \
    || fail "CoinInfoData derivation ABI generation guard is missing"
grep -Fq 'string(REGEX REPLACE "[^A-Za-z0-9]+" ";" _parts' \
    "${ROOT}/scripts/generate-wallet-core-registry.cmake" \
    || fail "Delimiter-aware derivation CamelCase conversion is missing"
pass "Wallet Core 4.8.4 Sailfish build contract"

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
    [[ "${MARKER}" == "4.8.4-d40d24a63d92-sailvault-m45r1" ]] \
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
echo "M52 adds an Ethereum-only sign/verify proof for the published development wallet; raw signatures are discarded, production signing and broadcasting remain unavailable."
