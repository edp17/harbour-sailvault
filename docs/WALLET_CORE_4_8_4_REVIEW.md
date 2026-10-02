# SailVault M45 — Wallet Core 4.8.4 maintained-tip review

## Scope

M45 advances SailVault's Wallet Core source from the device-proven 4.8.3 baseline to upstream release 4.8.4 while keeping the application read-only. The upstream release was published on 2026-09-24 at commit `d40d24a63d92619167903369308bf0e2f7eb3a59`.

The official upstream comparison from 4.8.3 (`5031fe6dd14b6d11a5de9892ea4ea1ee443fa13f`) to 4.8.4 contains one commit. Its production-source delta is limited to `registry.json`, where Robinhood Chain's explorer URL changes from Blockscout to RobinScan/Etherscan. The corresponding Robinhood Chain coin-type test is the only test-source change. No BTC, ETH, SOL, derivation, key, signing, Rust, protobuf, secp256k1 or CryptoBox source is changed by that release delta.

## Source pinning

SailVault no longer downloads Wallet Core for M45 through a mutable tag URL. `prepare-wallet-core.sh` downloads the immutable upstream commit archive for:

`d40d24a63d92619167903369308bf0e2f7eb3a59`

The preparation step also requires the 4.8.4-specific Robinhood registry marker before applying SailVault's compatibility layer. The prepared-tree marker is:

`4.8.4-d40d24a63d92-sailvault-m45r1`

This does not replace HTTPS or upstream repository security, but it prevents a later tag movement from silently changing the Wallet Core source accepted by this SailVault milestone.

## Sailfish compatibility layer

The M40/M41 compatibility recipe is retained with version labels advanced to 4.8.4:

- Sailfish Rust/Cargo 1.75;
- `aarch64-unknown-linux-gnu` Rust target through Scratchbox2;
- `aarch64-meego-linux-gnu` C/C++ toolchain;
- Cargo `-j1`;
- project-local cbindgen 0.26.0;
- protobuf 3.20.3;
- C++17/GNU compatibility rewrites;
- Cargo.lock v4-to-v3 compatibility;
- Rust feature scope limited to BTC, ETH/EVM and SOL plus required shared utilities;
- Rust-backed C++ codegen-v2 bridge generation and validation.

Because upstream 4.8.4 changes none of the patched source contexts, the exact compatibility transforms are expected to remain applicable. The Scratchbox2/aarch64 build is still an on-device/SDK proof requirement.

## Security gate

M45 must not be treated as approval for real funds merely because 4.8.4 is upstream's current release. Before this version becomes SailVault's accepted baseline on device, all M41 diagnostics must pass repeatedly, including:

- BIP-39 positive/negative validation;
- canonical BTC/ETH/SOL derivation and address vectors;
- malformed and wrong-chain address rejection;
- private-key → public-key → address round-trip without exporting private bytes;
- derivation-path bounds;
- secp256k1 genuine/mutated/short-signature checks;
- Rust/C++ CryptoBox bridge validation.

M42-M44 network/privacy/protocol gates must remain PASS as well. Transaction construction, signing and broadcasting remain unavailable in M45.
