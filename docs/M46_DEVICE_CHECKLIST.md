# SailVault M46 device checklist

Use the Xperia 10 III/aarch64 build produced by the normal Sailfish SDK command.

1. Open the main wallet and Watch-only portfolio. Confirm each ETH/BTC/SOL row presents the chain name smaller than its primary balance, with fiat/address/freshness progressively secondary.
2. Open Chain, Token, Activity, Portfolio history and a representative transaction detail. Confirm hero values remain prominent while row titles/status text no longer look larger than the value they describe.
3. Open a Receive QR page. Confirm the page has one header, the QR follows naturally, and copy/safety text is unchanged.
4. Open Settings → Developer diagnostics. Pull down and run “Run all diagnostics again”. Confirm the storage probe, Network boundary and Wallet Core self-test refresh and remain PASS.
5. Open Settings → Release readiness. Confirm the build identity is `0.46.0-1 · Sailfish UI consistency`, Wallet Core is 4.8.4, and all automatic checks remain PASS.
6. Smoke-test Portfolio, Recent activity, Tokens, Provider health and Network fees to confirm no runtime regression.

M46 remains read-only. Do not enter a personal recovery phrase or use real funds.
