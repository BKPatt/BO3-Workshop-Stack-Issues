# Larger stacks and adaptive capacity — 1.1.0

This work is part of the unpublished 1.1.0 release. Provisional 1.1.1/1.2.0 labels in evidence refer to internal test builds only.

This is a local candidate, not a Steam upload. Automated tests and the eight-mod raw-file stress check passed. Further live checks were blocked by desktop focus errors; the exact scope is recorded below.

## Changes

Workshop Stack no longer imposes a four-mod selection cap. Every distinct selection appends in order; selecting it again does not remove, duplicate, or reorder it. Numbered menu entries remain. Package planning and HUD-provider storage grow dynamically.

Package aliases use stable storage, including short names held inline by C++ strings. Larger plans cannot invalidate the pointers already assigned to earlier packages. A shared package planner covers core, Zombies and multiplayer aliases, sparse per-mod package availability, custom-map isolation, and core startup restoration.

The supported executable's `DB_LoadXAssets` uses a 32-record temporary array before localized/patch expansion. The module redirects that preparation and its matching scheduling call to thread-local heap buffers sized to four records per input (at least 32). Existing conversion rules and the engine's scheduling/unloading flow remain in use. Unknown call sites retain their original behavior. A result above the engine's 64-record zone table is refused through its error flow before that table is written.

Raw-file and compiled-script header pools grow when they actually run out of free headers. Each growth adds 50% of current capacity, with a minimum of 128 headers, subject to available physical memory and commit headroom. The allocator leaves up to 1 GiB (one eighth of total RAM on smaller systems) available, and uses at most one eighth of the remaining available budget per growth. It allocates only the exhausted pool. Existing asset addresses and the original pool base remain intact; new free-list segments are retained for the process lifetime and reusable after assets unload. Allocation failure leaves the pool unchanged.

## Remaining engine limits

This removes a loader policy limit, not every BO3 limit. The existing MODS pool has 128 entries. The engine still has its loaded-zone table, asset-entry database, script VM limits and GPU/indexed asset limits. Localized and patch packages can consume additional zones, so a zone limit is not a fixed mod count. The memory adapter currently supports raw-file (type 47) and compiled-script (type 54) headers only. Image/model/weapon pools require separate handling of their index-based users and are not changed. RAM growth cannot merge conflicting scripts or game rules.

## Verification

Release x64 compilation and eight CTest tests passed. Selection tests retain 4,101 entries and preserve order under repeated polling. Package plans cover up to 512 providers / 1,537 records, including stable names, sparse modes, unload flags and map isolation. An isolated executable invokes the actual production preparation/scheduling callbacks with 34, 48 and 64 converted records, verifies names and order, and checks that the original 32-record buffers remain untouched. Unknown callers retain their original buffers. Pool tests cover repeated exhaustion, reuse, stable live pointers, low memory, allocation failure, invalid state and overflow. Existing T7, rollback and unsupported-host tests also pass. These isolated counts do not claim that BO3 can run that many mods or zones.

A live Steam session with Scroptss T7 v3.04 retained eight generated test mods. Six contained 1,000 unique raw files each; two contained one each. Raw-file capacity grew from 5,000 to 7,500 and then 11,250 slots as the pool ran out. Inspection found 8,657 live headers, an unchanged original pool base, and 36 loaded zones. Repeated mod-selection reloads remained responsive. The two growths added 150,000 bytes of header storage; this figure excludes the assets' own data. Evidence is recorded in `dev/capacity-1.2.0-eight-fixtures.json`, the earlier growth snapshots, and `dev/capacity-1.2.0-eight-fixtures-game.log`.

The isolated callback test also invokes the production asset-allocation adapter against both supported descriptor types. Each exhausts a pool twice, preserves existing headers, and reuses a freed extension header without further growth; unsupported types remain unchanged. This complements the live raw-file result but does not substitute for live compiled-script pressure testing.

The largest observed live conversion batch was 16 records; batches of 34, 48 and 64 were verified in the isolated callback test. A 1,200-script fixture compiled successfully, but the desktop controller repeatedly returned `failed to activate captured window`, including after a fresh game launch, so its in-game load was not performed. Fresh current-candidate custom-map gameplay, live batches above 32 records, co-op, long sessions, and arbitrary mod combinations remain unverified. The test fixtures were removed from the game installation after testing. Earlier gameplay results (internal label 1.1.1) are recorded separately in VERIFICATION.md.

## Inspection references

The supported Steam binary and its saved unpacked image were inspected directly. Relevant RVAs: asset allocator `0x13e9e50`, pool descriptors `0x93883f0`, package converter `0x1426970`, and scheduler `0x1423a70`. The loader checks the executable hash and each hook's original bytes before activation.

Public layout references: [BOIII XAssetPool](https://github.com/Ezz-lol/boiii-free/blob/03702428c7f1e793e289610a915acb3249ac21e9/src/client/game/structs/db/xasset/core.hpp), [zone structures](https://github.com/Ezz-lol/boiii-free/blob/03702428c7f1e793e289610a915acb3249ac21e9/src/client/game/structs/db/xzone.hpp), and [T7Overcharged](https://github.com/JariKCoding/T7Overcharged). The capacity policy and adapters are implemented independently here; these projects' pool-resizing implementations are not copied or distributed.
