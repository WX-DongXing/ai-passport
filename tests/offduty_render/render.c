#include "offduty_ui.h"
#include "offduty_theme.h"
#include "src/misc/lv_text_private.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

LV_FONT_DECLARE(od_font_9);LV_FONT_DECLARE(od_font_11);LV_FONT_DECLARE(od_font_10);LV_FONT_DECLARE(od_font_12);LV_FONT_DECLARE(od_font_14);
LV_FONT_DECLARE(od_font_29);LV_FONT_DECLARE(od_font_44);
LV_FONT_DECLARE(od_font_23);LV_FONT_DECLARE(od_font_21);
LV_FONT_DECLARE(od_font_24);
LV_FONT_DECLARE(od_font_8);LV_FONT_DECLARE(od_digits_24);LV_FONT_DECLARE(od_digits_45);
LV_FONT_DECLARE(od_wait_title);
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
static void check_relax_center(int theme,int mood) {
    uint16_t color=lv_color_to_u16(lv_color_hex((theme==2?od_theme_palette(theme,mood).pet_ink:od_theme_palette(theme,mood).pet)));int left=240,right=-1;
    for(int y=112;y<250;y++)for(int x=0;x<240;x++)if(pixels[y*240+x]==color) {
        if(x<left)left=x;if(x>right)right=x;
    }
    assert(right>=left && abs(left+right-239)<=20);
}
static double luminance(uint32_t rgb) {
    double channel[3];
    for(int i=0;i<3;i++) {
        double value=((rgb>>(16-i*8))&255)/255.0;
        channel[i]=value<=0.04045?value/12.92:pow((value+0.055)/1.055,2.4);
    }
    return channel[0]*0.2126+channel[1]*0.7152+channel[2]*0.0722;
}
static void check_theme_contrast(void) {
    for(int variant=0;variant<3;variant++)for(int mood=0;mood<8;mood++) {
        od_theme_t value=od_theme_palette(variant,mood);const od_theme_t *theme=&value;
        const uint32_t backgrounds[]={theme->paper,theme->ticket,theme->seal};
        for(unsigned i=0;i<sizeof(backgrounds)/sizeof(backgrounds[0]);i++)
            {uint32_t ink=i==0?theme->ink:i==1?theme->block_ink:theme->seal_ink;double a=luminance(backgrounds[i]),b=luminance(ink);double contrast=(fmax(a,b)+.05)/(fmin(a,b)+.05);if(contrast<4.5)fprintf(stderr,"Theme %d mood %d surface %u contrast %.2f\n",variant,mood,i,contrast);assert(contrast>=4.5);}
    }
    puts("Eight mood themes, body and ribbon text contrast: PASS");
}
int main(int argc,char **argv) {
    check_theme_contrast();
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
        const lv_font_t *digits[]={&od_digits_24,&od_digits_45,&od_digits_35,&od_digits_128,&od_digits_148};
        for(unsigned j=0;j<sizeof(digits)/sizeof(digits[0]);j++) for(const char *c="0123456789:-";*c;c++) {lv_font_glyph_dsc_t d;assert(lv_font_get_glyph_dsc(digits[j],&d,*c,0));assert(!d.is_placeholder);}
        const char *header="OFF DUTY CLUB / 困困绷住摸鱼元气专注饿饿放空暴走";uint32_t header_index=0;
        while(header[header_index]){uint32_t c=lv_text_encoded_next(header,&header_index);lv_font_glyph_dsc_t d;assert(lv_font_get_glyph_dsc(&od_font_8,&d,c,0));assert(!d.is_placeholder);}
        const char *heading="本人目前尚未营业。";uint32_t heading_index=0;
        while(heading[heading_index]){uint32_t c=lv_text_encoded_next(heading,&heading_index);lv_font_glyph_dsc_t d;assert(lv_font_get_glyph_dsc(&od_wait_title,&d,c,0));assert(!d.is_placeholder);}
        assert(121+od_digits_128.line_height<258);
        puts("UI font coverage and countdown clearance: PASS");
    }
    od_view_t v={.mood=3,.egg=-1,.relax=-1,.battery=82,.connected=true,.synced=true,.plan_ready=true,.animation_ms=1200,.confetti_ms=-1};
    od_settings_default(&v.settings);v.utc_us=INT64_C(1791790920000000);v.time=od_time_at(v.utc_us,&v.settings);
    lv_obj_t *scr=od_ui_create();char name[32];
    v.boot=true;
    for(int ms=0;ms<3000;ms+=100){v.boot_ms=ms;od_ui_update(scr,&v);lv_refr_now(NULL);}
    const int boot_frames[]={0,900,1500,2200,2999};
    for(unsigned i=0;i<sizeof(boot_frames)/sizeof(boot_frames[0]);i++){v.boot_ms=boot_frames[i];snprintf(name,sizeof(name),"boot-%d",v.boot_ms);render(scr,&v,name);}
    assert(pixels[50*240+5]==lv_color_to_u16(lv_color_hex(0xf7f3e8)));
    static uint16_t original_boot[240*320];memcpy(original_boot,pixels,sizeof(pixels));
    for(int variant=1;variant<3;variant++){v.settings.theme=variant;render(scr,&v,"boot-theme-check");assert(!memcmp(original_boot,pixels,sizeof(pixels)));}
    v.settings.theme=0;
    v.boot=false;render(scr,&v,"boot-to-work");
    v.connected=false;v.synced=false;v.plan_ready=false;render(scr,&v,"welcome-wifi");
    v.connected=true;render(scr,&v,"welcome-time");assert(pixels[50*240+5]==lv_color_to_u16(lv_color_hex(0xd5c8f3)));v.synced=true;render(scr,&v,"welcome-plan");v.plan_ready=true;
    v.connected=true;v.synced=false;v.plan_ready=false;render(scr,&v,"calibration-original");
    static uint16_t original_setup[240*320];memcpy(original_setup,pixels,sizeof(pixels));
    for(int variant=1;variant<3;variant++){v.settings.theme=variant;render(scr,&v,"calibration-theme-check");assert(!memcmp(original_setup,pixels,sizeof(pixels)));}
    v.settings.theme=0;v.synced=true;v.plan_ready=true;
    for(int variant=0;variant<3;variant++)for(int mood=0;mood<8;mood++){
        v.settings.theme=variant;v.mood=mood;v.time=od_time_at(v.utc_us,&v.settings);v.time.phase=OD_WORK;snprintf(name,sizeof(name),"theme-%d-%d-work",variant,mood);render(scr,&v,name);
        v.time.phase=OD_DONE;snprintf(name,sizeof(name),"theme-%d-%d-done",variant,mood);render(scr,&v,name);
        for(int mode=0;mode<6;mode++){v.off_home=true;v.relax=mode;v.effect_ms=1200;snprintf(name,sizeof(name),"theme-%d-%d-relax-%d",variant,mood,mode);render(scr,&v,name);
#ifndef OD_STAMP_REFERENCE
        check_relax_center(variant,mood);
#endif
        }
        v.relax=-1;snprintf(name,sizeof(name),"theme-%d-%d-off-home",variant,mood);render(scr,&v,name);v.off_home=false;
        v.time.phase=OD_BEFORE;v.time.remaining_us=INT64_C(1080000000);snprintf(name,sizeof(name),"theme-%d-%d-waiting",variant,mood);render(scr,&v,name);
        v.time.phase=OD_FINAL;v.time.remaining_us=INT64_C(5000000);snprintf(name,sizeof(name),"theme-%d-%d-final",variant,mood);render(scr,&v,name);
        v.time.phase=OD_WORK;for(int e=0;e<5;e++){v.egg=e;snprintf(name,sizeof(name),"theme-%d-%d-egg-%d",variant,mood,e);render(scr,&v,name);}v.egg=-1;
        v.menu=1;v.selected=11;snprintf(name,sizeof(name),"theme-%d-%d-menu",variant,mood);render(scr,&v,name);v.menu=0;
    }
#ifdef OD_STAMP_REFERENCE
    puts("Theme reference renders: PASS");lv_display_delete(display);lv_deinit();return 0;
#endif
    v.settings.theme=0;v.time=od_time_at(v.utc_us,&v.settings);
    v.mood=3;v.animation_ms=100;render(scr,&v,"pet-awake");
    uint16_t awake[240*320];memcpy(awake,pixels,sizeof(awake));v.animation_ms=4350;render(scr,&v,"pet-blink");assert(memcmp(awake,pixels,sizeof(awake))!=0);
    v.animation_ms=9500;render(scr,&v,"pet-wave");assert(memcmp(awake,pixels,sizeof(awake))!=0);v.animation_ms=1200;
    for(int i=0;i<8;i++) {
        v.mood=i;snprintf(name,sizeof(name),"mood-%d",i);render(scr,&v,name);
        lv_color_t tag=lv_color_hex(od_themes[i].seal);assert(pixels[151*240+45]==lv_color_to_u16(tag));
    }
    for(int i=0;i<5;i++) {
        v.egg=i;
        for(int age=0;age<3000;age+=200) {
            v.effect_ms=age;od_ui_update(scr,&v);lv_refr_now(NULL);
            uint16_t paper=lv_color_to_u16(lv_color_hex(od_themes[v.mood].paper));
            int left=240,right=-1,top=320,bottom=-1;
            for(int y=104;y<254;y++)for(int x=0;x<240;x++)if(pixels[y*240+x]!=paper) {
                if(x<left)left=x;if(x>right)right=x;if(y<top)top=y;if(y>bottom)bottom=y;
            }
            assert(left>=17 && right<223 && top>=112 && bottom<250);
            // The escaping soul moves toward its door; the other scenes stay centered.
            if(i!=3)assert(abs(left+right-239)<=6);
        }
        v.effect_ms=1500;snprintf(name,sizeof(name),"egg-%d",i);render(scr,&v,name);
    }
    puts("Break illustration centering and animation clearance: PASS");v.egg=-1;
    v.off_home=true;
    for(int variant=0;variant<3;variant++)for(int mood=0;mood<8;mood++)for(int effect=0;effect<6;effect++) {
        v.settings.theme=variant;
        v.mood=mood;v.relax=effect;v.effect_ms=0;od_ui_update(scr,&v);lv_refr_now(NULL);check_relax_center(variant,mood);
        uint16_t first[240*320];memcpy(first,pixels,sizeof(first));
        for(int age=200;age<3000;age+=200) {
            v.effect_ms=age;flushed_pixels=0;od_ui_update(scr,&v);lv_refr_now(NULL);
            assert(flushed_pixels<=57600);check_relax_center(variant,mood);
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
    v.settings.theme=0;v.relax=-1;v.off_home=false;
    v.menu=1;v.selected=11;v.edit=false;render(scr,&v,"theme-menu-read");
    static uint16_t forget_row[92*18];for(int y=0;y<18;y++)memcpy(forget_row+y*92,pixels+(85+y)*240+126,92*sizeof(uint16_t));
    v.edit=true;render(scr,&v,"theme-menu-edit");
    for(int y=0;y<18;y++)assert(!memcmp(forget_row+y*92,pixels+(85+y)*240+126,92*sizeof(uint16_t)));
    v.edit=false;v.menu=0;
    for(int i=0;i<OD_MENU_ROWS;i++){v.menu=1;v.selected=i;snprintf(name,sizeof(name),"setting-%d",i);render(scr,&v,name);}v.menu=0;
    v.menu=2;v.portal=true;strcpy(v.ap_name,"OFFDUTY-DEMO");strcpy(v.ap_password,"0123456789abcdef");
    v.phone_connected=false;render(scr,&v,"wifi-qr");
    uint16_t qr[123*123];
    for(int y=0;y<123;y++)memcpy(qr+y*123,pixels+(86+y)*240+58,123*sizeof(uint16_t));
    v.phone_connected=true;render(scr,&v,"settings-address");
    assert(pixels[50*240+5]==lv_color_to_u16(lv_color_hex(od_themes[0].paper)));
    assert(pixels[241*240+19]==lv_color_to_u16(lv_color_hex(od_themes[0].accent)));
    for(int y=0;y<123;y++)assert(!memcmp(qr+y*123,pixels+(86+y)*240+58,123*sizeof(uint16_t)));
    static uint16_t before_feedback[240*320];memcpy(before_feedback,pixels,sizeof(pixels));
    for(int result=1;result<=3;result++) {
        v.feedback=result;snprintf(name,sizeof(name),"phone-feedback-%d",result);render(scr,&v,name);
        assert(!memcmp(before_feedback,pixels,240*274*sizeof(uint16_t)));
        assert(memcmp(before_feedback+240*274,pixels+240*274,240*46*sizeof(uint16_t)));
    }
    v.feedback=0;render(scr,&v,"phone-feedback-dismissed");
    assert(!memcmp(before_feedback,pixels,sizeof(pixels)));
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
    v.time=od_time_between(v.utc_us,v.utc_us/1000000+9*3600,v.utc_us/1000000+18*3600);render(scr,&v,"waiting-home");
    v.settings.start_min=1020;v.settings.end_min=1080;v.settings.wait_min=960;
    v.utc_us=(INT64_C(1791763200)+8*3600+42*60)*1000000;v.time=od_time_at(v.utc_us,&v.settings);
    for(int mood=0;mood<8;mood++)for(int frame=0;frame<5;frame++) {
        v.mood=mood;v.animation_ms=frame*500;snprintf(name,sizeof(name),"waiting-charge-%d-%d",mood,frame);render(scr,&v,name);
        if(frame>0)assert(flushed_pixels<=28800);
        uint16_t background=lv_color_to_u16(lv_color_hex(od_themes[mood].paper)),band=lv_color_to_u16(lv_color_hex(od_themes[mood].ticket)),pet=lv_color_to_u16(lv_color_hex(od_theme_palette(0,mood).pet));
        assert(pixels[40*240+5]==background && pixels[180*240+5]==band);
        int pet_pixels=0;for(int y=151;y<215;y++)for(int x=164;x<219;x++)if(pixels[y*240+x]==pet)pet_pixels++;
        assert(pet_pixels>=100);
        for(int y=275;y<=280;y++){assert(pixels[y*240+20]==band);assert(pixels[y*240+209]!=band);}
        for(int x=20;x<220;x++)assert(pixels[319*240+x]==background);
        uint16_t snapshot[240*320];memcpy(snapshot,pixels,sizeof(snapshot));lv_obj_invalidate(scr);lv_refr_now(NULL);assert(!memcmp(snapshot,pixels,sizeof(snapshot)));
    }
    od_ui_update(scr,&v);flushed_pixels=0;lv_refr_now(NULL);assert(flushed_pixels==0);
    for(int tick=0;tick<70;tick++) {
        v.utc_us+=INT64_C(1000000);v.time=od_time_at(v.utc_us,&v.settings);v.animation_ms+=40;
        od_ui_update(scr,&v);lv_refr_now(NULL);uint16_t snapshot[240*320];memcpy(snapshot,pixels,sizeof(snapshot));
        lv_obj_invalidate(scr);lv_refr_now(NULL);assert(!memcmp(snapshot,pixels,sizeof(snapshot)));
    }
    puts("Waiting partial redraw and second/minute transitions: PASS");
    v.time.remaining_us=90*60*INT64_C(1000000);render(scr,&v,"waiting-charge-long");

    v.work_choice=true;v.work_option=0;render(scr,&v,"work-choice-continue");v.work_option=1;render(scr,&v,"work-choice-overtime");v.work_choice=false;
    v.resume_picker=true;v.overtime_picker=true;v.overtime_min=30;render(scr,&v,"overtime-picker");v.overtime_min=480;render(scr,&v,"overtime-picker-max");v.overtime_picker=false;
    v.resume_picker=true;v.resume_end_min=1110;render(scr,&v,"resume-picker");v.resume_picker=false;
    v.utc_us=INT64_C(1791799200)*1000000;
    v.time=od_time_between(v.utc_us,v.utc_us/1000000,od_resume_end(v.utc_us,1110));
    v.connected=false;render(scr,&v,"resumed-offline");
    v.overtime=true;render(scr,&v,"overtime-home");v.time.phase=OD_DONE;render(scr,&v,"overtime-complete");v.overtime=false;

    lv_display_delete(display);lv_deinit();puts("UI rendering: PASS");
}
