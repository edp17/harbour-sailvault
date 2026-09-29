#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 1 ]]; then
    echo "usage: validate-wallet-core-generated.sh <wallet-core-dir>" >&2
    exit 2
fi

WC="$(cd "$1" && pwd)"
PUBLIC_DIR="${WC}/include/TrustWalletCore"
GENERATED_DIR="${WC}/src/Generated"
BINDINGS_DIR="${WC}/rust/bindings"

fail() {
    echo "ERROR: $*" >&2
    exit 1
}

[[ -d "${PUBLIC_DIR}" ]] || fail "missing public header directory: ${PUBLIC_DIR}"
[[ -d "${GENERATED_DIR}" ]] || fail "missing generated C++ directory: ${GENERATED_DIR}"
[[ -d "${BINDINGS_DIR}" ]] || fail "missing Rust binding manifests: ${BINDINGS_DIR}"

for required in \
    "${PUBLIC_DIR}/TWCryptoBoxPublicKey.h" \
    "${PUBLIC_DIR}/TWCryptoBoxSecretKey.h" \
    "${GENERATED_DIR}/TWCryptoBoxPublicKey.cpp" \
    "${GENERATED_DIR}/TWCryptoBoxSecretKey.cpp" \
    "${GENERATED_DIR}/CryptoBoxPublicKey.h" \
    "${GENERATED_DIR}/CryptoBoxSecretKey.h"; do
    [[ -s "${required}" ]] || fail "required Rust/C++ generated artifact is missing: ${required}"
done

missing=0
while IFS= read -r header; do
    while IFS= read -r include_name; do
        case "${include_name}" in
            TW*.h)
                if [[ ! -s "${PUBLIC_DIR}/${include_name}" ]]; then
                    echo "ERROR: ${header#${WC}/} includes missing ${include_name}" >&2
                    missing=1
                fi
                ;;
        esac
    done < <(sed -n 's/^[[:space:]]*#include[[:space:]]*"\([^"]*\)".*/\1/p' "${header}")
done < <(find "${PUBLIC_DIR}" -maxdepth 1 -type f -name '*.h' -print | sort)
[[ "${missing}" -eq 0 ]] || exit 3

while IFS= read -r source; do
    base="$(basename "${source}" .cpp)"
    [[ -s "${PUBLIC_DIR}/${base}.h" ]] \
        || fail "${source#${WC}/} has no generated public header ${base}.h"
done < <(find "${GENERATED_DIR}" -maxdepth 1 -type f -name 'TW*.cpp' -print | sort)

echo "Wallet Core Rust/C++ generated binding surface validated."
