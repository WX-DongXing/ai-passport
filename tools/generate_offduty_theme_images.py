#!/usr/bin/env python3
"""Package three native theme renders; crop and losslessly compress small frames."""
import argparse, ctypes, json, struct, subprocess, tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]

def emit(lines,name,raw,w,h,compress,lib):
    flags='0'
    if compress:
        dst=ctypes.create_string_buffer(lib.LZ4_compressBound(len(raw)))
        size=lib.LZ4_compress_default(raw,dst,len(raw),len(dst));assert size>0
        packed=dst.raw[:size]
        restored=ctypes.create_string_buffer(len(raw))
        assert lib.LZ4_decompress_safe(packed,restored,len(packed),len(raw))==len(raw) and restored.raw==raw
        data=struct.pack('<III',2,len(packed),len(raw))+packed;flags='LV_IMAGE_FLAGS_COMPRESSED'
    else:data=raw
    lines.append(f'static const uint8_t {name}_data[]={{')
    lines.extend('    '+','.join(f'0x{v:02x}' for v in data[i:i+24])+',' for i in range(0,len(data),24))
    lines.extend(['};',f'const lv_image_dsc_t {name}={{.header={{.magic=LV_IMAGE_HEADER_MAGIC,.cf=LV_COLOR_FORMAT_RGB565A8,.flags={flags},.w={w},.h={h},.stride={w*2}}},.data_size={len(data)},.data={name}_data}};'])
    return len(data)

def encode(rgba):
    colors=bytearray();alpha=bytearray()
    for r,g,b,a in rgba:
        colors.extend(((r>>3)<<11 | (g>>2)<<5 | b>>3).to_bytes(2,'little'));alpha.append(a)
    return bytes(colors+alpha)

def main():
    parser=argparse.ArgumentParser();parser.add_argument('renders',type=Path);args=parser.parse_args()
    with tempfile.TemporaryDirectory(prefix='offduty-lz4-') as tmp:
        library=Path(tmp)/'lz4.so'
        memory=Path(tmp)/'memory.c'
        memory.write_text('#include <string.h>\n#include <stdint.h>\nvoid *lv_memcpy(void *d,const void *s,size_t n){return memcpy(d,s,n);}\nvoid *lv_memmove(void *d,const void *s,size_t n){return memmove(d,s,n);}\nvoid lv_memset(void *d,uint8_t v,size_t n){memset(d,v,n);}\n')
        subprocess.run(['cc','-shared','-fPIC','-DLV_CONF_PATH="'+str(ROOT/'tests/offduty_render/lv_conf.h')+'"',str(ROOT/'managed_components/lvgl__lvgl/src/libs/lz4/lz4.c'),str(memory),'-o',str(library)],check=True)
        lib=ctypes.CDLL(str(library));lib.LZ4_compressBound.argtypes=[ctypes.c_int]
        lib.LZ4_compress_default.argtypes=[ctypes.c_char_p,ctypes.c_void_p,ctypes.c_int,ctypes.c_int]
        lib.LZ4_decompress_safe.argtypes=[ctypes.c_char_p,ctypes.c_void_p,ctypes.c_int,ctypes.c_int]
        pets=['/* Generated from native LVGL renders by tools/generate_offduty_theme_images.py. */','#include "lvgl.h"'];crops=[];total=0
        for theme in range(3):
            states=[]
            for resting in range(2):
                moods=[]
                for mood in range(8):
                    raw=(args.renders/f'pet-{theme}-{resting}-{mood}.bgra').read_bytes();assert len(raw)==73*92*4
                    pts=[(i%73,i//73) for i in range(73*92) if raw[i*4+3]];assert pts
                    x=min(p[0] for p in pts);y=min(p[1] for p in pts);w=max(p[0] for p in pts)-x+1;h=max(p[1] for p in pts)-y+1
                    rgba=[(raw[i+2],raw[i+1],raw[i],raw[i+3]) for yy in range(y,y+h) for xx in range(x,x+w) for i in [(yy*73+xx)*4]]
                    name=f'od_pet_{"rest_" if resting else ""}{"t"+str(theme)+"_" if theme else ""}{mood}'
                    # Tall crown/halo frames stay in Flash to cap transform-time RAM.
                    total+=emit(pets,name,encode(rgba),w,h,h<=80,lib);moods.append([x,y])
                states.append(moods)
            crops.append(states)
        pets.append('const uint8_t od_pet_crop[3][2][8][2]='+json.dumps(crops).replace('[','{').replace(']','}')+';')
        (ROOT/'assets/images/offduty/od_pets.c').write_text('\n'.join(pets)+'\n')
        stamps=['/* Generated from native LVGL renders by tools/generate_offduty_theme_images.py. */','#include "lvgl.h"']
        for theme in range(3):
            for mood in range(8):
                raw=(args.renders/f'theme-{theme}-{mood}-done.ppm').read_bytes().split(b'\n255\n',1)[1];assert len(raw)==240*320*3
                # The paper corner at the bottom right is outside the oval stamp.
                bg=raw[(249*240+222)*3:(249*240+222)*3+3]
                rgba=[]
                for y in range(160,250):
                    for x in range(123,223):
                        c=raw[(y*240+x)*3:(y*240+x)*3+3];rgba.append((*c,0 if c==bg else 255))
                name=f'od_stamp_{"t"+str(theme)+"_" if theme else ""}{mood}'
                total+=emit(stamps,name,encode(rgba),100,90,True,lib)
        (ROOT/'assets/images/offduty/od_stamps.c').write_text('\n'.join(stamps)+'\n')
        print(f'48 pet frames and 24 stamps: {total} Flash bytes; compression round-trip PASS')
if __name__=='__main__':main()
