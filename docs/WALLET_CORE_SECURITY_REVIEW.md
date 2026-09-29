# SailVault M41 — Wallet Core 4.8.3 security review

## Scope

This is a targeted downstream review for SailVault's current BTC/ETH/SOL and key-management surface. It is not a claim that every Wallet Core chain or every upstream change has been independently audited. SailVault remains read-only and has no production transaction-signing/broadcast UI.

The previous Sailfish compatibility baseline was Wallet Core 4.0.27. M40r9 migrated successfully to 4.8.3 on Xperia 10 III/aarch64. M41 checks that security-relevant behavior from the maintained line is actually reachable through SailVault's compatibility layer.

## Relevant upstream hardening retained in 4.8.3

The upstream Wallet Core release notes between the old baseline and 4.8.3 include, among others:

- 4.4.0: public-key signature-length validation;
- 4.6.0: native random-source failure becomes fatal rather than silently continuing;
- 4.6.3: mnemonic memory clearing and Bitcoin invalid-OutPoint rejection;
- 4.6.7: ECDSA message-length validation;
- 4.6.8: CryptoBox small-order public-key rejection;
- 4.6.9: Base58/address/NULL-character decoding hardening;
- 4.6.12: Solana account-count overflow rejection;
- 4.6.14: broader signer/input validation hardening;
- 4.7.0: derivation indices outside the allowed range are rejected;
- 4.8.0: HD-wallet key/address retrieval APIs return nullable values on derivation failure instead of invalid/default objects.

Upstream release source: https://github.com/trustwallet/wallet-core/releases

## M41 device checks mapped to that surface

- BIP-39: accept the public test mnemonic and reject an invalid-word phrase.
- Address layer: accept deterministic BTC/ETH/SOL addresses; reject malformed and selected wrong-chain inputs.
- Derivation bounds: request an out-of-range hardened Ethereum derivation index and require a null key result.
- ECDSA: require a valid 65-byte secp256k1 signature to verify; require the same signature to fail for a mutated digest; require a one-byte signature to fail safely.
- ABI: compile-time assertions pin Wallet Core's canonical derivation IDs and BTC/ETH/SOL coin type IDs.
- Rust/C++ bridge: exercise generated CryptoBox wrappers, reject a zero/small-order public key, preserve fixed secret-key bytes through import/export, and perform an in-memory encrypt/decrypt round-trip.

No diagnostic result contains mnemonic, seed or private-key bytes. CryptoBox test keys are synthetic local diagnostics and are never exposed to QML.

## 4.8.4 disposition

Wallet Core 4.8.4 was released on 2026-09-24. Its published change is a Robinhood Chain block-explorer switch. That does not affect SailVault's BTC/ETH/SOL cryptographic surface, so M41 retains the M40r9-proven 4.8.3 baseline. A future Wallet Core upgrade should repeat the same compatibility and security gates.

## Real-funds gate

Passing M41 means the maintained Wallet Core baseline and selected security behaviors are proven on Sailfish. It does **not** by itself authorize real-funds use. Transaction construction, user confirmation, signing, broadcast, nonce/fee handling and chain-specific transaction validation must be introduced and reviewed separately.
