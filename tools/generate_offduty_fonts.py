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
    symbols = "".join(sorted(set(re.findall(r"[^\x00-\x7f]", source + "".join(re.findall(r'"([^"]+)"', (ROOT / "main/offduty_theme.h").read_text()))))))
    OUT.mkdir(parents=True, exist_ok=True)
    (OUT / "characters.txt").write_text(symbols + "\n")
    for size in (9, 10, 11, 12, 14, 21, 23, 24, 29, 44, 35, 45, 128, 148):
        digits = size in (35, 45, 128, 148)
        name = f"od_{'digits' if digits else 'font'}_{size}"
        cmd = ["node", args.converter, "--size", str(size), "--bpp", "4",
               "--format", "lvgl", "--font", args.mono if size in (35,45) else args.digits if digits else args.cjk,
               "--symbols", "0123456789:-" if digits else symbols,
               "--no-compress", "--no-kerning", "--lv-include", "lvgl.h", "--lv-font-name", name,
               "-o", str(OUT / f"{name}.c")]
        if not digits:
            cmd += ["--range", "32-126"]
        subprocess.run(cmd, check=True)
        output = OUT / f"{name}.c"
        output.write_text(output.read_text().rstrip() + "\n")
    labels = re.search(r"s_wait_mood\[.*?=\{(.*?)\};", source, re.S).group(1)
    header = "OFF DUTY CLUB / " + "".join(re.findall(r'"(.*?)"', labels))
    for name, size, text in (("od_font_8", 8, header), ("od_wait_title", 29, "本人目前尚未营业。"), ("od_boot_question", 42, "?"), ("od_boot_caption", 8, "POCKET COMPANION"), ("od_digits_24", 24, "0123456789:-")):
        subprocess.run(["node", args.converter, "--size", str(size), "--bpp", "4",
                        "--format", "lvgl", "--font", args.mono if name=="od_digits_24" else args.cjk, "--symbols", text,
                        "--no-compress", "--no-kerning", "--lv-include", "lvgl.h",
                        "--lv-font-name", name, "-o", str(OUT / f"{name}.c")], check=True)
        output = OUT / f"{name}.c"
        output.write_text(output.read_text().rstrip() + "\n")

if __name__ == "__main__":
    main()
