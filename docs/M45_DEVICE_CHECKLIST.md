# SailVault M45 device checklist

1. Build `0.45.0-1` normally for Xperia 10 III/aarch64. The preparation output must identify Wallet Core 4.8.4 and pinned commit `d40d24a63d92619167903369308bf0e2f7eb3a59`.
2. Install/upgrade from the proven M44 build and confirm the existing development wallet, Watch-only entries, Address book and non-sensitive settings remain intact.
3. Open About, Privacy & local data and Release readiness. Confirm Wallet Core displays `4.8.4` and build identity is `M45 · Wallet Core 4.8.4 validation`.
4. Run Settings → Developer diagnostics at least three times. Every Wallet Core item must remain PASS, especially Address validation hardening, Public-key/address round-trip, Derivation-path bounds, secp256k1 validation hardening and Rust/C++ CryptoBox bridge.
5. Run the storage/settings probe repeatedly; it must remain PASS and must not unlock/interrogate the secure wallet collection merely to satisfy Release readiness.
6. Confirm Network boundary diagnostics remain PASS, including endpoint policy, redirect blocking, response ceiling, stateless request policy, response media types and JSON-RPC envelopes.
7. Smoke-test ETH/BTC/SOL portfolio balances, prices, Recent activity, token holdings, representative transaction details, Provider health and Network fees.
8. Exercise Offline mode and confirm cached/public read-only data remains usable without provider requests.
9. Confirm transaction construction, signing and broadcasting remain unavailable everywhere in the UI.
10. Do not enter any personal recovery phrase and do not use real funds. M45 is still a development/read-only validation milestone.
