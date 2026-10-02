# SailVault M49 device checklist

1. Install `0.49.0-1` over the proven M48 installation and launch normally.
2. Confirm About/Cover identify Milestone 49 and `Ethereum unsigned construction` with Wallet Core 4.8.4.
3. Open Developer diagnostics and run **Run all diagnostics again**. Confirm the new **Ethereum unsigned construction** section passes RPC quantity, ETH→wei, reviewed-intent mutation, EIP-1559 RLP, Wallet Core Keccak-256 and construction-binding checks. All earlier diagnostics must remain PASS.
4. Open Release readiness and confirm automatic checks pass and Ethereum construction reports `Unsigned EIP-1559 · local self-test passed`.
5. Open the development wallet → Ethereum → **Prepare transfer**. Enter a valid Ethereum EOA destination and a small test amount. Do not fund the development wallet and never use a real recovery phrase.
6. Open **Review unsigned intent**. Confirm the M48 intent fingerprint is still shown and signing/broadcasting remain Disabled.
7. Tap **Construct unsigned Ethereum payload**. Confirm this is the first point where network construction starts.
8. On success, confirm the page shows: RPC provider host, chain ID `1`, pending nonce, `21000 · plain EOA transfer`, latest base fee, priority fee, max fee per gas, maximum network fee, construction time, unsigned type-2 payload, Wallet Core Keccak-256 signing hash and a 64-character construction fingerprint.
9. Tap **Reconstruct with fresh inputs** and confirm construction succeeds again without exposing any secret or enabling signing.
10. Confirm **Signing unavailable in M49** remains disabled and Broadcasting remains Disabled.
11. Repeat the M48 Bitcoin and Solana review flow and confirm they remain intent-only with no construction button.
12. If available, test an Ethereum address known to contain contract/delegated code and confirm M49 refuses construction with the EOA-only safety message.
13. Smoke-test normal portfolio, prices, activity, tokens, provider health and network fees.

M49 is development-only. Never use real funds or a personal recovery phrase.
