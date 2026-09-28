# BO3 Workshop Stack — experimental Workshop mod

**Version 1.1.1 — September 10, 2026:** restores support after the game update to Steam build 24784313 while keeping build 21201493 supported for people who downpatch for old T7. It retains the 1.1.0 selection editor, load-order priority, streaming, precache, capacity, T7 and localized startup improvements. See CHANGELOG.md and VERIFICATION.md for exact validation.

Version 1.1.1 is published on the existing [Steam Workshop item](https://steamcommunity.com/sharedfiles/filedetails/?id=3798094543). Its DLL SHA-256 is `413ccc7f6439e657851537f0fedafcb9649a5806acb814227d241b9604a5d7c7`. Steam confirmed the upload, and the public description and change notes were verified.

Select multiple installed Steam Workshop mods through BO3's existing **MODS** menu. The extension retains the selections, loads their fastfiles together, and keeps Workshop maps available through the normal map selector.

The Steam Workshop package includes a Windows native extension, loaded directly when you activate the mod in BO3. Subscribers do not need a separate launcher or Mod Tools. Activate Workshop Stack once per game session. The earlier Python mod-and-map launcher is legacy work and is not this implementation.

## Run

Download from [Steam Workshop](https://steamcommunity.com/sharedfiles/filedetails/?id=3798094543). Use the [GitHub issue tracker](https://github.com/BKPatt/BO3-Workshop-Stack-Issues/issues/new/choose) for bugs, feature requests, and setup help, rather than Workshop comments. Comments remain open for discussion and feedback. The public GitHub repository hosts this 1.1.1 source alongside the issue forms; binaries are not hosted there.

[Optional Patreon support](https://www.patreon.com/DejaBOOMcya) offers monthly Supporter ($3), Feature Input ($8), and Focused Bug Support ($20, five places) memberships. Extra attention depends on available development time, with no deadlines, monthly work quotas, guaranteed fixes, or commissioned development. Reports and feature requests stay free. Focused-support members should open a GitHub issue first, then privately send its link through Patreon.

1. Subscribe to [BO3 Workshop Stack (Experimental)](https://steamcommunity.com/sharedfiles/filedetails/?id=3798094543) and let Steam finish downloading. Subscribe separately to each gameplay mod and map you want.
2. Start Black Ops III through Steam and wait at the main menu, before starting a match.
3. Open **MODS** and activate **BO3 Workshop Stack (Experimental)** first.
4. Reopen **MODS** to see the Workshop Stack editor with its **READY** heading.
5. Toggle the mods you want with **Enter / A**, arrange their load order with **Q / LB** and **E / RB**, then press **F / Y — Apply**. All edits stay in the menu until you apply them together.
6. Open **Zombies → Private Game → Change Map**, select the map, and start.

The left pane lists installed gameplay mods; the right pane shows the proposed order. Selecting a mod toggles it in or out. Move a selected mod up or down to change priority. **Reset Changes** restores the applied stack, and **Cancel / Back** discards pending edits. **Apply** reloads once for the complete change. There is no fixed Workshop Stack selection cap; BO3’s menu, zone, asset and memory limits still apply. See CAPACITY_REPORT.md for the exact capacity changes.

The top-right **Press [L] — Loaded Mods** prompt opens the active stack in its applied order. Click the prompt or press the right stick on a controller. The prompt and viewer appear only in frontend menus/lobbies, never over gameplay. BO3 uses the last mod in the applied order for mod-specific saved settings; the editor shows this settings profile. For the tested COLDWAR HEALTH BAR / TF Options pair, keep TF last and check its options before starting. Rebuild the stack each time BO3 restarts. Closing BO3 removes the native extension and all of its memory patches.

Workshop Stack stays active for the session and consumes no gameplay selection. Apply an empty selection to unload all gameplay mods; exit BO3 completely to disable the native loader.

On build 21201493, the tested startup order is T7, then BO3, then Workshop Stack. Scroptss T7 v3.04 can also attach after Workshop Stack is READY at the main menu on that build. Downpatching to that exact supported executable remains supported. This does not establish old T7 compatibility with build 24784313. QUICK_START.md provides a step-by-step guide; COMPATIBILITY.md records exact support and useful bug-report details.

The late-T7 freeze found in an earlier internal build is fixed in this release. The loader initializes a missing exception-handler pointer in the recognized T7 v3.04 module when it loads, preserving existing non-null pointers. The T7 DLL on disk is unchanged. See T7_FIX_REPORT.md for the cause and tests. Attaching T7 during an active match has not been tested.

The downloadable ZIP remains an optional manual installation: extract it, start BO3, and run **Start Stack.cmd** before choosing your mods. `LaunchStack.cmd` in this workspace runs that packaged helper. Workshop subscribers do not need this step.

## Compatibility and limits

**Experimental: arbitrary Workshop mods are not automatically compatible.** Loading two packages cannot generally combine contradictory scripts, weapons, HUDs, or game rules. For duplicate definitions supplied by selected gameplay mods, **mod 1 has highest priority**, followed by mod 2, mod 3, and so on. This applies across core and Zombies loading phases. Use the editor's load-order pane to change priority, then Apply.

BO3 retains its native precedence for stock/map assets, placeholders and different packages from the same provider. Deliberately combined precache lists and supported startup adapters retain their combined result. Choosing one active definition does not remove the losing package's bytes from memory: the engine keeps its normal asset stashes for references and unloading.

Same-named mod precache lists are combined so each provider can register its weapons, characters and other resources. Streaming files remain associated with their source mod. A separate adapter preserves the inspected Nacht Revamped and Ultimis startup routines; it applies only when both original scripts match the tested fingerprints. These changes preserve loading requests, not contradictory weapon definitions or arbitrary script logic. See STREAMING_FIX_REPORT.md.

The loader has a specific compatibility adapter for the common `ui/hudinject/hudinject.lua` convention. When multiple selected mods provide that file, it preserves the versions and runs their HUD callbacks with separate `CoD.HUDInject` classes. This fixes the observed TF Options initialization freeze when stacked with COLDWAR HEALTH BAR. It does not merge arbitrary Lua globals or compiled GSC/CSC functions.

`bo3_stack.log`, next to the DLL, records selected publishers, loaded packages, HUD initialization, and differing raw/script asset collisions (`CONFLICT`). Conflicts are observations, not proof of incompatibility; their absence is not proof of compatibility. The HUD adapter does not include collisions with a map's own replacement HUD. Mods using identical streaming bank/resource names may also conflict.

Earlier builds reached round 1 on One Window Challenge with both Workshop mod packages loaded and both HUD modules executed. TF's saved options, custom starting weapon, points, zombie counter, and shooting worked. An earlier package also reached round 1 on Shadows of Evil with TF selected last; the health-bar mod's individual visual effect remains unverified. Current compatibility work has also brought the named Nacht Revamped / Ultimis pair into round 1 with working weapons. See **VERIFICATION.md** for exact binaries, load orders, and remaining limits. These are startup and brief gameplay checks; long-session and universal compatibility remain unverified.

Only solo/private Zombies testing has been performed. Multiplayer, campaign, demos, co-op, host migration, joining matches, and synchronized mod-stack negotiation are unverified. The loader does not advertise or synchronize the full stack to other clients.

## Supported game builds

Windows x64 Steam app **311210**. Version 1.1.1 recognizes both exact executables:

| Steam build | Executable SHA-256 |
| --- | --- |
| 24784313 | `51ca63bbc660e0826943c60da67606f6bcb4b3b519528b5e0548c68c9423a323` |
| 21201493 (retained for old T7/downpatching) | `9ba98dba41e18ef47de6c63937340f8eae7cb251f8fbc2e78d70047b64aa15b5` |

New game support is added alongside existing profiles. Unknown hashes and alternative clients remain unsupported; menu version strings alone do not identify a compatible executable.

Both the Workshop DLL and the optional manual bootstrap refuse other executable hashes. The extension verifies its native hook signatures and the complete compatibility patch set before applying it. Since 1.1.0, the loader accepts exact original or recognized compatible bytes already applied by T7. It leaves existing patches untouched and restores only its own changes if startup fails. Unknown patch bytes or changed hook functions still cause a refusal. An update to BO3 needs a new validated build of this loader. Other native patches/injectors require separate tests.

No game executable or other creators' Workshop files are modified on disk. The loader writes its own diagnostic log next to its DLL. Native code hooks and compatibility changes are applied to the running process and disappear on exit. Steam ownership, authentication, and network behavior are not replaced. BO3 Mod Tools are not required to run this package.

## Troubleshooting

- **No READY editor:** activate Workshop Stack first and reopen MODS. If activation fails, inspect its `bo3_stack.log` in `steamapps/workshop/content/311210/3798094543`.
- **Unsupported executable:** the error shows the observed SHA-256 and supported identities. Version 1.1.1 supports 24784313 and 21201493, including deliberate downpatching to 21201493 for old T7. Close BO3 and let Steam finish downloading the update before activating the loader. Unknown hashes stop before applying hooks.
- **Refused initialization / no ready status:** the loader displays a specific startup reason and records it in `bo3_stack.log`. Restart BO3 after addressing that reason. A loaded DLL alone is not successful initialization. If using T7, let it finish attaching first.
- **Missing `bp_core_mod.ff` or another language fastfile:** the old Workshop upload contained English only. The current package includes all 12 Mod Tools language variants. Let Steam finish downloading the update before restarting BO3.
- **Already attached:** rerunning the helper checks the existing extension's ready status; it does not attach a second copy. To update the DLL, close BO3 first.
- **Frozen round or script/HUD error:** unload all mods, test each alone, then a smaller stack. Another shared asset may require an author-level compatibility patch.
- **Edits have not loaded:** press **Apply** after selecting and arranging mods. The loaded-mod viewer shows the applied stack, not pending edits.

## Build from source

Requires CMake and Visual Studio 2022 C++ Build Tools with a Windows SDK. MinHook source is included under `native/third_party/minhook`.

```powershell
cmake -S native -B native/build -G "Visual Studio 17 2022" -A x64
cmake --build native/build --config Release
ctest --test-dir native/build -C Release --output-on-failure
```

Outputs: `native/build/Release/BO3Stack.exe` and `bo3_stack.dll`. Thirteen default native tests cover executable profiles, selection transactions, routing, priority, precache composition, package buffers, capacity, patch ownership, startup refusal, and T7 pointer initialization. An optional fourteenth test exercises both DLL load orders using an independently obtained Scroptss T7 v3.04 DLL; configure its path with `-DT7_COMPAT_TEST_DLL=...`. `py -3.11 dev/test-selection-ui.py` exercises the production frontend Lua with a BO3 UI fixture and requires `lupa` with Lua 5.1. The legacy launcher's tests do not validate this native implementation.

To build the Workshop package, install the base BO3 Mod Tools (Steam app 455130) and [L3akMod 1.0.4](https://dtzxporter.com/tools/l3akmod) in its `bin` folder, then run `dev/build-workshop.ps1`. Pass `-ToolsRoot` and `-GameRoot` if your Steam paths differ. The script builds the Lua fastfiles, copies the DLL and notices, and prepares `mods/bo3_workshop_stack/zone` for the official Mod Tools publisher. Close BO3 before building a local installation. Use `-SkipLocalInstall` when preparing an upload without adding a duplicate local mod. The Workshop item ID is preserved in existing metadata; a clean builder must use a new item rather than overwrite someone else's item.

The builder compiles the 12 localized startup fastfiles, including Portuguese `bp_core_mod.ff`. It checks that each localized output exists and rejects unexpected files in the publish folder. Research captures, downloaded Workshop files, T7 binaries, game binaries, and game dumps are excluded from the release package. See LICENSE and THIRD_PARTY.md for source and dependency notices.
