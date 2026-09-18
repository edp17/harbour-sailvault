#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
VERSION="4.0.27"
PREPARED_ID="${VERSION}-sailvault-m1r16"
VENDOR_DIR="${ROOT}/vendor/wallet-core"
PATCHER="${SCRIPT_DIR}/patch-wallet-core.sh"
GENERATOR="${SCRIPT_DIR}/generate-wallet-core-sources.sh"
ARCHIVE_URL="https://github.com/trustwallet/wallet-core/archive/refs/tags/${VERSION}.tar.gz"

for cmd in curl tar awk grep mktemp; do
    if ! command -v "${cmd}" >/dev/null 2>&1; then
        echo "ERROR: required build tool '${cmd}' was not found." >&2
        exit 1
    fi
done

if [[ -f "${VENDOR_DIR}/.sailvault-wallet-core-version" ]] &&
   grep -qx "${PREPARED_ID}" "${VENDOR_DIR}/.sailvault-wallet-core-version" &&
   [[ -s "${VENDOR_DIR}/include/TrustWalletCore/TWDerivation.h" ]] &&
   [[ -s "${VENDOR_DIR}/src/proto/Algorand.pb.h" ]] &&
   [[ -s "${VENDOR_DIR}/src/proto/EthereumRlp.pb.h" ]]; then
    echo "Wallet Core ${VERSION} is already prepared for SailVault M1r16."
    exit 0
fi

echo "Preparing Trust Wallet Core ${VERSION}..."
rm -rf "${VENDOR_DIR}"
mkdir -p "${VENDOR_DIR}"

TMP_ARCHIVE="$(mktemp)"
trap 'rm -f "${TMP_ARCHIVE}"' EXIT

curl --fail --location --retry 3 --output "${TMP_ARCHIVE}" "${ARCHIVE_URL}"
tar -xzf "${TMP_ARCHIVE}" --strip-components=1 -C "${VENDOR_DIR}"

echo "Downloading Wallet Core's pinned source dependencies..."
source "${VENDOR_DIR}/tools/dependencies-version"
(
    cd "${VENDOR_DIR}"

    mkdir -p build/local/src/gtest
    curl -fSsL \
      -o "build/local/src/gtest/release-${GTEST_VERSION}.tar.gz" \
      "https://github.com/google/googletest/archive/release-${GTEST_VERSION}.tar.gz"
    tar xzf "build/local/src/gtest/release-${GTEST_VERSION}.tar.gz" \
        -C build/local/src/gtest

    mkdir -p build/local/src/check
    curl -fSsL \
      -o "build/local/src/check/check-${CHECK_VERSION}.tar.gz" \
      "https://github.com/libcheck/check/releases/download/${CHECK_VERSION}/check-${CHECK_VERSION}.tar.gz"
    tar xzf "build/local/src/check/check-${CHECK_VERSION}.tar.gz" \
        -C build/local/src/check

    # Use nlohmann/json's standalone single-header distribution. This avoids
    # needing Python or unzip in the Sailfish target build environment.
    mkdir -p build/local/include/nlohmann
    curl -fSsL \
      -o "build/local/include/nlohmann/json.hpp" \
      "https://raw.githubusercontent.com/nlohmann/json/v${JSON_VERSION}/single_include/nlohmann/json.hpp"

    mkdir -p build/local/src/protobuf
    curl -fSsL \
      -o "build/local/src/protobuf/protobuf-java-${PROTOBUF_VERSION}.tar.gz" \
      "https://github.com/protocolbuffers/protobuf/releases/download/v${PROTOBUF_VERSION}/protobuf-java-${PROTOBUF_VERSION}.tar.gz"
    tar xzf "build/local/src/protobuf/protobuf-java-${PROTOBUF_VERSION}.tar.gz" \
        -C build/local/src/protobuf
)

PROTOBUF_SENTINEL="${VENDOR_DIR}/build/local/src/protobuf/protobuf-${PROTOBUF_VERSION}/src/google/protobuf/any.cc"
JSON_SENTINEL="${VENDOR_DIR}/build/local/include/nlohmann/json.hpp"

if [[ ! -f "${PROTOBUF_SENTINEL}" ]]; then
    echo "ERROR: Wallet Core protobuf source tree was not extracted as expected:" >&2
    echo "  ${PROTOBUF_SENTINEL}" >&2
    exit 2
fi

if [[ ! -s "${JSON_SENTINEL}" ]]; then
    echo "ERROR: nlohmann/json single header was not downloaded:" >&2
    echo "  ${JSON_SENTINEL}" >&2
    exit 3
fi

echo "Generating Wallet Core source artifacts omitted from the Git tag..."
"${GENERATOR}" "${VENDOR_DIR}"

echo "Applying SailVault's checked Sailfish cross-build adjustments..."
"${PATCHER}" "${VENDOR_DIR}"

printf '%s\n' "${PREPARED_ID}" > "${VENDOR_DIR}/.sailvault-wallet-core-version"

echo
echo "Wallet Core ${VERSION} source is ready."
