#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 1 ]]; then
    echo "usage: patch-wallet-core.sh <wallet-core-dir>" >&2
    exit 2
fi

ROOT="$(cd "$1" && pwd)"

replace_exact_line() {
    local src="$1" old="$2" new="$3" expected="${4:-1}"
    local tmp="${src}.sailvault.tmp"
    awk -v old="$old" -v new="$new" -v expected="$expected" '
    BEGIN { n=0 }
    {
        if ($0 == old) { print new; n++; next }
        print
    }
    END {
        if (n != expected) {
            printf("ERROR: exact-line patch mismatch in %s: matched=%d expected=%d\n", FILENAME, n, expected) > "/dev/stderr"
            exit 70
        }
    }' "$src" > "$tmp"
    mv "$tmp" "$src"
}

patch_cmake() {
    local src="$ROOT/CMakeLists.txt"
    local tmp="${src}.sailvault.tmp"
    [[ -f "$src" ]] || { echo "ERROR: missing $src" >&2; exit 10; }

    awk '
    BEGIN { project=0; compiler=0; prefix=0; rustdir=0; boost=0; skip=0 }
    {
        if (skip > 0) { skip--; next }

        if ($0 == "project(TrustWalletCore)") {
            project++
            print
            print ""
            print "option(SAILVAULT_ALLOW_GNU \"Allow the downstream Sailfish GNU toolchain\" OFF)"
            print "option(SAILVAULT_CROSS_COMPILE \"Use target packages while cross compiling\" OFF)"
            print ""
            print "if (SAILVAULT_ALLOW_GNU AND \"${CMAKE_CXX_COMPILER_ID}\" STREQUAL \"GNU\")"
            print "    add_compile_definitions(_Nonnull= _Nullable= _Null_unspecified=)"
            print "endif ()"
            next
        }

        if ($0 == "if (NOT (\"${CMAKE_CXX_COMPILER_ID}\" MATCHES \"Clang\"))") {
            compiler++
            print "if (NOT (\"${CMAKE_CXX_COMPILER_ID}\" MATCHES \"Clang\"))"
            print "    if (SAILVAULT_ALLOW_GNU AND \"${CMAKE_CXX_COMPILER_ID}\" STREQUAL \"GNU\")"
            print "        message(WARNING \"SailVault: checked GNU/C++17 compatibility build of Wallet Core 4.8.4\")"
            print "    else ()"
            print "        message(FATAL_ERROR \"You should use clang compiler\")"
            print "    endif ()"
            print "endif ()"
            skip=2
            next
        }

        if ($0 == "    set(PREFIX \"${CMAKE_SOURCE_DIR}/build/local\")") {
            prefix++
            print "    set(PREFIX \"${CMAKE_CURRENT_SOURCE_DIR}/build/local\")"
            next
        }

        if ($0 == "set(WALLET_CORE_RS_TARGET_DIR ${CMAKE_SOURCE_DIR}/rust/target)") {
            rustdir++
            print "set(WALLET_CORE_RS_TARGET_DIR ${CMAKE_CURRENT_SOURCE_DIR}/rust/target)"
            next
        }

        if ($0 == "find_host_package(Boost REQUIRED)") {
            boost++
            print "if (SAILVAULT_CROSS_COMPILE)"
            print "    find_package(Boost REQUIRED)"
            print "else ()"
            print "    find_host_package(Boost REQUIRED)"
            print "endif ()"
            next
        }

        print
    }
    END {
        if (project != 1 || compiler != 1 || prefix != 1 || rustdir != 1 || boost != 1) {
            printf("ERROR: CMakeLists.txt patch context mismatch: project=%d compiler=%d prefix=%d rustdir=%d boost=%d\n", project, compiler, prefix, rustdir, boost) > "/dev/stderr"
            exit 50
        }
    }' "$src" > "$tmp"
    mv "$tmp" "$src"
    echo "Patched CMakeLists.txt"
}

patch_warnings_and_standard() {
    local src="$ROOT/cmake/CompilerWarnings.cmake"
    local tmp="${src}.sailvault.tmp"
    [[ -f "$src" ]] || { echo "ERROR: missing $src" >&2; exit 11; }

    awk '
    BEGIN { fatal=0; a=0; b=0; standard=0 }
    {
        if ($0 == "        -Wfatal-errors # short error report") {
            fatal++
            print "        $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:-Wfatal-errors> # short error report"
            next
        }
        if ($0 == "        -Wshorten-64-to-32") {
            a++
            print "        $<$<CXX_COMPILER_ID:Clang>:-Wshorten-64-to-32>"
            next
        }
        if ($0 == "        -Wno-nullability-completeness") {
            b++
            print "        $<$<CXX_COMPILER_ID:Clang>:-Wno-nullability-completeness>"
            next
        }
        if ($0 == "target_compile_features(tw_defaults_features INTERFACE cxx_std_20)") {
            standard++
            print "if (SAILVAULT_ALLOW_GNU AND \"${CMAKE_CXX_COMPILER_ID}\" STREQUAL \"GNU\")"
            print "    target_compile_features(tw_defaults_features INTERFACE cxx_std_17)"
            print "else ()"
            print "    target_compile_features(tw_defaults_features INTERFACE cxx_std_20)"
            print "endif ()"
            next
        }
        print
    }
    END {
        if (fatal != 1 || a != 1 || b != 1 || standard != 1) {
            printf("ERROR: CompilerWarnings.cmake patch context mismatch: fatal=%d shorten=%d nullability=%d standard=%d\n", fatal, a, b, standard) > "/dev/stderr"
            exit 51
        }
    }' "$src" > "$tmp"
    mv "$tmp" "$src"
    echo "Patched cmake/CompilerWarnings.cmake"
}

patch_protobuf() {
    local src="$ROOT/cmake/Protobuf.cmake"
    local tmp="${src}.sailvault.tmp"
    [[ -f "$src" ]] || { echo "ERROR: missing $src" >&2; exit 12; }

    awk '
    BEGIN { n=0; linkflags=0 }
    {
        if ($0 == "target_compile_options(protobuf PRIVATE -DHAVE_PTHREAD=1 -Wno-inconsistent-missing-override -Wno-shorten-64-to-32 -Wno-invalid-noreturn)") {
            n++
            print "target_compile_options(protobuf PRIVATE"
            print "    -DHAVE_PTHREAD=1"
            print "    $<$<CXX_COMPILER_ID:Clang>:-Wno-inconsistent-missing-override>"
            print "    $<$<CXX_COMPILER_ID:Clang>:-Wno-shorten-64-to-32>"
            print "    $<$<CXX_COMPILER_ID:Clang>:-Wno-invalid-noreturn>"
            print ")"
            next
        }
        if ($0 == "    LINK_FLAGS -no-undefined") {
            linkflags++
            print "    LINK_FLAGS \"-Wl,--no-undefined\""
            next
        }
        print
    }
    END {
        if (n != 1 || linkflags != 1) {
            printf("ERROR: Protobuf.cmake patch context mismatch: compile_options=%d link_flags=%d\n", n, linkflags) > "/dev/stderr"
            exit 52
        }
    }' "$src" > "$tmp"
    mv "$tmp" "$src"
    echo "Patched cmake/Protobuf.cmake"
}

patch_trezor_cmake() {
    local src="$ROOT/trezor-crypto/CMakeLists.txt"
    local tmp="${src}.sailvault.tmp"
    [[ -f "$src" ]] || { echo "ERROR: missing $src" >&2; exit 14; }

    awk '
    BEGIN { n=0 }
    {
        if ($0 == "target_compile_options(TrezorCrypto PRIVATE ${TW_WARNING_FLAGS} -Werror PUBLIC -Wno-deprecated-volatile)") {
            n++
            print "target_compile_options(TrezorCrypto"
            print "    PRIVATE ${TW_WARNING_FLAGS} -Werror"
            print "    PUBLIC $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:-Wno-deprecated-volatile>"
            print ")"
            next
        }
        print
    }
    END {
        if (n != 1) {
            printf("ERROR: trezor-crypto CMake patch context mismatch: deprecated_volatile=%d\n", n) > "/dev/stderr"
            exit 54
        }
    }' "$src" > "$tmp"
    mv "$tmp" "$src"
    echo "Patched trezor-crypto/CMakeLists.txt"
}

patch_fee_calculator() {
    local src="$ROOT/src/Bitcoin/FeeCalculator.cpp"
    local tmp="${src}.sailvault.tmp"
    [[ -f "$src" ]] || { echo "ERROR: missing $src" >&2; exit 15; }

    awk '
    BEGIN { n=0 }
    {
        if ($0 ~ /^static constexpr (DefaultFeeCalculator|DecredFeeCalculator|SegwitFeeCalculator|Zip0317FeeCalculator)/) {
            n++
            sub(/^static constexpr /, "static const ")
        }
        print
    }
    END {
        if (n != 7) {
            printf("ERROR: FeeCalculator.cpp patch context mismatch: static_instances=%d\n", n) > "/dev/stderr"
            exit 55
        }
    }' "$src" > "$tmp"
    mv "$tmp" "$src"
    echo "Patched src/Bitcoin/FeeCalculator.cpp"
}

patch_twbase() {
    local src="$ROOT/include/TrustWalletCore/TWBase.h"
    local tmp="${src}.sailvault.tmp"
    [[ -f "$src" ]] || { echo "ERROR: missing $src" >&2; exit 13; }

    awk '
    BEGIN { n=0 }
    {
        if ($0 == "#if !defined(TW_EXTERN_C_BEGIN)") {
            n++
            print "#ifndef __has_feature"
            print "#define __has_feature(x) 0"
            print "#endif"
            print ""
        }
        print
    }
    END {
        if (n != 1) {
            printf("ERROR: TWBase.h patch context mismatch: insertion_points=%d\n", n) > "/dev/stderr"
            exit 53
        }
    }' "$src" > "$tmp"
    mv "$tmp" "$src"
    echo "Patched include/TrustWalletCore/TWBase.h"
}

patch_cpp20_surface() {
    local src

    src="$ROOT/src/THORChain/Swap.cpp"
    replace_exact_line "$src" \
        '    if (value.empty() || !std::ranges::all_of(value, [](unsigned char c) {' \
        '    if (value.empty() || !std::all_of(value.begin(), value.end(), [](unsigned char c) {'
    replace_exact_line "$src" \
        '        return {.status_code = static_cast<SwapErrorCode>(Proto::ErrorCode::Error_Invalid_from_address), .error = "Invalid from address"};' \
        '        return SwapBundled{Data{}, static_cast<SwapErrorCode>(Proto::ErrorCode::Error_Invalid_from_address), "Invalid from address"};'
    replace_exact_line "$src" \
        '        return {.status_code = static_cast<SwapErrorCode>(Proto::ErrorCode::Error_Invalid_to_address), .error = "Invalid to address"};' \
        '        return SwapBundled{Data{}, static_cast<SwapErrorCode>(Proto::ErrorCode::Error_Invalid_to_address), "Invalid to address"};'
    replace_exact_line "$src" \
        '        return {.status_code = static_cast<SwapErrorCode>(Proto::ErrorCode::Error_general), .error = "Invalid from amount"};' \
        '        return SwapBundled{Data{}, static_cast<SwapErrorCode>(Proto::ErrorCode::Error_general), "Invalid from amount"};'
    replace_exact_line "$src" \
        '        return {.status_code = static_cast<SwapErrorCode>(Proto::ErrorCode::Error_general), .error = "Invalid to amount limit"};' \
        '        return SwapBundled{Data{}, static_cast<SwapErrorCode>(Proto::ErrorCode::Error_general), "Invalid to amount limit"};'
    replace_exact_line "$src" \
        '        return {.status_code = static_cast<SwapErrorCode>(Proto::ErrorCode::Error_general), .error = "Invalid stream params"};' \
        '        return SwapBundled{Data{}, static_cast<SwapErrorCode>(Proto::ErrorCode::Error_general), "Invalid stream params"};'
    replace_exact_line "$src" \
        '        return {.status_code = static_cast<SwapErrorCode>(Proto::ErrorCode::Error_Unsupported_from_chain), .error = "Unsupported from chain: " + std::to_string(fromChain)};' \
        '        return SwapBundled{Data{}, static_cast<SwapErrorCode>(Proto::ErrorCode::Error_Unsupported_from_chain), "Unsupported from chain: " + std::to_string(fromChain)};'
    replace_exact_line "$src" '    return {.out = std::move(out)};' '    return SwapBundled{std::move(out), 0, ""};' 5
    replace_exact_line "$src" \
        '        return {.status_code = static_cast<int>(Proto::ErrorCode::Error_Invalid_vault_address), .error = "Invalid vault address: " + mVaultAddress};' \
        '        return SwapBundled{Data{}, static_cast<int>(Proto::ErrorCode::Error_Invalid_vault_address), "Invalid vault address: " + mVaultAddress};' 2
    replace_exact_line "$src" \
        '        return {.status_code = static_cast<int>(Proto::ErrorCode::Error_Invalid_router_address), .error = "Invalid router address: " + *mRouterAddress};' \
        '        return SwapBundled{Data{}, static_cast<int>(Proto::ErrorCode::Error_Invalid_router_address), "Invalid router address: " + *mRouterAddress};'

    src="$ROOT/src/Move/Address.h"
    replace_exact_line "$src" '        if (address.starts_with("0x")) {' '        if (address.compare(0, 2, "0x") == 0) {'
    replace_exact_line "$src" '            if (string.starts_with("0x") && (isExpectedLen || (StrictPadding && (hexLen < hexSizeAddress)))) {' '            if (string.compare(0, 2, "0x") == 0 && (isExpectedLen || (StrictPadding && (hexLen < hexSizeAddress)))) {'

    src="$ROOT/src/Hedera/DER.cpp"
    replace_exact_line "$src" '    if (!input.starts_with(prefix)) {' '    if (input.compare(0, prefix.size(), prefix) != 0) {'

    src="$ROOT/src/Cardano/AddressV3.cpp"
    replace_exact_line "$src" '        if (const auto expectedHrp = getHrp(kind); !addr.starts_with(expectedHrp)) {' '        if (const auto expectedHrp = getHrp(kind); addr.compare(0, expectedHrp.size(), expectedHrp) != 0) {'

    src="$ROOT/src/Coin.cpp"
    replace_exact_line "$src" \
        '            return isValid || dispatcher->validateAddress(coin, string, Base58Prefix{.p2pkh = p2pkh, .p2sh = p2sh});' \
        '            return isValid || dispatcher->validateAddress(coin, string, Base58Prefix{p2pkh, p2sh});'

    src="$ROOT/src/AnyAddress.cpp"
    replace_exact_line "$src" \
        '    return new AnyAddress{.address = std::move(normalized), .coin = coin};' \
        '    return new AnyAddress{std::move(normalized), coin};'
    replace_exact_line "$src" \
        '    return new AnyAddress{.address = std::move(derivedAddress), .coin = coin};' \
        '    return new AnyAddress{std::move(derivedAddress), coin};'

    src="$ROOT/src/Keystore/AESParameters.cpp"
    replace_exact_line "$src" \
        '    {TWStoredKeyEncryptionAes128Ctr, Keystore::AESParameters{.mKeyLength = Keystore::A128, .mCipher = Keystore::gAes128Ctr, .mCipherEncryption = TWStoredKeyEncryptionAes128Ctr, .iv{}}},' \
        '    {TWStoredKeyEncryptionAes128Ctr, Keystore::AESParameters{Keystore::gBlockSize, Keystore::A128, Keystore::gAes128Ctr, TWStoredKeyEncryptionAes128Ctr, {}}},'
    replace_exact_line "$src" \
        '    {TWStoredKeyEncryptionAes192Ctr, Keystore::AESParameters{.mKeyLength = Keystore::A192, .mCipher = Keystore::gAes192Ctr, .mCipherEncryption = TWStoredKeyEncryptionAes192Ctr, .iv{}}},' \
        '    {TWStoredKeyEncryptionAes192Ctr, Keystore::AESParameters{Keystore::gBlockSize, Keystore::A192, Keystore::gAes192Ctr, TWStoredKeyEncryptionAes192Ctr, {}}},'
    replace_exact_line "$src" \
        '    {TWStoredKeyEncryptionAes256Ctr, Keystore::AESParameters{.mKeyLength = Keystore::A256, .mCipher = Keystore::gAes256Ctr, .mCipherEncryption = TWStoredKeyEncryptionAes256Ctr, .iv{}}}' \
        '    {TWStoredKeyEncryptionAes256Ctr, Keystore::AESParameters{Keystore::gBlockSize, Keystore::A256, Keystore::gAes256Ctr, TWStoredKeyEncryptionAes256Ctr, {}}}'

    src="$ROOT/src/Tezos/Michelson.cpp"
    replace_exact_line "$src" '    MichelsonValue::MichelsonVariant address = StringValue{.string = data.from()};' '    MichelsonValue::MichelsonVariant address = StringValue{data.from()};'
    replace_exact_line "$src" '    MichelsonValue::MichelsonVariant to = StringValue{.string = data.to()};' '    MichelsonValue::MichelsonVariant to = StringValue{data.to()};'
    replace_exact_line "$src" '    MichelsonValue::MichelsonVariant amount = IntValue{._int = data.value()};' '    MichelsonValue::MichelsonVariant amount = IntValue{data.value()};'
    replace_exact_line "$src" '    auto primTransferInfos = PrimValue{.prim = "Pair", .args{{to}, {amount}}, .anots{}};' '    auto primTransferInfos = PrimValue{"Pair", {{to}, {amount}}, {}};' 1
    replace_exact_line "$src" '    return PrimValue{.prim = "Pair", .args{{address}, {primTransferInfos}}, .anots{}};' '    return PrimValue{"Pair", {{address}, {primTransferInfos}}, {}};'
    replace_exact_line "$src" '    MichelsonValue::MichelsonVariant from = StringValue{.string = txObj.from()};' '    MichelsonValue::MichelsonVariant from = StringValue{txObj.from()};'
    replace_exact_line "$src" '    MichelsonValue::MichelsonVariant tokenId = IntValue{._int = txTransferInfos.token_id()};' '    MichelsonValue::MichelsonVariant tokenId = IntValue{txTransferInfos.token_id()};'
    replace_exact_line "$src" '    MichelsonValue::MichelsonVariant amount = IntValue{._int = txTransferInfos.amount()};' '    MichelsonValue::MichelsonVariant amount = IntValue{txTransferInfos.amount()};'
    replace_exact_line "$src" '    auto primTransferInfos = PrimValue{.prim = "Pair", .args{{tokenId}, {amount}}, .anots{}};' '    auto primTransferInfos = PrimValue{"Pair", {{tokenId}, {amount}}, {}};'
    replace_exact_line "$src" '    MichelsonValue::MichelsonVariant to = StringValue{.string = txTransferInfos.to()};' '    MichelsonValue::MichelsonVariant to = StringValue{txTransferInfos.to()};'
    replace_exact_line "$src" '    MichelsonValue::MichelsonVariant txs = MichelsonValue::MichelsonArray{PrimValue{.prim = "Pair", .args{{to}, {primTransferInfos}}, .anots{}}};' '    MichelsonValue::MichelsonVariant txs = MichelsonValue::MichelsonArray{PrimValue{"Pair", {{to}, {primTransferInfos}}, {}}};'
    replace_exact_line "$src" '    auto primTxs = PrimValue{.prim = "Pair", .args{{from}, {txs}}, .anots{}};' '    auto primTxs = PrimValue{"Pair", {{from}, {txs}}, {}};'

    src="$ROOT/src/Cardano/Signer.cpp"
    replace_exact_line "$src" \
        '    TransactionPlan plan{.utxos = selectedInputs, .extraOutputs = extraOutputs, .amount = amount, .deposit = deposit, .undeposit = undeposit, .availableTokens{}, .outputTokens{}, .changeTokens{}};' \
        $'    TransactionPlan plan;\n    plan.utxos = selectedInputs;\n    plan.extraOutputs = extraOutputs;\n    plan.amount = amount;\n    plan.deposit = deposit;\n    plan.undeposit = undeposit;'

    # std::countl_zero and <bit> are C++20. Wallet Core compiles every C++
    # implementation in src/, including Everscale/CommonTON even though SailVault
    # does not expose that chain. Use the GCC/Clang builtin that is available on
    # the proven Sailfish GNU toolchain and does not depend on C++20 libstdc++.
    src="$ROOT/src/Everscale/CommonTON/CellBuilder.cpp"
    replace_exact_line "$src" '#include <bit>' '#include <cstdint>'
    replace_exact_line "$src" '#include <cstdint>' $'#include <cstdint>\n#include <stdexcept>'
    replace_exact_line "$src" \
        '        return static_cast<uint8_t>(std::countl_zero(lo) + 64);' \
        '        return static_cast<uint8_t>(__builtin_clzll(lo) + 64);'
    replace_exact_line "$src" \
        '        return static_cast<uint8_t>(std::countl_zero(hi));' \
        '        return static_cast<uint8_t>(__builtin_clzll(hi));'

    # Wallet Core 4.8.4 has several production translation units that use
    # standard exception classes without directly including <stdexcept>.
    # Clang's transitive include graph tolerated these upstream; Sailfish's
    # GNU 10 / libstdc++ build does not. Make those dependencies explicit.
    src="$ROOT/src/Everscale/CommonTON/Cell.cpp"
    replace_exact_line "$src" '#include <optional>' $'#include <optional>\n#include <stdexcept>'

    src="$ROOT/src/Everscale/CommonTON/CellSlice.cpp"
    replace_exact_line "$src" '#include <cassert>' $'#include <cassert>\n#include <stdexcept>'

    src="$ROOT/src/Ontology/ParamsBuilder.cpp"
    replace_exact_line "$src" '#include <list>' $'#include <list>\n#include <stdexcept>'

    src="$ROOT/src/Nano/Signer.cpp"
    replace_exact_line "$src" '#include <algorithm>' $'#include <algorithm>\n#include <stdexcept>'

    src="$ROOT/src/Tron/Deserialization.cpp"
    replace_exact_line "$src" '#include <unordered_set>' $'#include <stdexcept>\n#include <unordered_set>'

    # GCC 10 does not find chrono conversion helpers through unqualified lookup.
    # Wallet Core 4.8.4 has two production uses of unqualified duration_cast;
    # qualify both explicitly to match the standard API.
    src="$ROOT/src/MultiversX/TransactionFactoryConfig.cpp"
    replace_exact_line "$src" \
        '    const uint64_t timestamp = duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();' \
        '    const uint64_t timestamp = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();'

    src="$ROOT/src/Tron/Signer.cpp"
    replace_exact_line "$src" \
        '    const uint64_t now = duration_cast<std::chrono::milliseconds>(' \
        '    const uint64_t now = std::chrono::duration_cast<std::chrono::milliseconds>('

    echo "Patched Wallet Core 4.8.4 C++20/stdlib production surface for Sailfish C++17"
}

patch_rust_175_stdlib_surface() {
    local src

    # Rust 1.75 predates the integer `is_multiple_of()` convenience method
    # used by Wallet Core 4.8.4. Keep the arithmetic equivalent explicit so
    # the source remains compatible with Sailfish's packaged compiler.
    src="$ROOT/rust/tw_encoding/src/hex.rs"
    replace_exact_line "$src" \
        '    if hex_string.len().is_multiple_of(2) {' \
        '    if hex_string.len() % 2 == 0 {'

    src="$ROOT/rust/tw_evm/src/abi/uint.rs"
    replace_exact_line "$src" \
        '    if !bits.is_multiple_of(8) || bits == 0 || bits > 256 {' \
        '    if bits % 8 != 0 || bits == 0 || bits > 256 {'

    # These TON SDK crates are outside SailVault's feature set, but patch
    # them too so the vendored 4.8.4 tree itself is Rust-1.75 compatible if the
    # feature scope is widened later.
    src="$ROOT/rust/frameworks/tw_ton_sdk/src/cell/level_mask.rs"
    replace_exact_line "$src" \
        '        level == 0 || !(self.mask >> (level - 1)).is_multiple_of(2)' \
        '        level == 0 || ((self.mask >> (level - 1)) % 2 != 0)'

    src="$ROOT/rust/frameworks/tw_ton_sdk/src/cell/cell_parser.rs"
    replace_exact_line "$src" \
        '        let high_word_bits = if bit_len.is_multiple_of(32) {' \
        '        let high_word_bits = if bit_len % 32 == 0 {'

    echo "Patched Wallet Core 4.8.4 post-Rust-1.75 integer APIs"
}

patch_rust_registry_for_sailvault() {
    local cargo="$ROOT/rust/tw_coin_registry/Cargo.toml"
    local blockchain="$ROOT/rust/tw_coin_registry/src/blockchain_type.rs"
    local dispatcher="$ROOT/rust/tw_coin_registry/src/dispatcher.rs"
    local lock="$ROOT/rust/Cargo.lock"
    local tmp="${lock}.sailvault.tmp"

    for f in "$cargo" "$blockchain" "$dispatcher" "$lock"; do
        [[ -f "$f" ]] || { echo "ERROR: missing Rust registry file $f" >&2; exit 80; }
    done

    cat > "$cargo" <<'CARGOEOF'
[package]
name = "tw_coin_registry"
version = "0.1.0"
edition = "2021"

[dependencies]
lazy_static = "1.4.0"
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
strum = "0.25"
strum_macros = "0.25"
tw_bitcoin = { path = "../chains/tw_bitcoin" }
tw_coin_entry = { path = "../tw_coin_entry" }
tw_ethereum = { path = "../chains/tw_ethereum" }
tw_evm = { path = "../tw_evm" }
tw_hash = { path = "../tw_hash" }
tw_keypair = { path = "../tw_keypair" }
tw_memory = { path = "../tw_memory" }
tw_misc = { path = "../tw_misc" }
tw_solana = { path = "../chains/tw_solana" }

[build-dependencies]
itertools = "0.10.5"
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
CARGOEOF

    cat > "$blockchain" <<'RUSTEOF'
// SPDX-License-Identifier: Apache-2.0
//
// Copyright © 2017 Trust Wallet.
// SailVault downstream scope: BTC / ETH-EVM / SOL only.

use serde::Deserialize;

#[derive(Copy, Clone, Debug, Deserialize, PartialEq)]
pub enum BlockchainType {
    Bitcoin,
    Ethereum,
    Solana,
    #[serde(other)]
    Unsupported,
}

impl BlockchainType {
    pub fn is_supported(&self) -> bool {
        !matches!(self, BlockchainType::Unsupported)
    }
}
RUSTEOF

    cat > "$dispatcher" <<'RUSTEOF'
// SPDX-License-Identifier: Apache-2.0
//
// Copyright © 2017 Trust Wallet.
// SailVault downstream scope: BTC / ETH-EVM / SOL only.

use crate::blockchain_type::BlockchainType;
use crate::coin_context::CoinRegistryContext;
use crate::coin_type::CoinType;
use crate::error::{RegistryError, RegistryResult};
use crate::registry::get_coin_item;
use tw_bitcoin::entry::BitcoinEntry;
use tw_coin_entry::coin_entry_ext::CoinEntryExt;
use tw_ethereum::entry::EthereumEntry;
use tw_evm::evm_entry::EvmEntryExt;
use tw_solana::entry::SolanaEntry;

pub type CoinEntryExtStaticRef = &'static dyn CoinEntryExt;
pub type EvmEntryExtStaticRef = &'static dyn EvmEntryExt;

const BITCOIN: BitcoinEntry = BitcoinEntry;
const ETHEREUM: EthereumEntry = EthereumEntry;
const SOLANA: SolanaEntry = SolanaEntry;

pub fn blockchain_dispatcher(blockchain: BlockchainType) -> RegistryResult<CoinEntryExtStaticRef> {
    match blockchain {
        BlockchainType::Bitcoin => Ok(&BITCOIN),
        BlockchainType::Ethereum => Ok(&ETHEREUM),
        BlockchainType::Solana => Ok(&SOLANA),
        BlockchainType::Unsupported => Err(RegistryError::Unsupported),
    }
}

pub fn coin_dispatcher(
    coin: CoinType,
) -> RegistryResult<(CoinRegistryContext, CoinEntryExtStaticRef)> {
    let item = get_coin_item(coin)?;
    let coin_entry = blockchain_dispatcher(item.blockchain)?;
    let coin_context = CoinRegistryContext::with_coin_item(item);
    Ok((coin_context, coin_entry))
}

pub fn evm_dispatcher(coin: CoinType) -> RegistryResult<EvmEntryExtStaticRef> {
    let item = get_coin_item(coin)?;
    match item.blockchain {
        BlockchainType::Ethereum => Ok(&ETHEREUM),
        _ => Err(RegistryError::Unsupported),
    }
}
RUSTEOF

    # Cargo 1.75 cannot parse upstream lockfile v4. Wallet Core 4.8.4 has only
    # one git SourceId and it has no URL-encoded branch data, so v3 is
    # semantically sufficient. Also update tw_coin_registry's resolved edge
    # list to match the deliberately reduced manifest above; stale unreachable
    # package records may remain in Cargo.lock safely.
    awk '
    BEGIN { target=0; skipdeps=0; version=0; deps=0 }
    {
        if (NR == 3 && $0 == "version = 4") {
            print "version = 3"
            version++
            next
        }
        if ($0 == "[[package]]") {
            target=0
            print
            next
        }
        if ($0 == "name = \"tw_coin_registry\"") {
            target=1
            print
            next
        }
        if (target && $0 == "dependencies = [") {
            deps++
            print "dependencies = ["
            print " \"itertools\","
            print " \"lazy_static\","
            print " \"serde\","
            print " \"serde_json\","
            print " \"strum\","
            print " \"strum_macros\","
            print " \"tw_bitcoin\","
            print " \"tw_coin_entry\","
            print " \"tw_ethereum\","
            print " \"tw_evm\","
            print " \"tw_hash\","
            print " \"tw_keypair\","
            print " \"tw_memory\","
            print " \"tw_misc\","
            print " \"tw_solana\","
            print "]"
            skipdeps=1
            next
        }
        if (skipdeps) {
            if ($0 == "]") { skipdeps=0 }
            next
        }
        print
    }
    END {
        if (version != 1 || deps != 1) {
            printf("ERROR: Cargo.lock compatibility patch mismatch: version=%d registry_deps=%d\n", version, deps) > "/dev/stderr"
            exit 81
        }
    }' "$lock" > "$tmp"
    mv "$tmp" "$lock"

    grep -qx 'version = 3' <(sed -n '3p' "$lock") \
        || { echo "ERROR: Cargo.lock is not v3 after compatibility patch." >&2; exit 82; }
    if grep -q '"tw_zcash"' "$cargo"; then
        echo "ERROR: tw_zcash remains in SailVault Rust registry dependencies." >&2
        exit 83
    fi

    echo "Scoped Wallet Core Rust registry to BTC / ETH-EVM / SOL and Cargo.lock v3"
}


patch_codegen_v2_for_rust_175() {
    local lock="$ROOT/codegen-v2/Cargo.lock"
    local tmp="${lock}.sailvault.tmp"

    [[ -f "$lock" ]] || { echo "ERROR: missing codegen-v2 lockfile $lock" >&2; exit 84; }

    awk '
    BEGIN { version=0 }
    {
        if (NR == 3 && $0 == "version = 4") {
            print "version = 3"
            version++
            next
        }
        print
    }
    END {
        if (version != 1) {
            printf("ERROR: codegen-v2 Cargo.lock compatibility patch mismatch: version=%d\n", version) > "/dev/stderr"
            exit 85
        }
    }' "$lock" > "$tmp"
    mv "$tmp" "$lock"

    grep -qx 'version = 3' <(sed -n '3p' "$lock") \
        || { echo "ERROR: codegen-v2 Cargo.lock is not v3 after compatibility patch." >&2; exit 86; }

    echo "Patched Wallet Core codegen-v2 Cargo.lock v4 -> v3 for Cargo 1.75"
}

patch_cmake
patch_warnings_and_standard
patch_protobuf
patch_trezor_cmake
patch_fee_calculator
patch_twbase
patch_cpp20_surface
patch_rust_175_stdlib_surface
patch_rust_registry_for_sailvault
patch_codegen_v2_for_rust_175

echo "Wallet Core 4.8.4 Sailfish/GNU/Rust-1.75 compatibility adjustments applied successfully."
