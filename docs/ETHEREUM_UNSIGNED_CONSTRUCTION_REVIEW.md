# SailVault M49 — Ethereum unsigned construction review

M49 is the first chain-specific construction milestone. It is intentionally limited to native ETH transfers to Ethereum mainnet externally-owned accounts (EOAs). It constructs the EIP-1559 signing payload but does not access private keys, sign or broadcast.

## Trust boundary

The M48 reviewed intent remains the authoritative human input. Before any network request, M49 revalidates the complete M48 model and SHA-256 intent fingerprint. Construction is refused if chain metadata, addresses, amount canonicalisation or the fingerprint no longer match.

The Ethereum constructor receives only:

- the reviewed public source address;
- the reviewed public destination address;
- the reviewed native ETH amount;
- the configured public HTTPS Ethereum RPC endpoint.

It has no WalletVault or Sailfish Secrets dependency.

## Explicit network step

No construction request is sent automatically. The user must tap **Construct unsigned Ethereum payload** from the Ethereum review page.

The constructor then obtains:

- `eth_chainId` and requires exactly mainnet chain ID 1;
- `eth_getTransactionCount(source, "pending")` for the pending nonce;
- `eth_getBlockByNumber("latest", false)` for `baseFeePerGas` and the block gas limit;
- `eth_maxPriorityFeePerGas`, with `eth_gasPrice` as a compatibility fallback;
- `eth_getCode(destination, "latest")` and requires exactly `0x`.

The amount is not sent to the provider in M49. A plain EOA transfer uses the protocol 21,000 gas limit. Contract recipients and accounts exposing delegated code are deliberately refused; later milestones can add contract/token construction with explicit calldata review.

All requests inherit SailVault's shared HTTPS-only, redirect-free, stateless, bounded-response, timeout and JSON-RPC envelope policy.

## EIP-1559 payload

M49 constructs the type-2 signing preimage:

`0x02 || RLP([chainId, nonce, maxPriorityFeePerGas, maxFeePerGas, gasLimit, to, value, data, accessList])`

For M49:

- `chainId = 1`;
- `gasLimit = 21000` after the EOA code check;
- `data = 0x`;
- `accessList = []`;
- `maxFeePerGas = 2 × latestBaseFee + priorityFee` with checked 64-bit fee arithmetic;
- the ETH amount is converted to wei with decimal-string arithmetic, never floating point.

Wallet Core `TWHashKeccak256` computes the signing hash. No signing API is called.

The review page shows the unsigned type-2 payload, the Wallet Core Keccak-256 signing hash and a separate SHA-256 construction fingerprint binding the original M48 intent fingerprint to the exact unsigned payload.

## Local self-test

Developer diagnostics and Release readiness run a network-free M49 self-test covering:

- strict Ethereum JSON-RPC quantity parsing;
- exact ETH-to-wei conversion;
- reviewed-intent validation plus fingerprint-mutation rejection;
- a deterministic EIP-1559 RLP vector;
- a known Keccak-256 signing-hash vector through Wallet Core;
- construction-fingerprint mutation behavior.

## Still unavailable

M49 does not provide:

- ERC-20 construction or calldata;
- contract-recipient native ETH transfers;
- Bitcoin UTXO/change/PSBT construction;
- Solana blockhash/instruction construction;
- Sailfish Secrets access from the transaction layer;
- private-key signing;
- transaction broadcasting.

Network nonce and fee fields are a time-sensitive snapshot and are not reserved. A future signing gate must refresh/revalidate them immediately before signing.
