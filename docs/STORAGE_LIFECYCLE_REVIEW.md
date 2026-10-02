# SailVault M47 — storage lifecycle and release-gate review

M47 validates the non-sensitive settings lifecycle before transaction work begins.

## Real profile

The application continues to use one explicit INI file under SailVault's configuration directory. Schema v5 startup records a stable internal install identity, first/last start, launch count, current schema/build identity, profile origin, earliest version observed by the M47 lifecycle tracker, and the source build of the latest observed upgrade. The install identity itself is never exposed to QML.

For profiles created before M47, the first M47 start classifies the profile as `existing`, uses the previous stored app version as the earliest version observed, and records it as the upgrade source when the build version changed.

## Isolated lifecycle probe

The self-test uses `QTemporaryDir` and temporary `QSettings::IniFormat` files only. It validates:

1. empty-profile initialization produces schema/build metadata, one launch, a new internal install identity and `fresh` origin;
2. an upgrade from schema v4 preserves install identity, first-start metadata, fiat/offline/security/hidden-token preferences, increments launch count, advances to schema v5 and records the previous build;
3. the known-key legacy importer copies supported non-sensitive preferences, marks `legacy-import`, and removes an imported plaintext HTTP endpoint during normal sanitisation.

The test neither opens nor authenticates the Sailfish Secrets wallet collection.

## Release gate

`storageReleaseGatePassed` requires:

- the real INI write/sync/reopen/read round-trip;
- the isolated fresh/upgrade/legacy simulation;
- coherent current-profile schema, build identity, install identity, timestamps and lineage metadata.

The result participates in Settings → Release readiness.
