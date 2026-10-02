# SailVault M44 device checklist

1. Build and install `0.44.0-1` normally on the Xperia 10 III/aarch64.
2. Open Settings → Developer diagnostics. Require **Local network-boundary checks passed**.
3. Require **Response media types → Validated** and **JSON-RPC envelopes → Validated**.
4. Confirm the existing HTTPS endpoint, redirect, cookie, HTTP-auth, cache, Referer/Origin and 16 MiB response-ceiling rows still pass.
5. Open Settings → Release readiness. Require **Provider response validation → ✓ Media type / JSON-RPC validated** and all other automatic checks to pass.
6. Run Developer diagnostics and the storage probe at least twice; results must remain stable.
7. Refresh the development portfolio and prices. ETH/BTC/SOL balances and fiat prices should load normally.
8. Open Recent activity, token holdings and representative transaction details for ETH/BTC/SOL; existing read-only data should still load.
9. Open Provider health and run the full check. Ethereum mainnet chain ID, Bitcoin mainnet genesis and both Solana mainnet genesis checks should remain verified.
10. Open Network fees and confirm ETH/BTC/SOL estimates still load.
11. Enable Offline mode and confirm provider requests remain blocked; disable it and refresh normally.
12. Confirm no transaction construction, signing or broadcasting UI exists.
