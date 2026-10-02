# SailVault M52 device checklist

1. Build/install `0.52.0-1` normally on Xperia 10 III/aarch64.
2. Open **Settings → Developer diagnostics**, run all diagnostics again and confirm every previous M41-M51 check remains PASS.
3. Confirm **M52 development signing boundary** passes: published wallet identity, 65-byte secp256k1 sign/verify, mutated-digest rejection, proof binding and **Raw signature to QML → Not exported**.
4. Open **Settings → Release readiness** and confirm **Development signing boundary → ✓ Published test wallet / non-exporting sign-verify validated**.
5. Ensure the public development wallet is loaded/unlocked.
6. Open **Ethereum → Prepare transfer**. Use a plain EOA destination such as `0x000000000000000000000000000000000000dEaD` and an amount such as `0.001 ETH`.
7. Review the intent and tap **Construct unsigned Ethereum payload**. Confirm M49 construction succeeds as before.
8. Within two minutes, tap **Development sign / verify**.
9. Confirm the result reports **PASS · Public test-wallet Ethereum signature created, verified and discarded · no signed transaction assembled**.
10. Confirm the signer is `0x9858EfFD232B4033E47d90003D41EC34EcaEda94`, signature handling says `65 bytes · verified · discarded`, and a 64-character discarded-signature proof fingerprint is shown.
11. Confirm no raw signature, recovery phrase or private key appears anywhere in the UI.
12. Confirm **Production signing → Disabled** and **Broadcasting → Disabled** remain visible.
13. Reconstruct the Ethereum payload, wait more than two minutes if practical, and confirm a stale construction is refused by the sign/verify gate until reconstructed.
14. Confirm Bitcoin and Solana still have no signing action; their M50/M51r2 construction behavior is unchanged.
15. Lock/clear the wallet session while the review page is open and confirm the development sign/verify action becomes unavailable/reset.
16. Do not send real funds to the public development wallet.
