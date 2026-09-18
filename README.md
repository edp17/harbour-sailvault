# SailVault — Milestone 1 revision 17

**Package:** `harbour-sailvault`  
**Version:** `0.1.0-17`  
**Wallet Core compatibility baseline:** `4.0.27`

## r16 result

r16 successfully compiled and linked the complete SailVault executable and
installed on the Xperia 10 III.

At runtime the application showed a white screen because the root QML file
instantiated `MainPage` without importing the local `qml/pages` directory:

`MainPage is not a type`

## r17 fix

`qml/harbour-sailvault.qml` now includes:

```qml
import "pages"
```

No Wallet Core, Rust, protobuf, cbindgen, signing, derivation, CMake link-order,
or GNU compatibility code has been changed from the successful r16 build.

## Expected M1 test

Launching the app should run the deterministic offline self-test and show:

- Ethereum:
  `0x9858EfFD232B4033E47d90003D41EC34EcaEda94`
- Bitcoin BIP84:
  `bc1qcr8te4kr609gcawutmrza0j4xv80jy8z306fyu`
- secp256k1 fixed-digest signature verification
- final result:
  `PASS · Wallet Core works on this Sailfish build`

Never use a real recovery phrase during Milestone 1.

Wallet Core 4.0.27 remains a Sailfish compatibility baseline only, not the
intended real-funds production version.
