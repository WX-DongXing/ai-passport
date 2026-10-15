#!/usr/bin/env python3
"""Generate the Sound Wave UI subsets with lv_font_conv 1.5.3."""
import argparse
import json
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "assets/fonts/sound-wave"


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--converter", required=True)
    args = parser.parse_args()
    converter = Path(args.converter).resolve()
    package = json.loads((converter.parent / "package.json").read_text())
    if package["version"] != "1.5.3":
        raise SystemExit("Use lv_font_conv 1.5.3")
    symbols = (OUT / "characters.txt").read_text().strip()
    for size in (14, 22):
        subprocess.run(["node", str(converter), "--font", str(OUT / "SoundWaveSans.otf"),
                        "--symbols", symbols, "--range", "32-126", "--size", str(size),
                        "--bpp", "4", "--format", "lvgl", "--no-compress", "--no-kerning",
                        "--lv-include", "lvgl.h", "--lv-font-name", f"sound_font_{size}",
                        "--output", str(OUT / f"sound_font_{size}.c")], check=True)
        generated = OUT / f"sound_font_{size}.c"
        generated.write_text(generated.read_text().rstrip() + "\n")


if __name__ == "__main__":
    main()
