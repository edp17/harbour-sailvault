#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 1 ]]; then
    echo "usage: generate-wallet-core-sources.sh <wallet-core-dir>" >&2
    exit 2
fi

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
WC="$(cd "$1" && pwd)"
WC_VERSION="4.0.27"
PROTOBUF_VERSION="3.19.2"
TOOLS_ROOT="${ROOT}/.sailvault-tools/wallet-core-codegen-${WC_VERSION}"
REGISTRY_GENERATOR="${SCRIPT_DIR}/generate-wallet-core-registry.cmake"
TYPEDEF_AWK="${SCRIPT_DIR}/generate-c-proto-typedefs.awk"

mkdir -p "${TOOLS_ROOT}"

for cmd in curl unzip cmake awk grep find wc chmod rm mkdir mv basename; do
    if ! command -v "${cmd}" >/dev/null 2>&1; then
        echo "ERROR: required Wallet Core source-generation tool '${cmd}' was not found." >&2
        exit 10
    fi
done

if [[ ! -f "${WC}/registry.json" ]]; then
    echo "ERROR: Wallet Core ${WC_VERSION} registry.json is missing." >&2
    exit 11
fi
if [[ ! -f "${REGISTRY_GENERATOR}" ]] || [[ ! -f "${TYPEDEF_AWK}" ]]; then
    echo "ERROR: SailVault source generators are missing." >&2
    exit 12
fi

download_atomic() {
    local url="$1" destination="$2" temporary="${2}.part"
    rm -f "${temporary}"
    curl --fail --location --retry 3 --output "${temporary}" "${url}"
    mv -f "${temporary}" "${destination}"
}

# Wallet Core's tag omits files normally emitted by codegen/bin/coins. Recreate
# the C/C++ outputs directly from registry.json with CMake's native JSON parser;
# this avoids requiring Ruby in the Sailfish target repository.
echo "Generating registry-derived Wallet Core C/C++ sources..."
cmake -DWC_DIR="${WC}" -P "${REGISTRY_GENERATOR}"

# Reproduce protobuf-plugin/c_typedef.cc: one TW*Proto.h per public proto,
# containing TWData* typedefs for top-level messages.
echo "Generating Wallet Core public C protobuf typedef headers..."
mkdir -p "${WC}/include/TrustWalletCore"
for proto in "${WC}"/src/proto/*.proto; do
    [[ -f "${proto}" ]] || { echo "ERROR: no public proto files found." >&2; exit 13; }
    base="$(basename "${proto}" .proto)"
    awk -f "${TYPEDEF_AWK}" "${proto}" > "${WC}/include/TrustWalletCore/TW${base}Proto.h"
done

find_protoc() {
    local platform zip_url zip_file install_dir bin output
    for platform in linux-x86_64 linux-aarch_64; do
        install_dir="${TOOLS_ROOT}/protoc-${PROTOBUF_VERSION}-${platform}"
        bin="${install_dir}/bin/protoc"
        zip_file="${TOOLS_ROOT}/protoc-${PROTOBUF_VERSION}-${platform}.zip"
        zip_url="https://github.com/protocolbuffers/protobuf/releases/download/v${PROTOBUF_VERSION}/protoc-${PROTOBUF_VERSION}-${platform}.zip"

        if [[ ! -x "${bin}" ]]; then
            if [[ ! -s "${zip_file}" ]]; then
                echo "Downloading protoc ${PROTOBUF_VERSION} (${platform})..." >&2
                download_atomic "${zip_url}" "${zip_file}"
            fi
            rm -rf "${install_dir}"
            mkdir -p "${install_dir}"
            if ! unzip -q "${zip_file}" -d "${install_dir}"; then
                echo "Cached protoc archive is invalid; retrying download..." >&2
                rm -f "${zip_file}"
                rm -rf "${install_dir}"
                download_atomic "${zip_url}" "${zip_file}"
                mkdir -p "${install_dir}"
                unzip -q "${zip_file}" -d "${install_dir}"
            fi
            chmod +x "${bin}"
        fi

        if output="$("${bin}" --version 2>&1)" && [[ "${output}" == "libprotoc ${PROTOBUF_VERSION}" ]]; then
            printf '%s\n' "${bin}"
            return 0
        fi
        echo "protoc ${platform} could not run in this build environment; trying fallback." >&2
    done
    return 1
}

PROTOC="$(find_protoc)" || {
    echo "ERROR: neither official protoc ${PROTOBUF_VERSION} binary could run under Scratchbox2." >&2
    exit 14
}
PROTOC_ROOT="$(cd "$(dirname "${PROTOC}")/.." && pwd)"
echo "Using $("${PROTOC}" --version): ${PROTOC}"

generate_proto_dir() {
    local relative_dir="$1" first_proto
    first_proto="$(find "${WC}/${relative_dir}" -maxdepth 1 -type f -name '*.proto' -print -quit 2>/dev/null || true)"
    if [[ -z "${first_proto}" ]]; then
        echo "ERROR: expected .proto files under ${relative_dir}." >&2
        exit 15
    fi
    echo "Generating C++ protobuf sources in ${relative_dir}..."
    (
        cd "${WC}"
        "${PROTOC}" -I="${PROTOC_ROOT}/include" -I="${relative_dir}" \
            --cpp_out="${relative_dir}" "${relative_dir}"/*.proto
    )
}

generate_proto_dir "src/proto"
generate_proto_dir "src/Tron/Protobuf"
generate_proto_dir "src/Zilliqa/Protobuf"
generate_proto_dir "src/Hedera/Protobuf"

verify_proto_dir() {
    local relative_dir="$1" proto base missing=0
    for proto in "${WC}/${relative_dir}"/*.proto; do
        base="${proto%.proto}"
        if [[ ! -s "${base}.pb.h" ]] || [[ ! -s "${base}.pb.cc" ]]; then
            echo "ERROR: missing generated protobuf pair for ${proto}" >&2
            missing=1
        fi
    done
    [[ "${missing}" -eq 0 ]]
}
verify_proto_dir "src/proto"
verify_proto_dir "src/Tron/Protobuf"
verify_proto_dir "src/Zilliqa/Protobuf"
verify_proto_dir "src/Hedera/Protobuf"

for generated in \
    "include/TrustWalletCore/TWDerivation.h" \
    "include/TrustWalletCore/TWHRP.h" \
    "include/TrustWalletCore/TWEthereumChainID.h" \
    "include/TrustWalletCore/TWBitcoinProto.h" \
    "include/TrustWalletCore/TWEthereumProto.h" \
    "src/Generated/CoinInfoData.cpp" \
    "src/Generated/TWHRP.cpp" \
    "src/proto/Algorand.pb.h" \
    "src/proto/EthereumRlp.pb.h"; do
    if [[ ! -s "${WC}/${generated}" ]]; then
        echo "ERROR: generated Wallet Core source is still missing: ${generated}" >&2
        exit 16
    fi
done

PROTO_FILE_COUNT="$(find "${WC}/src/proto" -maxdepth 1 -type f -name '*.proto' | wc -l)"
PROTO_API_COUNT="$(find "${WC}/include/TrustWalletCore" -maxdepth 1 -type f -name 'TW*Proto.h' | wc -l)"
if [[ "${PROTO_API_COUNT}" -ne "${PROTO_FILE_COUNT}" ]]; then
    echo "ERROR: Wallet Core C protobuf typedef header count mismatch:" >&2
    echo "  proto files: ${PROTO_FILE_COUNT}" >&2
    echo "  TW*Proto.h: ${PROTO_API_COUNT}" >&2
    exit 17
fi

echo "Wallet Core generated-source set is ready (${PROTO_API_COUNT} public TW*Proto.h headers)."
