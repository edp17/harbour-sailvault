# SailVault M44 — provider response validation review

## Scope

M44 treats provider payloads as untrusted input even after HTTPS transport succeeds. It adds protocol-level checks before existing JSON parsing and chain-specific field validation. This is a read-only hardening milestone; it does not add transaction construction, signing or broadcasting.

## Media-type boundary

- JSON providers must return `application/json`, `application/json-rpc`, or another media type ending in `+json`; parameters such as `charset=utf-8` are accepted.
- The Bitcoin Esplora `block-height/0` identity check is intentionally the only plain-text provider response and must return `text/plain`.
- Missing or mismatched response media types are rejected before JSON/text content is trusted.
- HTTP/network failures still take precedence, so an HTML error page for a non-2xx response is reported as the transport/HTTP failure rather than as a misleading media-type failure.

## JSON-RPC boundary

Every SailVault JSON-RPC consumer now requires:

1. `jsonrpc` exactly equal to `2.0`;
2. a response `id` that exactly matches the request ID without numeric/string coercion;
3. exactly one of `result` or `error`;
4. `error`, when present, to be a JSON object.

The validator covers Ethereum and Solana balance RPCs, Solana activity/current-page RPCs, Ethereum and Solana fee RPCs, Ethereum/Solana Provider-health identity RPCs, SPL/Token-2022 discovery and Solana transaction details.

## Local regression gate

`AppTools::networkSecurityDiagnostics()` exercises accepted/rejected JSON and text media types plus valid numeric/string JSON-RPC IDs and wrong-ID/version/ambiguous-envelope negative cases. No provider request is made by these tests. The combined result remains part of Release readiness.

## Residual trust

M44 validates protocol framing, not the truth of provider content. Provider health continues to verify Ethereum mainnet chain ID, Bitcoin mainnet genesis and Solana mainnet genesis. A malicious provider that deliberately returns internally consistent but false chain data is still outside this milestone's protection. TLS certificate validation remains the platform/Qt stack's responsibility.
