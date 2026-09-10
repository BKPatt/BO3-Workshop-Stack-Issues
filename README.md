# BO3 Workshop Stack — issues and feature requests

This is the public issue tracker for **BO3 Workshop Stack (Experimental)** by DejaBOOMcya. It contains support documentation and issue templates only. The mod's source code, game files, and binaries are not hosted here.

- **Download / update:** [Steam Workshop](https://steamcommunity.com/sharedfiles/filedetails/?id=3798094543)
- **Report a bug, request a feature, or ask for help:** [Open an issue](https://github.com/BKPatt/BO3-Workshop-Stack-Issues/issues/new/choose)
- **Support development:** [Patreon](https://www.patreon.com/DejaBOOMcya)

## Latest release — 1.1.1

Version **1.1.1**, published September 10, 2026, restores support after the BO3 update to Steam build **24784313**. The previous build **21201493** remains supported for players who downpatch for old T7. Future game updates will preserve previously supported builds.

Close BO3 and let Steam finish downloading the update before activating Workshop Stack again. See the [Steam change notes](https://steamcommunity.com/sharedfiles/filedetails/changelog/3798094543) for release details.

## Where to report problems

Please put bug reports, compatibility problems, support requests, and feature requests in **GitHub Issues**, not Steam Workshop comments or Patreon messages. Search existing issues first and add useful information to a matching report instead of opening a duplicate.

Workshop comments remain open for discussion and feedback, but they are not the support queue. GitHub keeps reproduction steps, load order, logs, and progress together.

Reporting issues and suggesting features is free. Patreon support is optional and does not guarantee a fix, compatibility with a particular mod, or a delivery date. If your Patreon membership includes focused help, open the GitHub issue first and privately share its link through Patreon; do not post payment or membership details here.

## Quick start — 1.1.1

1. Subscribe to Workshop Stack and each gameplay mod/map you want. Let Steam finish downloading.
2. Start BO3 through Steam. For the tested Scroptss T7 v3.04 setup on build 21201493, start T7 first and wait for its version to appear before activating Workshop Stack. Old T7 on build 24784313 is unverified.
3. Open **MODS** and activate **BO3 Workshop Stack (Experimental)**, then reopen **MODS**. The editor should show **WORKSHOP STACK / READY**.
4. Click a mod or press **Enter / A** to add or remove it. Use **Left / Right** to switch panes. Use **Q / LB** or **E / RB**, or the Move buttons, to reorder.
5. Choose **Apply Changes** (**F / Y**) once you are finished. All changes apply in one menu reload. **Reset Changes** restores the applied order; **Cancel / Back** discards edits. **Clear All** (**U / left stick**) is staged until you apply.
6. In menus and lobbies, press **L / right stick** or click the top-right prompt for the active mods in their applied order. Close with **L**, **Esc**, or **B**. The viewer is unavailable during gameplay.
7. Choose your Zombies map normally and start a solo/private game. Rebuild your stack after restarting BO3.

Mod **1** has the highest priority for duplicate gameplay-mod assets. The **last** mod in the applied order supplies the saved-settings profile, which the editor displays. For COLDWAR HEALTH BAR plus TF Options, put TF last. Shared precache requests and supported startup adapters retain both mods' contributions; reordering cannot make every conflicting script, weapon, or HUD compatible.

## Useful bug reports

Include the Workshop Stack version, exact error, steps to reproduce, every mod/map Workshop link, the **numbered applied load order**, language, game build, and whether you use T7 or another native patch. Include the T7 version and startup order when applicable. For compatibility issues, try the smallest pair that reproduces the problem when practical.

**Bug, crash, freeze, and incompatibility reports should include `bo3_stack.log`.** Find it in the Steam library where BO3 is installed, under `steamapps/workshop/content/311210/3798094543/bo3_stack.log`. If practical, reproduce the failure once, close BO3, and attach a copy by dragging it into the bug form's required log field. If `.log` is rejected, rename the copy to `bo3_stack.txt` or ZIP only the log. Include the approximate failure time. Prefer the complete file; for a large log, include the complete failing session from its startup/initialization lines through the final error. An error screenshot is helpful extra context, but does not replace the log.

If the log was not created, cannot be found, or cannot be uploaded, explain why in that field; you can still file the report. Remove personal Windows usernames, private paths, IP addresses, and other personal information from the copy while keeping mod IDs, load order, errors, and diagnostic lines intact. Do not upload game files, downloaded mods, executables, DLLs, memory dumps, credentials, or payment information.

## Supported game builds and T7

Version 1.1.1 supports these exact Windows x64 Steam executables:

| Steam build | BlackOps3.exe SHA-256 |
| --- | --- |
| **24784313** — updated game | `51ca63bbc660e0826943c60da67606f6bcb4b3b519528b5e0548c68c9423a323` |
| **21201493** — retained for old T7/downpatching | `9ba98dba41e18ef47de6c63937340f8eae7cb251f8fbc2e78d70047b64aa15b5` |

Downpatching to the exact supported build 21201493 is supported. Unknown executable hashes and alternative clients remain unsupported. If startup still reports an unsupported executable after updating, include the observed hash and the log in your issue.

Scroptss T7 v3.04 compatibility was tested with **21201493**. Start T7 before BO3 for the recommended sequence. The recognized T7 layout can also attach after Workshop Stack reaches READY at the main menu on that build. Old T7 on **24784313**, other T7 versions, and attaching during gameplay require separate verification.

The compatibility fix passed brief solo/private Nacht Revamped + Ultimis gameplay checks on 24784313 without T7 and on 21201493 with T7 started first.

## Current scope

Experimental and primarily tested in solo/private Zombies. Co-op stack synchronization, matchmaking, campaign, multiplayer, arbitrary native patches, and long-session stability are unverified. There is no fixed loader selection cap, but BO3's zone, asset, and memory limits still apply.

Version 1.1.1 retains the 1.1.0 batch editor, first-mod asset priority, secondary-mod streaming and precache fixes, guarded Nacht Revamped/Ultimis startup compatibility, T7 compatibility improvements, dynamic package/asset capacity, and all 12 localized startup variants. Tests are smoke tests, not a promise that every mod combination works.
