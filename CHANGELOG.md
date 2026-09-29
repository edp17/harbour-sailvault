# Changelog

## 0.43.0-1 — Milestone 43 — stateless provider privacy

- Preserve the proven M41 Wallet Core 4.8.3 security/ABI baseline and M42 network boundary.
- Make every provider request explicitly stateless at the Qt request layer.
- Disable provider cookie loading/saving and cached Basic/Digest HTTP credential reuse.
- Force provider refreshes to bypass local HTTP cache reads and prevent response cache storage.
- Remove Cookie, Authorization, Referer and Origin headers before provider dispatch.
- Send `Cache-Control: no-store, no-cache` and set the redirect budget explicitly to zero.
- Extend local Developer diagnostics and Release readiness with request-state isolation checks.
- Add the M43 provider-privacy review and device checklist.
- Keep transaction construction, user signing and broadcasting unavailable.

## 0.42.0-1 — Milestone 42 — network boundary hardening

- Preserve the proven M41 Wallet Core 4.8.3 build, security/ABI diagnostics and read-only architecture.
- Centralize provider reply policy in `networkrequestutils.h`.
- Require HTTPS provider URLs without embedded credentials and add checked fallback when ordinary provider services read endpoint settings.
- Explicitly disable redirect following before every provider dispatch and reject redirect metadata before redirected data is accepted.
- Bound each provider response to 16 MiB using a bounded reply buffer, response metadata, transfer progress and accumulated reply bytes.
- Give portfolio, activity, token, transaction-detail, price, provider-health and fee-estimate paths consistent policy-aware network errors.
- Add a local endpoint-policy self-test to Developer diagnostics and Release readiness.
- Add the M42 network-boundary review and device checklist.
- Keep transaction construction, user signing and broadcasting unavailable.

## 0.41.0-1 — Milestone 41 — Wallet Core 4.8.3 security/ABI validation

- Promote the M40r9-proven Wallet Core 4.8.3 build to SailVault's current Sailfish compatibility baseline.
- Add compile-time ABI guards for canonical Wallet Core derivation IDs and BTC/ETH/SOL coin types.
- Add BIP-39 positive/negative validation and malformed/wrong-chain address rejection checks.
- Add BTC/ETH/SOL private-key → public-key → address round-trip diagnostics without exposing private bytes.
- Verify out-of-range derivation indices return no key.
- Strengthen secp256k1 diagnostics with mutated-digest and short-signature rejection.
- Runtime-test M40r9's Rust-generated CryptoBox bridge, including small-order public-key rejection and encrypt/decrypt.
- Add targeted Wallet Core security-review and M41 device-checklist documentation.
- Keep the application read-only; no transaction construction, user signing or broadcasting UI is introduced.

## 0.40.0-9 — Milestone 40 revision 9 — complete Rust/C++ binding generation

- Restore Wallet Core 4.8.3's upstream `codegen-v2 ... cpp` stage after the Rust build.
- Pin `CARGO_WORKSPACE_DIR` so `#[tw_ffi]` manifests are generated under `rust/bindings`.
- Add Cargo 1.75 compatibility for the separate codegen-v2 lockfile.
- Validate generated public headers/sources before CMake, including CryptoBox key wrappers.
- Fix the final missing-generated-header blocker; M40r9 subsequently built, installed and ran successfully on Xperia 10 III/aarch64.
- Accept 4.8.3 as the proven Sailfish compatibility baseline, while keeping real-funds transaction functionality blocked.

## 0.40.0-8 — Milestone 40 revision 8 — GNU chrono qualification

- Fix Wallet Core 4.8.3 `MultiversX/TransactionFactoryConfig.cpp` for GCC 10 by qualifying `std::chrono::duration_cast`.
- Apply the same qualification proactively to the identical unqualified usage in `Tron/Signer.cpp`.
- Add preflight guards for both production source patches.
- Advance the Wallet Core preparation marker to `4.8.3-sailvault-m40r9`.

## 0.40.0-7 — Milestone 40 revision 7 — derivation identifier normalization

- Fix registry derivation-name conversion so snake_case identifiers such as `stable_account` map to canonical C ABI symbols such as `TWDerivationSmartChainStableAccount`.
- Use delimiter-aware CamelCase conversion for underscores, hyphens, dots and whitespace rather than only uppercasing the first character.
- Retain the canonical Wallet Core 4.8.3 derivation ABI table and generation-time membership guard from revision 6.
- Advance the preparation marker to `4.8.3-sailvault-m40r7`.

## 0.40.0-6 — Milestone 40 revision 6 — canonical derivation ABI generation

- Fix Wallet Core 4.8.3 generated `TWDerivation.h` so its numeric values exactly match the upstream Rust `TWDerivation` ABI.
- Restore `TWDerivationStratisSegwit = 7` and `TWDerivationSmartChainStableAccount = 11`, including all canonical 4.8.3 derivation IDs.
- Preserve the upstream append-only ordering (`BitcoinTaproot = 8` rather than registry-order numbering).
- Add a generation-time guard: named derivations referenced by `CoinInfoData.cpp` must exist in the canonical ABI table.
- Advance the preparation marker to `4.8.3-sailvault-m40r6`.

## 0.40.0-5 — Milestone 40 revision 5 — standard exception include hardening

- Fix the first real C++ compiler error from the complete M40r4 Sailfish build log: `CellSlice.cpp` uses `std::runtime_error` without directly including `<stdexcept>`.
- Add explicit `<stdexcept>` includes to the six Wallet Core 4.8.3 production translation units identified as directly using `std::runtime_error` without that standard header.
- Preserve the proven M40r2 Rust 1.75, M40r3 C++20 and M40r4 fee-calculator compatibility fixes.
- Advance the preparation marker to `4.8.3-sailvault-m40r5` so the vendor tree is automatically regenerated.
- No SailVault application behaviour change; transaction construction, signing and broadcasting remain unavailable.

## 0.40.0-4 — Milestone 40 revision 4 — Bitcoin fee-calculator C++17 fix

- Fix the first real C++ compiler error from the complete M40r3 Sailfish build log.
- Convert all seven static Bitcoin fee-calculator singleton objects from `constexpr` to `const`, including the previously missed `Zip0317FeeCalculator`.
- Add a preflight guard requiring the Zip0317 compatibility rewrite.
- Advance the preparation marker to `4.8.3-sailvault-m40r4` so the vendor tree is automatically regenerated.
- Keep the Rust 1.75/Cargo and broader C++20 compatibility patches from M40r2/r3 unchanged.
- No SailVault application behaviour change; transaction construction, signing and broadcasting remain unavailable.

## 0.40.0-3 — Milestone 40 revision 3 — C++17 compatibility fix

- Replace Wallet Core 4.8.3's C++20 `std::countl_zero()` use in the always-compiled Everscale/CommonTON source with the equivalent GCC/Clang `__builtin_clzll()` operation.
- Remove the accompanying C++20 `<bit>` dependency from that translation unit.
- Keep the complete M40r2 Rust 1.75 compatibility layer unchanged.
- Advance the preparation marker to `4.8.3-sailvault-m40r3` so the vendor tree is automatically regenerated and repatched.
- No SailVault application/runtime behaviour change.

## 0.40.0-2 — Milestone 40 revision 2 — Rust 1.75 compatibility fix

- Replace Wallet Core 4.8.3 uses of the post-Rust-1.75 integer `is_multiple_of()` API with equivalent modulo expressions during source preparation.
- Covers the active SailVault `tw_encoding` and `tw_evm` paths plus the currently disabled TON SDK occurrences.
- Advance the preparation marker to `4.8.3-sailvault-m40r2` so an M40r1 vendor tree is automatically rebuilt and repatched.
- No SailVault application/runtime behaviour change.

## 0.40.0-1 — Milestone 40 — Wallet Core 4.8.3 migration candidate

- Move the vendored Wallet Core target from 4.0.27 to maintained release 4.8.3.
- Preserve the proven Sailfish Rust/Cargo 1.75 + Scratchbox2 aarch64 build path.
- Add an audited C++17 compatibility layer for Wallet Core's small C++20 production surface.
- Convert upstream Cargo.lock v4 metadata to Cargo-1.75-compatible v3 semantics.
- Limit the Rust coin registry used by SailVault to Bitcoin, Ethereum/EVM and Solana, avoiding unrelated newer-chain dependencies that exceed the Sailfish Rust MSRV.
- Update protobuf generation to Wallet Core's pinned protobuf 3.20.3 and registry generation for nativeTokenName.
- Keep the application read-only; construction, signing and broadcasting remain unavailable.
- Require the existing ETH/BTC/SOL deterministic self-test to pass on-device before 4.8.3 is accepted as the new SailVault baseline.

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
