# SailVault M40 — Wallet Core 4.8.3 migration checklist

> M40r9 is the proven compatibility result. M41 keeps the same Wallet Core 4.8.3 compatibility recipe but advances the preparation marker to `4.8.3-sailvault-m41r1` so the security-validation build is prepared from a clean tree.
## Build gate

- Build with the normal `build_packages.sh` command; do not manually bootstrap Wallet Core.
- Confirm preparation reports Wallet Core 4.8.3 and marker `4.8.3-sailvault-m40r9`.
- Confirm the preparation log reports the C++17 compatibility patch; no `std::countl_zero` / `<bit>` C++20 failure should remain.
- Confirm Sailfish Rust remains 1.75 and target remains `aarch64-unknown-linux-gnu`.
- If the build fails, capture the first Cargo/compiler/linker error only.

## Device gate

- Upgrade M39 → M40 without clearing application data.
- Launch from the application icon.
- Open Settings → Release readiness and confirm automatic checks pass.
- Confirm Wallet Core self-test passes.
- Confirm deterministic addresses remain exactly:
  - ETH `0x9858EfFD232B4033E47d90003D41EC34EcaEda94`
  - BTC `bc1qcr8te4kr609gcawutmrza0j4xv80jy8z306fyu`
  - SOL `GjJyeC1r2RgkuoCWMyPYkCWSGSGLcz266EaAkLA27AhL`
- Rerun the automatic checks from the pulley; the result must remain passed.

## Read-only regression gate

- Portfolio balances and cached values load.
- Activity switching/pagination works for ETH/BTC/SOL.
- Token holdings and transaction details open.
- Receive QR remains offline.
- Watch-only and Address book survive the upgrade.
- Portfolio history/analytics survive the upgrade.
- Offline mode blocks provider requests and cached/local pages remain available.
- Provider health and network fees still work after Offline mode is disabled.

## Security gate

- Development restore still accepts only the published public BIP39 test mnemonic.
- No construction/signing/broadcast UI is present.
- Do not use real funds.
- Passing M40 validates the compatibility migration only; it does not by itself approve transaction signing.
