# M50 Bitcoin unsigned construction review

M50 extends SailVault's reviewed transaction-intent model with a Bitcoin-only unsigned construction layer. It remains a development-only, no-secrets, no-signing milestone.

## Live boundary

- revalidates the complete M48 Bitcoin intent and SHA-256 fingerprint before network access;
- accepts only the development wallet's native SegWit BIP84 P2WPKH source format;
- verifies the configured API reports the Bitcoin mainnet genesis block;
- sends only the public source address to the API for confirmed UTXO enumeration;
- fetches public fee estimates independently;
- keeps destination and amount local;
- ignores unconfirmed UTXOs;
- caps the provider response, UTXO set and selected input count;
- uses a conservative integer ceiling of the provider sat/vB estimate;
- uses a 546 sat conservative dust floor;
- serializes version 2 with opt-in RBF sequence `0xfffffffd` and locktime 0;
- builds an incomplete PSBT v0 with witness UTXO and `SIGHASH_ALL` metadata only;
- computes the unsigned transaction SHA256d identifier through Wallet Core;
- binds intent + transaction + PSBT with SHA-256.

## Deliberate limitation

M50 does not derive a dedicated Bitcoin change address because doing so would require a carefully designed public-key derivation handoff from the secret-owning wallet layer. If change is needed, the unsigned development artifact returns it to the public source address and labels that policy prominently. Real-funds signing remains blocked until dedicated secure change derivation is implemented and reviewed.

The published BIP84 development address has no spendable UTXOs, so a successful live mainnet query is expected to stop at the zero-UTXO gate. The complete positive construction path is covered by deterministic local diagnostics using synthetic public UTXOs.

## Explicitly absent

- WalletVault/Sailfish Secrets access;
- private keys, mnemonic or HD-wallet APIs;
- PSBT partial signatures/final scripts;
- transaction signing;
- transaction broadcast;
- real-funds enablement.
