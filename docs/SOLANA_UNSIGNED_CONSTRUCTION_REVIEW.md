# SailVault M51 — Solana unsigned construction review

> **M51r2 correction:** M51r1 accidentally used a mistyped mainnet genesis constant. M51r2 uses the actual Solana mainnet `getGenesisHash` value and retains the same fail-closed network identity check.

M51 completes SailVault's first native-chain unsigned construction layer by adding native SOL transfers after the proven M48 reviewed-intent step. It remains a development-only, no-secrets, no-signing milestone.

## Live boundary

- revalidates the complete M48 Solana intent and SHA-256 fingerprint before network access;
- validates source and destination as 32-byte Base58 public keys through Wallet Core;
- requires the configured RPC to return the exact Solana mainnet genesis hash `5eykt4UsFv8P8NJdTREpY1vzqKqZKvdpKuc147dw2N9d`;
- obtains a finalized recent blockhash and validates it as exactly 32 decoded bytes;
- converts SOL to lamports using exact decimal/integer arithmetic with at most 9 fractional digits;
- manually serializes only the public Solana `SigningInput` protobuf fields needed for a native transfer;
- calls Wallet Core `TWTransactionCompilerPreImageHashes` with no private key to obtain the exact legacy transaction signing message;
- requires Wallet Core to return exactly the reviewed source account as signer;
- requests `getFeeForMessage` for the exact Base64 signing message and rejects absent, fractional, negative or implausibly large fee quotes;
- applies a defensive 0.1 SOL maximum fee-quote ceiling;
- creates an inspection-only transaction template containing one 64-byte zero signature followed by the Wallet Core message;
- binds intent + exact mainnet identity + blockhash validity horizon + fee quote + message with a SHA-256 construction fingerprint.

## Privacy note

The initial `getGenesisHash` and `getLatestBlockhash` calls contain no wallet information. The later `getFeeForMessage` request sends the complete public Solana signing message to the configured RPC, so the provider can observe the public source, destination and transfer amount. M51 displays this before the user starts construction.

## Wallet Core external-signing path

Wallet Core 4.8.4's Solana transaction compiler supports producing `PreSigningOutput.data` from sender, recipient, amount and recent blockhash without a private key. M51 intentionally stops at that pre-signing message. The local diagnostic pins the same published Wallet Core transfer vector used upstream and requires byte-for-byte equality.

## Explicitly absent

- WalletVault/Sailfish Secrets access;
- mnemonic, HD-wallet or private-key APIs;
- signature creation or insertion;
- `TWAnySigner` signing;
- transaction broadcast / `sendTransaction`;
- SPL/Token-2022 transfer construction;
- priority-fee controls;
- real-funds enablement.

M51 therefore completes unsigned native ETH/BTC/SOL construction, but does not make SailVault a sending wallet yet.
