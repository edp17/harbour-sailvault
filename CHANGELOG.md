# Changelog

## 0.52.0-1 — Milestone 52 — development signing boundary

- Add the first Sailfish Secrets → Wallet Core transaction signing boundary while keeping production signing and all broadcasting disabled.
- Restrict live signing to the published BIP39 development wallet with independent ETH/BTC/SOL address gates in both WalletVault and the signing service.
- Add a private C++-only WalletVault callback that reopens Sailfish Secrets, revalidates the stored wallet and exposes only a transient `TWHDWallet*`; mnemonic bytes never enter QML.
- Add C++ revalidation of the complete M49 signing snapshot: reviewed intent, EIP-1559 payload reconstruction, Wallet Core Keccak-256 hash and construction fingerprint must all match.
- Enforce a two-minute maximum age for the M49 construction before the development sign/verify action can run.
- Derive the Ethereum key inside Wallet Core, create a 65-byte recoverable secp256k1 signature and immediately verify it against the derived public key.
- Never call `TWPrivateKeyData`; no private-key bytes are exported.
- Keep the raw signature out of QML: hash it into a SHA-256 proof fingerprint and wipe the temporary signature buffer after verification.
- Add Developer diagnostics and Release-readiness gating for published-wallet identity, sign/verify, mutated-digest rejection and non-exporting signature-proof binding.
- Preserve the proven M49 Ethereum, M50 Bitcoin and M51r2 Solana unsigned constructors; Bitcoin/Solana signing remain disconnected.
- Keep signed-transaction assembly, production signing policy/user confirmation and all broadcast APIs unavailable.

## 0.51.0-1 — Milestone 51 — Solana unsigned construction

### M51r2 device-test correction

- Correct the Solana mainnet genesis hash constant from the invalid M51r1 value ending `...dw2dM` to the actual mainnet `getGenesisHash` value ending `...dw2N9d`.
- Update the deterministic construction fingerprint vector and all preflight/review/device-check documentation to bind the corrected network identity.
- Keep the Wallet Core pre-signing message, Ethereum/Bitcoin constructors, Sailfish Secrets boundary and signing/broadcast-disabled architecture unchanged.

- Complete unsigned native-chain construction by adding reviewed native-SOL transfer construction while signing and broadcasting remain disabled.
- Revalidate the complete M48 intent fingerprint before any Solana construction network request.
- Require source, destination and recent blockhash to decode to exactly 32 bytes through Wallet Core Base58.
- Require the configured RPC to return the exact Solana mainnet genesis hash before trusting construction data.
- Fetch a finalized recent blockhash and preserve its last-valid-block-height horizon in the construction snapshot.
- Convert SOL to lamports using exact integer/string arithmetic with no floating-point transaction amount path.
- Use Wallet Core `TWTransactionCompilerPreImageHashes` without a private key to construct the canonical native-SOL transfer signing message.
- Require Wallet Core's returned signer to exactly match the reviewed source address.
- Query `getFeeForMessage` for the exact public signing message with a defensive 0.1 SOL fee ceiling.
- Expose Base64/hex signing-message data plus an explicitly non-broadcastable zero-signature transaction template.
- Add a SHA-256 construction fingerprint binding reviewed intent, mainnet identity, blockhash validity horizon, fee quote and message.
- Add deterministic network-free diagnostics pinned to Wallet Core's published Solana external-signing transfer vector and independent template/fingerprint constants.
- Preserve M49 Ethereum construction, M50 Bitcoin construction and the proven Wallet Core 4.8.4 / Sailfish Secrets / network-security boundary.
- Keep private-key access, signing, broadcasting, SPL/Token-2022 construction and real-funds use unavailable.

## 0.50.0-1 — Milestone 50 — Bitcoin unsigned construction

- Add reviewed Bitcoin unsigned transaction construction while keeping signing and broadcasting disabled.
- Revalidate the complete M48 intent fingerprint before network access and independently verify Bitcoin mainnet genesis.
- Query only the public BIP84 source address for confirmed UTXOs; destination and transfer amount remain local.
- Add exact BTC-to-satoshi conversion, deterministic largest-first UTXO selection and a conservative integer sat/vB fee-rate ceiling.
- Add conservative P2WPKH vbyte estimation, 546 sat dust floor, RBF sequence and bounded input/UTXO policy.
- Serialize a deterministic version-2 unsigned Bitcoin transaction and compute its SHA256d transaction ID through Wallet Core.
- Produce an incomplete PSBT v0 with witness UTXO + SIGHASH_ALL metadata, but no keys, signatures or finalization.
- Add a SHA-256 construction fingerprint binding reviewed intent, unsigned transaction and PSBT.
- Add network-free deterministic Bitcoin diagnostics using synthetic public UTXOs and published BIP84 addresses.
- Keep the live public development address unfunded; its expected zero-UTXO result is a valid safety-path test.
- Preserve M49 Ethereum construction and the proven Wallet Core 4.8.4 / Sailfish Secrets / provider-security boundary.
- Keep Solana construction, dedicated secure Bitcoin change derivation, signing and broadcasting unavailable.

## 0.49.0-1 — Milestone 49 — Ethereum unsigned construction

- Add the first chain-specific construction path while keeping signing and broadcasting disabled.
- Revalidate the complete M48 reviewed intent and SHA-256 fingerprint before any construction network request.
- Add explicit hardened Ethereum RPC retrieval for chain ID, pending nonce, latest EIP-1559 base fee/block gas limit, priority fee and destination code.
- Require Ethereum mainnet chain ID 1 and refuse contract/delegated-code recipients; M49 supports plain EOA native-ETH transfers only.
- Keep the transfer amount local to the device during construction and use the fixed 21,000 gas limit only after the EOA check.
- Add exact decimal ETH-to-wei conversion and deterministic EIP-1559 type-2 RLP signing-payload construction.
- Compute the signing hash through Wallet Core `TWHashKeccak256`; no signing API is called.
- Add a SHA-256 construction fingerprint binding the M48 intent fingerprint to the exact unsigned payload.
- Extend Developer diagnostics and Release readiness with deterministic quantity/wei/RLP/Keccak/binding self-tests.
- Preserve WalletVault/Sailfish Secrets, Wallet Core 4.8.4 compatibility scripts and the existing provider security boundary; keep Bitcoin/Solana construction unavailable.

## 0.48.0-1 — Milestone 48 — unsigned transaction intent review

- Begin the transaction phase without enabling chain transaction construction, signing or broadcasting.
- Add `UnsignedTransactionService`, a common ETH/BTC/SOL native transfer-intent model.
- Validate source/destination addresses with Wallet Core and enforce 18/8/9-decimal ETH/BTC/SOL amount precision with exact string parsing.
- Add advisory displayed-balance/funding warnings while explicitly leaving network-fee reservation to later construction milestones.
- Add a versioned immutable review snapshot with a SHA-256 fingerprint binding chain, network, source, destination, normalized amount and symbol.
- Add **Prepare transfer** to unlocked development-wallet chain pages only; Watch-only and Address-book public pages remain non-sending.
- Add chain-aware address-book destination shortcuts and human-readable unsigned review UI.
- Add local M48 diagnostics and Release-readiness gating for chain metadata, Wallet Core address checks, decimal parsing and fingerprint mutation.
- Preserve Wallet Core 4.8.4, WalletVault/Sailfish Secrets, storage/session security and M42-M44 provider policy unchanged.
- Keep nonce/UTXO/blockhash lookup, serialized transaction construction, user signing and broadcasting unavailable.

## 0.47.0-1 — Milestone 47 — storage lifecycle validation

- Advance the non-sensitive settings schema from v4 to v5 and add explicit profile lineage: fresh/existing/legacy-import origin, earliest tracked app version, and last upgrade source/time.
- Keep the install UUID internal; QML receives only whether an install identity is present.
- Refactor startup metadata handling so the same logic can be exercised against isolated temporary profiles.
- Add local lifecycle simulations for fresh initialization, upgrade preservation and legacy-known-key import/sanitisation.
- Preserve install identity, first-start metadata and representative user preferences across the simulated upgrade path.
- Gate Release readiness on real INI persistence, lifecycle simulation and coherent current-profile metadata.
- Extend Developer diagnostics with storage lifecycle status and profile lineage.
- Update the read-only beta regression documentation for the Wallet Core 4.8.4 baseline.
- Preserve Wallet Core 4.8.4, Sailfish Secrets/session security, M42-M44 provider policy and M46 UI behaviour.
- Keep transaction construction, user signing and broadcasting unavailable.

## 0.46.0-1 — Milestone 46 — Sailfish UI consistency

- Complete the deferred systematic typography and information-hierarchy pass using Sailfish `Theme.fontSize*` tiers only.
- Align stored-wallet and Watch-only chain rows: Medium chain titles, Large primary balances, Small fiat values and ExtraSmall address/freshness metadata.
- Reduce secondary Watch-only, transaction-summary and diagnostic headlines that previously competed with page headers or hero values.
- Remove the redundant Receive-page “Offline address QR” heading; the PageHeader remains the single page title.
- Replace the two Developer-diagnostics rerun buttons with one Sailfish-native pulley action that refreshes storage, network-boundary and Wallet Core diagnostics together.
- Extend preflight with a QML typography/UI contract that rejects literal font sizes and guards the consolidated diagnostics interaction.
- Preserve Wallet Core 4.8.4, Sailfish Secrets/session security and M42-M44 provider policy unchanged.
- Keep transaction construction, user signing and broadcasting unavailable.

## 0.45.0-1 — Milestone 45 — Wallet Core 4.8.4 validation

- Advance the Wallet Core candidate baseline from device-proven 4.8.3 to upstream 4.8.4.
- Pin source preparation to immutable upstream commit `d40d24a63d92619167903369308bf0e2f7eb3a59` rather than a mutable tag URL.
- Verify the 4.8.4-specific Robinhood registry marker before applying SailVault's compatibility layer.
- Preserve the complete M40/M41 Rust 1.75, GNU/C++17, protobuf 3.20.3, canonical derivation ABI and Rust/C++ codegen-v2 compatibility path.
- Keep the M41 cryptographic/ABI probe implementation unchanged so the upgraded baseline must pass the identical acceptance gate.
- Preserve M42-M44 provider transport/privacy/protocol controls and all existing read-only functionality.
- Keep transaction construction, user signing and broadcasting unavailable.

## 0.44.0-1 — Milestone 44 — provider response validation

- Add strict provider media-type validation before parsing: JSON APIs must return a JSON media type and the Bitcoin genesis scalar must return `text/plain`.
- Add a shared JSON-RPC 2.0 envelope validator requiring the exact request ID and exactly one of `result` or object-valued `error`.
- Apply JSON-RPC envelope validation to all ten RPC response paths, including numeric and string IDs.
- Extend local network diagnostics and Release readiness with provider-response policy checks.
- Preserve M42 HTTPS/redirect/response-size controls and M43 stateless cookie/auth/cache/context isolation.
- Preserve the proven Wallet Core 4.8.3 / M41 security-validation baseline unchanged; no transaction construction, signing or broadcasting is added.

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
