# Changelog

## 0.39.0-1 — Milestone 39 — read-only beta 1

- Promote the proven M38 RC2 source to the first public read-only beta release.
- Keep wallet/provider runtime behaviour unchanged from M38.
- Finalize centralized version/build identity as `0.39.0-1 · read-only beta 1`.
- Finalize packaged release notes and the device regression checklist.
- Add explicit completely fresh-install and M38 → M39 upgrade test paths.
- Extend preflight so current release-facing documentation cannot retain stale
  RC/candidate wording.
- Preserve RPM `%check` release preflight and the top-level `harbour-sailvault/`
  ZIP layout.
- Preserve Wallet Core 4.0.27 / `sailvault-m1r16`; transaction construction,
  signing and broadcasting remain unavailable.

## 0.38.0-1 — Milestone 38 — read-only beta RC2

- Complete the release-packaging and regression-polish pass for the read-only beta.
- Replace remaining hard-coded RC labels in Cover/About/Release readiness with centralized build metadata.
- Replace the stale M24 Rust-build banner with RPM-spec-derived version/release identity.
- Run `scripts/preflight.sh` automatically from the RPM `%check` stage.
- Extend preflight checks to version/release/milestone identity, packaged release documentation, development-test-mnemonic enforcement and RC UI metadata.
- Add packaged RC2 release notes and clean the device regression checklist.
- Preserve the top-level `harbour-sailvault/` milestone ZIP layout.
- Preserve Wallet Core 4.0.27 / `sailvault-m1r16`; transaction construction, signing and broadcasting remain unavailable.

## 0.37.0-2 — Milestone 37 RC1 revision 2

- Fix **Run automatic checks again** falsely changing Release readiness to attention.
- Readiness now verifies Sailfish Secrets backend availability without probing, unlocking or authenticating the DeviceLockRelock wallet collection.
- Keep secure-wallet protection state informational only; it is not a release-readiness pass/fail gate.
- Add an explicit list of whichever automatic check failed when attention is genuinely required.
- Restore the release ZIP layout with a top-level `harbour-sailvault/` directory.

## 0.37.0-1 — Milestone 37 — read-only beta RC1

- Fix manual Release readiness reruns falsely failing when the Secrets wallet
  collection has correctly relocked under DeviceLockRelock.
- Distinguish a non-interactively protected secure wallet from a genuine Secrets
  backend/storage error.
- Keep readiness checks non-interactive: no automatic wallet unlock or recovery
  material access is introduced.
- Report healthy relocked storage as `Device-lock protected`.
- Remove stale user-visible M3 wording from secure-wallet status/error text.
- Add the repeated/manual readiness check to the RC regression checklist and
  source-package preflight.
- Preserve Wallet Core 4.0.27 / sailvault-m1r16 and the read-only architecture.

## 0.36.0-1 — Milestone 36

- Prepare the proven read-only feature set for release-candidate regression.
- Centralize runtime version, package release, milestone, build label and Wallet
  Core baseline metadata.
- Add Settings → Release readiness with local automatic beta checks.
- Record first/last startup, launch count and last build identity in INI schema v4.
- Show startup/version metadata in Developer diagnostics.
- Strengthen source-package preflight with version, packaging, Sailjail, HTTPS,
  QML-page-reference and Wallet Core marker checks.
- Install release documentation in the RPM package.
- Preserve Wallet Core 4.0.27 / sailvault-m1r16 and the read-only architecture.

## 0.35.0-1 — Milestone 35

- Add Privacy & local data audit/status page.
- Add selective cached-public-data and quarantine cleanup.
- Document the explicit read-only beta security gate in-app.
