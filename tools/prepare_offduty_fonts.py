#!/usr/bin/env python3
"""Prepare shared OFL subsets; requires fonttools 4.60.1 and brotli 1.1.0."""
import argparse
import math
import re
from pathlib import Path
from fontTools import subset
from fontTools.ttLib import TTFont
from fontTools.varLib.instancer import instantiateVariableFont

ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'assets/fonts/offduty'

def rename(font,name):
    for record in font['name'].names:
        if record.nameID in (1,2,3,4,6):
            value='Regular' if record.nameID==2 else name
            record.string=value.encode(record.getEncoding())

def save(font,name,characters):
    opts=subset.Options();opts.desubroutinize=True
    sub=subset.Subsetter(options=opts);sub.populate(text=characters);sub.subset(font)
    rename(font,name)
    path=OUT/(name+('.otf' if 'CFF ' in font else '.ttf'))
    font.save(path)
    font.flavor='woff2';font.save(OUT/(name+'.woff2'))
    return path

def main():
    parser=argparse.ArgumentParser();parser.add_argument('--cjk',required=True);parser.add_argument('--anton',required=True);parser.add_argument('--mono',required=True)
    args=parser.parse_args();OUT.mkdir(parents=True,exist_ok=True)
    chars=''.join(sorted(set(re.findall(r'[^\x00-\x7f]',(ROOT/'main/offduty_ui.c').read_text()))))+''.join(map(chr,range(32,127)))
    cjk=TTFont(args.cjk);missing=set(map(ord,chars))-set(cjk.getBestCmap());assert not missing,missing
    save(cjk,'OffDutySans',chars)
    anton=TTFont(args.anton)
    for name in anton.getGlyphOrder():
        glyph=anton['glyf'][name]
        if hasattr(glyph,'coordinates'):
            for i,(x,y) in enumerate(glyph.coordinates):glyph.coordinates[i]=(round(x+math.tan(math.radians(6))*y),y)
    save(anton,'OffDutyDigits','0123456789:-')
    mono=instantiateVariableFont(TTFont(args.mono),{'wght':700},inplace=True)
    save(mono,'OffDutyClock','0123456789:-')

if __name__=='__main__':main()
