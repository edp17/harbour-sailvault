# SailVault M52 — development signing boundary review

M52 is the first SailVault milestone that deliberately crosses from public unsigned transaction data into Sailfish Secrets-backed signing. It remains **development-only** and is intentionally incapable of assembling or broadcasting a signed transaction.

## Threat boundary

The M52 signer is not a generic signing API. It accepts only an already-constructed M49 Ethereum transaction and only when all of the following hold:

1. the M48 reviewed intent validates and its SHA-256 fingerprint still matches;
2. the M49 C++ signing snapshot reconstructs byte-for-byte from the reviewed destination, exact amount, nonce, fee fields and 21,000-gas EOA policy;
3. the Wallet Core Keccak-256 signing hash and M49 construction fingerprint both match that reconstructed payload;
4. the construction is no more than two minutes old;
5. the QML session reports the exact published ETH/BTC/SOL development addresses;
6. `WalletVault` reopens Sailfish Secrets and independently re-derives the same three published addresses from the stored recovery material.

Only then does `WalletVault` provide a transient `TWHDWallet*` to the C++ signer callback. The mnemonic itself never leaves `WalletVault` and no secret-returning method is QML-invokable.

## Signing behavior

`DevelopmentSigningService` derives the Ethereum key with `TWHDWalletGetKeyForCoin`, signs the exact 32-byte M49 signing hash with `TWPrivateKeySign(..., TWCurveSECP256k1)`, requires a 65-byte recoverable signature, obtains the corresponding public key and immediately verifies the signature with `TWPublicKeyVerify`.

The service never calls `TWPrivateKeyData` and never stores or returns private-key bytes.

The raw signature is also deliberately **not exported**. SailVault computes a SHA-256 proof fingerprint binding:

- M48 intent fingerprint;
- M49 construction fingerprint;
- Wallet Core signing hash;
- published development signer address;
- generated signature bytes.

The raw signature buffer is then wiped. QML receives only public metadata: signer address, signing hash, construction fingerprint, signature size, proof fingerprint and timestamp.

## Why no signed transaction yet

A verified signature is sufficient to prove the secure key path works, but M52 deliberately does not insert `r/s/yParity` into the EIP-1559 payload. That keeps SailVault from producing a broadcastable Ethereum transaction while the signing policy is still being reviewed.

Bitcoin and Solana signing remain disconnected. Broadcasting remains absent for every chain.

## Automatic diagnostics

The automatic M52 signing self-test is network-free and Sailfish-Secrets-free. It uses only the published BIP39 test mnemonic already embedded by the development wallet, verifies the expected ETH/BTC/SOL identities, performs secp256k1 sign/verify, rejects a mutated digest and checks that changing signature bytes changes the SHA-256 proof fingerprint.

The live device test is separate and is the proof that Sailfish Secrets → WalletVault → transient Ethereum key handle → signer works on-device.
