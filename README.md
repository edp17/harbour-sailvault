# SailVault — Milestone 2 revision 2

**Package:** `harbour-sailvault`  
**Version:** `0.2.0-2`  
**Wallet Core compatibility baseline:** `4.0.27`

M1 is complete and remains unchanged: Wallet Core compiles, links and runs
natively on Sailfish OS aarch64 and passes deterministic Ethereum, Bitcoin
BIP84 and secp256k1 checks.

## M2r2

M2r1 introduced the C++ Sailfish Secrets storage probe, but device testing
exposed an SQLCipher backend naming restriction:

`SQLCipher plugin only supports collection names with alphanumeric Latin-1 characters`

The M2r1 collection identifier was:

`harbour-sailvault`

The hyphen is invalid for this backend.

M2r2 changes only the secure collection identifier to:

`harboursailvault`

The identifier is deliberately punctuation-free and should remain so.

The storage model is otherwise unchanged:

- Sailfish Secrets default encrypted storage;
- owner-only collection;
- device-lock protection;
- public BIP39 test vector only;
- recovery material remains in C++ and is never exposed to QML.

## Test sequence

1. Store public test secret.
2. Close SailVault completely.
3. Reopen SailVault.
4. Verify stored test secret.
5. Delete public test secret.
6. Verify again; the app should report that no M2 test secret is stored.

Never use a real recovery phrase in this development build.

## Build

```sh
cd ~/mer/android/droid
rpm/dhd/helpers/build_packages.sh -o -b hybris/mw/harbour-sailvault -s rpm/harbour-sailvault.spec
```
