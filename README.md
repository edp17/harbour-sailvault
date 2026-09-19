# SailVault — Milestone 3 revision 1

**Package:** `harbour-sailvault`  
**Version:** `0.3.0-1`  
**Wallet Core compatibility baseline:** `4.0.27`

M1 proved Wallet Core can compile, link and run natively on Sailfish OS aarch64.
M2 proved Sailfish Secrets encrypted persistence works from the native C++ layer.

## M3 goal

M3 joins those two pieces into the first real wallet lifecycle layer.

It still uses only the public BIP39 test vector:

`abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon about`

No real recovery phrase should be used.

## M3r1 lifecycle

### Create secure demo wallet

The C++ wallet layer validates the public test mnemonic with Wallet Core,
creates a dedicated Sailfish Secrets collection, stores the mnemonic, reads it
back, derives the expected Ethereum and Bitcoin BIP84 addresses, and performs
an internal secp256k1 sign/verify test.

### Stronger wallet collection

M3 does not reuse the M2 probe collection.

Collection:

`sailvaultwalletv1`

Secret:

`mnemonicv1`

The collection is:

- stored with Sailfish Secrets' default encrypted storage plugin;
- owner-only;
- protected by the device lock;
- configured with `DeviceLockRelock`, so it relocks when the device locks and
  subsequent access is system-authentication mediated.

The collection name is deliberately alphanumeric to satisfy the SQLCipher
backend restriction discovered during M2 testing.

### Load stored wallet

The recovery bytes are fetched in C++, passed directly to Wallet Core, and used
to derive:

Ethereum:

`0x9858EfFD232B4033E47d90003D41EC34EcaEda94`

Bitcoin BIP84:

`bc1qcr8te4kr609gcawutmrza0j4xv80jy8z306fyu`

Wallet Core also performs a fixed-digest secp256k1 signing verification.

Only the public addresses and status information are exposed to QML. Recovery
material never crosses the C++/QML boundary.

After use, SailVault explicitly overwrites its temporary `QByteArray`
containing the recovered mnemonic before releasing it.

### Clear session

Clears the public derived addresses from application state without deleting
the encrypted wallet.

### Delete demo wallet

Deletes the dedicated Sailfish Secrets collection, including the stored
recovery material.

## Device test sequence

1. Confirm the Wallet Core diagnostic checks still pass.
2. Tap **Create secure demo wallet**.
3. Confirm both expected addresses are shown.
4. Close SailVault completely.
5. Reopen it.
6. Tap **Load stored wallet**.
7. Confirm the same Ethereum and Bitcoin addresses reappear.
8. Tap **Clear wallet session**; addresses should disappear but storage should remain.
9. Tap **Load stored wallet** again; addresses should return.
10. Tap **Delete secure demo wallet**.
11. Confirm storage reports no wallet.
12. Try **Load stored wallet**; it should report that no wallet is stored.

Locking the phone between steps 3 and 6 is also useful: M3 uses
`DeviceLockRelock`, so Sailfish may request system-mediated authentication when
the wallet is accessed again.

## Security status

This is still a development harness, not a real-funds wallet.

Wallet Core 4.0.27 is only the Sailfish compatibility baseline. Before any
real-funds beta, move to a maintained Wallet Core version or carefully review
and backport relevant security fixes.

## Build

No manual bootstrap is required:

```sh
cd ~/mer/android/droid
rpm/dhd/helpers/build_packages.sh -o -b hybris/mw/harbour-sailvault -s rpm/harbour-sailvault.spec
```
