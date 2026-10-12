#!/usr/bin/env python3
"""Generate the fixed UI subsets using lv_font_conv 1.5.3 and OFL source fonts."""
import argparse
import re
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "assets/fonts/offduty"

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--converter", required=True)
    parser.add_argument("--cjk", required=True)
    parser.add_argument("--digits", required=True)
    parser.add_argument("--mono", required=True)
    args = parser.parse_args()
    source = (ROOT / "main/offduty_ui.c").read_text()
    symbols = "".join(sorted(set(re.findall(r"[^\x00-\x7f]", source))))
    OUT.mkdir(parents=True, exist_ok=True)
    (OUT / "characters.txt").write_text(symbols + "\n")
    for size in (9, 10, 11, 12, 14, 21, 23, 24, 29, 44, 35, 128, 148):
        digits = size in (35, 128, 148)
        name = f"od_{'digits' if digits else 'font'}_{size}"
        cmd = ["node", args.converter, "--size", str(size), "--bpp", "4",
               "--format", "lvgl", "--font", args.mono if size==35 else args.digits if digits else args.cjk,
               "--symbols", "0123456789:-" if digits else symbols,
               "--no-compress", "--no-kerning", "--lv-include", "lvgl.h", "--lv-font-name", name,
               "-o", str(OUT / f"{name}.c")]
        if not digits:
            cmd += ["--range", "32-126"]
        subprocess.run(cmd, check=True)
        output = OUT / f"{name}.c"
        output.write_text(output.read_text().rstrip() + "\n")

if __name__ == "__main__":
    main()
