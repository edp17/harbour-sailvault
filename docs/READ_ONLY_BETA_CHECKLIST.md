# SailVault read-only beta 1 device checklist

This checklist is for the M39 / 0.39.0-1 read-only beta 1 release. Never use a
real recovery phrase or real funds while Wallet Core 4.0.27 is the compatibility
baseline.

## A. Completely fresh installation

- Use a device/profile on which SailVault has never been installed.
- Install 0.39.0-1 and launch from the application icon before using a terminal.
- Open Settings → Release readiness; automatic device checks should pass.
- Run **Run automatic checks again** repeatedly; the result must remain passed
  and must not prompt for wallet authentication.
- Open Developer diagnostics; the INI round-trip must pass and launch count must
  be non-zero.
- Change at least one non-sensitive preference, close SailVault completely,
  reopen it and verify the setting persists.
- Reboot the device and verify the same preference still persists.

## B. M38 RC2 → M39 upgrade

- Install 0.39.0-1 over the proven M38/0.38.0-1 installation.
- Confirm normal startup without a first-run/reset prompt.
- Confirm the development wallet remains present when it existed before upgrade.
- Confirm Watch-only addresses and Address book entries remain present.
- Confirm network endpoints, fiat, Offline mode, session-lock policy and hidden
  token choices remain unchanged.
- Confirm cached balances/prices and local portfolio history remain available.
- Confirm Settings → Release readiness passes before and after **Run automatic
  checks again**.
- Confirm About/Cover identify `0.39.0-1`, Milestone 39 and `read-only beta 1`.

## C. Development wallet

- Create/restore only the published BIP39 development test vector.
- Verify that any other phrase is refused before Sailfish Secrets is written.
- Confirm deterministic ETH/BTC/SOL addresses.
- Lock/unlock and verify session locking behaviour.
- Verify Backup shows only the published development phrase.
- Verify Delete development wallet removes only the Secrets-backed development wallet.

## D. Portfolio and adverse-network behaviour

- Refresh BTC/ETH/SOL balances and fiat prices.
- Confirm last-verification timestamps update independently.
- Break one provider and verify last-known values remain visible with failure state.
- Enable Offline mode and confirm provider requests are blocked while cached
  portfolio/history/Address book/QR remain usable.
- Disable Offline mode and confirm refresh resumes.

## E. Public addresses and recent activity

- Verify development, Watch-only and Address-book public-address pages are consistent.
- Verify the 492×492 offline QR is readable and contains only the plain public address.
- Verify public-balance refresh updates the exact-address cache.
- Verify Address book reuses cached balance/fiat values without network access.
- Enter Recent activity from ETH/BTC/SOL; the originating chain should be preselected.
- All configured wallet addresses should still load in the background context.
- Switch chain filters and verify already-loaded activity appears.
- Load older history where available; no-op pagination must be disabled.
- Open transaction detail pages for each supported chain.

## F. Tokens, history and privacy

- Verify ERC-20, SPL and Token-2022 holdings.
- Verify hidden-token persistence.
- Verify local portfolio-history snapshots and analytics series/windows.
- Confirm history pages make no provider request by themselves.
- Review Settings → Privacy & local data.
- Clear cached public data and confirm Watch-only, Address book, preferences and
  Sailfish Secrets remain intact.

## G. Release/security gate

- Confirm Release readiness states construction/signing/broadcasting are unavailable.
- Confirm About and Cover identify the same package version/milestone/build label.
- Confirm no UI offers transaction construction, signing or broadcasting.
- Do not proceed to real-funds functionality until Wallet Core is upgraded or
  its security baseline has been carefully reviewed/backported.
