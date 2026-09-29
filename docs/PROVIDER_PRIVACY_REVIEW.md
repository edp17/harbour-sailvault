# SailVault M43 — provider privacy and request-state review

## Scope

M42 hardened the transport boundary for every read-only provider request. M43 keeps those HTTPS, redirect, timeout and response-size controls and removes unnecessary per-request state that could otherwise link provider calls or allow stale local HTTP state to influence a refresh.

SailVault still queries public blockchain/provider APIs. Providers can observe the public addresses and network metadata involved in those requests. M43 reduces avoidable application-layer state; it does not provide anonymity or conceal public addresses from the selected provider.

## Stateless request policy

Every provider request passes through `SailVaultNetwork::hardenRequest()` before dispatch. M43 extends that shared policy so all current provider calls have the same behavior:

- cookie loading is `Manual`, so Qt's cookie jar is not consulted for the request;
- cookie saving is `Manual`, so provider `Set-Cookie` state is not retained for later requests;
- cached Basic/Digest HTTP authorization reuse is `Manual`;
- local HTTP cache loading is forced to `AlwaysNetwork`;
- provider responses are marked not to be saved to Qt's network cache;
- `Cookie`, `Authorization`, `Referer` and `Origin` headers are removed if a future call site accidentally supplies them before hardening;
- `Cache-Control: no-store, no-cache` is sent with provider requests;
- automatic redirects remain disabled and the redirect budget is explicitly zero.

These controls are request-local. They do not disable platform proxying, DNS, TLS session reuse or ordinary TCP/TLS connection reuse, all of which remain managed by the Sailfish/Qt networking stack.

## Local diagnostics

`AppTools::networkSecurityDiagnostics()` remains completely local. M43 seeds a synthetic `QNetworkRequest` with cookies, authorization, cache state, referrer/origin and redirect allowance, applies the shared hardener, then verifies that all of those state channels are closed.

Developer diagnostics exposes the resulting checks for cookies, HTTP authentication reuse, cache handling and referrer/origin context. Release readiness continues to gate on the combined network-boundary result.

## Existing M42 controls retained

M43 does not loosen the proven M42 network boundary:

- provider endpoints must be valid HTTPS URLs without embedded credentials;
- automatic redirect following is blocked and redirect metadata is rejected;
- ordinary provider requests retain the 15-second timeout;
- health/fee probes retain the 12-second timeout;
- individual provider replies remain capped at 16 MiB before parsing;
- superseded requests are cancelled and generation checks prevent stale replies replacing newer state.

## Real-funds gate

M43 remains a read-only milestone. It does not add transaction construction, confirmation, signing, broadcast, nonce handling, fee selection or chain-specific transaction validation. Passing these request-privacy checks therefore does not authorize real-funds use.
