# SailVault M51r2 device checklist

1. Build/install the M51r2 `0.51.0-1` candidate normally on Xperia 10 III.
2. Open **Settings → Developer diagnostics** and run all diagnostics again.
3. Confirm **Solana unsigned construction** reports PASS for exact SOL→lamports conversion, Wallet Core Base58 validation, the upstream external-signing preimage vector, zero-signature template, reviewed-intent mutation rejection, construction fingerprint and fee formatting.
4. Confirm existing Wallet Core, storage, network-boundary, M48 intent, M49 Ethereum and M50 Bitcoin diagnostics remain PASS.
5. Open **Solana → Prepare transfer**.
6. For a safe development test, use the public development address itself as destination: `GjJyeC1r2RgkuoCWMyPYkCWSGSGLcz266EaAkLA27AhL`, and enter `0.000001 SOL`. No funding is required because M51 does not send anything.
7. Review the intent and tap **Construct unsigned Solana message**.
8. Confirm the result shows the exact mainnet genesis `5eykt4UsFv8P8NJdTREpY1vzqKqZKvdpKuc147dw2N9d`, a recent blockhash, last-valid block height, Wallet Core signing message, fee quote and a 64-hex construction fingerprint.
9. Confirm the instruction reads **System Program transfer · 1 signer · 1 instruction** and the signer/fee payer is the development Solana address.
10. Confirm **Signing → Disabled**, **Broadcasting → Disabled**, and the signing button remains disabled as **Signing unavailable in M51**.
11. Use **Reconstruct with fresh blockhash** once; the recent blockhash/fingerprint may change while the reviewed transfer intent remains the same.
12. Do not use real funds or a real recovery phrase.
