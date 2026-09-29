# SailVault M41 device checklist

Target: Xperia 10 III / Sailfish OS 5.x / aarch64

1. Build with the normal `build_packages.sh` command; no manual Wallet Core bootstrap.
2. Upgrade over the proven M40r9 installation without clearing application data.
3. Launch from the application icon and unlock the development wallet normally.
4. Open Settings → Release readiness and confirm all automatic checks pass.
5. Open Settings → Developer diagnostics and inspect Wallet Core self-test. Require PASS for:
   - BIP-39 validation;
   - deterministic Ethereum derivation;
   - deterministic Bitcoin BIP-84 derivation;
   - deterministic Solana derivation;
   - address validation hardening;
   - public-key/address round-trip;
   - derivation-path bounds;
   - secp256k1 validation hardening;
   - Rust/C++ CryptoBox bridge.
6. Run diagnostics again twice. Results must remain stable and must not unlock/interrogate the protected Sailfish Secrets collection.
7. Confirm the deterministic public addresses remain exactly:
   - ETH `0x9858EfFD232B4033E47d90003D41EC34EcaEda94`
   - BTC `bc1qcr8te4kr609gcawutmrza0j4xv80jy8z306fyu`
   - SOL `GjJyeC1r2RgkuoCWMyPYkCWSGSGLcz266EaAkLA27AhL`
8. Smoke-test portfolio refresh, Recent activity, receive QR, Watch-only, Address book and Offline mode.
9. Restart SailVault from the launcher and confirm settings/public data persist.
10. Confirm there is still no transaction construction, user signing or broadcast UI.

Never use a personal recovery phrase or real funds during this milestone.
