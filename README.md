# SailVault — Milestone 39

**Package:** `harbour-sailvault`  
**Version:** `0.39.0-1`  
**Build:** read-only beta 1  
**Wallet Core compatibility baseline:** `4.0.27`

M39 is the first public read-only beta packaging milestone. It promotes the
proven M38 RC2 feature set without adding transaction construction, signing or
broadcasting and without changing wallet/provider behaviour.

## Read-only beta 1

- Promote the proven RC2 source to `0.39.0-1 / read-only beta 1`.
- Keep application/runtime behaviour unchanged from M38 RC2.
- Finalize RPM, About/Cover/Release-readiness build identity through the
  centralized compile-time metadata.
- Package final read-only beta release notes and regression checklist.
- Add explicit fresh-install and M38 → M39 upgrade checks to the device
  checklist.
- Strengthen source-package preflight so release-facing metadata cannot still
  identify the current build as an RC/candidate.
- Preserve the established release ZIP layout with a top-level
  `harbour-sailvault/` directory.

## Proven read-only feature set

- native Trust Wallet Core Sailfish/aarch64 compatibility path;
- Sailfish Secrets development mnemonic storage with `DeviceLockRelock`;
- development wallet restricted to the published BIP39 test vector;
- BTC/ETH/SOL public derivation and balances;
- ERC-20, SPL and Token-2022 holdings;
- fiat prices, local portfolio history and history analytics;
- paginated multi-chain activity and chain-specific transaction details;
- Wallet Core-validated Watch-only and Address book;
- per-public-address cached balance snapshots;
- completely offline 492×492 receive QR codes;
- Provider health and Network fees;
- automatic session locking;
- explicit Offline mode and bounded provider timeouts;
- schema-versioned explicit INI storage, startup persistence probe and
  malformed non-sensitive-data quarantine;
- selective privacy/local-data cleanup;
- completely fresh-install launcher-first persistence verified on device;
- local Release readiness checks which do not unlock/probe the protected wallet
  collection.

## Security gate

This is a **read-only beta**, not a real-funds wallet release. The development
wallet restore path accepts only the public test phrase:

`abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon about`

Never enter a personal recovery phrase or use real funds with this Wallet Core
4.0.27 compatibility baseline.

Before real-funds transaction functionality is introduced, Wallet Core must be
upgraded to a maintained version or the relevant security baseline must be
carefully reviewed/backported.

## Build

```sh
cd ~/mer/android/droid
rpm/dhd/helpers/build_packages.sh -o -b hybris/mw/harbour-sailvault -s rpm/harbour-sailvault.spec
```

No manual Wallet Core bootstrap is required.
