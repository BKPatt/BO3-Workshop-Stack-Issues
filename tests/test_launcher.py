import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

import bo3_launcher as bo3


class LauncherTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.steam = self.root / "Steam with spaces"
        self.game = self.steam / "steamapps/common/Call of Duty Black Ops III"
        self.game.mkdir(parents=True)
        (self.game / "BlackOps3.exe").write_bytes(b"fake exe; never run")
        (self.game / "zone").mkdir()
        (self.game / "zone/zm_zod.ff").write_bytes(b"zone")
        (self.steam / "steamapps/appmanifest_311210.acf").write_text('"AppState" { "installdir" "Call of Duty Black Ops III" }')
        self.workshop = self.steam / "steamapps/workshop/content/311210"

    def item(self, ident, kind="mod", folder_name=None, **changes):
        folder = self.workshop / ident
        folder.mkdir(parents=True, exist_ok=True)
        data = {"Title": "Test " + ident, "Type": kind, "FolderName": folder_name or ident, "Tags": "Mod,Zombies"}
        data.update(changes)
        (folder / "workshop.json").write_text(json.dumps(data), encoding="utf-8-sig")
        (folder / ("zm_mod.ff" if kind == "mod" else (folder_name or ident) + ".ff")).write_bytes(b"ff")
        return folder

    def scan(self):
        return bo3.discover(steam=self.steam)

    def test_discovers_separate_workshop_mod_and_map(self):
        self.item("100")
        self.item("200", "map", "zm_custom")
        result = self.scan()
        self.assertEqual(result.game, self.game.resolve())
        self.assertEqual(len(result.of_kind("mod")), 1)
        self.assertEqual(len(result.of_kind("map")), 2)
        self.assertEqual(result.warnings, [])

    def test_command_uses_workshop_id_for_mod_but_foldername_for_map(self):
        self.item("100", FolderName="authors_mod_name")
        self.item("200", "map", "zm_custom")
        command = bo3.launch_command(self.scan(), "workshop:mod:100", "workshop:map:200")
        self.assertEqual(command, [str(self.game / "BlackOps3.exe"), "+set", "fs_game", "100", "+devmap", "zm_custom"])

    def test_coop_menu_does_not_start_a_solo_game(self):
        self.item("100")
        command = bo3.launch_command(self.scan(), "workshop:mod:100", "stock:map:zm_zod", menu=True)
        self.assertNotIn("+devmap", command)
        self.assertEqual(command[-1], "100")

    def test_prefers_steam_applaunch_when_available(self):
        (self.steam / "steam.exe").write_bytes(b"fake Steam; never run")
        self.item("100")
        command = bo3.launch_command(self.scan(), "workshop:mod:100", "stock:map:zm_zod")
        self.assertEqual(command, [str(self.steam / "steam.exe"), "-applaunch", "311210", "+set", "fs_game", "100", "+devmap", "zm_zod"])

    def test_no_mod_still_enables_custom_maps(self):
        self.assertEqual(bo3.launch_command(self.scan(), "", "stock:map:zm_zod")[2:4], ["fs_game", "usermaps"])

    def test_invalid_or_missing_selection_never_silently_falls_back(self):
        for mod, map_key in (("workshop:mod:gone", "stock:map:zm_zod"), ("", "workshop:map:gone"), ("", "")):
            with self.subTest(mod=mod, map=map_key), self.assertRaises(bo3.LauncherError):
                bo3.launch_command(self.scan(), mod, map_key)

    def test_incomplete_download_is_not_launchable(self):
        folder = self.item("200", "map", "zm_custom")
        (folder / "zm_custom.ff").unlink()
        result = self.scan()
        self.assertEqual(len(result.of_kind("map")), 1)
        self.assertIn("incomplete", result.warnings[0])

    def test_malformed_metadata_does_not_hide_other_downloads(self):
        folder = self.item("100")
        (folder / "workshop.json").write_text("[]")
        self.item("200", "map", "zm_custom")
        result = self.scan()
        self.assertEqual(len(result.of_kind("mod")), 0)
        self.assertEqual(len(result.of_kind("map")), 2)
        self.assertTrue(result.warnings)

    def test_metadata_cannot_inject_launch_commands_or_paths(self):
        folder = self.item("200", "map", "zm_custom")
        for payload in ("../../outside", "zm_custom +quit", "zm_custom;quit", "zm_custom\nquit", "C:/zm_custom"):
            with self.subTest(payload=payload):
                (folder / "workshop.json").write_text(json.dumps({"Type": "map", "FolderName": payload}))
                self.assertEqual(len(self.scan().of_kind("map")), 1)

    def test_mode_mismatch_is_rejected(self):
        self.item("100", Tags="Mod,Multiplayer")
        with self.assertRaisesRegex(bo3.LauncherError, "game mode"):
            bo3.launch_command(self.scan(), "workshop:mod:100", "stock:map:zm_zod")

    def test_duplicate_map_names_are_hidden_instead_of_loading_wrong_map(self):
        self.item("200", "map", "zm_custom")
        self.item("201", "map", "zm_custom")
        result = self.scan()
        self.assertEqual(len(result.of_kind("map")), 1)
        self.assertIn("multiple packages", result.warnings[0])

    def test_local_mod_zone_layout(self):
        content = self.game / "mods/local_mod/zone"
        content.mkdir(parents=True)
        (content / "zm_mod.ff").write_bytes(b"ff")
        result = self.scan()
        self.assertEqual(result.of_kind("mod")[0].token, "local_mod")
        self.assertEqual(bo3.launch_command(result, "local:mod:local_mod", menu=True)[-1], "local_mod")

    def test_multiple_steam_libraries_and_escaped_vdf(self):
        secondary = self.root / "Second library"
        secondary.mkdir()
        escaped = str(secondary).replace("\\", "\\\\")
        (self.steam / "steamapps/libraryfolders.vdf").write_text('// test comment\n"libraryfolders" { "0" { "path" "' + escaped + '" } }')
        roots = bo3.steam_libraries(self.steam)
        self.assertIn(secondary.resolve(), roots)
        # Downloads in a second Steam library must also be discovered.
        self.workshop = secondary / "steamapps/workshop/content/311210"
        self.item("300", "map", "zm_second")
        self.assertIsNotNone(self.scan().get("workshop:map:300", "map"))

    def test_profiles_roundtrip_unicode_and_replace(self):
        path = self.root / "profiles.json"
        first = {"version": 1, "game": str(self.game), "profiles": {"Co-op ✨": {"mod": "", "map": "stock:map:zm_zod"}}}
        bo3.save_profiles(path, first)
        self.assertEqual(bo3.load_profiles(path), first)
        first["profiles"]["Another"] = {"mod": "", "map": ""}
        bo3.save_profiles(path, first)
        self.assertEqual(bo3.load_profiles(path), first)
        self.assertEqual(list(self.root.glob("profiles-*.tmp")), [])

    def test_same_mod_in_two_libraries_is_ambiguous(self):
        self.item("100")
        secondary = self.root / "Second library"
        secondary.mkdir()
        escaped = str(secondary).replace("\\", "\\\\")
        (self.steam / "steamapps/libraryfolders.vdf").write_text('"libraryfolders" { "1" { "path" "' + escaped + '" } }')
        self.workshop = secondary / "steamapps/workshop/content/311210"
        self.item("100")
        result = self.scan()
        self.assertEqual(result.of_kind("mod"), [])
        self.assertIn("multiple library locations", result.warnings[0])

    def test_profiles_reject_corruption(self):
        path = self.root / "profiles.json"
        for value in ([], {"version": 2, "profiles": {}}, {"version": 1, "profiles": {"bad": []}}):
            path.write_text(json.dumps(value))
            with self.assertRaises(bo3.LauncherError):
                bo3.load_profiles(path)

    def test_launch_refuses_running_game_without_killing_it(self):
        with patch.object(bo3, "game_running", return_value=True), patch.object(bo3.subprocess, "Popen") as popen:
            with self.assertRaisesRegex(bo3.LauncherError, "already running"):
                bo3.launch(self.scan(), "", "stock:map:zm_zod")
            popen.assert_not_called()

    def test_launch_passes_arguments_as_array_without_shell(self):
        self.item("100")
        with patch.object(bo3, "game_running", return_value=False), patch.object(bo3.subprocess, "Popen") as popen:
            command = bo3.launch(self.scan(), "workshop:mod:100", "stock:map:zm_zod")
            popen.assert_called_once_with(command, cwd=self.game.resolve(), shell=False)


if __name__ == "__main__":
    unittest.main()
