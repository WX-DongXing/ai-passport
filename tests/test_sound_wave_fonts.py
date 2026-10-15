"""Check generated cmap coverage and bitmap bounds for both UI font sizes."""
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[1]
FONTS = ROOT / "assets/fonts/sound-wave"


def cmap(source):
    lists = {}
    for name, body in re.findall(r"static const uint16_t (unicode_list_\d+)\[\] = \{(.*?)\};", source, re.S):
        lists[name] = [int(v, 16) for v in re.findall(r"0x([0-9a-f]+)", body)]
    maps = re.search(r"cmaps\[\]\s*=\s*\{(.*?)\n\};", source, re.S).group(1)
    result = set()
    for body in re.findall(r"\{([^{}]+)\}", maps):
        start = int(re.search(r"range_start = (\d+)", body).group(1))
        length = int(re.search(r"range_length = (\d+)", body).group(1))
        name = re.search(r"unicode_list = (\w+)", body).group(1)
        result.update(range(start, start + length) if name == "NULL" else (start + v for v in lists[name]))
    return result


class SoundWaveFontTest(unittest.TestCase):
    def test_inventory_and_ui_text_are_covered(self):
        inventory = (FONTS / "characters.txt").read_text().strip()
        ui = (ROOT / "main/sound_wave_ui.c").read_text()
        required = {ord(c) for c in inventory} | set(range(32, 127))
        required |= {ord(c) for c in ui if '\u4e00' <= c <= '\u9fff'}
        for size in (14, 22):
            source = (FONTS / f"sound_font_{size}.c").read_text()
            coverage = cmap(source)
            self.assertTrue(required <= coverage, f"Missing glyphs: {required - coverage}")
            self.assertNotIn(0x9F98, coverage)
            bitmap = re.search(r"glyph_bitmap\[\] = \{(.*?)\};", source, re.S).group(1)
            bitmap = re.sub(r"/\*.*?\*/", "", bitmap, flags=re.S)
            count = len(re.findall(r"0x[0-9a-f]+", bitmap))
            for index, width, height in re.findall(r"bitmap_index = (\d+).*?box_w = (\d+), .box_h = (\d+)", source):
                self.assertLessEqual(int(index) + (int(width) * int(height) + 1) // 2, count)


if __name__ == "__main__":
    unittest.main()
