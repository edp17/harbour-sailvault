# SailVault — Milestone 52

**Version:** `0.52.0-1`  
**Build:** development signing boundary  
**Wallet Core baseline:** `4.8.4`  
**Pinned upstream commit:** `d40d24a63d92619167903369308bf0e2f7eb3a59`

M51r2 is proven on Xperia 10 III/aarch64. M52 starts SailVault's signing phase without enabling a general sending wallet: the proven M49 Ethereum construction may now cross a deliberately narrow **development-only sign/verify boundary** that accepts only the published BIP39 test wallet. The raw signature is verified inside C++, hashed into a proof fingerprint, then discarded before QML can see it. No signed transaction is assembled and broadcasting remains unavailable.

## M52 goals

- preserve the proven M49 Ethereum, M50 Bitcoin and M51r2 Solana unsigned constructors;
- revalidate the complete M48 intent and M49 signing snapshot before any secret access;
- require the M49 construction snapshot to be no more than two minutes old;
- independently require the loaded ETH/BTC/SOL addresses to match the published development test vectors;
- add a C++-only WalletVault callback that reopens Sailfish Secrets, revalidates all three addresses and derives and lends only a transient Ethereum private-key handle to the signer;
- derive the Ethereum key only inside Wallet Core and never call `TWPrivateKeyData`;
- sign the exact Wallet Core Keccak-256 M49 hash with secp256k1 and immediately verify the resulting signature against the derived public key;
- keep mnemonic, private-key bytes and raw signature bytes out of QML;
- hash the verified signature into a SHA-256 proof fingerprint, then wipe the temporary signature buffer;
- add network-free/Secrets-free deterministic diagnostics using only the published public test mnemonic;
- keep Bitcoin/Solana signing, signed-transaction assembly, broadcast APIs and real-funds use unavailable.

## Build

Normal Sailfish SDK build:

```sh
cd ~/mer/android/droid
rpm/dhd/helpers/build_packages.sh -o -b hybris/mw/harbour-sailvault -s rpm/harbour-sailvault.spec
```

No manual Wallet Core bootstrap should be required. M52 retains the proven M45 Wallet Core 4.8.4 preparation marker and compatibility recipe.

On a build failure, use the **first real compiler/linker/Cargo error**; later make/RPM failures are normally consequences.

## Security status

M52 is still a development build and must not be used with real funds. The live signing proof is hard-gated to the same public test mnemonic SailVault already accepts for development restore/create. `WalletVault` revalidates the secure stored wallet immediately before lending a transient Ethereum key handle to `DevelopmentSigningService`. The service signs/verifies with that least-privilege key handle inside C++, never exports private-key bytes, never returns the raw signature to QML, and contains no network or broadcast API.

M52 therefore proves the **Secrets → Wallet Core signing boundary**, not production transaction signing. Production signing policy, user confirmation, signed transaction assembly, Bitcoin secure change derivation, Bitcoin/Solana signatures and broadcasting remain later milestones.

## Project

Repository: `https://github.com/edp17/harbour-sailvault`  
License: BSD 3-Clause
