# T7 late-attachment fix — 1.1.0

This work is part of the unpublished 1.1.0 release. Provisional 1.1.1/1.2.0 labels in evidence refer to internal test builds only.

This local update is not uploaded to Steam. It fixes the Scroptss T7 v3.04 late-attachment failure observed with an earlier local Workshop Stack 1.1.0 candidate. The previously failing order passed a live gameplay test with two mods and a custom map.

## Current priority candidate — 2026-09-10

DLL `6d0092090aaa9aed2d96f625b3b39cd821fdeed7b370300fb7df621fc31d3a4d` passes all twelve native tests, including both isolated real-T7 DLL orders. A fresh T7-first Steam session reached READY through normal Workshop activation, reused 1,365 recognized patch sites, and repaired the continuation pointer. Both orders of the duplicate-asset fixtures passed core and Zombies loading in that session. Ultimis then Nacht Revamped, followed by clear/reload with Nacht then Ultimis, both passed round 1, firing and normal End Game on stock Nacht. Both registered all 174 expected weapons and retained the 484-entry precache union. See VERIFICATION.md, `dev/priority-zonefix-t7-ready-1.1.0.json`, and `dev/priority-gameplay-verification-1.1.0.json` for current evidence. Live late attachment was not repeated on this binary; its earlier results below retain their exact build identities.

## Pre-priority 1.1.0 compatibility regression — 2026-09-10

Pre-priority DLL SHA-256 `c6be1f09d7295477d5cd7437ff344111a4a1747e5801e955a69428467ac9e22f` passed all eleven then-enabled native tests, including both isolated real-T7 DLL orders. A fresh Steam game with T7 v3.04 already running reached READY through normal Workshop activation, reused all 1,365 recognized native patch sites, and repaired the missing continuation pointer to the OS export.

The same final process passed round 1 and Colt M1911 firing with Ultimis then Nacht Revamped on stock Nacht. After clearing the gameplay stack and reloading in the opposite order, it again reached round 1 and fired successfully. Read-only module inspection confirmed T7 remained loaded with the repaired pointer in both runs. See `dev/final-t7-first-ready-1.1.0.json`, the `dev/final-t7-reverse-*` / `dev/final-t7-forward-*` evidence, and VERIFICATION.md. The live late-attachment/custom-map result below belongs to the earlier provisional binary; it was not repeated on this final DLL.

## Cause

The saved failure was an access violation at address zero. Its return address was inside T7's `Protection::ExceptHook`, at the indirect call through `Protection::ZwContinue` (DLL RVA `0x48bc8`). T7's source initializes that member to `NULL` and never assigns it. The separate global `ZwContinue` in `dllmain.cpp` is initialized, but the class handler calls its own null member. See [Protection.cpp](https://github.com/Scroptss/T7Patch-src/blob/2e818a35220a3092b53124dda4ce22024fd8caec/Protection.cpp) and [dllmain.cpp](https://github.com/Scroptss/T7Patch-src/blob/2e818a35220a3092b53124dda4ce22024fd8caec/dllmain.cpp).

The binary agrees with the source: the handler's indirect call is at RVA `0xda6a`, and loading the unmodified release DLL in an isolated process leaves the member zero. The failing game was drawing its active-mod text when the handler ran. T7 then handled the secondary null-call failure by suspending the game. The missing member initialization, rather than overlapping Workshop Stack/T7 hook addresses, explains the captured fatal call.

Tested T7 DLL SHA-256:

```text
bc4c9e4edb895f237477a9074552a8186e69b7d36db4886d983dc13c143645c6
```

## Implementation

Workshop Stack subscribes to Windows DLL-load notifications and also checks an already loaded T7 module. For the known v3.04 layout it initializes the null member to the process's actual `ntdll!ZwContinue` export. The callback runs synchronously during DLL loading, so the pointer is ready before T7 begins installing its hooks. No polling window or extra launcher is involved.

The check requires the matching PE architecture, timestamp, image size, entry point, writable data-section layout, and a fingerprint of the entire 600-byte handler. The handler has no relocations. A different handler/layout is left untouched. An atomic compare-exchange changes only a null pointer; existing non-null values are preserved. This is a runtime compatibility repair, not a change to the T7 DLL on disk or a replacement for T7's protections.

The notification callback performs only memory checks and interlocked operations. It does not allocate, log, acquire locks, call into T7, or call functions in other DLLs. Diagnostics are deferred to a normal game-menu hook, following Microsoft's [callback restrictions](https://learn.microsoft.com/en-us/windows/win32/devnotes/ldrdllnotification).

## Earlier late-attachment verification (provisional 1.1.1 binary)

- Release x64 build passed and all five enabled CTest tests passed.
- The original three tests still pass: selection/routing, patch coexistence/rollback, and actual-DLL unsupported-host refusal.
- New guard tests cover null-only repair, an existing different pointer, missing OS export, 16 concurrent repair attempts, DLL-name checks, and unknown/truncated images.
- The optional real-T7 fixture loads the independently obtained DLL without invoking any game-patching export. It reproduces the null member, verifies repair when T7 is already loaded, verifies three successive loads after listener registration, checks rejection of an altered handler, and verifies listener unregistration.
- In BO3, Workshop Stack reached READY before T7 existed in the process. Starting T7 afterward preserved responsive menus. The log recorded the repair and read-only memory inspection confirmed that the member equals the OS export. The prior crash log did not grow.
- With that same late-attached T7 process, COLDWAR HEALTH BAR and TF's Zombie Options loaded together on One Window Challenge. Round 1 began with 750 points, a randomized Argus, TF's options message, and a six-zombie counter. Firing reduced ammunition from 10 to 9. Both HUD providers executed and the log recorded `HUD combined successfully`. End Game returned normally to the private lobby.
- A second fresh Steam launch with T7 already running reproduced the null member before Workshop Stack activation. Selecting the normal Workshop item loaded version 1.1.1, reached READY, preserved all 1,365 existing T7 patch sites (`applied=0 reused=1365`), and repaired the member to the OS export. This second check covered startup and responsive menus; the full 1.1.1 gameplay test used late attachment.

Late attachment was tested at the main menu after Workshop Stack reached READY. Starting T7 during an active match, long sessions, co-op, and arbitrary mod combinations remain untested. The health-bar mod's individual enemy indicator was not visually verified.

The T7 binary is not distributed with this project. The real-DLL test is enabled only when its local path is explicitly configured using `T7_COMPAT_TEST_DLL` in CMake.

## Evidence

`dev/native-tests-1.1.1.log`, `dev/late-t7-1.1.1-before.json`, `dev/late-t7-1.1.1-stack-ready.json`, `dev/late-t7-1.1.1-attached.json`, `dev/late-t7-1.1.1-gameplay.json`, `dev/late-t7-1.1.1-game.log`, and `dev/late-t7-1.1.1-crash-log-state.json` record the automated tests and live gameplay check. The 1.1.0 failed-run evidence remains preserved for comparison.

`dev/t7-first-1.1.1-before.json`, `dev/t7-first-1.1.1-after.json`, and `dev/t7-first-1.1.1-game.log` record the reverse-order startup check. `research/late-t7-1.1.1-firing.jpg` captures round 1 after firing. Research and test logs are excluded from release packages.
