# Streaming and startup compatibility fixes — unpublished 1.1.0

This work is local and has not been uploaded to Steam. The reported Nacht Revamped / Ultimis crash was reproduced and fixed. Final verification results are recorded below.

## Reproduced combination

- Nacht der Untoten [REVAMPED], preserved ImmaSqueeze reupload, Workshop 3596966808.
- Ultimis Character Pack with WaW Vox, TameCoot, Workshop 3543204459.
- Supported Steam executable, SHA-256 `9ba98dba41e18ef47de6c63937340f8eae7cb251f8fbc2e78d70047b64aa15b5`.
- Private match on stock `zm_prototype` (Nacht). Nacht Revamped is a gameplay overhaul using this map.

Both mods independently reached round 1 and fired their starting weapon. The combined test reproduced a corrupted lobby character and a crash during map asset linking. T7 was absent in these reproductions; asset-pool exhaustion was not observed.

## Confirmed loader defects and changes

BO3's XPAK dependency iterator did not recognize the secondary mod's generated fastfile name. The loader now translates that dependency group, retains BO3's native iterator cursor, and gives each mod-owned XPAK a distinct resource identity. Shared stock dependencies retain their names. The same alias identities remain available for unloading after the selected stack is cleared.

An asynchronous sound read lost the source mod's thread-local context. One half of Ultimis's sound bank came from Ultimis and the other from Nacht. The loader now records the bank header's provider, including headers swapped by asset linking, and restores that provider around both sound-open callbacks.

With these fixes, the lobby character rendered normally, Ultimis's XPAK opened, both sound-bank halves came from Ultimis, and the map fastfile finished loading. This exposed a second failure: a missing weapon registry caused a client crash while loading a wall-weapon model. The precache fix below resolves that remaining crash.

## Startup investigation

The two mods replace `scripts/zm/gametypes/_clientids.gsc` with different initialization code. A first attempt retaining Ultimis's body and adding Nacht's dependency did not fix the failure. It is superseded in the current candidate.

The startup adapter retains Nacht's original code and dependencies and invokes the character initialization from Ultimis before Nacht's callback registration. It executes in game but did not alone resolve the pair's invalid-weapon crash. It applies only to the inspected Workshop IDs and complete original-script fingerprints; changed versions are left untouched. The source scripts are read from installed assets at runtime. No third-party scripts or game assets are bundled in the release.

The diagnostic build logs script errors and available bytecode call positions while retaining the game's original error handling. It does not suppress invalid weapon operations or substitute dummy models.

A control test with Workshop Stack active and Nacht alone reached round 1, fired the Colt M1911 (8 to 7 rounds), and ended normally. The flag-array and client upgraded-weapon-array warnings also occur in this working control, so those warnings are not evidence of the crash's cause. The combined run additionally produces eight server `UseWeaponHideTags` calls with weapon `none`; its client then crashes while obtaining a wall-weapon model name.

Loading Ultimis before Nacht reached round 1 and fired successfully on that diagnostic build. Its 174 registered weapons exactly matched the working Nacht control. This did not establish full Ultimis character or voice behavior.

## Precache-list collision

Both mods contain a BG_CACHE asset named `zm_mod`. With Nacht first, the active DB entry was Ultimis's 32-entry list, which contains no weapons. Nacht's 458-entry list, including 35 weapon requests, was stashed behind it. Nacht's actual weapon assets and weapon CSV were still present. Reversing the order exposes Nacht's list, explaining why changing order restored its weapon initialization while potentially hiding character precaches.

The new general adapter combines same-named mod precache lists from selected providers. It keeps records ordered by type/name, removes exact duplicates, and owns the copied inline names through DB stash lifetimes. Deselected and previous-batch sources are excluded. Live inspection confirms a 484-entry union containing all 35 weapon requests and the character resources. The working game registers 174 weapons, matching the Nacht-alone control.

## Duplicate definitions and priority

The current 1.1.0 DLL gives the first selected mod (`[ON 1]`) priority for duplicate asset definitions between selected gameplay providers, including across core/Zombies phases. The native DB still owns its stash chain and unload promotions. Supported composed startup scripts and combined precaches retain both providers' data; priority does not discard their contributions. Exact duplicate precache requests are removed by type/name.

Sound-bank ownership follows the resulting native entry's zone after asset linking. A lower-priority incoming bank can no longer relabel the active winning header. The loader also restores earlier selected Zombies packages when BO3 omits them because its last selected mod has only a core package.

The current DLL is `6d0092090aaa9aed2d96f625b3b39cd821fdeed7b370300fb7df621fc31d3a4d`. All twelve native tests pass, including production comparison and sound-ownership cases. In-game fixture checks passed both selection orders, both core and Zombies phases, clearing/reloading, and the omitted-package case with T7 v3.04 active.

The same current-DLL session passed Ultimis then Nacht, followed by clearing/reloading Nacht then Ultimis: round 1, Colt M1911 firing from 8 to 7, and normal End Game to the lobby in both orders. All 174 weapon names and indices matched the working control; each combined list contained 484 unique precache requests including 35 weapon requests. The earlier weapon-none errors were absent. Evidence: `dev/priority-gameplay-verification-1.1.0.json` and the `dev/priority-t7-reverse-*` / `dev/priority-t7-forward-*` captures. See VERIFICATION.md for full scope.

## Earlier compatibility verification

The pre-priority candidate passed eleven native tests, including production streaming callbacks, background sound ownership, unload dependency identity, production precache-union callbacks, startup metadata bounds, preserved bytecode, and isolated T7 load-order tests. The startup adapter also passed structural checks against the two locally captured original scripts. These checks do not replace gameplay verification.

The first precache-fix candidate (`2a35f107781b43130fba3290707d0b7560d019774ec5eda58c1ed4252acfc841`) passed Nacht then Ultimis without T7: round 1, Colt M1911 firing from 8 to 7 rounds, visible wall-weapon model, and normal End Game. The eight server weapon-none errors disappeared. Evidence: `dev/precache-fixed-forward-passed-1.1.0.log` and `dev/precache-fixed-forward-game-weapons-1.1.0.json`.

The pre-priority DLL (`c6be1f09d7295477d5cd7437ff344111a4a1747e5801e955a69428467ac9e22f`) adds bounded diagnostic-header tracking, fresh startup-source batches, and safe error unwinding. With Scroptss T7 v3.04 already running, normal Workshop activation reached READY, preserved all 1,365 recognized patch sites, and repaired the missing continuation pointer. Ultimis then Nacht passed round 1, Colt M1911 firing from 8 to 7, 174 registered weapons, and normal End Game. Evidence: `dev/final-t7-reverse-passed-1.1.0.log`, `dev/final-t7-reverse-weapons-1.1.0.json`, and `dev/final-t7-reverse-exit-1.1.0.log`.

In the same final process, U — Unload Mods cleared the gameplay stack while Workshop Stack stayed READY. Loading Nacht then Ultimis repeated the successful round 1, firing from 8 to 7, and normal End Game back to the lobby. The 484-entry precache union was restored, all 174 registered weapons exactly matched the Nacht control, and there were no weapon-none errors. Evidence: `dev/final-t7-forward-passed-1.1.0.log`, `dev/final-t7-forward-exit-1.1.0.log`, the corresponding weapon/precache/module snapshots, and `dev/final-weapon-comparison-1.1.0.json`.

The unidentified character-and-weapon report cannot be reproduced exactly without the weapon mod's identity. These general streaming and precache fixes may address the same causes elsewhere, but arbitrary conflicting scripts and asset definitions still need individual adapters or author changes. Downpatched executables remain unsupported. The Ultimis author describes the pack as intended for custom maps; the test here uses stock Nacht with Nacht Revamped. Complete voice behavior, all character features, co-op and long-session stability are not established by these brief gameplay checks.
