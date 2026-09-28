# Third-party notices

The Workshop Lua fastfiles are built using **L3akMod v1.0.4 by the D3V Team (DTZxPorter, SE2Dev, Nukem)**. The compiler extension is installed only in the local Mod Tools; its DLL is not included in the Workshop item or loader release. [Author's installation page](https://dtzxporter.com/tools/l3akmod).

The native Lua loading convention was checked against [T7Overcharged](https://github.com/JariKCoding/T7Overcharged). This project's bootstrap and native extension are separate implementations; no T7Overcharged binary is included.

MinHook v1.3.4 (commit `c3fcafdc10146beb5919319d0683e44e3c30d537`) is statically linked into `bo3_stack.dll`.
Its BSD-style redistribution terms and embedded HDE notices are reproduced in `MinHook-LICENSE.txt` in the binary package and in `native/third_party/minhook/LICENSE.txt` in source.
Source: https://github.com/TsudaKageyu/minhook

The supported-build address list was checked against the installed executable using the public address facts in [T7Patch-src Arxan.h](https://github.com/Scroptss/T7Patch-src/blob/2e818a35220a3092b53124dda4ce22024fd8caec/Arxan.h).
The compatibility writer, validation, and thread handling are implemented in this project. No T7Patch binary is distributed.

Version 1.1.1 also uses the public `Protection::ExceptHook` / `Protection::ZwContinue` definitions from that T7Patch-src revision to identify the missing continuation initialization in Scroptss T7 v3.04. Workshop Stack's module notification adapter, layout/fingerprint validation, and null-only initialization are implemented here. This repair changes a pointer only in the running process; the T7 DLL on disk remains unchanged. The source and manual-download packages include T7_FIX_REPORT.md with the diagnosis and tests.

Public BO3 data-structure and Lua API references were consulted in [boiii-free](https://github.com/Ezz-lol/boiii-free/tree/03702428c7f1e793e289610a915acb3249ac21e9/src/client/game).
The loader uses only the installed Steam game; it does not distribute a replacement game client.

Version 1.2.0's asset-pool and zone layout investigation also consulted those public BOIII structures and the T7Overcharged project. The dynamic package buffers and demand-based free-list growth are independently implemented here and verified against the supported executable. Neither project's pool-resizing code is copied or distributed. The source and manual-download packages include CAPACITY_REPORT.md with the precise scope and evidence.

TF's Zombie Options, COLDWAR HEALTH BAR, One Window Challenge, and BO3 assets are not included. Users obtain mods from their creators through Steam Workshop. Runtime HUD copies remain in the local game process.

Call of Duty and Black Ops are trademarks of their respective owners. This is an independent project.
