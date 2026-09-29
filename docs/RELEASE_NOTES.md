# SailVault 0.43.0-1 — stateless provider privacy

M43 builds directly on the proven M42 network-boundary milestone. It does not add transaction functionality and does not modify the Wallet Core 4.8.3 or Sailfish Secrets compatibility baseline.

Every provider request was already required to use HTTPS, reject redirects, respect bounded timeouts and stay below the 16 MiB response ceiling. M43 additionally makes those requests stateless: Qt cookie loading/saving is disabled, cached Basic/Digest HTTP authorization reuse is disabled, local HTTP cache loading/storage is disabled, and any accidentally supplied Cookie, Authorization, Referer or Origin headers are removed before dispatch. Provider requests also carry `Cache-Control: no-store, no-cache`, and the redirect budget is explicitly zero.

Developer diagnostics now validates those request-state controls with a synthetic local request and exposes separate cookie, authentication, cache and context-header results. Release readiness continues to gate on the combined network-boundary check.

Wallet Core 4.8.3 preparation, derivation, M41 cryptographic/ABI diagnostics, development-wallet test-mnemonic restriction and Sailfish Secrets handling are unchanged. Transaction construction, user signing and broadcasting remain unavailable. Never use a personal recovery phrase or real funds with this development build.
