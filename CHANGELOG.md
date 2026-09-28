# Changes

## 1.1.1 — September 10, 2026

- Add exact Steam executable build 24784313 support with its own native addresses and 1,365 verified compatibility sites.
- Retain build 21201493 and its old-T7 compatibility for deliberate downpatching. Preserve supported builds when adding future updates.
- Share executable profiles between the Workshop DLL and optional manual helper; retain hash, mapped-image, signature and patch ownership checks.
- Add regression coverage for both build identities, wrong-build refusal, and full-profile patch reuse/rollback. Exact live validation is recorded in VERIFICATION.md.

## 1.1.0 — September 10, 2026

All work since the published 1.0.0 belongs to this release. Provisional internal labels 1.1.1 and 1.2.0 were never published; their filenames remain in archived test evidence so earlier results retain their original identities.

- Recognize existing compatible T7 native patches and preserve them. Initialization rollback restores only changes made by Workshop Stack; unknown patch bytes and changed hook functions are refused.
- Fix late attachment of the recognized Scroptss T7 v3.04 DLL by initializing its missing exception-continuation pointer. Preserve non-null pointers and leave T7 files on disk unchanged.
- Include all 12 localized startup fastfiles, including Portuguese `bp_core_mod.ff`.
- Add a two-pane selection editor with immediate toggles, individual removal, reordering, reset/cancel, and one Apply transition for the complete stack.
- Replace the last-mod label with a clickable **Press [L] — Loaded Mods** prompt (right stick on controller). Show the applied order in a read-only list, restricted to frontend menus and lobbies.
- Replace the generic initialization traceback with specific errors for unsupported builds, native hook conflicts, DLL loading, log access and timeouts. Downpatched executables remain unsupported.
- Remove the four-mod selection cap and reject duplicate IDs. New selections append to the draft; selecting an existing entry toggles it off.
- Grow package plans with stable alias storage and expand temporary package-conversion buffers according to the actual batch, while guarding the engine's remaining zone-table limit.
- Grow exhausted raw-file and compiled-script header pools according to actual demand and available physical/commit memory, without moving existing assets.
- Keep secondary mods' XPAK dependency identities separate and preserve their native iterator cursors through loading and unloading.
- Route asynchronous sound-bank reads to the mod that supplied the bank, including both `.sabs` and `.sabl` files.
- Combine same-named mod precache lists so a character mod cannot hide another mod's weapon-registration list. Deduplicate requests without merging the weapon definitions themselves.
- Give the first selected gameplay mod ([ON 1]) highest priority for duplicate asset definitions across loading phases. Preserve native stock/map and placeholder handling, combined compatibility assets, and DB stash/unload behavior. Track sound ownership from the winning asset's actual zone after header swaps.
- Restore earlier selected mods' Zombies packages when BO3 omits them because the last selected mod has only core content. Use the observed native package flags and reset compatibility source lists from the complete expanded load plan.
- Preserve the inspected Nacht Revamped and Ultimis startup routines with an adapter guarded by both original-script fingerprints. Updated or different source versions are left untouched.
- Add script-error call positions and precache diagnostics. Keep diagnostic storage bounded by script headers and reuse identical owned precache unions across reloads.
- Add regression coverage for large selections, package planning, production capacity callbacks, allocation failure, native patch rollback, unsupported-host diagnostics and T7 load orders.

- Publish a public GitHub tracker with bug, feature-request, and setup-support forms; it contains no mod source code or binaries. Add optional Patreon memberships with best-effort support and no deadlines or guaranteed fixes. Replace stale Workshop instructions and direct reports to GitHub while keeping Workshop comments open.

Published to the existing [Steam Workshop item](https://steamcommunity.com/sharedfiles/filedetails/?id=3798094543) on September 10, 2026. See VERIFICATION.md, T7_FIX_REPORT.md, CAPACITY_REPORT.md and STREAMING_FIX_REPORT.md for the exact tested binaries, gameplay results and remaining limitations.
