# SailVault M42 device checklist

Target: Xperia 10 III / Sailfish OS 5.x / aarch64

1. Build with the normal `build_packages.sh` command; no manual Wallet Core bootstrap.
2. Upgrade over the proven M41 installation without clearing application data.
3. Launch from the application icon and unlock the development wallet normally.
4. Open Settings → Release readiness. Require **Provider endpoint policy** to pass and all other automatic checks to remain passed.
5. Open Settings → Developer diagnostics. Under **Network boundary**, require:
   - Local network-boundary checks passed;
   - Endpoint policy reports `5/5` configured endpoints passing;
   - Policy self-test is `Passed`;
   - Provider redirects are `Blocked`;
   - Response ceiling is `16 MiB`.
6. On the same diagnostics page, rerun the Wallet Core diagnostics at least twice and confirm every M41 Wallet Core check still passes.
7. Open Settings → Network & portfolio and confirm all five configured endpoints remain HTTPS URLs. An attempted plaintext `http://` replacement must not become the active endpoint.
8. With normal online mode, smoke-test portfolio refresh, Recent activity, token holdings, a transaction-detail page, Provider health, Network fees and fiat prices.
9. Enable Offline mode and confirm the application continues to use cached/public local data without provider refreshes; then disable Offline mode and refresh normally.
10. Restart SailVault from the launcher and confirm preferences/public caches persist and the Release readiness network-boundary check still passes.
11. Confirm there is still no transaction construction, user signing or broadcast UI.

Never use a personal recovery phrase or real funds during this milestone.
