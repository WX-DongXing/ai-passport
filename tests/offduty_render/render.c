#include "offduty_ui.h"
#include "src/misc/lv_text_private.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

LV_FONT_DECLARE(od_font_9);LV_FONT_DECLARE(od_font_11);LV_FONT_DECLARE(od_font_10);LV_FONT_DECLARE(od_font_12);LV_FONT_DECLARE(od_font_14);
LV_FONT_DECLARE(od_font_29);LV_FONT_DECLARE(od_font_44);
LV_FONT_DECLARE(od_font_23);LV_FONT_DECLARE(od_font_21);
LV_FONT_DECLARE(od_font_24);
LV_FONT_DECLARE(od_digits_35);LV_FONT_DECLARE(od_digits_128);LV_FONT_DECLARE(od_digits_148);
static uint16_t pixels[240*320],draw_buffer[240*40];
static unsigned flushed_pixels;
static void flush(lv_display_t *display,const lv_area_t *a,uint8_t *map) {
    flushed_pixels+=(a->x2-a->x1+1)*(a->y2-a->y1+1);
    uint16_t *p=(uint16_t *)map;
    for(int y=a->y1;y<=a->y2;y++) for(int x=a->x1;x<=a->x2;x++) pixels[y*240+x]=*p++;
    lv_display_flush_ready(display);
}
static void save(const char *name) {
    char path[256];snprintf(path,sizeof(path),"%s.ppm",name);
    FILE *f=fopen(path,"wb");assert(f);fprintf(f,"P6\n240 320\n255\n");
    for(int y=0;y<320;y++) for(int x=0;x<240;x++) {
        int cx=x<30?30:x>=210?209:x, cy=y<30?30:y>=290?289:y;
        uint16_t c=(x-cx)*(x-cx)+(y-cy)*(y-cy)>900?0:pixels[y*240+x];
        unsigned char rgb[]={((c>>11)&31)*255/31,((c>>5)&63)*255/63,(c&31)*255/31};
        assert(fwrite(rgb,1,3,f)==3);
    }
    fclose(f);
}
static void render(lv_obj_t *scr,od_view_t *view,const char *name) {
    flushed_pixels=0;od_ui_update(scr,view);lv_refr_now(NULL);save(name);
    if(!strncmp(name,"done-",5) && name[strlen(name)-1]!='0')assert(flushed_pixels<5000);
    if(!strcmp(name,"confetti"))assert(flushed_pixels<=57600);
    if(!strcmp(name,"setting-0"))for(int row=0;row<5;row++)for(int side=0;side<2;side++) {
        int y=74+row*38,top=320,bottom=-1;
        for(int yy=y+3;yy<y+31;yy++)for(int x=side?126:20;x<(side?218:120);x++) {
            uint16_t p=pixels[yy*240+x];
            if((p>>11)<12 && ((p>>5)&63)<24 && (p&31)<12){if(yy<top)top=yy;if(yy>bottom)bottom=yy;}
        }
        assert(bottom>=top && abs(top+bottom-(2*y+33))<=2);
    }
}
int main(int argc,char **argv) {
    lv_init();lv_display_t *display=lv_display_create(240,320);assert(display);
    lv_display_set_color_format(display,LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(display,draw_buffer,NULL,sizeof(draw_buffer),LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(display,flush);
    const lv_font_t *fonts[]={&od_font_9,&od_font_11,&od_font_10,&od_font_12,&od_font_14,&od_font_21,&od_font_23,&od_font_24,&od_font_29,&od_font_44};
    if(argc==2) {
        FILE *f=fopen(argv[1],"rb");assert(f);char text[4096];size_t n=fread(text,1,sizeof(text)-1,f);text[n]=0;fclose(f);
        for(unsigned i=0;i<sizeof(fonts)/sizeof(fonts[0]);i++) {
            uint32_t index=0;while(index<n) {uint32_t c=lv_text_encoded_next(text,&index);if(c<32)continue;lv_font_glyph_dsc_t d;
                assert(lv_font_get_glyph_dsc(fonts[i],&d,c,0));assert(!d.is_placeholder);}
        }
        for(unsigned i=0;i<95;i++) for(unsigned j=0;j<sizeof(fonts)/sizeof(fonts[0]);j++){lv_font_glyph_dsc_t d;assert(lv_font_get_glyph_dsc(fonts[j],&d,i+32,0));assert(!d.is_placeholder);}
        const lv_font_t *digits[]={&od_digits_35,&od_digits_128,&od_digits_148};
        for(unsigned j=0;j<3;j++) for(const char *c="0123456789:-";*c;c++) {lv_font_glyph_dsc_t d;assert(lv_font_get_glyph_dsc(digits[j],&d,*c,0));assert(!d.is_placeholder);}
        assert(121+od_digits_128.line_height<258);
        puts("UI font coverage and countdown clearance: PASS");
    }
    od_view_t v={.mood=3,.egg=-1,.relax=-1,.battery=82,.connected=true,.synced=true,.plan_ready=true,.animation_ms=1200,.confetti_ms=-1};
    od_settings_default(&v.settings);v.utc_us=INT64_C(1791790920000000);v.time=od_time_at(v.utc_us,&v.settings);
    lv_obj_t *scr=od_ui_create();char name[32];
    v.connected=false;v.synced=false;v.plan_ready=false;render(scr,&v,"welcome-wifi");
    v.connected=true;render(scr,&v,"welcome-time");v.synced=true;render(scr,&v,"welcome-plan");v.plan_ready=true;
    v.mood=3;v.animation_ms=100;render(scr,&v,"pet-awake");
    uint16_t awake[240*320];memcpy(awake,pixels,sizeof(awake));v.animation_ms=4350;render(scr,&v,"pet-blink");assert(memcmp(awake,pixels,sizeof(awake))!=0);
    v.animation_ms=9500;render(scr,&v,"pet-wave");assert(memcmp(awake,pixels,sizeof(awake))!=0);v.animation_ms=1200;
    const uint32_t tag_colors[]={0xb6a6ed,0xff9b76,0xeed068,0x91cbb2,0x8dbcec,0xeea6c6,0xa6bf87,0xffa078};
    for(int i=0;i<8;i++) {
        v.mood=i;snprintf(name,sizeof(name),"mood-%d",i);render(scr,&v,name);
        lv_color_t tag=lv_color_hex(tag_colors[i]);assert(pixels[151*240+45]==lv_color_to_u16(tag));
    }
    for(int i=0;i<5;i++){v.egg=i;v.effect_ms=1500;snprintf(name,sizeof(name),"egg-%d",i);render(scr,&v,name);}v.egg=-1;
    v.off_home=true;
    for(int mood=0;mood<8;mood++)for(int effect=0;effect<6;effect++) {
        v.mood=mood;v.relax=effect;v.effect_ms=0;od_ui_update(scr,&v);lv_refr_now(NULL);
        uint16_t first[240*320];memcpy(first,pixels,sizeof(first));
        for(int age=200;age<3000;age+=200) {
            v.effect_ms=age;flushed_pixels=0;od_ui_update(scr,&v);lv_refr_now(NULL);
            assert(flushed_pixels<=57600);
            uint16_t partial[240*320];memcpy(partial,pixels,sizeof(partial));lv_obj_invalidate(scr);lv_refr_now(NULL);
            if(memcmp(partial,pixels,sizeof(partial))) {
                int x1=240,y1=320,x2=0,y2=0;
                for(int y=0;y<320;y++)for(int x=0;x<240;x++)if(partial[y*240+x]!=pixels[y*240+x]){if(x<x1)x1=x;if(x>x2)x2=x;if(y<y1)y1=y;if(y>y2)y2=y;}
                int delta=0;for(int i=0;i<240*320;i++){int r=abs((partial[i]>>11)-(pixels[i]>>11)),g=abs(((partial[i]>>5)&63)-((pixels[i]>>5)&63)),b=abs((partial[i]&31)-(pixels[i]&31));if(r>delta)delta=r;if(g>delta)delta=g;if(b>delta)delta=b;}
                fprintf(stderr,"Relax mismatch mood=%d effect=%d age=%d bounds=%d,%d..%d,%d delta=%d\n",mood,effect,age,x1,y1,x2,y2,delta);
                assert(!memcmp(partial,pixels,sizeof(partial)));
            }
        }
        assert(memcmp(first,pixels,sizeof(first))!=0);
        v.effect_ms=1500;snprintf(name,sizeof(name),"relax-%d-%d",mood,effect);render(scr,&v,name);
    }
    v.relax=-1;v.off_home=false;
    for(int i=0;i<OD_MENU_ROWS;i++){v.menu=1;v.selected=i;snprintf(name,sizeof(name),"setting-%d",i);render(scr,&v,name);}v.menu=0;
    v.menu=2;v.portal=true;strcpy(v.ap_name,"OFFDUTY-DEMO");strcpy(v.ap_password,"0123456789abcdef");
    v.phone_connected=false;render(scr,&v,"wifi-qr");
    uint16_t qr[123*123];
    for(int y=0;y<123;y++)memcpy(qr+y*123,pixels+(86+y)*240+58,123*sizeof(uint16_t));
    v.phone_connected=true;render(scr,&v,"settings-address");
    for(int y=0;y<123;y++)assert(!memcmp(qr+y*123,pixels+(86+y)*240+58,123*sizeof(uint16_t)));
    v.menu=0;v.portal=false;v.phone_connected=false;
    v.hold_remaining_ms=OD_HOLD_MS;render(scr,&v,"hold");v.hold_remaining_ms=0;
    v.early_done=true;render(scr,&v,"early");v.off_home=true;v.free_us=18000000;render(scr,&v,"off-home");v.off_home=false;v.early_done=false;
    const int seconds[]={60,30,10,5,1};
    for(unsigned i=0;i<sizeof(seconds)/sizeof(seconds[0]);i++){v.time.phase=OD_FINAL;v.time.remaining_us=seconds[i]*1000000;snprintf(name,sizeof(name),"final-%d",seconds[i]);render(scr,&v,name);}
    v.time.phase=OD_DONE;
    for(int i=0;i<8;i++)for(int frame=0;frame<3;frame++) {
        v.mood=i;v.animation_ms=(int[]){1200,4350,9500}[frame];
        snprintf(name,sizeof(name),"done-%d-%d",i,frame);render(scr,&v,name);
    }
    v.mood=3;v.animation_ms=1200;render(scr,&v,"done");
    assert(186+od_font_11.line_height<206);
    assert(262<275 && 275+od_font_11.line_height<299);
    assert(299+od_font_9.line_height<=320);
    uint16_t celebration[240*320];memcpy(celebration,pixels,sizeof(celebration));
    int first_center=0,max_flush=0;bool covered_text=false;
    for(int age=0;age<=2040;age+=40) {
        v.confetti_ms=age<2000?age:-1;
        flushed_pixels=0;od_ui_update(scr,&v);lv_refr_now(NULL);
        if((int)flushed_pixels>max_flush)max_flush=flushed_pixels;
        assert(flushed_pixels<=57600);
        uint16_t partial[240*320];memcpy(partial,pixels,sizeof(partial));
        lv_obj_invalidate(scr);lv_refr_now(NULL);
        assert(!memcmp(partial,pixels,sizeof(partial)));
        if(age==600 || age==1000) {
            int sum=0,count=0;for(int y=0;y<320;y++)for(int x=0;x<240;x++)if(celebration[y*240+x]!=pixels[y*240+x]){sum+=y;count++;}
            assert(count>40);if(age==600)first_center=sum/count;else assert(sum/count>first_center+60);
        }
        if(age>=1000 && age<2000) {
            int covered=0;for(int y=275;y<294;y++)for(int x=17;x<223;x++)if(celebration[y*240+x]!=pixels[y*240+x] && celebration[y*240+x]!=celebration[y*240+17])covered++;
            if(covered){covered_text=true;save("confetti-over-text");}
        }
        if(age>=2000)assert(!memcmp(celebration,pixels,sizeof(celebration)));
    }
    assert(covered_text);
    v.confetti_ms=600;v.confetti_seed=1;render(scr,&v,"random-confetti-1");
    uint16_t burst[240*320];memcpy(burst,pixels,sizeof(burst));
    v.confetti_seed=2;render(scr,&v,"random-confetti-2");assert(memcmp(burst,pixels,sizeof(burst))!=0);
    uint16_t partial[240*320];memcpy(partial,pixels,sizeof(partial));lv_obj_invalidate(scr);lv_refr_now(NULL);assert(!memcmp(partial,pixels,sizeof(partial)));
    v.confetti_ms=-1;
    printf("Top-to-bottom confetti, text overlay, no trails: PASS; max flushed pixels/frame: %d\n",max_flush);
    v.resume_picker=true;v.resume_end_min=1110;render(scr,&v,"resume-picker");v.resume_picker=false;
    v.utc_us=INT64_C(1791799200)*1000000;
    v.time=od_time_between(v.utc_us,v.utc_us/1000000,od_resume_end(v.utc_us,1110));
    v.connected=false;render(scr,&v,"resumed-offline");

    lv_display_delete(display);lv_deinit();puts("UI rendering: PASS");
}
