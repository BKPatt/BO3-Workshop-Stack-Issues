# Current release: 1.1.1 — 2026-09-10

The user explicitly requested packaging and uploading the dual-build fix as 1.1.1. Earlier provisional evidence labeled 1.1.1 or 1.2.0 predates published 1.1.0; historical filenames and hashes continue to identify those earlier binaries.

## 1.1.1 release build and packaging

Release DLL SHA-256: `413ccc7f6439e657851537f0fedafcb9649a5806acb814227d241b9604a5d7c7`.
Optional helper SHA-256: `2fb940df5799ce4cc29d858b96ce1b04afef22fc81cad323b0e5ffe9de57fe0a`.

The only native source change from the live-tested compatibility candidate below is the version string, from 1.1.0 to 1.1.1. Both rebuilt binaries have byte-identical executable `.text` sections to that candidate. This comparison is recorded in `dev/release-version-validation-1.1.1.json`; it does not reattribute the earlier live sessions to a different binary hash.

All fourteen native tests passed again on the 1.1.1 Release build, including supported-build profiles, refusal, full-profile patch ownership/rollback, and both real-T7 DLL load orders. The existing CMake T7 configuration was preserved. Evidence: `dev/native-tests-release-1.1.1.log`.

Steam confirmed publication to existing Workshop item **3798094543** at **2026-09-10T17:54:32Z** with result 1, no API failure, and no legal-agreement requirement. The public description exactly matches the packaged metadata, and the browser renders the 1.1.1 change notes and both supported builds. The page shows 498.708 KB; public visibility, tags, title and preview are unchanged. Comments remain present and their settings were not modified. Evidence: `dev/community/steam-publication-1.1.1.json`. This verifies the submitted 18-file payload and public metadata, not an independent subscriber download or another gameplay run. The thirteen startup fastfiles are unchanged. `dev/package-verification-1.1.1.json` verifies both archive allowlists, all file hashes, generic archive aliases, installed Workshop equality, and exact equality to the published payload; the earlier provisional record was preserved as `dev/archived-candidates/package-verification-provisional-1.1.1.json`.

## September 10: dual Steam build gameplay validation before the version bump

The live-tested, unpublished candidate was labeled 1.1.0 and had DLL SHA-256
`0f96b0bb9bc9c211f3bea68b4a3c5a9ab725a4848918709201235742735811ee`.
Its optional helper is `3775116379ec8d6f89321ab62f9d9256326a461f7a31cd8c3c2adbc5fafe14e7`.
At the time of those tests, the public 1.1.0 payload was the D052 binary documented below.

Steam build 24784313 uses executable SHA-256
`51ca63bbc660e0826943c60da67606f6bcb4b3b519528b5e0548c68c9423a323`.
Fourteen directly used engine/Lua functions moved; the other directly used
addresses remain unchanged. All 23 configured hook/call signatures and 1,365
native compatibility sites passed read-only verification in a fresh unhooked
process. The unchanged global addresses also have matching old/new native
references. Evidence: `dev/game-build-live-verification-24784313.json`,
`dev/game-build-verification-24784313.json`, and
`dev/game-build-global-references-24784313.json`.

The Release x64 build and all fourteen native tests passed, including both exact
game identities, wrong-profile refusal, full-profile original/T7/mixed patch
ownership and rollback, unknown-host diagnostics, production callbacks, and
both existing real-T7 load-order tests. Evidence:
`dev/native-tests-game-update-1.1.0.log`. The old executable profile and its
original patch table are retained; future updates must add supported builds
without dropping 21201493 for people downpatching for old T7. Unknown hashes
remain unsupported. This policy is also recorded in AGENTS.md and COMPATIBILITY.md.

On 24784313, a fresh windowed Steam session activated the candidate through
Workshop MODS, reached READY, applied Nacht Revamped (`3596966808`) followed by
Ultimis (`3543204459`), and entered a private one-player game on stock Nacht
(`zm_prototype`). Round 1 and Colt M1911 firing from 8 to 7 rounds were observed.
Normal End Game returned to the private lobby. The gameplay prompt was hidden;
the frontend showed both applied mods. Logs contain the 484-entry precache
union, including 35 weapon requests, and the fingerprinted startup adapter,
with no weapon-none errors. Some script-array warnings remain, as in prior
working controls. T7 was absent from this updated-build test.
Evidence: `dev/game-update-new-round1-1.1.0.json`, the matching screenshot/log,
`dev/game-update-new-exit-1.1.0.json`, and
`dev/game-update-new-gameplay-modules-1.1.0.json`. The manual helper also accepted
24784313 and detected this existing READY module; that check did not repeat
manual injection (`dev/game-update-new-bootstrap-1.1.0.log`).

The same candidate also reached READY on exact build 21201493 with the owned
Scroptss T7 v3.04 helper started first. It preserved all 1,365 preexisting native
patches (`applied=0 reused=1365`) and repaired the previously null continuation
pointer to the OS export. All 23 hook/call signatures had passed read-only
verification before Stack attached. The helper also recognized this existing
READY module. Evidence: `dev/game-build-live-verification-21201493-t7.json`,
`dev/game-update-old-t7-before-1.1.0.json`,
`dev/game-update-old-t7-ready-modules-1.1.0.json`, and
`dev/game-update-old-t7-ready-1.1.0.log`.

In that fresh 21201493/T7-first process, the reverse gameplay order (Ultimis,
then Nacht Revamped) reached round 1 on private stock Nacht and fired the
Colt M1911 from 8 to 7 rounds. Read-only gameplay snapshots confirmed 174
registered weapons, the 484-entry precache union with 35 weapon requests, and
the same candidate DLL plus repaired T7 pointer. Evidence:
`dev/game-update-old-round1-1.1.0.json`, the matching screenshot/log,
`dev/game-update-old-weapons-1.1.0.json`,
`dev/game-update-old-precaches-1.1.0.json`, and
`dev/game-update-old-gameplay-modules-1.1.0.json`.
The 174 registered weapon names/indices matched the prior Nacht control.
Normal End Game returned to the private lobby, captured in
`dev/game-update-old-exit-1.1.0.json` and its log.

These are brief solo/private checks: new build without T7 in forward order,
old build with T7 first in reverse order. They are not both-order gameplay
coverage on each build, a new late-T7 gameplay run, old T7 support on the new
executable, co-op coverage, full voice/character verification, or long-session
stability evidence. Existing diagnostic scripts retain their original hash
guards unless separately updated and verified; the older weapon/asset probes
were only used on 21201493.

Both test game processes were closed normally after returning to their lobbies.
The exact updated 24784313 executable was restored and its hash checked. Only
the three freshly saved display keys were restored (fullscreen 2, 1920x1080,
auto-restore 1); other current config text was preserved. The owned T7 helper
was stopped after checking its path and creation time. No test fixtures were
installed and no subscriptions were changed. Cleanup state is in
`dev/game-update-progress-1.1.0.json`.

Before the release version bump, that tested DLL and intended local Workshop metadata were installed in both the
actual Workshop item and Mod Tools candidate. All thirteen startup fastfiles
are unchanged. Manual and Workshop candidate archives plus generic aliases
pass exact allowlist/hash checks and installed Workshop equality in
`dev/package-verification-1.1.0.json`; `published` and
`matches_published_payload` are false. The earlier published archives were
preserved under ignored `dist/published-1.1.0`. Nothing was uploaded or posted during that earlier candidate step; the later 1.1.1 release is documented above.

## September 10: AAE / Apparition reproduction and Steam build change

The reported connection interruption was not reproduced on stock Nacht in a
private one-player match with Apparition, an owned round-completion fixture, and
AAE full v3.9.5. The fixture advanced the game through the normal death/round
logic to round 3; firing worked there. Reversing the two gameplay mods without
the fixture reached round 1. Both later runs used same-process reloads. Repeated
AAE health-bar script errors also appeared with Apparition removed. See
[AAE_APPARITION_REPORT.md](AAE_APPARITION_REPORT.md) for exact scope, hashes,
evidence, and unverified cases.

**Limitation at the time of this AAE test:** Steam installed build 24784313 during setup.
Published 1.1.0 refused its executable hash before applying patches. Gameplay
testing therefore used a temporary exact copy of supported build 21201493 from
Steam, with unchanged loader guards. The new executable and saved display keys
were restored afterward, and the owned test fixture was retired. The new Steam
build had not yet been validated or added to the supported builds. The later
dual-build candidate above adds that support without dropping 21201493.

## Historical 1.1.0 Workshop publication and community verification

Steam's Mod Tools SDK returned a successful `SubmitItemUpdateResult_t` (`EResult=1`, no API failure or legal-agreement requirement) at **2026-09-10 14:33:52 UTC** for the existing item **3798094543**, authenticated as its owner. The uploaded folder contained exactly the 18 allowlisted release files and DLL SHA-256 `d052b102e7c9779188aa977980a1eb9a11b4b56028a723882b0b06b90bcb6328`. No game/T7 binaries, downloaded mods, source, dumps, or research captures were included.

The public [Workshop description](https://steamcommunity.com/sharedfiles/filedetails/?id=3798094543) matched the then-current `workshop/description.txt` exactly through Steam's public metadata API at the 1.1.0 publication. The [change notes](https://steamcommunity.com/sharedfiles/filedetails/changelog/3798094543) render the full 1.0.0-to-1.1.0 update. The page shows its new update time and 483.489 KB size; its public visibility and tags are unchanged. Workshop comments remain present and enabled. Both GitHub and Patreon links resolve to the intended destinations. Evidence is in `dev/community/steam-publication-1.1.0.json`; this proves the submitted payload and public metadata, not a fresh subscriber download or another gameplay run.

[GitHub](https://github.com/BKPatt/BO3-Workshop-Stack-Issues) contains six documentation/issue-template files only, with bug, feature-request, and setup-support forms. No mod source code or binaries were uploaded. The [Patreon page](https://www.patreon.com/DejaBOOMcya) is published with $3/$8/$20 monthly tiers, a five-member focused-support cap, and a public welcome post. Support is best effort with no deadlines, monthly work quotas, or guaranteed fixes. Reports are directed to GitHub; Workshop comments stay open for discussion.

## Selection UI candidate: automated verification

DLL SHA-256 `d052b102e7c9779188aa977980a1eb9a11b4b56028a723882b0b06b90bcb6328` passed the Release x64 build and all thirteen enabled native tests. The selection test exercises the production deferred-reload callback: secondary clients, an unfinished mod transition and busy assets leave the request pending; an idle frontend consumes it exactly once and keeps the last provider as the settings context. It also covers atomic ordered replacement, removal, empty selection, invalid/duplicate IDs, 1,024 ordered fixture providers, Lua escaping, and HKS boolean union values with stale upper bytes.

`dev/test-selection-ui.py` passed against the production Lua with Lua 5.1. It checks batch toggles, bounded reordering, focus identity, matching child IDs for CoD.Menu navigation, pane switching, one-shot Apply, unchanged Apply, reset, staged Clear All, cancel, failed-Apply recovery, the read-only active viewer, and gameplay exclusion. It also checks that the generated C++ literals preserve the exact Lua source. These fixtures supplement live testing; they do not emulate the whole BO3 UI. Native evidence: `dev/native-tests-selection-ui-1.1.0.log`.

The UI is embedded in the DLL and installed only into the frontend VM after BO3 loads its menu definitions. Native requests independently check the frontend map. The stock last-mod draw is suppressed while the loader is active. Changed settings profiles use BO3's normal lobby LoadMod process; directly changing fs_game left lobby UGC state stale and restored the previous mod after clearing. Unchanged profiles use the guarded deferred reload. Reload detours and their native callees have verified signatures on the pinned Steam executable; hash checks and owned-patch rollback remain in place.

### Corrected selection UI: live menu verification and cleanup

The corrected DLL above loaded from the actual Workshop item and reached READY in a fresh windowed Steam session. A three-mod draft (Nacht Revamped, Ultimis, COLDWAR HEALTH BAR) stayed open through selection and applied as one batch. Reordering to Ultimis, Nacht, Health Bar with the same last provider completed exactly one deferred reload. Removing Health Bar then applied through the normal lobby transition. L opened the read-only viewer with Ultimis followed by Nacht, and L closed it. Left/Right pane focus preserved the selected mod. Finally, Clear All returned to the stock frontend and the viewer displayed zero gameplay mods; the old provider did not reappear.

The captured native log verifies the four applied orders, the single same-profile reload, an empty native stack after clearing, and no UI bridge/constructor/install errors. Evidence: `dev/selection-ui-ready-1.1.0.json`, `dev/selection-ui-1.1.0.log`, `dev/selection-ui-verification-1.1.0.json`, and the `selection-ui-editor`, `selection-ui-loaded`, and `selection-ui-cleared` screenshots in `dev/`. Native controller callbacks are exercised by the Lua fixture; a hardware controller was not tested. This corrected DLL's live checks cover the frontend; the preceding DLL's gameplay observations are identified below.

BO3 exited normally after desktop control reconnected. Only the three saved display keys were restored; all other current config bytes were preserved. No T7 helper was started and no game fixtures were installed for these UI checks. Evidence: `dev/selection-ui-cleanup-1.1.0.json`. The corrected DLL and updated usage metadata were installed in the local Workshop override and Mod Tools candidate. All thirteen startup fastfiles and the optional manual helper were unchanged. Local archive allowlists and installed equality are checked by `dev/verify-release-1.1.0.py`. Publication happened afterward, as recorded above.

### Earlier selection UI gameplay check (before lobby-state correction)

The preceding DLL `0d431596c9f1729c6999043233a103db73c1a1e7f53c9c6c3c7fbd85081321d9` passed live batch additions, mouse and keyboard reordering, same-profile reload, individual removal, Reset Changes and Cancel. Its main-menu and private-lobby viewer displayed Ultimis (`3543204459`) followed by Nacht Revamped (`3596966808`). A private match on stock Nacht reached round 1 with no top-right prompt or stock mod label; read-only snapshots confirmed 174 registered weapons and the 484-request union, including 35 weapon requests. The game returned to the private lobby.

The desktop-control tool reported `failed to activate captured window` during gameplay despite windowed mode. Firing, the in-game L key, and a deliberate End Game action were not verified for that run. T7 was absent. The ready snapshot's false continuation comparison reflects the absent T7 module, not a failed T7 repair. Evidence is in `dev/selection-ui-pre-lobby-fix-*`; these gameplay observations belong to the preceding DLL, not the corrected candidate above. Its Clear All check exposed the stale lobby-state defect and was not a pass.

## Prior priority candidate and automated verification (historical)

DLL SHA-256 `6d0092090aaa9aed2d96f625b3b39cd821fdeed7b370300fb7df621fc31d3a4d` passed the Release x64 build and all twelve native tests. The added production-comparator test covers first-selected priority, both comparison directions, cross-phase and localized names, clearing/reordering, native fallbacks, composed precaches/startup scripts, and sound ownership after native header swaps. Package-plan regression coverage includes an earlier Zombies provider followed by a core-only mod. Evidence: `dev/native-tests-priority-zone-fix-1.1.0.log`.

In a fresh Steam session with T7 v3.04 already running, normal Workshop activation reached READY, reused all 1,365 recognized native patch sites, and repaired the missing continuation pointer. Evidence: `dev/priority-zonefix-t7-ready-1.1.0.json`.

Two inert fixtures verified actual active raw-file definitions in both selected orders, through core loading and entry into Zombies. The first selected mod won both same-phase and cross-phase collisions. Clearing and rebuilding the stack reversed the winner. The B-first case also verified that an earlier Zombies package is restored when BO3 omits it because the last selected mod contains only a core package. All four live snapshots passed `dev/verify-priority-fixtures.py`; evidence is in `dev/priority-fixture-verification-1.1.0.json` and the `dev/priority-zonefix-*-passed-1.1.0.log` captures. All four snapshots use the current DLL.

Priority applies to duplicate definitions between selected gameplay mods, identified by ON number. Shared precache requests remain combined and deduplicated; supported composed initializers retain both routines. Native DB stashes, references, map/stock precedence and same-provider precedence remain intact. This is conflict selection, not physical removal of duplicate fastfile data or a guarantee that conflicting mod designs will work together.

## Prior priority candidate: real-mod gameplay and reload (historical)

The same current-DLL/T7 process passed private stock Nacht (`zm_prototype`) with Ultimis (`3543204459`) selected first and Nacht Revamped (`3596966808`) second. After U — Unload Mods, selecting Nacht first and Ultimis second also passed. Both reached round 1, fired the Colt M1911 from 8 to 7 rounds, and ended normally back to the private lobby.

Read-only snapshots in both matches confirmed the current DLL, the repaired T7 pointer, 28 loaded zones, and all 174 registered weapons matching the Nacht-only control by name and index. Both had 484 unique combined precache requests, including 35 weapon requests. Neither session contained the earlier weapon-none errors. Log captures confirm the guarded startup adapter and priority decisions for the first selected provider. Evidence: `dev/priority-t7-reverse-*`, `dev/priority-t7-forward-*`, and `dev/priority-gameplay-verification-1.1.0.json`; `dev/verify-priority-gameplay.py` checks the captured data and DLL identity.

These are brief solo/private startup, firing, exit and reload checks. Full character/voice behavior, co-op, long sessions and arbitrary character/weapon combinations remain unverified. The unnamed weapon mod cannot be reproduced exactly. Downpatched executables remain unsupported. The current priority binary's live tests used T7 already running; the earlier late-attachment/custom-map tests below identify their own binaries.

## Prior priority candidate installation and cleanup (historical)

The tested current DLL is installed in the local Workshop override and Mod Tools candidate. Version remains 1.1.0. The thirteen startup fastfiles and optional manual helper are unchanged. Release archives use explicit file allowlists and exclude test fixtures, third-party gameplay assets, research, dumps and logs. `dev/verify-release-1.1.0.py` verifies archived hashes, exact file lists, build identity, installed Workshop equality and the two generic archive aliases; its results are written to `dev/package-verification-1.1.0.json`.

BO3 exited normally after both matches. The owned T7 test helper was stopped, the three saved display settings were restored, and both inert priority fixtures were moved out of the game's MODS folder into `research/priority-fixtures-1.1.0-tested`. Other config text was preserved. Evidence: `dev/compatibility-cleanup-1.1.0.json` and `dev/priority-fixture-cleanup-1.1.0.json`. Nothing was published or posted to Steam; other subscribers still receive the published version.

## Pre-priority compatibility candidate (historical C6 binary)

The final Release x64 build and all eleven enabled CTest tests passed. The three added tests cover production streaming-resource callbacks, the guarded startup-script adapter, and the production mod-precache union. Existing capacity, patch ownership, unsupported-host refusal, continuation guards, and real-T7 DLL load-order checks also pass. Evidence: `dev/native-tests-final-1.1.0.log`. The startup adapter additionally passed structural checks against the two captured original scripts.

```text
db3ab9b55c9e0e009aee21c02c7d30c355c173aeaf9cb3b609d0dbe2b6ed2911  BO3Stack.exe
c6be1f09d7295477d5cd7437ff344111a4a1747e5801e955a69428467ac9e22f  bo3_stack.dll
```

### Historical C6 T7 gameplay and reload verification

A fresh Steam launch with Scroptss T7 v3.04 already running activated Workshop Stack through its normal MODS entry. It loaded the final DLL from Workshop folder `3798094543`, reached READY, preserved all 1,365 recognized patch sites (`applied=0 reused=1365`), and repaired the missing T7 continuation pointer. Read-only inspection confirmed the pointer equals the operating-system export.

Both selection orders passed a private match on stock Nacht (`zm_prototype`): Ultimis Character Pack with WaW Vox (`3543204459`) then Nacht der Untoten [REVAMPED] (`3596966808`), followed by U — Unload Mods and a rebuilt stack with Nacht first and Ultimis second in the same game process. Both reached round 1 and fired the Colt M1911, reducing ammunition from 8 to 7. This is the inspected preserved reupload of ImmaSqueeze's gameplay overhaul, not a separate custom-map package.

Both runs had the same 484-entry combined precache list, including all 35 weapon requests. All 174 registered weapons exactly matched the working Nacht-only control. The earlier eight server weapon-none errors disappeared. Both matches ended normally through End Game and returned to the private lobby. The loader retained both inspected startup routines and routed the secondary mod's XPAK and sound-bank files to their own provider. Zone count was 28 in both gameplay snapshots; asset-pool exhaustion did not cause the reported crash.

Evidence: `dev/final-t7-first-ready-1.1.0.json`, the `dev/final-t7-reverse-*` and `dev/final-t7-forward-*` logs and snapshots, and `dev/final-weapon-comparison-1.1.0.json`. STREAMING_FIX_REPORT.md explains the reproduced defects and their fixes.

### Historical C6 scope and local package

These are brief startup, firing and reload checks. Full voice/character behavior, co-op, long sessions, arbitrary character/weapon combinations, other native patch versions, non-English live launches and downpatched executables are not verified. Downpatched executables remain unsupported. The unnamed weapon-mod report cannot be reproduced exactly without its identity. Remaining game-script warnings also occur in the working control; the loader preserves native error handling.

The local Workshop override and Mod Tools candidate use the final DLL. The 13 startup fastfiles are unchanged from the preceding localized candidate. Release archives use explicit allowlists and exclude third-party gameplay assets, research files, dumps and logs; `dev/package-verification-1.1.0.json` records final archive and payload hash checks. Steam may replace the local override with the older published item. Other subscribers receive no changes until an update is published.

BO3 exited normally after the tests, the owned T7 helper was stopped, and only the three temporary display keys were restored to their saved values: fullscreen mode 2, 1920×1080, automatic fullscreen restoration enabled. Other configuration text was preserved. The game executable and T7 DLL on disk were not modified. Evidence: `dev/compatibility-cleanup-1.1.0.json`.

---

# Version 1.2.0 candidate verification — 2026-09-09

This historical candidate removed the four-mod policy cap and added dynamic package-preparation buffers plus demand-based raw-file and compiled-script header growth. At the time of these checks it was local only; the later 1.1.0 publication is recorded at the top of this file. See [CAPACITY_REPORT.md](CAPACITY_REPORT.md) for the implementation, remaining engine limits and inspection references.

## Automated checks — passed

The Release x64 build and all eight enabled CTest tests passed. Coverage includes 4,101 retained selections, package plans for 512 providers / 1,537 records, stable alias pointers, map isolation, pool exhaustion/reuse, low-memory and overflow guards, rollback, unsupported-host diagnostics and both real-T7 DLL load orders. The actual preparation/scheduling callbacks were exercised in an isolated executable with 34, 48 and 64 converted records; the original 32-record arrays remained untouched. The production allocation callback grew and reused both raw-file and script header pools without moving existing headers, and left unsupported pools unchanged. These fixture sizes are not supported in-game mod-count claims. Evidence: `dev/native-tests-1.2.0.log`.

## Live T7 and eight-mod raw-file pressure — passed

A fresh Steam session with Scroptss T7 v3.04 already running activated the normal Workshop item and loaded the 1.2.0 DLL from `steamapps/workshop/content/311210/3798094543`. It reached READY, reused all 1,365 recognized native patch sites and repaired T7's missing continuation pointer. The selected order was `stack_stress_1`, `stack_stress_2`, `stack_stress_4`, `stack_stress_3`, `stack_stress_5`, `stack_stress_6`, `stack_stress_10`, `stack_stress_11`. All eight stayed in the stack; selecting the fifth no longer replaced the fourth.

The six heavy fixtures supplied 6,000 unique raw files in total; the two light fixtures supplied one each. The raw-file pool grew from 5,000 to 7,500 and then 11,250 slots, with 8,657 allocated at the final snapshot. Its original base remained `0x7ff6363c4850`; extension headers came from separate retained allocations. The game remained responsive through repeated menu reloads. Final zone count was 36, and the largest conversion batch observed was 16 records. T7's existing crash log did not grow.

Evidence: `dev/capacity-1.2.0-t7-ready.json`, the first/five/eight-fixture capacity snapshots, `dev/capacity-1.2.0-eight-fixtures-game.log`, and `dev/capacity-1.2.0-crash-log-state.json`. The generated fixtures contain our own inert test data; they are not third-party gameplay compatibility tests and are excluded from release packages.

## Limits and cleanup

Desktop control repeatedly failed with `failed to activate captured window`, including after a fresh game launch. A generated 1,200-script fixture compiled successfully but was not loaded in game. Live script-pool growth, live preparation batches above 32 records, and fresh 1.2.0 custom-map gameplay remain unverified. Earlier version-specific gameplay results are retained below. Co-op, long sessions, non-English launches and arbitrary combinations also remain unverified.

The owned test game and T7 helper were stopped. All 19 generated fixture folders were removed from the game's MODS directory and preserved under `research/capacity-fixtures-1.2.0-tested`. Only the three temporary display settings were restored to their saved values. The local Workshop payload and Mod Tools candidate match by hash; all 13 fastfiles remain identical to 1.1.1. The game executable and T7 DLL on disk are unchanged. Prior local payloads are preserved, including `research/workshop-local-1.1.1-before-1.2.0`.

```text
9e9a940f16910e37ddfca245d461fc31e7bd4baa36e71d8303e151213fae34e7  BO3Stack.exe
407a60ced1b3b111201c5372e01367440a6ba9f6aa716bf71f6f6d04d787b9e0  bo3_stack.dll
```

The local Workshop override contains the tested DLL, but the published Steam item remains unchanged. Steam may replace the override when it refreshes the older published package.

---

# Previous version 1.1.1 candidate verification — 2026-09-09

This historical candidate fixed the late-T7 freeze from the preceding 1.1.0 test. At that test stage it was installed locally only and the public Workshop item was unchanged; the later 1.1.0 publication is recorded at the top of this file. The preceding local payload was preserved in `research/workshop-local-1.1.0-before-1.1.1`.

## Diagnosis and automated checks

The captured null call was through Scroptss T7 v3.04's uninitialized `Protection::ZwContinue` member. Version 1.1.1 identifies the tested handler and image layout, then atomically initializes only a null member to `ntdll!ZwContinue`. It handles both an already loaded T7 module and future loads synchronously. Unknown layouts and other non-null values are left untouched. See [T7_FIX_REPORT.md](T7_FIX_REPORT.md) for the source-backed diagnosis and implementation details.

The Release x64 build and all five enabled CTest tests passed: selection/routing, native patch coexistence/rollback, actual-DLL unsupported-host diagnostics, continuation guards, and real-T7 DLL load orders. The real-DLL fixture reproduces the original null member without invoking T7's game-patching exports, verifies an existing load plus three late loads, rejects an altered handler, and checks listener unregistration. Test evidence is in `dev/native-tests-1.1.1.log`.

## Workshop Stack first, then T7 — passed

A fresh Steam game process initially contained neither DLL. Activating the normal Workshop item loaded version 1.1.1 from `steamapps/workshop/content/311210/3798094543` and reached READY with `applied=1365 reused=0`. Starting the T7 helper afterward loaded the tested v3.04 DLL. The menu remained responsive, the log recorded the missing-pointer repair, and read-only inspection confirmed that the member matched the OS export.

COLDWAR HEALTH BAR was selected first, followed by TF's Zombie Options (No Weapons), then One Window Challenge through Zombies → Private Game. Round 1 began with 750 points, a randomized Argus (10/60), TF's options message, and `6 Zombies Left`. Firing reduced the magazine to 9. The log recorded both packages, the map, execution of both HUD providers, and `HUD combined successfully`. End Game returned normally to the private lobby. The existing T7 crash log did not grow.

Evidence: `dev/late-t7-1.1.1-before.json`, `dev/late-t7-1.1.1-stack-ready.json`, `dev/late-t7-1.1.1-attached.json`, `dev/late-t7-1.1.1-gameplay.json`, `dev/late-t7-1.1.1-game.log`, and `dev/late-t7-1.1.1-crash-log-state.json`.

## T7 first, then Workshop Stack — passed

In another fresh Steam process, read-only inspection found T7 loaded with its continuation member still null and no Workshop Stack DLL. Activating the normal Workshop item loaded 1.1.1 and produced READY with responsive menus. The log recorded `applied=0 reused=1365` and the continuation repair; inspection confirmed that the member now matched the OS export. This 1.1.1 check covered startup and menus. Full gameplay with T7 first was tested on 1.1.0 below; full 1.1.1 gameplay used the late-attachment order above.

Evidence: `dev/t7-first-1.1.1-before.json`, `dev/t7-first-1.1.1-after.json`, and `dev/t7-first-1.1.1-game.log`.

## Scope and binaries

The supported BO3 executable remains unchanged. The 12 localized startup fastfiles and core fastfile are identical to the 1.1.0 files verified below; this update changes the native DLL and documentation. Only the tested Scroptss v3.04 handler/layout receives the continuation repair. No T7 binary is distributed or changed on disk.

```text
4f7d4488ba41c522f6934295767f21c62c43aee4296691fd3d5b806aa1840465  BO3Stack.exe
1b716c1490c84394d3a74076a63f32454ce9ebbc1451fbfff6e9fb8845a63007  bo3_stack.dll
bc4c9e4edb895f237477a9074552a8186e69b7d36db4886d983dc13c143645c6  tested T7 DLL (not distributed)
```

Late attachment was tested at the main menu after READY. Attachment during an active match, long-session behavior, co-op, non-English launches, and arbitrary mod combinations remain untested. The health-bar mod's individual enemy indicator remains visually unverified. Steam may replace the local override when it refreshes the older published package.

---

# Previous version 1.1.0 candidate verification — 2026-09-09

The following is historical evidence. Its late-T7 failure and unsupported status are superseded by the 1.1.1 fix and tests above.

This is a local, unpublished candidate. The public Workshop upload was left unchanged. After testing, the local downloaded copy was backed up and replaced with the candidate for a final activation check; this is a local override, not a Steam update. No comments were posted. Tests used newly launched game processes after confirming BO3 was closed.

## Automated checks

Release x64 compilation succeeded with MSVC 19.44. All three CTest tests passed:

- `stack_selection_and_routing`: additive selection, repeated polling, four-slot replacement, clearing, ID/path validation, map isolation, and core/Zombies/MP aliases for all 12 language prefixes.
- `native_patch_coexistence_and_rollback`: all 16 original/prepatched arrangements of a four-site fixture spanning all three instruction forms, failures before/after copying at each write, exact rollback ownership, and refusal of unknown/unreadable bytes before any writes.
- `unsupported_host_diagnostic`: loaded the actual release DLL into a separate non-BO3 executable. It rejected that executable before accessing game addresses, signaled failure without signaling ready, and delivered the specific reason plus observed/expected hashes through the shared diagnostic channel.

The last test exercises the real DLL's startup rejection; it does not certify every Windows error-dialog path or a particular downpatched BO3 executable.

## T7 first, then Workshop Stack

Tested [Scroptss T7 v3.04 Windows Only](https://github.com/Scroptss/T7Patch/releases/tag/v3.04), released July 30, 2026. The helper was downloaded into the research folder and started before the test game. BO3 displayed the T7 3.04 version and community-patch window title. Module inspection confirmed the actual `t7patch.dll` and the candidate `mods/bo3_workshop_stack/zone/bo3_stack.dll` loaded together.

Before Workshop Stack, read-only inspection found all 1,365 compatibility sites already contained the recognized T7 bytes. Activation through MODS succeeded without the manual helper. The candidate logged:

```text
VERSION 1.1.0
COMPAT verified 1365 sites; applied=0 reused=1365
HOOKS ready with native compatibility
```

The MODS list displayed READY and both numbered entries after selecting COLDWAR HEALTH BAR (`2242220555`) first, then TF's Zombie Options (No Weapons) (`2602139181`). One Window Challenge (`771838448`, `zm_owchallenge`) was selected through Zombies → Private Game → Change Map.

The game reached round 1 with 750 points, a randomized Weevil (50/350), and `6 Zombies Left`. Firing reduced ammunition to 49. The log recorded successful map/mod fastfile loads, execution of both HUD providers, and `HUD combined successfully`. Read-only inspection during gameplay still found all 1,365 sites in the recognized state. End Game returned normally to the private lobby, then BO3 was closed.

This verifies native coexistence, Workshop-style activation, two gameplay packages, a custom map, round initialization and firing. The health-bar mod's individual enemy indicator remains visually unverified. No long-session, co-op, public match, or four-real-mod compatibility claim is made.

## Stack first, then T7 — failed

A separate fresh Steam process had neither DLL loaded and all 1,365 original instruction sequences. Workshop activation of the same candidate reached READY with `applied=1365 reused=0`, verifying standalone startup. The manual helper then recognized the already loaded candidate and reported ready without reinjecting.

Starting Scroptss T7 v3.04 afterward made the game unresponsive. Both DLLs were present; all 112 observed game threads were suspended. T7 wrote `crashes.log` with an exception at RIP zero and a return address in its DLL. These observations do not establish the underlying cause. The failed test process was stopped after preserving the log. Late T7 attachment is unsupported; the documented workflow requires T7 before BO3 and before Workshop Stack.

Evidence: `dev/stack-first-before-1.1.0.json`, `dev/stack-first-t7-late-1.1.0.log`, `dev/t7-late-failure-1.1.0.json`, and `research/t7-late-1.1.0-crashes.log`.

## Installed Workshop-path check

The original downloaded item was backed up to `research/workshop-public-1.0-before-local-1.1.0`. All 18 candidate payload files were copied into the local Workshop item folder and checked against the built payload by SHA-256. The duplicate local test mod was moved out of BO3.

A fresh Steam launch with Scroptss T7 v3.04 already running loaded the candidate by selecting the normal Workshop item in MODS. Module inspection confirmed `steamapps/workshop/content/311210/3798094543/bo3_stack.dll`, the log recorded version 1.1.0 with `applied=0 reused=1365`, and the menu displayed READY. No helper EXE was used for this activation. This final path check used the same DLL/fastfiles as the round-1 gameplay test above.

Evidence: `dev/local-candidate-install-1.1.0.json`, `dev/workshop-candidate-t7-modules-1.1.0.json`, and `dev/workshop-candidate-t7-1.1.0.log`. The public Steam item remains unchanged; Steam may replace this local override if it refreshes the old published package.

## Localized package

The official Mod Tools linker built `core_mod.ff` and 12 localized startup fastfiles: `bp`, `ea`, `en`, `es`, `fr`, `ge`, `it`, `ja`, `po`, `ru`, `sc`, and `tc` followed by `_core_mod.ff`. Each localized output is 896 bytes; the core output is 2,304 bytes. English startup and gameplay used this exact multi-language package. The Portuguese file that was missing from the original upload is now present.

Only English game data is installed on this PC. Non-English game launches were not performed; compilation, file presence, and all 12 routing prefixes were verified. Packaging a localized loader does not translate the loader's English labels or another creator's mod.

## Candidate binaries

```text
6a9121e7b423cfaf647ea2797905fe6160739ea5ca71d1e3ba0b0dad0d43ac4d  BO3Stack.exe
933696929a7b5b8bfdf2ff456eeb41808c8eef7181eb8e6504d233b8fda84f7b  bo3_stack.dll
bc4c9e4edb895f237477a9074552a8186e69b7d36db4886d983dc13c143645c6  tested T7 t7patch.dll (not distributed)
```

Local evidence: `dev/t7-before-stack-1.1.0.json`, `dev/t7-after-stack-1.1.0.json`, `dev/t7-first-modules-1.1.0.json`, `dev/t7-first-stack-1.1.0.log`, `dev/workshop-build-1.1.0.log`, and `research/t7-3.04-stack-1.1.0-round1.jpg`. Research, logs, other creators' mods, and T7 binaries are excluded from release archives.

---

# Previous public release verification — 2026-09-08

These checks apply to the native Workshop Stack implementation. The old Python one-mod-plus-map launcher and its 18 tests are not the implementation under test.

## Environment

- Windows x64, Steam app 311210, installed Steam build 21201493.
- In-game versions 101.22.7.125.0 and 101.22.7.129.0, with the same supported executable hash.
- Game executable SHA-256 remained `9ba98dba41e18ef47de6c63937340f8eae7cb251f8fbc2e78d70047b64aa15b5` after development and game tests.
- Compiled with Visual Studio 2022 Build Tools, MSVC 19.44, Windows SDK 10.0.26100, CMake, static C/C++ runtime, Release x64.

## Automated and startup checks

- Native build succeeded.
- CTest `stack_selection_and_routing` passed. It checks repeated engine selection polling, keeping two selected IDs, deterministic fourth-slot replacement, clearing, invalid IDs, localized aliases, underscore-containing IDs, unrelated map names, and path traversal rejection.
- Running the bootstrap with BO3 closed reported that BO3 must be started first.
- Running the final packaged bootstrap with BO3 open reported **BO3 Workshop Stack is ready** after native initialization.
- Running it again reported the already attached DLL path and its ready status without reinjecting.
- All 1,365 native compatibility signatures matched the supported build before application; native hook prologues matched as well.
- The game executable on disk retained its original SHA-256.

Previous public release native binary hashes (Workshop DLL also includes its own executable hash check and native Lua entry point):

```text
77a416a1a9934c905087efd677c8414d03d41709f61ba6a3d2200b051bc9c282  BO3Stack.exe
1666b376dc8978794ba37cc46ddc071604c020f29f91130dd150e263b951678b  bo3_stack.dll
```

## In-game menu checks

- Two separate Workshop gameplay mods appeared as **[ON 1] COLDWAR HEALTH BAR** and **[ON 2] TF's Zombie Options (No Weapons)**.
- Selecting an already enabled mod retained both publishers and completed the normal game transition.
- Selecting the health-bar mod last retained/reloaded TF's core fastfile, although health-bar has no core fastfile itself.
- **U — Unload Mods** cleared the stack, unloaded the native mod state, and returned to the main menu; the refreshed list showed no active prefixes.
- Four-slot replacement was covered by the native test; only two real Workshop mods were used in-game.

## Workshop startup conversion

The official BO3 linker, with L3akMod 1.0.4 Lua support, built `core_mod.ff` and `en_core_mod.ff`. The package contains its own native DLL and license notices; it does not contain the helper EXE or other creators' mods.

A fresh Steam launch had no `bo3_stack.dll` loaded. Selecting the local Workshop Stack item in MODS loaded the DLL through the game's native Lua module API. The log confirmed the executable and all 1,365 compatibility sites, then reported **HOOKS ready**. The MODS list changed the loader entry to **[READY]**. Selecting Health and then TF retained **[ON 1]** and **[ON 2]**, with the loader still ready. No helper EXE was run for this test.

The official Mod Tools publisher uploaded item `3798094543`. Steam subscription downloaded all seven package files, including the DLL with the exact hash above. The local test mod was moved out of BO3 before testing the subscribed copy.

The subscribed-copy test also passed in a fresh Steam process. Before activation there was no loaded DLL and no Workshop log. Activating the item loaded `steamapps/workshop/content/311210/3798094543/bo3_stack.dll`; its log reported **HOOKS ready**. After activating Health and then TF, the MODS list showed **[READY] BO3 Workshop Stack**, **[ON 1] COLDWAR HEALTH BAR**, and **[ON 2] TF's Zombie Options (No Weapons)**. The stock label showed TF, demonstrating that the native loader survives replacement of its normal active-mod label. All seven downloaded files matched the uploaded files by SHA-256.

The same subscribed copy then reached round 1 on **One Window Challenge**, with **750 points**, a randomized **PPSh-41 (71/426)**, and **6 Zombies Left**. The native log confirmed both Zombies mod packages, the map package, execution of both HUD providers, and **HUD combined successfully**. This verifies Workshop activation through entry into gameplay; the health-bar mod's individual enemy indicator remains unverified.

The item was set to **Public** after these checks. Steam's owner control showed Public selected, and a separate unauthenticated HTTP request returned the item title and subscription control successfully (HTTP 200).

## Two mods plus a Workshop map

Mods: [COLDWAR HEALTH BAR](https://steamcommunity.com/sharedfiles/filedetails/?id=2242220555), publisher `2242220555`; [TF's Zombie Options (No Weapons)](https://steamcommunity.com/sharedfiles/filedetails/?id=2602139181), publisher `2602139181`.

Map: [One Window Challenge](https://steamcommunity.com/sharedfiles/filedetails/?id=771838448), publisher `771838448`, internal name `zm_owchallenge`.

The log recorded successful loading of both Zombies mod fastfiles and the map fastfile, including each mod's script assets. This was two gameplay mods plus a map, not the earlier one-mod-plus-map test.

Initially, TF froze round initialization because the two mods both supplied `ui/hudinject/hudinject.lua`. TF's copy was 5,546 bytes; health-bar's was 11,102 bytes. The loader now retains both copies and executes them through the game's Lua runtime, composing the HUD callbacks.

After that fix the log reported both providers executed and **HUD combined successfully**. The map displayed **Options Loaded!**, 750 starting points, a randomized Man-O-War, a six-zombie counter, and round 1. Firing reduced ammunition from 30 to 29 and then 28. Zombies spawned, the player was downed during the input test, and the game returned normally to the private lobby. This confirms round initialization and controls, not a successful full-length game.

The health-bar module's loading and Lua execution were verified. A visible health bar above an enemy was not captured during this custom-map test, so its complete gameplay effect is not certified by that test alone.

## Final-build built-in-map check

The final packaged DLL loaded both mod fastfiles and Shadows of Evil (`zm_zod`), and logged successful execution of both HUD providers. The game reached first-person gameplay and returned normally after End Game.

Reselecting COLDWAR HEALTH BAR last switched the saved-settings context: TF read unconfigured values (including zero starting points), so this particular run is not evidence of normal configured gameplay. For the known working setup, select TF last so it reads its configured mod profile. This context dependence is documented in the run instructions.

After selecting TF last again, the same final packaged build reached round 1 on Shadows of Evil with **Options Loaded!**, 750 points, a random VMP (40/240 ammunition), and the six-zombie counter. The player was subsequently downed while stationary. A visible enemy health bar was not captured, so that individual visual effect remains unverified.

## Limits

No full session, co-op, synchronization/host migration, public match, campaign, multiplayer, demo, four-real-mod stack, game update, or combination with another native injector was tested. Compiled script conflicts and arbitrary Lua global conflicts are not automatically merged. The conventional HUDInject adapter is a limited compatibility mechanism, not universal composition of all Workshop mods.
