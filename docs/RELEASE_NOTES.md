# SailVault 0.39.0-1 — read-only beta 1

SailVault is a native Sailfish OS multi-chain wallet project using Trust Wallet
Core. Version 0.39.0-1 is the first public **read-only beta** and promotes the
proven M38 RC2 code without changing wallet or provider behaviour.

## Included

- Ethereum, Bitcoin and Solana public balances and fiat valuation.
- ERC-20, SPL and Token-2022 holdings.
- Paginated public-chain activity and transaction details.
- Watch-only profiles and Address book public-address explorer.
- Completely offline receive QR codes.
- Provider health and network-fee estimates.
- Local portfolio history and analytics.
- Cached last-known values, provider timeout handling and explicit Offline mode.
- Sailfish Secrets-backed development wallet lifecycle using only the published
  public BIP39 test mnemonic.
- Startup/session locking, persistence diagnostics, privacy/local-data cleanup
  and local release-readiness checks.

## Beta 1 release notes

- Promotes the proven M38 read-only beta RC2 to the first beta release.
- No C++ wallet/provider behaviour changes are introduced by M39.
- Release-facing build identity is now `0.39.0-1 · read-only beta 1`.
- Final packaged regression checklist covers both a completely fresh install and
  an upgrade from the proven M38 RC2.
- RPM `%check` continues to run the project preflight automatically.
- Release preflight rejects stale current-build RC/candidate wording.

## Deliberately unavailable

- Transaction construction.
- Transaction signing.
- Transaction broadcasting.
- Real-funds wallet support.

Wallet Core 4.0.27 is retained only as the proven Sailfish compatibility/build
baseline. Do not use personal recovery material or real funds with this beta.
