#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 1 ]]; then
    echo "usage: patch-wallet-core.sh <wallet-core-dir>" >&2
    exit 2
fi

ROOT="$(cd "$1" && pwd)"

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
            print "        message(WARNING \"SailVault M1: experimental GNU build of Wallet Core\")"
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

patch_warnings() {
    local src="$ROOT/cmake/CompilerWarnings.cmake"
    local tmp="${src}.sailvault.tmp"
    [[ -f "$src" ]] || { echo "ERROR: missing $src" >&2; exit 11; }

    awk '
    BEGIN { fatal=0; a=0; b=0 }
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
        print
    }
    END {
        if (fatal != 1 || a != 1 || b != 1) {
            printf("ERROR: CompilerWarnings.cmake patch context mismatch: fatal=%d shorten=%d nullability=%d\n", fatal, a, b) > "/dev/stderr"
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
        if ($0 ~ /^static constexpr (DefaultFeeCalculator|DecredFeeCalculator|SegwitFeeCalculator)/) {
            n++
            sub(/^static constexpr /, "static const ")
        }
        print
    }
    END {
        if (n != 6) {
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

patch_cmake
patch_warnings
patch_protobuf
patch_trezor_cmake
patch_fee_calculator
patch_twbase

echo "Wallet Core 4.0.27 Sailfish/GNU cross-build adjustments applied successfully."
