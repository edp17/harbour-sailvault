cmake_minimum_required(VERSION 3.18)
project(harbour-sailvault VERSION 0.39.0 LANGUAGES C CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt5 REQUIRED COMPONENTS Core Gui Qml Quick Network)
find_package(Threads REQUIRED)
include(FindPkgConfig)
pkg_search_module(SAILFISH sailfishapp REQUIRED)
pkg_search_module(SAILFISHSECRETS sailfishsecrets REQUIRED)

set(WALLET_CORE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vendor/wallet-core")
set(WALLET_CORE_RUST_LIB "${WALLET_CORE_DIR}/rust/target/release/libwallet_core_rs.a")
set(WALLET_CORE_RUST_HEADER "${WALLET_CORE_DIR}/src/rust/bindgen/WalletCoreRSBindgen.h")
set(WALLET_CORE_DERIVATION_HEADER "${WALLET_CORE_DIR}/include/TrustWalletCore/TWDerivation.h")
set(WALLET_CORE_PROTO_SENTINEL "${WALLET_CORE_DIR}/src/proto/Algorand.pb.h")

if(NOT EXISTS "${WALLET_CORE_DIR}/CMakeLists.txt")
    message(FATAL_ERROR
        "Wallet Core is not prepared. Run scripts/prepare-wallet-core.sh first.")
endif()

if(NOT EXISTS "${WALLET_CORE_DERIVATION_HEADER}" OR NOT EXISTS "${WALLET_CORE_PROTO_SENTINEL}")
    message(FATAL_ERROR
        "Wallet Core generated C/C++ sources are missing after preparation. "
        "See the first generate-wallet-core-sources.sh error above this CMake invocation.")
endif()

if(NOT EXISTS "${WALLET_CORE_RUST_LIB}")
    message(FATAL_ERROR
        "Wallet Core Rust static library is missing after the RPM Rust build step. "
        "See the first Cargo error above this CMake invocation.")
endif()

if(NOT EXISTS "${WALLET_CORE_RUST_HEADER}")
    message(FATAL_ERROR
        "Wallet Core Rust FFI header is missing after the RPM Rust build step. "
        "See the cbindgen error above this CMake invocation.")
endif()

# Wallet Core defaults are aimed at its own native development builds.
# Milestone 1 deliberately disables tests/examples and enables only the
# small downstream cross-build adaptations from patches/.
set(TW_UNIT_TESTS OFF CACHE BOOL "" FORCE)
set(TW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(TW_ENABLE_CCACHE OFF CACHE BOOL "" FORCE)
set(TW_WARNINGS_AS_ERRORS OFF CACHE BOOL "" FORCE)
set(SAILVAULT_ALLOW_GNU ON CACHE BOOL "" FORCE)
set(SAILVAULT_CROSS_COMPILE ON CACHE BOOL "" FORCE)

# Embed Wallet Core's helper libraries into SailVault. The Sailfish %cmake
# macro defaults BUILD_SHARED_LIBS to ON, which would otherwise turn protobuf
# and TrezorCrypto into runtime shared-library dependencies.
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

add_subdirectory("${WALLET_CORE_DIR}"
                 "${CMAKE_CURRENT_BINARY_DIR}/wallet-core"
                 EXCLUDE_FROM_ALL)

# TrustWalletCore is a static library. Its Rust archive introduces references
# to libdl, libc math/realtime functions and pthreads only after the archive is
# encountered by the linker. Attach these system libraries to TrustWalletCore
# itself so CMake emits them after Wallet Core's propagated static archives.
# This is important with Sailfish's --as-needed link behavior.
target_link_libraries(TrustWalletCore PUBLIC
    ${CMAKE_DL_LIBS}
    m
    rt
    Threads::Threads
)

add_executable(harbour-sailvault
    src/main.cpp
    src/walletcoreprobe.cpp
    src/walletcoreprobe.h
    src/walletvault.cpp
    src/walletvault.h
    src/portfolioservice.cpp
    src/portfolioservice.h
    src/portfoliohistoryservice.cpp
    src/portfoliohistoryservice.h
    src/apptools.cpp
    src/apptools.h
    src/activityservice.cpp
    src/activityservice.h
    src/tokenservice.cpp
    src/tokenservice.h
    src/priceservice.cpp
    src/priceservice.h
    src/sessionsecurity.cpp
    src/sessionsecurity.h
    src/settingsstore.cpp
    src/settingsstore.h
    src/transactiondetailservice.cpp
    src/transactiondetailservice.h
    src/watchonlyservice.cpp
    src/watchonlyservice.h
    src/addressbookservice.cpp
    src/addressbookservice.h
    src/qrimageprovider.cpp
    src/qrimageprovider.h
    src/networkhealthservice.cpp
    src/networkhealthservice.h
    src/networkfeeservice.cpp
    src/networkfeeservice.h
    src/networkrequestutils.h
)

target_compile_definitions(harbour-sailvault PRIVATE
    SAILVAULT_WALLET_CORE_VERSION="4.0.27"
    SAILVAULT_APP_VERSION="${PROJECT_VERSION}"
    SAILVAULT_PACKAGE_RELEASE="1"
    SAILVAULT_MILESTONE="39"
    SAILVAULT_BUILD_LABEL="read-only beta 1"
)

target_include_directories(harbour-sailvault PRIVATE
    ${SAILFISH_INCLUDE_DIRS}
    ${SAILFISHSECRETS_INCLUDE_DIRS}
)

target_link_libraries(harbour-sailvault PRIVATE
    Qt5::Core
    Qt5::Gui
    Qt5::Qml
    Qt5::Quick
    Qt5::Network
    ${SAILFISH_LDFLAGS}
    ${SAILFISHSECRETS_LDFLAGS}
    TrustWalletCore
)

install(TARGETS harbour-sailvault
    RUNTIME DESTINATION bin
)
install(DIRECTORY qml
    DESTINATION share/harbour-sailvault
)
install(FILES harbour-sailvault.desktop
    DESTINATION share/applications
)
install(FILES harbour-sailvault.png
    DESTINATION share/icons/hicolor/86x86/apps
)

file(WRITE "${CMAKE_BINARY_DIR}/QtCreatorDeployment.txt"
    "${CMAKE_INSTALL_PREFIX}\n${CMAKE_BINARY_DIR}/harbour-sailvault:bin\n")
