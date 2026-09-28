# Compatibility and reporting

Version 1.1.1, published September 10, 2026, adds Steam build 24784313 support and retains build 21201493. See VERIFICATION.md for exact binaries and checks.

## Game builds and downpatching

The loader selects separate native address and patch profiles by exact executable SHA-256:

| Steam build | Executable SHA-256 |
| --- | --- |
| 24784313 | `51ca63bbc660e0826943c60da67606f6bcb4b3b519528b5e0548c68c9423a323` |
| 21201493 | `9ba98dba41e18ef47de6c63937340f8eae7cb251f8fbc2e78d70047b64aa15b5` |

Support for 21201493 is retained specifically for people downpatching for old T7. Future game updates must add profiles while preserving previously supported builds unless the user changes that policy. This is support for these exact executables; unknown hashes and alternative clients remain unsupported. File identity, mapped-image headers, hook signatures and all native patch bytes are checked before initialization proceeds.

## T7

The Scroptss T7 v3.04 compatibility results below apply to build 21201493. Start T7 first, start BO3 through Steam, and wait for its version to appear before activating Workshop Stack in MODS. Start a new game process when changing either native component. Retaining old T7 support on 21201493 does not mean that old T7 works on 24784313; that combination is unverified.

**Late attachment is fixed in this update:** Scroptss T7 v3.04 can now be started after Workshop Stack is READY at the main menu. The prior freeze was a null exception-continuation pointer inside T7; the loader initializes that missing pointer for the recognized module layout. The late-order test reached round 1 with two mods and One Window Challenge. See T7_FIX_REPORT.md. Attaching during an active match remains untested.

The previous loader rejected native compatibility bytes that T7 had already installed. Version 1.1.0 accepts the exact recognized bytes, leaves them untouched, and restores only changes it owns if initialization fails. Unknown bytes and changed hook functions still cause a refusal. Version 1.1.1 retains those protections separately for each supported build, including downpatching to 21201493.

See VERIFICATION.md for the exact T7 version, load order, and gameplay results tested. Other versions and native patch combinations need separate verification.

## Mod combinations

Workshop Stack has no fixed selection cap. Its buffers grow with the stack, and supported raw-file/script-header pools grow on demand. BO3’s zone, indexed-asset and memory limits remain; CAPACITY_REPORT.md describes the exact scope. Start with two mods that work independently, then add one at a time. A custom map can replace the same scripts, weapons, or HUD as a gameplay mod. Selecting fewer mods or changing order may help, but some combinations need changes by their authors.

Duplicate definitions between selected gameplay mods use the numbered selection priority: [ON 1] first, then [ON 2], and so on, across core/Zombies phases. Map and stock precedence still follows BO3. Combined precaches and supported startup adapters preserve both providers. This selects an active definition; it does not remove complete packages from memory or reconcile incompatible script logic. Clear the stack with U and select again to change priority.

| Combination | Evidence and current status |
| --- | --- |
| COLDWAR HEALTH BAR → TF's Zombie Options (No Weapons) + One Window Challenge | Locally tested in the previous release; both packages and HUD providers ran. See VERIFICATION.md for current release tests. |
| ZM Bots + Presidential Zombies + Director's Cut | One community report of three working together; not independently verified. The same report described crashes when adding more. |
| Bots + Nacht/Verruckt Revamped | Community report of working combinations; not independently verified. |
| Nacht Revamped (3596966808) + TameCoot Ultimis (3543204459) | Streaming, precache and inspected startup fixes were tested in earlier priority binaries in both orders with T7. The dual-build candidate reached round 1 and fired on stock Nacht: Nacht first on 24784313 without T7, and Ultimis first on 21201493 with T7 v3.04. See VERIFICATION.md for exact DLLs and scope, and STREAMING_FIX_REPORT.md for the original diagnosis. |
| Rogue Run / All Around Enhancement / AA Elite / Ultimate Character / UEM / Zombie Plus combinations | Community reports of crashes or lobby freezes; exact pairs, versions, and causes are not yet established. |
| Other character and weapon replacement mods | Community crash reports; arbitrary compiled scripts and assets are not automatically merged. |

Community reports come from the 26 comments reviewed on September 9, 2026. They are observations, not compatibility guarantees. No comments were answered. Creator support policies remain their own; Workshop Stack does not promise support from other mod authors.

The named-pair test uses the preserved Nacht Revamped reupload and the inspected Ultimis source fingerprints. Complete character/voice behavior, co-op and long sessions remain unverified. Ultimis's author describes the pack as intended for custom maps; this crash reproduction used stock Nacht with the Nacht overhaul. Shared precache requests are combined, but contradictory weapon definitions and arbitrary compiled scripts are not. The separate character-plus-weapon report did not identify the weapon mod, so its exact combination remains unverified.

## Useful bug report

Include all of the following so a failure can be reproduced:

- Workshop Stack version and whether it was activated through Workshop or the manual helper.
- Every mod's Workshop link, exact selection order, and the map's Workshop link.
- Solo/co-op, game mode, and the exact step where it fails.
- Game language, Steam build, and whether the executable is downpatched.
- T7 version and whether it finished attaching before Workshop Stack was activated; other native patches, if any.
- The complete error text and the relevant session from `bo3_stack.log` beside the loader DLL. It includes the executable hash, patch counts, selected IDs, and observed asset collisions.
- A crash dump if BO3 produced one. Review logs/dumps before sharing; dumps can include private process data.

For a crash involving several mods, first record whether each mod and the map work alone, then which smallest pair reproduces it. A `CONFLICT` log line identifies differing assets; it is not by itself proof of the crash's cause.
