#!/usr/bin/env python3
"""Host checks for the semantic theme-layer contract."""

import json
from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[1]
MANIFEST = ROOT / "assets/themes/derived/theme-layers.json"
THEMES = ("pixel", "watercolor", "realistic", "ink", "starlight")
ICONS = ("water", "leaf", "fruit", "mic", "play", "stop", "back", "up", "down", "confirm")


class ThemeLayerManifestTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.data = json.loads(MANIFEST.read_text(encoding="utf-8"))

    def test_manifest_has_all_runtime_themes(self):
        self.assertEqual(set(THEMES), set(self.data["themes"]))

    def test_each_theme_has_five_stages_and_controls(self):
        for theme in THEMES:
            entry = self.data["themes"][theme]
            self.assertEqual({f"stage_{i}" for i in range(5)}, set(entry["stage"]))
            self.assertEqual({f"icon_{name}" for name in ICONS}, set(entry["icon"]))
            for asset in list(entry["stage"].values()) + list(entry["icon"].values()):
                path = ROOT / asset["firmware"]
                self.assertTrue(path.is_file(), path)
                self.assertEqual(path.stat().st_size, asset["bytes"])
                self.assertGreater(asset["bytes"], 0)

    def test_list_rows_do_not_allocate_clip_layers(self):
        source = (ROOT / "main" / "worry_seed_app.c").read_text(encoding="utf-8")
        self.assertNotIn("lv_obj_set_style_clip_corner", source)


if __name__ == "__main__":
    unittest.main()
