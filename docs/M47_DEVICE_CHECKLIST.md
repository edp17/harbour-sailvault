# SailVault M47 device checklist

1. Install `0.47.0-1` over the proven M46 installation and launch normally from the Sailfish application icon.
2. Open Settings → Developer diagnostics. Confirm **Storage release checks passed**, **INI round-trip → Passed**, **Lifecycle self-test → Passed**, and **Profile metadata → Coherent**, and **Schema → v5**.
3. On this M46 → M47 upgrade, confirm **Profile origin → Existing profile**, **Tracked from → 0.46.0**, **Last upgrade from → 0.46.0 · M46**, and **Install identity → Present**.
4. Pull down **Run all diagnostics again** two or three times. Storage, Network boundary and Wallet Core checks must remain PASS.
5. Open Settings → Release readiness. Confirm **Storage release gate → Fresh / upgrade / persistence validated** and all other automatic checks pass.
6. Change one non-sensitive preference (for example fiat currency), close SailVault completely, reopen it and confirm the preference persists. The profile lineage above must remain stable.
7. Smoke-test portfolio, prices, activity, tokens, Provider health and Network fees. No functional regression is expected.
8. Confirm transaction construction, signing and broadcasting remain unavailable.

A destructive clean-install test on a genuinely unused profile/device remains part of final public-release regression. M47's local fresh-profile simulation is intentionally non-destructive and does not replace that final manual test.

Never use a personal recovery phrase or real funds with this development build.
