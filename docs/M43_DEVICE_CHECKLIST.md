# SailVault M43 device checklist

Target: Xperia 10 III / Sailfish OS 5.x / aarch64

1. Build with the normal `build_packages.sh` command; no manual Wallet Core bootstrap.
2. Upgrade directly over the proven M42 installation without clearing application data.
3. Launch from the application icon and unlock the development wallet normally.
4. Open Settings → Release readiness. Require **Provider request policy** and every other automatic device check to pass.
5. Open Settings → Developer diagnostics. Under **Network boundary**, require:
   - Local network-boundary checks passed;
   - Endpoint policy reports `5/5` endpoints and `requests stateless=yes`;
   - Policy self-test is `Passed`;
   - Provider redirects are `Blocked`;
   - Cookies are `Not sent / stored`;
   - HTTP auth reuse is `Disabled`;
   - HTTP cache is `Bypassed / not stored`;
   - Referer / Origin is `Cleared`;
   - Response ceiling remains `16 MiB`.
6. Rerun Developer diagnostics several times and confirm the request-state checks and every M41 Wallet Core check remain PASS.
7. With normal online mode, smoke-test portfolio refresh, Recent activity, token holdings, a transaction-detail page, Provider health, Network fees and fiat prices.
8. Confirm the above provider operations still succeed normally; M43's stateless request policy must not introduce authentication/cookie/cache-related errors.
9. Enable Offline mode and confirm cached/public local data remains available without provider refreshes; then disable Offline mode and refresh normally.
10. Restart SailVault from the launcher and confirm preferences/public caches persist and Release readiness continues to pass.
11. Confirm there is still no transaction construction, user signing or broadcast UI.

Never use a personal recovery phrase or real funds during this milestone.
