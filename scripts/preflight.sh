#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
WC="${ROOT}/vendor/wallet-core"

echo "SailVault Milestone 2 r2 preflight"
echo "=================================="

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

if [[ -f "${WC}/.sailvault-wallet-core-version" ]]; then
    echo "PASS  Wallet Core source already prepared: $(cat "${WC}/.sailvault-wallet-core-version")"
    if [[ -s "${WC}/include/TrustWalletCore/TWDerivation.h" ]] &&
       [[ -s "${WC}/src/proto/Algorand.pb.h" ]]; then
        echo "PASS  Wallet Core generated C/C++ sources"
    else
        echo "FAIL  Wallet Core preparation marker exists but generated C/C++ sources are incomplete."
        exit 1
    fi
else
    echo "INFO  Wallet Core is not prepared yet; normal RPM build will prepare it."
fi

echo
echo "No manual bootstrap is required."
