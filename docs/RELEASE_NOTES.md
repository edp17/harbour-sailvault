# SailVault 0.52.0-1 — development signing boundary

M52 begins SailVault's signing phase without enabling a general sending wallet. The proven M49 Ethereum unsigned constructor can now be followed by an explicit **Development sign / verify** action, but only for SailVault's published BIP39 test wallet and only while production signing and broadcasting remain disabled.

Before secret access, M52 revalidates the complete M48 intent and reconstructs the M49 EIP-1559 signing snapshot in C++. The payload, Wallet Core Keccak-256 hash and SHA-256 construction fingerprint must all agree with the reviewed destination/amount and current nonce/fee fields. The construction must also be no more than two minutes old.

`WalletVault` gains a private C++-only callback used exclusively by the development signer. It requires an already-loaded wallet session, reopens the DeviceLockRelock Sailfish Secrets collection, re-derives ETH/BTC/SOL and refuses the operation unless all three addresses still match the published test vectors. Recovery bytes remain inside `WalletVault`; QML receives no mnemonic or private-key data.

Inside the callback, `DevelopmentSigningService` derives the Ethereum key through Wallet Core, signs the exact 32-byte M49 hash with recoverable secp256k1, and immediately verifies the 65-byte signature against the derived public key. `TWPrivateKeyData` is never called.

M52 deliberately does not export the raw signature. Instead it hashes the verified signature together with the intent fingerprint, construction fingerprint, signing hash and signer address into a SHA-256 proof fingerprint, then wipes the temporary signature buffer. SailVault therefore proves the live Secrets → Wallet Core signing path without assembling a signed EIP-1559 transaction that could be broadcast.

Developer diagnostics add a network-free and Secrets-free signing-boundary self-test using only the published development mnemonic. Release readiness now requires that test in addition to all M41-M51 gates.

M49 Ethereum construction, M50 Bitcoin transaction/PSBT construction and M51r2 Solana message construction remain otherwise unchanged. Bitcoin/Solana signing, production user confirmation, dedicated secure Bitcoin change derivation, signed-transaction assembly and every broadcast API remain unavailable. Do not use the development wallet with real funds.
