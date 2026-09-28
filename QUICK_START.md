# Quick start

Version 1.1.1 is available on [Steam Workshop](https://steamcommunity.com/sharedfiles/filedetails/?id=3798094543). No separate Workshop Stack launcher or Mod Tools is needed.

This update supports Steam build 24784313 and keeps build 21201493 supported for downpatching with old T7. COMPATIBILITY.md lists both exact executable hashes.

1. Subscribe to Workshop Stack and the gameplay mods/maps you want. Let Steam finish downloading.
2. Start BO3 through Steam. If using the tested T7 v3.04 setup on build 21201493, start T7 first and wait for its version to appear before activating the loader. Old T7 on 24784313 is unverified.
3. In the main-menu **MODS** list, activate **BO3 Workshop Stack (Experimental)**.
4. Reopen **MODS**. The new editor displays **WORKSHOP STACK / READY**.
5. Select mods in the installed list with **Enter / A** or a click. Select again to remove one. The right pane updates immediately without reloading.
6. Use **Left / Right** to switch panes. Move a selected mod with **Q / LB — Move Up** or **E / RB — Move Down**, or use the buttons. Mod **1** has the highest priority.
7. Press **F / Y — Apply** once you are finished. BO3 reloads the menu with the complete stack.
8. Open **Zombies → Private Game → Change Map**, choose your map, and start.

**Reset Changes** (**R / X**) restores the currently applied selection. **Cancel / Back** (**Esc / B**) discards pending edits. **Clear All** (**U / left stick**) empties the draft; Apply to unload all gameplay mods. Selections have no fixed loader cap, but BO3's zone, asset and memory limits still apply.

The top-right **Press [L] — Loaded Mods** prompt opens a read-only list of the active mods in their applied order. You can also click the prompt or press the right stick on a controller. Close with **Esc / B** or the same shortcut. Pending editor changes do not appear in this list until applied. This UI appears in frontend menus and lobbies only, never in a match.

For duplicate gameplay-mod assets, mod 1 wins over mod 2, then mod 3, across core and Zombies phases. Supported combined precaches and startup adapters retain their combined contributions. Reordering does not make contradictory scripts or game rules automatically compatible.

BO3 uses the last mod in the applied order for its saved-settings profile, which the editor displays. For COLDWAR HEALTH BAR plus TF Options, keep TF last and check its options before starting. The native loader stays active regardless of the settings profile; exit BO3 completely to disable it. Rebuild your stack after each restart.

On build 21201493, Scroptss T7 v3.04 can also start after Workshop Stack is READY at the main menu. Wait for it to finish attaching before starting a match. Attaching during gameplay remains unverified. Downpatching to the exact supported build 21201493 is supported; other executable hashes remain unsupported.

Keep testing to solo/private play: Workshop Stack does not synchronize the complete stack to other players. See COMPATIBILITY.md and VERIFICATION.md for tested combinations and limits.

If activation fails, record the specific error and read `bo3_stack.log` in `steamapps/workshop/content/311210/3798094543`. A local Mod Tools installation uses `mods/bo3_workshop_stack/zone` instead.

Use [GitHub Issues](https://github.com/BKPatt/BO3-Workshop-Stack-Issues/issues/new/choose) for bug reports, feature requests, and setup help. Include mod/map links, applied load order, game/T7 setup, reproduction steps, and relevant logs with personal information removed. Workshop comments remain open for discussion and feedback; please keep reports in GitHub so they can be tracked. Reporting is free. [Patreon](https://www.patreon.com/DejaBOOMcya) offers optional monthly support and extra attention as development time allows, without deadlines or guaranteed fixes.
