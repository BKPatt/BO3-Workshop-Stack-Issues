"""BO3's existing one-gameplay-mod + one-map launch path, with live discovery.

This is not an engine patch or a loader for multiple gameplay mods.
Python 3.11+, standard library only. Run without arguments for the desktop UI.
"""
from __future__ import annotations

import argparse
from dataclasses import asdict, dataclass, field
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile
from typing import Any

APP_ID = "311210"
APP_DIR = Path(sys.executable).parent if getattr(sys, "frozen", False) else Path(__file__).resolve().parent
TOKEN = re.compile(r"[a-zA-Z0-9_][a-zA-Z0-9_-]{0,127}\Z")
MAP_TOKEN = re.compile(r"(?:zm|mp|cp)_[a-zA-Z0-9_]+\Z", re.I)
STOCK_NAMES = {
    "zm_zod": "Shadows of Evil", "zm_factory": "The Giant",
    "zm_castle": "Der Eisendrache", "zm_island": "Zetsubou No Shima",
    "zm_stalingrad": "Gorod Krovi", "zm_genesis": "Revelations",
    "zm_prototype": "Nacht der Untoten", "zm_asylum": "Verrückt",
    "zm_sumpf": "Shi No Numa", "zm_theater": "Kino der Toten",
    "zm_cosmodrome": "Ascension", "zm_temple": "Shangri-La",
    "zm_moon": "Moon", "zm_tomb": "Origins",
}


class LauncherError(Exception):
    pass


def read_vdf(path: Path) -> dict[str, Any]:
    """Read Steam's quoted KeyValues format, including comments and escaped paths."""
    source = path.read_text(encoding="utf-8-sig")
    tokens = []
    position = 0
    pattern = re.compile(r'\s+|//[^\n]*|"((?:\\.|[^"\\])*)"|([{}])')
    while position < len(source):
        match = pattern.match(source, position)
        if not match:
            raise ValueError(f"Invalid Steam metadata near character {position}")
        position = match.end()
        if match.group(1) is not None:
            tokens.append(("text", re.sub(r'\\([\\"])', r'\1', match.group(1))))
        elif match.group(2):
            tokens.append((match.group(2), match.group(2)))
    index = 0

    def parse(nested: bool = False) -> dict[str, Any]:
        nonlocal index
        result = {}
        while index < len(tokens):
            kind, key = tokens[index]
            index += 1
            if kind == "}" and nested:
                return result
            if kind != "text" or index >= len(tokens):
                raise ValueError("Malformed Steam metadata")
            kind, value = tokens[index]
            index += 1
            if kind == "{":
                result[key] = parse(True)
            elif kind == "text":
                result[key] = value
            else:
                raise ValueError("Malformed Steam metadata value")
        if nested:
            raise ValueError("Unclosed Steam metadata block")
        return result

    return parse()


def unique_paths(paths: list[Path]) -> list[Path]:
    result = {}
    for path in paths:
        resolved = path.resolve()
        result.setdefault(str(resolved).casefold(), resolved)
    return list(result.values())


def steam_libraries(steam: Path | None = None) -> list[Path]:
    candidates = [steam] if steam else []
    if not steam and sys.platform == "win32":
        import winreg
        try:
            with winreg.OpenKey(winreg.HKEY_CURRENT_USER, r"Software\Valve\Steam") as key:
                candidates.append(Path(winreg.QueryValueEx(key, "SteamPath")[0]))
        except OSError:
            pass
        candidates.append(Path(os.environ.get("ProgramFiles(x86)", "C:/Program Files (x86)")) / "Steam")
    roots = [p for p in candidates if p and p.is_dir()]
    for root in list(roots):
        try:
            entries = read_vdf(root / "steamapps/libraryfolders.vdf").get("libraryfolders", {})
            for key, value in entries.items():
                if key.isdigit():
                    location = value.get("path") if isinstance(value, dict) else value
                    if isinstance(location, str):
                        roots.append(Path(location))
        except (OSError, ValueError, AttributeError):
            continue
    return unique_paths(roots)


def find_game(libraries: list[Path]) -> Path | None:
    for root in libraries:
        try:
            state = read_vdf(root / f"steamapps/appmanifest_{APP_ID}.acf")["AppState"]
            name = state["installdir"]
            if not isinstance(name, str) or Path(name).name != name or name in (".", ".."):
                continue
            game = root / "steamapps/common" / name
            if (game / "BlackOps3.exe").is_file():
                return game.resolve()
        except (OSError, ValueError, KeyError, TypeError):
            continue
    return None


@dataclass(frozen=True)
class Item:
    key: str
    title: str
    kind: str
    origin: str
    path: Path
    token: str
    modes: tuple[str, ...] = ()

    @property
    def label(self) -> str:
        return f"{self.title}  [{self.origin}: {self.token}]"


@dataclass
class Library:
    game: Path | None
    items: list[Item] = field(default_factory=list)
    warnings: list[str] = field(default_factory=list)
    steam_exe: Path | None = None

    def of_kind(self, kind: str) -> list[Item]:
        return sorted((i for i in self.items if i.kind == kind), key=lambda i: i.title.casefold())

    def get(self, key: str, kind: str) -> Item | None:
        if not key:
            return None
        for item in self.items:
            if item.key == key and item.kind == kind:
                return item
        raise LauncherError(f"The saved {kind} is no longer installed: {key}. Refresh and select another.")


def inspect_item(folder: Path, origin: str, expected: str | None, library: Library) -> None:
    """Metadata stays data. Never execute Workshop files or trust paths from JSON."""
    content = folder / "zone" if (folder / "zone").is_dir() else folder
    metadata = content / "workshop.json"
    if not metadata.is_file():
        metadata = folder / "workshop.json"
    data = {}
    if metadata.is_file():
        try:
            data = json.loads(metadata.read_text(encoding="utf-8-sig"))
            if not isinstance(data, dict):
                raise ValueError("workshop.json must be an object")
        except (OSError, ValueError) as exc:
            library.warnings.append(f"Skipped {folder.name}: {exc}")
            return
    kind = str(data.get("Type", expected or "")).casefold()
    if kind not in ("mod", "map"):
        library.warnings.append(f"Skipped {folder.name}: missing/unknown item Type.")
        return
    if expected and kind != expected:
        library.warnings.append(f"Skipped {folder.name}: a {kind} is in the {expected} directory.")
        return
    title = str(data.get("Title") or folder.name).replace("\n", " ").replace("\r", " ")[:180]
    tags = str(data.get("Tags", "")).casefold().split(",")
    modes = tuple(mode for tag, mode in (("zombies", "zm"), ("multiplayer", "mp"), ("campaign", "cp")) if tag in [t.strip() for t in tags])
    if kind == "mod":
        token = folder.name
        if not TOKEN.fullmatch(token) or (origin == "Workshop" and not token.isdigit()):
            library.warnings.append(f"Skipped {title}: invalid mod directory name.")
            return
        if not any((content / f"{mode}_mod.ff").is_file() for mode in ("core", "zm", "mp", "cp")):
            library.warnings.append(f"Skipped {title}: no compiled mod fastfiles yet.")
            return
    else:
        token = data.get("FolderName", folder.name)
        if not isinstance(token, str) or not MAP_TOKEN.fullmatch(token):
            library.warnings.append(f"Skipped {title}: invalid or missing map FolderName.")
            return
        if not (content / f"{token}.ff").is_file():
            library.warnings.append(f"Skipped {title}: {token}.ff is missing (download/build incomplete).")
            return
        modes = (token[:2].lower(),)
    library.items.append(Item(f"{origin.lower()}:{kind}:{folder.name}", title, kind, origin, folder.resolve(), token, modes))


def discover(game: Path | None = None, steam: Path | None = None) -> Library:
    roots = steam_libraries(steam)
    game = game.resolve() if game else find_game(roots)
    library = Library(game)
    library.steam_exe = next((root / "steam.exe" for root in roots if (root / "steam.exe").is_file()), None)
    if game:
        # An explicitly selected game might be in a library absent from libraryfolders.vdf.
        if game.parent.name.casefold() == "common" and game.parent.parent.name.casefold() == "steamapps":
            roots = unique_paths(roots + [game.parent.parent.parent])
    directories = [(root / f"steamapps/workshop/content/{APP_ID}", "Workshop", None) for root in roots]
    if game:
        directories += [(game / "mods", "Local", "mod"), (game / "usermaps", "Local", "map")]
        for token, title in STOCK_NAMES.items():
            if (game / "zone" / f"{token}.ff").is_file():
                library.items.append(Item(f"stock:map:{token}", title, "map", "Built-in", game / "zone", token, ("zm",)))
    for directory, origin, expected in directories:
        if not directory.is_dir():
            continue
        try:
            for folder in sorted(directory.iterdir()):
                if folder.is_dir():
                    inspect_item(folder, origin, expected, library)
        except OSError as exc:
            library.warnings.append(f"Could not scan {directory}: {exc}")
    # Same map basename from different packages is ambiguous to the engine.
    map_tokens: dict[str, Item] = {}
    duplicates = set()
    for item in library.of_kind("map"):
        token = item.token.casefold()
        if token in map_tokens:
            duplicates.add(token)
        map_tokens[token] = item
    if duplicates:
        library.warnings.append("Maps hidden because multiple packages use the same map name: " + ", ".join(sorted(duplicates)))
        library.items = [i for i in library.items if i.kind != "map" or i.token.casefold() not in duplicates]
    keys: dict[str, int] = {}
    for item in library.items:
        keys[item.key] = keys.get(item.key, 0) + 1
    repeated = {key for key, count in keys.items() if count > 1}
    if repeated:
        library.warnings.append("Items hidden because they exist in multiple library locations: " + ", ".join(sorted(repeated)))
        library.items = [i for i in library.items if i.key not in repeated]
    return library


def launch_command(library: Library, mod_key: str, map_key: str = "", *, menu: bool = False) -> list[str]:
    game = library.game
    if not game or not (game / "BlackOps3.exe").is_file():
        raise LauncherError("Select the BO3 folder containing BlackOps3.exe.")
    mod = library.get(mod_key, "mod")
    map_item = library.get(map_key, "map")
    if not menu and not map_item:
        raise LauncherError("Choose a map to start a solo game, or use Open modded menu.")
    if mod and map_item and mod.modes and map_item.modes[0] not in mod.modes:
        raise LauncherError("The mod's Workshop tags do not match this map's game mode.")
    fs_game = mod.token if mod else "usermaps"
    if not TOKEN.fullmatch(fs_game):
        raise LauncherError("Invalid mod launch name.")
    command = ([str(library.steam_exe), "-applaunch", APP_ID] if library.steam_exe
               else [str(game / "BlackOps3.exe")])
    command += ["+set", "fs_game", fs_game]
    if map_item and not menu:
        if not MAP_TOKEN.fullmatch(map_item.token):
            raise LauncherError("Invalid map launch name.")
        command += ["+devmap", map_item.token]
    return command


def game_running() -> bool:
    if sys.platform != "win32":
        return False
    result = subprocess.run(["tasklist", "/FI", "IMAGENAME eq BlackOps3.exe", "/FO", "CSV", "/NH"],
                            capture_output=True, text=True, creationflags=subprocess.CREATE_NO_WINDOW, check=True)
    return '"blackops3.exe"' in result.stdout.casefold()


def launch(library: Library, mod_key: str, map_key: str = "", *, menu: bool = False) -> list[str]:
    command = launch_command(library, mod_key, map_key, menu=menu)
    if game_running():
        raise LauncherError("BO3 is already running. Exit it normally before changing a launch profile.")
    subprocess.Popen(command, cwd=library.game, shell=False)
    return command


def load_profiles(path: Path) -> dict[str, Any]:
    if not path.exists():
        return {"version": 1, "game": "", "profiles": {}}
    data = json.loads(path.read_text(encoding="utf-8-sig"))
    if not isinstance(data, dict) or data.get("version") != 1 or not isinstance(data.get("profiles"), dict):
        raise LauncherError("Invalid profiles.json. Restore it or choose a new --profiles file.")
    if not isinstance(data.get("game", ""), str):
        raise LauncherError("Invalid game folder in profiles.json.")
    for name, profile in data["profiles"].items():
        if not isinstance(name, str) or not isinstance(profile, dict) or not all(isinstance(profile.get(k, ""), str) for k in ("mod", "map")):
            raise LauncherError("Invalid saved profile.")
    return data


def save_profiles(path: Path, data: dict[str, Any]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = None
    try:
        with tempfile.NamedTemporaryFile(mode="w", encoding="utf-8", dir=path.parent, prefix="profiles-", suffix=".tmp", delete=False) as stream:
            temporary = Path(stream.name)
            json.dump(data, stream, indent=2, ensure_ascii=False)
            stream.write("\n")
        os.replace(temporary, path)
    finally:
        if temporary and temporary.exists():
            temporary.unlink()


def run_gui(profiles_path: Path, game: Path | None, steam: Path | None) -> None:
    import tkinter as tk
    from tkinter import filedialog, messagebox, ttk

    class App(tk.Tk):
        def __init__(self):
            super().__init__()
            self.title("BO3 • Mod + Map Launcher")
            self.geometry("980x760")
            self.minsize(840, 700)
            self.configure(bg="#101317")
            style = ttk.Style(self)
            style.theme_use("clam")
            style.configure("TFrame", background="#101317")
            style.configure("TLabel", background="#101317", foreground="#e5e7eb", font=("Segoe UI", 10))
            style.configure("Title.TLabel", font=("Segoe UI Semibold", 26), foreground="#ff9b42")
            style.configure("Muted.TLabel", foreground="#a9b3c0")
            style.configure("TButton", padding=(12, 8), font=("Segoe UI", 10))
            style.configure("TCombobox", padding=7, font=("Segoe UI", 10))
            style.configure("TEntry", padding=7)
            self.data = load_profiles(profiles_path)
            self.game_var = tk.StringVar(value=str(game or self.data.get("game") or ""))
            self.profile_var = tk.StringVar()
            self.mod_var = tk.StringVar()
            self.map_var = tk.StringVar()
            self.status_var = tk.StringVar()
            panel = ttk.Frame(self, padding=26)
            panel.pack(fill="both", expand=True)
            panel.columnconfigure(1, weight=1)
            ttk.Label(panel, text="MOD + MAP", style="Title.TLabel").grid(row=0, column=0, columnspan=3, sticky="w")
            ttk.Label(panel, text="Choose a gameplay mod. Choose a map. Play them together.", style="Muted.TLabel").grid(row=1, column=0, columnspan=3, sticky="w", pady=(2, 14))
            ttk.Label(panel, text="Uses BO3’s existing mod + custom map support. Multiple gameplay mods are not supported.", wraplength=880).grid(row=2, column=0, columnspan=3, sticky="w", pady=(0, 20))
            ttk.Label(panel, text="BO3 folder").grid(row=3, column=0, sticky="w", padx=(0, 12))
            ttk.Entry(panel, textvariable=self.game_var).grid(row=3, column=1, sticky="ew")
            ttk.Button(panel, text="Browse…", command=self.browse).grid(row=3, column=2, padx=(10, 0))
            ttk.Label(panel, text="Saved profile").grid(row=4, column=0, sticky="w", pady=12)
            self.profile_box = ttk.Combobox(panel, textvariable=self.profile_var)
            self.profile_box.grid(row=4, column=1, sticky="ew")
            self.profile_box.bind("<<ComboboxSelected>>", self.restore)
            ttk.Button(panel, text="Save profile", command=self.save).grid(row=4, column=2, padx=(10, 0))
            ttk.Label(panel, text="Gameplay mod").grid(row=5, column=0, sticky="w", pady=12)
            self.mod_box = ttk.Combobox(panel, textvariable=self.mod_var, state="readonly")
            self.mod_box.grid(row=5, column=1, columnspan=2, sticky="ew")
            ttk.Label(panel, text="Map").grid(row=6, column=0, sticky="w", pady=12)
            self.map_box = ttk.Combobox(panel, textvariable=self.map_var, state="readonly")
            self.map_box.grid(row=6, column=1, columnspan=2, sticky="ew")
            actions = ttk.Frame(panel)
            actions.grid(row=7, column=0, columnspan=3, sticky="ew", pady=(18, 12))
            ttk.Button(actions, text="Refresh downloads", command=self.refresh).pack(side="left")
            ttk.Button(actions, text="Open modded menu", command=lambda: self.start(True)).pack(side="right")
            ttk.Button(actions, text="Try direct map launch", command=lambda: self.start(False)).pack(side="right", padx=10)
            ttk.Label(panel, textvariable=self.status_var, style="Muted.TLabel", wraplength=880).grid(row=8, column=0, columnspan=3, sticky="w", pady=(0, 12))
            self.log = tk.Text(panel, height=10, bg="#1b2027", fg="#ccd5df", relief="flat", wrap="word", font=("Consolas", 10), padx=14, pady=12, state="disabled")
            self.log.grid(row=9, column=0, columnspan=3, sticky="nsew")
            panel.rowconfigure(9, weight=1)
            ttk.Label(panel, text="Co-op: open the modded menu, host a Private Game, then select the custom map.\nAll players need the same downloads. A mod and map can still conflict.", style="Muted.TLabel", wraplength=880).grid(row=10, column=0, columnspan=3, sticky="w", pady=(16, 0))
            self.refresh()

        def report(self, text):
            self.log.configure(state="normal")
            self.log.insert("end", text + "\n\n")
            self.log.see("end")
            self.log.configure(state="disabled")

        def browse(self):
            folder = filedialog.askdirectory(title="Choose the folder containing BlackOps3.exe", parent=self)
            if folder:
                self.game_var.set(folder)
                self.refresh()

        def refresh(self):
            try:
                previous_mod, previous_map = self.mod_var.get(), self.map_var.get()
                self.library = discover(Path(self.game_var.get()) if self.game_var.get().strip() else None, steam)
                if self.library.game:
                    self.game_var.set(str(self.library.game))
                self.mods = {"No gameplay mod (custom maps enabled)": ""}
                self.mods.update({i.label: i.key for i in self.library.of_kind("mod")})
                self.maps = {"Choose in game": ""}
                self.maps.update({i.label: i.key for i in self.library.of_kind("map")})
                self.mod_box["values"] = list(self.mods)
                self.map_box["values"] = list(self.maps)
                self.mod_var.set(previous_mod if previous_mod in self.mods else next(iter(self.mods)))
                self.map_var.set(previous_map if previous_map in self.maps else next(iter(self.maps)))
                self.profile_box["values"] = list(self.data["profiles"])
                custom = sum(i.kind == "map" and i.origin != "Built-in" for i in self.library.items)
                self.status_var.set(f"{len(self.mods)-1} gameplay mod(s) • {custom} custom map(s) • {len(self.maps)-1-custom} built-in Zombies map(s)")
                for warning in self.library.warnings:
                    self.report(warning)
                if not custom:
                    self.report("No downloaded custom maps found. Subscribe to a map in Steam Workshop, wait for its download to finish, then Refresh downloads. Maps do not occupy the gameplay-mod slot.")
            except (LauncherError, OSError, ValueError) as exc:
                messagebox.showerror("Scan failed", str(exc), parent=self)

        def restore(self, _event=None):
            profile = self.data["profiles"].get(self.profile_var.get())
            if not profile:
                return
            try:
                self.library.get(profile.get("mod", ""), "mod")
                self.library.get(profile.get("map", ""), "map")
                self.mod_var.set(next(label for label, key in self.mods.items() if key == profile.get("mod", "")))
                self.map_var.set(next(label for label, key in self.maps.items() if key == profile.get("map", "")))
            except LauncherError as exc:
                messagebox.showerror("Profile unavailable", str(exc), parent=self)

        def save(self):
            name = self.profile_var.get().strip()
            if not name:
                messagebox.showinfo("Name your profile", "Type a name in Saved profile, then click Save profile.", parent=self)
                return
            self.data["game"] = self.game_var.get()
            self.data["profiles"][name] = {"mod": self.mods[self.mod_var.get()], "map": self.maps[self.map_var.get()]}
            try:
                save_profiles(profiles_path, self.data)
                self.profile_box["values"] = list(self.data["profiles"])
                self.report(f"Saved profile: {name}")
            except OSError as exc:
                messagebox.showerror("Save failed", str(exc), parent=self)

        def start(self, menu):
            try:
                mod_key, map_key = self.mods[self.mod_var.get()], self.maps[self.map_var.get()]
                # Refresh again: never launch a removed item or an obsolete executable path.
                fresh = discover(Path(self.game_var.get()) if self.game_var.get() else None, steam)
                command = launch(fresh, mod_key, map_key, menu=menu)
                self.report("Launch requested:\n" + subprocess.list2cmdline(command))
                self.report("Steam may show 'Launch Game with custom arguments'. Select Continue for this requested launch. If BO3 does not appear, check the Steam window.")
                if not menu:
                    self.report("Direct map launch uses BO3's developer launch command. Some builds return to the menu instead; select the saved map in game if this happens.")
                chosen = fresh.get(map_key, "map")
                self.report("In BO3: Zombies → Private Game → Change Map. Select " + (chosen.title if chosen else "your custom map") + ". Leave the gameplay mod loaded.")
            except (LauncherError, OSError, subprocess.SubprocessError) as exc:
                messagebox.showerror("Could not start BO3", str(exc), parent=self)

    app = App()
    app.mainloop()


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--game", type=Path, help="BO3 install folder; detected automatically by default")
    parser.add_argument("--steam", type=Path, help="Steam root; detected from the Windows registry by default")
    parser.add_argument("--profiles", type=Path, default=APP_DIR / "profiles.json")
    sub = parser.add_subparsers(dest="action")
    sub.add_parser("scan", help="List installed mods, maps, and diagnostics as JSON")
    run = sub.add_parser("launch", help="Launch an installed mod + map")
    run.add_argument("--mod", default="", help="Item key from scan, e.g. workshop:mod:2602139181")
    run.add_argument("--map", default="", help="Item key from scan")
    run.add_argument("--menu", action="store_true", help="Open the modded menu for co-op; choose the map there")
    run.add_argument("--dry-run", action="store_true", help="Print the command without starting BO3")
    args = parser.parse_args(argv)
    try:
        if args.action is None:
            run_gui(args.profiles, args.game, args.steam)
        else:
            library = discover(args.game, args.steam)
            if args.action == "scan":
                print(json.dumps({"game": library.game, "items": [asdict(i) for i in library.items], "warnings": library.warnings}, default=str, indent=2, ensure_ascii=False))
            else:
                method = launch_command if args.dry_run else launch
                command = method(library, args.mod, args.map, menu=args.menu)
                print(subprocess.list2cmdline(command))
        return 0
    except (LauncherError, OSError, ValueError, subprocess.SubprocessError) as exc:
        if args.action is None:
            from tkinter import messagebox
            messagebox.showerror("BO3 launcher", str(exc))
        else:
            print(f"Error: {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
