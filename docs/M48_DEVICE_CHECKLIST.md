# SailVault M48 device checklist

1. Install `0.48.0-1` over the proven M47 installation and launch normally.
2. Open Settings → Developer diagnostics. Confirm **Unsigned transaction intent → Local transaction-intent checks passed** and that Chain metadata, Address validation, Decimal amounts and Intent fingerprint all pass. Existing Storage, Network boundary and Wallet Core diagnostics must remain PASS.
3. Open Settings → Release readiness. Confirm **Unsigned intent model → Address / amount / fingerprint validated** and all automatic checks pass.
4. Open the unlocked development wallet → Ethereum → **Prepare transfer**. Confirm the draft identifies Ethereum mainnet/chain ID 1, the public source address and displayed balance.
5. Enter an invalid destination and confirm review remains disabled. Enter a valid Ethereum address (the published development address is safe for a self-transfer test) and confirm Wallet Core validation passes.
6. Verify amount validation: `0` is rejected; `1e-3` is rejected; a normal positive decimal is accepted; more than 18 ETH decimal places is rejected.
7. Open **Review unsigned intent**. Confirm chain/network, amount, source, destination and a 64-character SHA-256 fingerprint appear. Confirm **Signing unavailable in M48** is disabled.
8. Return, change the amount, review again and confirm the fingerprint changes.
9. Repeat basic draft/review checks for Bitcoin (8 decimals) and Solana (9 decimals). Address-book entries for the selected chain should be tappable destination shortcuts when present.
10. Confirm Watch-only and Address-book public-address pages do not gain a send/prepare-transfer action.
11. Smoke-test portfolio, prices, activity, tokens, Provider health and Network fees for regressions.
12. Confirm no chain transaction bytes, signing action or broadcast action is available anywhere.

M48 remains safe for development only. Never use a personal recovery phrase or real funds.
