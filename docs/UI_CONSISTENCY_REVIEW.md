# SailVault M46 — Sailfish UI consistency review

M46 closes the deferred typography/UI-consistency item before transaction construction begins.

## Semantic typography

- Huge: page hero balance/value/amount only.
- Large: primary value inside a row/card.
- Medium: row titles and important status headlines.
- Small: ordinary explanatory/body text and secondary values.
- ExtraSmall: addresses, identifiers, timestamps, provider endpoints and freshness metadata.
- Runtime QML must use Sailfish `Theme.fontSize*`; literal pixel/point font sizes are rejected by preflight.

## Interaction cleanup

Developer diagnostics now exposes one pulley action to rerun all local checks. This follows the existing Sailfish-native pattern already used by Release readiness, Provider health, Network fees, Activity and Tokens, and removes two redundant page buttons.

Receive keeps a single PageHeader title; its former “Offline address QR” subheading duplicated the same information and is removed.

## Security boundary

This milestone does not change Wallet Core, WalletVault/Sailfish Secrets, session security, network request hardening or provider parsing. It is intentionally a UI-only delta plus build/release documentation and preflight assertions.
