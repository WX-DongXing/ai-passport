#include "offduty_ui.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static uint32_t buffer[240*320],pixels[240*320];
static void flush(lv_display_t *display,const lv_area_t *area,uint8_t *map) {
    const uint32_t *p=(uint32_t *)map;
    for(int y=area->y1;y<=area->y2;y++)for(int x=area->x1;x<=area->x2;x++)pixels[y*240+x]=*p++;
    lv_display_flush_ready(display);
}
int main(void) {
    lv_init();lv_display_t *display=lv_display_create(240,320);
    lv_display_set_color_format(display,LV_COLOR_FORMAT_ARGB8888);
    lv_display_set_buffers(display,buffer,NULL,sizeof(buffer),LV_DISPLAY_RENDER_MODE_FULL);
    lv_display_set_flush_cb(display,flush);lv_obj_t *screen=od_ui_create();
    for(int theme=0;theme<3;theme++)for(int resting=0;resting<2;resting++)for(int mood=0;mood<8;mood++) {
        memset(buffer,0,sizeof(buffer));memset(pixels,0,sizeof(pixels));
        od_view_t view={.mood=mood,.relax=resting?1:2};view.settings.theme=theme;od_ui_update(screen,&view);lv_obj_invalidate(screen);lv_refr_now(NULL);
        char name[40];snprintf(name,sizeof(name),"pet-%d-%d-%d.bgra",theme,resting,mood);FILE *file=fopen(name,"wb");assert(file);
        for(int y=139;y<=230;y++)assert(fwrite(pixels+y*240+84,4,73,file)==73);
        fclose(file);
    }
    lv_display_delete(display);lv_deinit();return 0;
}
