# SailVault — Milestone 43

**Version:** `0.43.0-1`  
**Build:** stateless provider privacy  
**Wallet Core baseline:** `4.8.3`

M42 is proven on Xperia 10 III/aarch64 with every Release readiness and Developer diagnostics check passing. M43 keeps the M42 HTTPS/redirect/timeout/response-size boundary unchanged and makes every read-only provider request explicitly stateless at the Qt request layer.

## M43 goals

- preserve the proven M41 Wallet Core 4.8.3 security/ABI baseline and M42 network boundary;
- prevent provider cookies from being loaded from or saved to Qt's cookie jar;
- prevent cached Basic/Digest HTTP credentials from being reused for provider requests;
- force provider refreshes to bypass the local HTTP cache and prevent responses being saved there;
- remove accidentally supplied `Cookie`, `Authorization`, `Referer` and `Origin` headers before provider dispatch;
- send `Cache-Control: no-store, no-cache` on provider calls;
- keep automatic redirects blocked and explicitly set the redirect budget to zero;
- extend the completely local network diagnostic and Release readiness gate to cover request-state isolation;
- keep transaction construction, signing and broadcasting unavailable.

The development wallet continues to accept only the published BIP39 test mnemonic:

`abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon about`

Never enter a personal recovery phrase or use real funds with this development build.

## Build

Normal Sailfish SDK build:

```sh
cd ~/mer/android/droid
rpm/dhd/helpers/build_packages.sh -o -b hybris/mw/harbour-sailvault -s rpm/harbour-sailvault.spec
```

No manual Wallet Core bootstrap should be required. M43 deliberately retains the proven M41 Wallet Core preparation marker and does not alter the M40/M41 compatibility recipe.

On a build failure, use the **first real compiler/linker/Cargo error**; later make/RPM failures are normally consequences.

## Provider privacy boundary

SailVault's public providers do not require browser-style state. M43 therefore makes each request independent of cookies, cached HTTP credentials, local HTTP cache contents and referrer/origin context. This reduces avoidable linkability between provider calls and prevents stale local HTTP state from influencing a user-requested refresh.

This does not make provider requests anonymous. The selected provider can still see network metadata and any public wallet address or transaction identifier required to answer the request. Offline mode remains the only mode that performs no provider request.

## Proven controls retained

M42's HTTPS-only endpoint policy, no-credential URLs, blocked redirects, 16 MiB response ceiling, 15/12-second request timeouts, cancellation and stale-generation protection remain active. M41's BIP-39, BTC/ETH/SOL derivation/address, derivation-boundary, secp256k1 negative and CryptoBox bridge diagnostics remain unchanged.

## Project

Repository: `https://github.com/edp17/harbour-sailvault`  
License: BSD 3-Clause
