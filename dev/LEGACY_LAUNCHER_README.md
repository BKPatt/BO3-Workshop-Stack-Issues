# BO3 Mod + Map Launcher

**Project status: this launcher does not fulfill the requested in-game Workshop multi-mod loader.** It was built from an incorrect interpretation of the request. Its successful mod-plus-map test is not evidence of multiple gameplay mods running together. See [MULTIMOD_STATUS.md](MULTIMOD_STATUS.md) for the corrected requirement and engine research. No working multi-mod extension has been implemented or installed.

A Windows launcher for dynamically selecting **one gameplay mod and one map** from your installed BO3 Workshop downloads. It uses BO3's existing launch support. It does **not** implement multiple gameplay-mod slots, hot loading during a match, or automatic merging of compiled mods.

## Why this exists

A Workshop custom map normally does not occupy BO3's gameplay-mod slot. Load the gameplay mod from **Mods**, then select the custom map in **Zombies → Private Game → Change Map**. A map distributed as a gameplay mod, or a map with a separate required patch mod, is a different case: the patch competes for the single mod slot.

This launcher makes the two selections explicit and saves them as reusable profiles. Refreshing discovers new downloads without recompiling or editing source. It never copies, renames, merges, or modifies game or Workshop files.

## Run

Use `dist/BO3-Mod-Map-Launcher.exe` if the packaged build is present. Alternatively, double-click `Launch.cmd` with Python 3.11 installed, or run:

```powershell
py -3.11 bo3_launcher.py
```

1. Keep Steam running and finish downloading your Workshop items.
2. Select a **Gameplay mod** and a **Map**. Built-in Zombies maps are included when their fastfiles are present.
3. **Open modded menu** is the normal path: after launch, select your saved map in BO3. **Try direct map launch** also sends `+devmap`, but this is a developer command and is not reliable on every build. On the locally tested build it returned to the menu, so manual map selection was required. If Steam shows **Launch Game with custom arguments**, select **Continue** for the launch you requested. Check the Steam window if nothing appears.
4. For either solo play or co-op, create a **Private Game → Change Map** and select the custom map inside BO3. A private game can be started alone. The launcher does not create a lobby or invite players.
5. Type a profile name and click **Save profile** to remember the combination. Profiles are stored alongside the launcher in `profiles.json`.

Exit BO3 normally before launching a different profile. The launcher will not terminate a running game. Compatibility is specific to each mod/map combination; metadata checks cannot detect conflicting scripts or asset limits. All co-op players should have the same downloads and mod version.

The launcher supports ordinary Steam Workshop packages and local `mods/<name>/zone` / `usermaps/<name>/zone` layouts. It skips missing or malformed metadata, uncompiled packages, invalid map names, and ambiguous duplicate map names. Workshop **mod** IDs become `fs_game`; map **FolderName** values become the map argument. These are different identifiers.

## Command line

```powershell
# Inventory and diagnostics; reads metadata, not asset contents.
py -3.11 bo3_launcher.py scan

# Preview only; use the actual keys returned by scan.
py -3.11 bo3_launcher.py launch --mod workshop:mod:2602139181 --map stock:map:zm_zod --dry-run

# Open the selected mod's menu for co-op.
py -3.11 bo3_launcher.py launch --mod workshop:mod:2602139181 --menu

# Override automatic detection. Global options go before the subcommand.
py -3.11 bo3_launcher.py --game "D:\SteamLibrary\steamapps\common\Call of Duty Black Ops III" scan
```

The `scan` command is read-only. Launch prefers Steam's `-applaunch 311210` path and falls back to the game executable if Steam's executable cannot be found. It uses an argument array with `shell=False`; Workshop metadata is never executed as shell code. The application has no network client or account authentication. Steam manages subscriptions and game ownership.

## What is still outside the scope of this launcher

There is no universal dynamic gameplay-mod loader here. Two mods can both replace `zm_mod.ff`, override the same scripts or assets, and rely on incompatible initialization code. Copying them into one directory would overwrite content; a file selector cannot resolve those conflicts. Compiling a combined mod from source is one approach. A runtime loader would require engine integration for mounting packages, loading/unloading assets and scripts, defining override semantics, and coordinating matching clients. Compatibility would still not be automatic.

No Mod Tools download is needed for the mod + map workflow. No game executable patch, DLL injection, custom client, or anti-cheat changes are part of this project.

## Development and verification

Python 3.11 or later; no runtime dependencies beyond the standard library and Tkinter.

```powershell
py -3.11 -m unittest discover -s tests -v
py -3.11 -m venv .venv
.\.venv\Scripts\python.exe -m pip install -r requirements-build.txt
.\.venv\Scripts\python.exe -m PyInstaller --noconfirm --onefile --windowed --name BO3-Mod-Map-Launcher bo3_launcher.py
```

Automated tests cover discovery, Steam libraries, metadata validation, map/mod identifier handling, mode mismatches, duplicate maps, incomplete downloads, saved profiles, and launch behavior. They do not prove that arbitrary game content is compatible. See `VERIFICATION.md` for the actual local in-game verification status.

## Sources

- [Treyarch's Mod Tools launcher source](https://github.com/TreyarchGames/ModLauncher/blob/master/mlMainWindow.cpp): `OnBuild` and `OnRunMapOrMod` construct `+set fs_game` / `+devmap` arguments; Workshop metadata uses `Type` and `FolderName`.
- [TF's Zombie Options — author's Workshop page](https://steamcommunity.com/sharedfiles/filedetails/?id=2602139181): describes custom-map support and its limits.
- [Mob of the Dead Remastered Patch — authors' instructions](https://steamcommunity.com/sharedfiles/filedetails/?id=1725568279): demonstrates the separate mod-then-map workflow. This is a reference, not a recommendation or dependency.

Independent project; not affiliated with Activision or Treyarch. Game and Workshop assets are not included.
