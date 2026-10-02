# M50 device checklist

1. Build/install `0.50.0-1` normally on Xperia 10 III.
2. Open **Settings → Developer diagnostics** and run all diagnostics again.
3. Confirm **Bitcoin unsigned construction** reports PASS for exact BTC→sats, Wallet Core scripts, coin selection/fee, unsigned serialization/SHA256d, PSBT v0 and construction binding.
4. Confirm existing Wallet Core, storage, network-boundary, unsigned-intent and Ethereum-construction diagnostics remain PASS.
5. Open **Bitcoin → Prepare transfer** and use any valid Bitcoin mainnet destination (for example the second published BIP84 vector `bc1qnjg0jd8228aq7egyzacy8cys3knf9xvrerkf9g`) with `0.001 BTC`.
6. Review the intent and tap **Construct unsigned Bitcoin transaction**.
7. The public development address is expected to return **no confirmed spendable UTXOs**. That is the correct live safety-path result; do not fund the test wallet to force a positive live construction.
8. Confirm **Signing → Disabled**, **Broadcasting → Disabled**, and the signing button remains disabled.
9. Re-check Ethereum construction with a plain EOA destination to confirm M49 behavior is unchanged.
