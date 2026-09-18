#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
WC="${ROOT}/vendor/wallet-core"
TARGET="${SB2_RUST_TARGET_TRIPLE:-aarch64-unknown-linux-gnu}"

CBINDGEN_VERSION="0.26.0"
TOOLS_DIR="${ROOT}/.sailvault-tools"
CBINDGEN_ROOT="${TOOLS_DIR}/cbindgen-${CBINDGEN_VERSION}"
CBINDGEN_BIN="${CBINDGEN_ROOT}/bin/cbindgen"
CBINDGEN_TARGET_DIR="${ROOT}/.sailvault-cbindgen-target"

echo "================================================"
echo " SailVault M1r17 · native Sailfish Rust build"
echo "================================================"
echo

echo "Ensuring Wallet Core 4.0.27 source and generated artifacts are prepared..."
"${SCRIPT_DIR}/prepare-wallet-core.sh"
echo

for tool in rustc cargo cc c++ ar awk; do
    if ! command -v "${tool}" >/dev/null 2>&1; then
        echo "ERROR: required Sailfish build tool '${tool}' is unavailable." >&2
        exit 20
    fi
done

echo "Sailfish Rust toolchain:"
rustc -vV
cargo -V
echo

RUST_HOST="$(rustc -vV | awk '/^host:/ {print $2}')"
CC_TARGET="$(cc -dumpmachine 2>/dev/null || true)"

echo "Scratchbox2 target:"
echo "  SB2_RUST_TARGET_TRIPLE=${TARGET}"
echo "  rustc host=${RUST_HOST}"
echo "  cc tuple=${CC_TARGET}"
echo

if [[ "${TARGET}" != "aarch64-unknown-linux-gnu" ]]; then
    echo "ERROR: expected SB2 Rust target aarch64-unknown-linux-gnu, got '${TARGET}'." >&2
    exit 21
fi

if [[ "${CC_TARGET}" != *aarch64* ]]; then
    echo "ERROR: Scratchbox2 cc is not targeting aarch64: '${CC_TARGET}'." >&2
    exit 22
fi

TMP_BASE="${ROOT}/.sailvault-cargo-tmp"
mkdir -p "${TMP_BASE}" "${TOOLS_DIR}"
export TMPDIR="$(cd "${TMP_BASE}" && pwd)"

# The target tool settings are also required by cbindgen's own Rust build and
# by Wallet Core crates containing C/C++ code.
export CARGO_TARGET_AARCH64_UNKNOWN_LINUX_GNU_LINKER="$(command -v cc)"
export CC_aarch64_unknown_linux_gnu="$(command -v cc)"
export CXX_aarch64_unknown_linux_gnu="$(command -v c++)"
export AR_aarch64_unknown_linux_gnu="$(command -v ar)"
export CFLAGS_aarch64_unknown_linux_gnu="${CFLAGS:-}"
export CXXFLAGS_aarch64_unknown_linux_gnu="${CXXFLAGS:-}"

# ---------------------------------------------------------------------------
# Generate Wallet Core's Rust FFI header first.
#
# Sailfish 5 currently supplies cbindgen 0.19, which predates Rust `let ... else`
# syntax used by Wallet Core 4.0.27 and fails while parsing legacy.rs.
# cbindgen 0.26 supports that syntax and is compatible with our Rust 1.75.
# Build it once per source tree using the exact same proven Sailfish cross path.
# ---------------------------------------------------------------------------
if [[ ! -x "${CBINDGEN_BIN}" ]]; then
    echo "Building project-local cbindgen ${CBINDGEN_VERSION}..."
    rm -rf "${CBINDGEN_ROOT}" "${CBINDGEN_TARGET_DIR}"
    mkdir -p "${CBINDGEN_ROOT}" "${CBINDGEN_TARGET_DIR}"

    (
        export CARGO_TARGET_DIR="${CBINDGEN_TARGET_DIR}"
        cargo install cbindgen \
            --version "${CBINDGEN_VERSION}" \
            --locked \
            --root "${CBINDGEN_ROOT}" \
            --target "${TARGET}" \
            -j1
    )
fi

if [[ ! -x "${CBINDGEN_BIN}" ]]; then
    echo "ERROR: project-local cbindgen was not installed at ${CBINDGEN_BIN}" >&2
    exit 23
fi

echo
echo "Project-local cbindgen:"
"${CBINDGEN_BIN}" --version
echo

BINDGEN_DIR="${WC}/src/rust/bindgen"
BINDGEN_HEADER="${BINDGEN_DIR}/WalletCoreRSBindgen.h"
mkdir -p "${BINDGEN_DIR}"
rm -f "${BINDGEN_HEADER}"

echo "Generating Wallet Core Rust C/C++ FFI header..."
(
    cd "${WC}/rust"
    # Header generation is architecture-independent source analysis.
    unset CARGO_BUILD_TARGET
    "${CBINDGEN_BIN}" \
        --crate wallet-core-rs \
        --output "${BINDGEN_HEADER}"
)

if [[ ! -s "${BINDGEN_HEADER}" ]]; then
    echo "ERROR: cbindgen did not produce ${BINDGEN_HEADER}" >&2
    exit 24
fi

echo "Wallet Core Rust FFI header ready:"
ls -lh "${BINDGEN_HEADER}"
echo

# ---------------------------------------------------------------------------
# Build Wallet Core Rust itself.
# ---------------------------------------------------------------------------
export CARGO_TARGET_DIR="${WC}/rust/target"
export CARGO_BUILD_TARGET="${TARGET}"
export CARGO_BUILD_JOBS=1

echo "Fetching Wallet Core 4.0.27 locked Rust dependencies..."
(
    cd "${WC}/rust"
    cargo fetch --locked
)

echo
echo "Compiling Wallet Core Rust with Sailfish's packaged compiler..."
echo "  package: wallet-core-rs"
echo "  target:  ${TARGET}"
echo "  jobs:    1"
echo

cd "${WC}/rust"

cargo build \
    --locked \
    --release \
    --target "${TARGET}" \
    -j1 \
    -p wallet-core-rs

SOURCE_LIB="${WC}/rust/target/${TARGET}/release/libwallet_core_rs.a"
DEST_DIR="${WC}/rust/target/release"
DEST_LIB="${DEST_DIR}/libwallet_core_rs.a"

if [[ ! -f "${SOURCE_LIB}" ]]; then
    echo "ERROR: Cargo completed without producing ${SOURCE_LIB}" >&2
    exit 25
fi

mkdir -p "${DEST_DIR}"
cp -f "${SOURCE_LIB}" "${DEST_LIB}"

echo
echo "Wallet Core Rust static library ready:"
if command -v file >/dev/null 2>&1; then
    file "${DEST_LIB}" || true
fi
ls -lh "${DEST_LIB}"
