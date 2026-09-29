# SailVault M42 — network boundary review

## Scope

SailVault is still read-only, but balances, activity, token discovery, transaction details, fee estimates and fiat prices consume untrusted remote data. M42 hardens that boundary without changing Wallet Core, recovery-material handling or wallet capabilities.

## Endpoint policy

SailVault already sanitized persisted provider preferences at startup. M42 retains that behavior and additionally applies the shared HTTPS policy when ordinary services read endpoint settings. A valid endpoint must:

- parse as a valid URL;
- use `https`;
- contain a host;
- contain no username/password in the URL.

If a stored ordinary-provider endpoint fails that policy, SailVault uses its compiled HTTPS fallback. Provider health and fee-estimate services also validate their directly supplied endpoint parameters before issuing a request.

## Reply policy

Every provider reply is armed with the shared SailVault network guard immediately after creation.

- **Timeout:** ordinary provider requests retain the 15-second limit; health and fee probes retain 12 seconds.
- **Redirects:** every `QNetworkRequest` explicitly disables Qt redirect following before dispatch. A reply carrying `RedirectionTargetAttribute` is independently treated as a policy failure, so SailVault does not accept redirected provider data.
- **Response size:** the reply buffer is bounded to one byte beyond the 16 MiB policy ceiling, and a response is stopped if declared `Content-Length`, transfer progress or accumulated reply data exceeds that ceiling.
- **Errors before parsing:** endpoint, redirect, size, timeout and HTTP/network failures are reported before JSON/text payload processing.
- **Cancellation:** superseded requests continue to be cancelled and generation checks prevent stale replies replacing newer state.

The response ceiling is deliberately generous for the current BTC/ETH/SOL read-only APIs while preventing an unexpectedly large reply from growing without an application-level bound.

## Local diagnostics

`AppTools::networkSecurityDiagnostics()` performs no network request. It checks all five configured provider endpoints against the local policy, tests the URL validator with a valid HTTPS URL plus plaintext, credential-bearing and malformed negative cases, and verifies that the shared request hardener switches redirect following off. The result appears in Developer diagnostics and is a Release readiness automatic gate.

## What this does not claim

M42 does not authenticate provider content, pin provider certificates, hide the user's IP address or hide public wallet addresses from providers. TLS certificate validation remains the platform/Qt network stack's responsibility. A malicious or incorrect provider can still return plausible but false public-chain data inside an otherwise valid HTTPS response.

M42 also does not make SailVault ready for real funds. Transaction construction, confirmation, signing, broadcast, nonce handling, fee selection and chain-specific transaction validation remain separate future security work.
