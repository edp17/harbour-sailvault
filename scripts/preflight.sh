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

printf 'SailVault %s read-only beta preflight\n' "${PACKAGE_VERSION}"
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
    || fail "Device checklist is not the beta 1 release checklist"
grep -Fq "This read-only beta release provides" "${SPEC}" \
    || fail "RPM description does not identify the current build as a beta release"
case "${CMAKE_BUILD_LABEL}" in
    *RC*|*rc*|*candidate*|*Candidate*)
        fail "Current build label still identifies a release candidate: ${CMAKE_BUILD_LABEL}"
        ;;
esac
pass "Read-only beta release identity"

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
    [[ "${MARKER}" == "4.0.27-sailvault-m1r16" ]] \
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
echo "Read-only beta guards remain: no transaction construction, signing UI or broadcasting."
