# SailVault M48 — unsigned transaction intent and review model

M48 begins the transaction phase without crossing the private-key or network boundary.

## Scope

`UnsignedTransactionService` models only the human transaction intent:

- chain/network identity;
- source public address;
- destination public address;
- native-asset amount;
- displayed public balance context;
- a SHA-256 fingerprint over the canonical review fields.

The service has no network manager, no settings persistence and no reference to `WalletVault` or Sailfish Secrets.

## Validation

Destination and source addresses are checked with `TWAnyAddressIsValid` for the selected Wallet Core coin type. Supported M48 native chains are:

- Ethereum mainnet — ETH, 18 decimal places;
- Bitcoin mainnet — BTC, 8 decimal places;
- Solana mainnet-beta — SOL, 9 decimal places.

Amounts use an exact decimal-string parser rather than floating point. The parser rejects signs, exponent notation, malformed decimals, zero values for review readiness and precision beyond the chain's native unit. Trailing fractional zeros are removed for the canonical reviewed amount.

The displayed balance is advisory in M48. An amount above the displayed balance can still be reviewed because no transaction can be constructed or sent; the review snapshot carries an explicit funding warning. Network fees are not reserved yet.

## Review snapshot and fingerprint

Once the source, destination and amount are valid, QML receives an immutable `QVariantMap` snapshot. Its SHA-256 fingerprint is computed from a versioned canonical string containing:

1. model namespace/version;
2. chain;
3. mainnet identity;
4. canonical source address;
5. canonical destination address;
6. normalized amount;
7. asset symbol.

Ethereum addresses are lower-cased only for fingerprint canonicalisation; the user-entered display address remains unchanged.

The fingerprint is not a blockchain transaction hash and is not a signature. Its purpose is to bind the human review screen to one exact public intent before chain-specific transaction construction is introduced.

## Explicit non-goals

M48 does **not**:

- query Ethereum nonce/gas or create RLP/EIP-1559 transaction bytes;
- select Bitcoin UTXOs/change/fee rate or produce a PSBT;
- query a Solana recent blockhash or create transaction instructions/messages;
- access the recovery phrase or private key;
- sign any digest/message/transaction;
- broadcast anything.

Watch-only and Address-book public pages remain non-sending surfaces. The entry point exists only on an unlocked development-wallet `ChainPage`.

## Local self-test

Developer diagnostics and Release readiness run a local M48 self-test covering:

- ETH/BTC/SOL metadata and Wallet Core coin IDs;
- valid deterministic public test addresses and malformed-address rejection;
- exact decimal normalization, chain precision limits, zero and exponent handling;
- fingerprint stability/field-mutation behavior.

No network request and no Sailfish Secrets access occurs during this self-test.
