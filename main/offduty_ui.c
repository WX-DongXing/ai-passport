#include "offduty_ui.h"
#include "offduty_theme.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "src/libs/qrcode/qrcodegen.h"
#ifdef OD_UI_VALIDATE
#include "src/misc/lv_text_private.h"
#include <assert.h>
#endif

LV_FONT_DECLARE(od_font_9);
LV_FONT_DECLARE(od_font_8);
LV_FONT_DECLARE(od_font_11);
LV_FONT_DECLARE(od_font_10);
LV_FONT_DECLARE(od_font_12);
LV_FONT_DECLARE(od_font_14);
LV_FONT_DECLARE(od_font_29);
LV_FONT_DECLARE(od_font_23);
LV_FONT_DECLARE(od_font_24);
LV_FONT_DECLARE(od_font_21);
LV_FONT_DECLARE(od_font_44);
LV_FONT_DECLARE(od_digits_35);

LV_FONT_DECLARE(od_digits_45);
LV_FONT_DECLARE(od_digits_24);
LV_FONT_DECLARE(od_digits_128);
LV_FONT_DECLARE(od_digits_148);
LV_FONT_DECLARE(od_boot_question);
LV_FONT_DECLARE(od_wait_title);
LV_FONT_DECLARE(od_boot_caption);

static od_view_t s_view;
static lv_layer_t *s_layer;
static char s_qr_payload[96];
static uint8_t s_qr[qrcodegen_BUFFER_LEN_FOR_VERSION(5)];
LV_IMAGE_DECLARE(od_stamp_0);
LV_IMAGE_DECLARE(od_stamp_1);
LV_IMAGE_DECLARE(od_stamp_2);
LV_IMAGE_DECLARE(od_stamp_3);
LV_IMAGE_DECLARE(od_stamp_4);
LV_IMAGE_DECLARE(od_stamp_5);
LV_IMAGE_DECLARE(od_stamp_6);
LV_IMAGE_DECLARE(od_stamp_7);
LV_IMAGE_DECLARE(od_pet_0);LV_IMAGE_DECLARE(od_pet_1);LV_IMAGE_DECLARE(od_pet_2);LV_IMAGE_DECLARE(od_pet_3);
LV_IMAGE_DECLARE(od_pet_4);LV_IMAGE_DECLARE(od_pet_5);LV_IMAGE_DECLARE(od_pet_6);LV_IMAGE_DECLARE(od_pet_7);
LV_IMAGE_DECLARE(od_pet_rest_0);LV_IMAGE_DECLARE(od_pet_rest_1);LV_IMAGE_DECLARE(od_pet_rest_2);LV_IMAGE_DECLARE(od_pet_rest_3);
LV_IMAGE_DECLARE(od_pet_rest_4);LV_IMAGE_DECLARE(od_pet_rest_5);LV_IMAGE_DECLARE(od_pet_rest_6);LV_IMAGE_DECLARE(od_pet_rest_7);
static void egg(void);
static uint32_t INK=0x241b34;
#define THEME (od_themes[s_view.mood])
#define PAPER (THEME.paper)
static const char *s_mood[]={"困困中","努力绷住","摸鱼中","元气满格","专注结界","饿饿待投喂","放空一下","暴走模式"};
static const char *s_quote[]={"灵魂还在被窝加载中。","问题不大，还能再撑一下。","人坐在工位，心已在地铁。","做完这一点，就离自由更近。","别打扰，正在消灭待办。","脑子里只剩晚饭菜单。","短暂离线，也是充电。","今天的 KPI：准点撤退。"};
static const char *s_wait_mood[]={"困困","绷住","摸鱼","元气","专注","饿饿","放空","暴走"};
static const char *s_wait_copy[]={"再眯一会，到点叫我。","先深呼吸，开工不抢跑。","营业之前，先给自己放风。","快乐攒满，到点出发。","脑袋预热，暂不接单。","先填饱，再开始营业。","此刻只管发会儿呆。","先存体力，到点再冲。"};
static const char *s_eggs[]={"摸鱼许可证","咖啡补给站","老板雷达","灵魂先下班","快乐预存成功"};
static const char *s_egg_quote[]={"已批准：钓一条快乐小鱼。","今天的你，值得加一份奶。","别紧张，屏幕在认真工作。","肉身随后就到，请稍等。","今日快乐，正在排队到账。"};
static const char *s_menu[]={"上班时间","下班时间","准备时间","屏幕亮度","声音大小","熄屏方式","随机彩蛋","连接 Wi-Fi","立即对时","对时状态","忘记网络","返回"};
const char *od_menu_name(int row) { return s_menu[row]; }

static float s_scale=1.f,s_scale_y=1.f;
static int s_relax_mode=-1;
static int s_origin_x,s_origin_y;
static int scaled_x(int x){return s_origin_x+(int)lroundf((x-s_origin_x)*s_scale);}
static int scaled_y(int y){return s_origin_y+(int)lroundf((y-s_origin_y)*s_scale_y);}
static int scaled_size(int n){int result=(int)lroundf(n*s_scale);return n && result==0?1:result;}
static void arc(lv_draw_arc_dsc_t *d) {
    d->center.x=scaled_x(d->center.x);d->center.y=scaled_y(d->center.y);
    d->radius=scaled_size(d->radius);d->width=scaled_size(d->width);lv_draw_arc(s_layer,d);
}

static void rect(int x,int y,int w,int h,uint32_t color,int radius,int border) {
    x=scaled_x(x);y=scaled_y(y);w=scaled_size(w);h=(int)lroundf(h*s_scale_y);radius=scaled_size(radius);border=scaled_size(border);
    lv_draw_rect_dsc_t d; lv_draw_rect_dsc_init(&d);
    d.bg_color=lv_color_hex(color); d.radius=radius;
    d.border_width=border; d.border_color=lv_color_hex(INK);
    lv_area_t a={x,y,x+w-1,y+h-1}; lv_draw_rect(s_layer,&d,&a);
}
static void line(int x,int y,int xx,int yy,uint32_t color,int width) {
    x=scaled_x(x);y=scaled_y(y);xx=scaled_x(xx);yy=scaled_y(yy);width=scaled_size(width);
    lv_draw_line_dsc_t d; lv_draw_line_dsc_init(&d);
    d.p1=(lv_point_precise_t){x,y}; d.p2=(lv_point_precise_t){xx,yy};
    d.color=lv_color_hex(color); d.width=width; d.round_start=1; d.round_end=1;
    lv_draw_line(s_layer,&d);
}
static void text(int x,int y,int w,const char *str,const lv_font_t *font,uint32_t color,lv_text_align_t align) {
    if(s_scale!=1.f){x=scaled_x(x);y=scaled_y(y);w=scaled_size(w);font=&od_font_9;}
    lv_draw_label_dsc_t d; lv_draw_label_dsc_init(&d);
    d.font=font; d.color=lv_color_hex(color); d.text=str; d.text_local=1; d.align=align;
    if(font==&od_font_29) d.letter_space=-1;
    if(font==&od_font_44) d.letter_space=-3;
    if(font==&od_digits_35 || font==&od_digits_45 || font==&od_digits_24) d.letter_space=-2;
    if(font==&od_digits_128 || font==&od_digits_148) d.letter_space=-11;
#ifdef OD_UI_VALIDATE
    lv_point_t size;lv_text_get_size(&size,str,font,d.letter_space,0,LV_COORD_MAX,LV_TEXT_FLAG_NONE);
    if(size.x>w){fprintf(stderr,"Text overflows %dpx box: %s (%ldpx)\n",w,str,(long)size.x);assert(size.x<=w);}
    assert(y>=0 && y+font->line_height<=320);
#endif
    lv_area_t a={x,y,x+w-1,y+font->line_height-1}; lv_draw_label(s_layer,&d,&a);
}
#define TXT(x,y,w,s,f,c) text(x,y,w,s,f,c,LV_TEXT_ALIGN_LEFT)
#define CENTER(x,y,w,s,f,c) text(x,y,w,s,f,c,LV_TEXT_ALIGN_CENTER)
static void clock_text(char out[6],int min) {
    unsigned m=(unsigned)min%1440;
    out[0]='0'+m/60/10;out[1]='0'+m/60%10;out[2]=':';out[3]='0'+m%60/10;out[4]='0'+m%10;out[5]=0;
}

static float boot_ease(float x) {
    float lo=0,hi=1;
    for(int i=0;i<12;i++){float u=(lo+hi)/2,v=1-u,xx=3*v*v*u*.25f+3*v*u*u*.25f+u*u*u;if(xx<x)lo=u;else hi=u;}
    float u=(lo+hi)/2,v=1-u;return 3*v*v*u*.1f+3*v*u*u+u*u*u;
}
static float boot_tween(const int *at,const float *value,int count,int ms) {
    if(ms<=at[0])return value[0];
    for(int i=1;i<count;i++)if(ms<=at[i]){float p=boot_ease((ms-at[i-1])/(float)(at[i]-at[i-1]));return value[i-1]+(value[i]-value[i-1])*p;}
    return value[count-1];
}
static uint32_t boot_color(uint32_t color,float opacity) {
    uint32_t result=0;
    for(int shift=0;shift<=16;shift+=8){int bg=(0xf7f3e8>>shift)&255,c=(color>>shift)&255;result|=(uint32_t)lroundf(bg+(c-bg)*opacity)<<shift;}
    return result;
}
static void boot_label(int x,int y,int width,const char *value,const lv_font_t *font,int spacing,float opacity) {
    lv_draw_label_dsc_t d;lv_draw_label_dsc_init(&d);d.font=font;d.color=lv_color_hex(boot_color(0x282933,opacity));d.text=value;d.letter_space=spacing;
#ifdef OD_UI_VALIDATE
    lv_point_t size;lv_text_get_size(&size,value,font,spacing,0,LV_COORD_MAX,LV_TEXT_FLAG_NONE);
    assert(size.x<=width && y>=0 && y+font->line_height<=320);
#endif
    lv_area_t a={x,y,x+width-1,y+font->line_height-1};lv_draw_label(s_layer,&d,&a);
}
static void boot(void) {
    int ms=s_view.boot_ms;
    rect(0,0,240,320,0xf7f3e8,0,0);rect(20,19,6,6,0xff805c,3,0);
    const int brand_at[]={0,360};const float fade[]={0,1},brand_y[]={5,0};
    float brand=boot_tween(brand_at,fade,2,ms);int dy=lroundf(boot_tween(brand_at,brand_y,2,ms));
    boot_label(20,35+dy,200,"OFF DUTY CLUB /",&od_font_11,2,brand);
    boot_label(20,57+dy,200,"准点快乐事务所",&od_font_11,2,brand);
    const int title_at[]={240,720};const float title_y[]={10,0};
    float title=boot_tween(title_at,fade,2,ms);dy=lroundf(boot_tween(title_at,title_y,2,ms));
    boot_label(19,93+dy,200,"什么时候",&od_font_29,-1,title);
    boot_label(19,127+dy,200,"下班呐",&od_font_29,-1,title);
    const int question_at[]={420,960,1320};const float question_opacity[]={0,1,1},question_angle[]={-15,13,9},question_scale[]={.6f,1.1f,1};
    lv_area_t qa={147,115,183,172};lv_layer_t *parent=s_layer,*question=lv_draw_layer_create(parent,LV_COLOR_FORMAT_ARGB8888,&qa);
    if(question){
        lv_draw_label_dsc_t label;lv_draw_label_dsc_init(&label);label.font=&od_boot_question;label.color=lv_color_hex(0xff805c);label.text="?";
        lv_area_t text_area={152,120,181,120+od_boot_question.line_height-1};lv_draw_label(question,&label,&text_area);
        lv_draw_image_dsc_t image;lv_draw_image_dsc_init(&image);image.src=question;image.pivot=(lv_point_t){18,29};image.antialias=true;
        image.rotation=(int)lroundf(boot_tween(question_at,question_angle,3,ms)*10+3600)%3600;
        image.scale_x=image.scale_y=lroundf(256*boot_tween(question_at,question_scale,3,ms));image.opa=lroundf(255*boot_tween(question_at,question_opacity,3,ms));lv_draw_layer(parent,&image,&qa);
    }
    rect(58,181,125,83,0xe5f0e1,42,0);rect(74,263,93,7,0xe4e1d7,4,0);
    const int pet_at[]={540,1080,1380,1650,1950,2280,2610,3000};
    const float pet_opacity[]={0,1,1,1,1,1,1,1},pet_x[]={70,-3,0,0,0,0,0,0},pet_angle[]={15,-7,4,-3,0,0,0,0},pet_sx[]={.9f,1.04f,1,1,1,1,1,1},pet_sy[]={.9f,1.04f,1,1,.95f,1,1,1},pet_y[]={0,0,0,0,0,-7,0,0};
    lv_draw_image_dsc_t pet;lv_draw_image_dsc_init(&pet);pet.src=&od_pet_3;pet.pivot=(lv_point_t){36,88};pet.antialias=true;
    pet.rotation=(int)lroundf(boot_tween(pet_at,pet_angle,8,ms)*10+3600)%3600;
    pet.scale_x=lroundf(256*boot_tween(pet_at,pet_sx,8,ms));pet.scale_y=lroundf(256*boot_tween(pet_at,pet_sy,8,ms));pet.opa=lroundf(255*boot_tween(pet_at,pet_opacity,8,ms));
    int x=84+lroundf(boot_tween(pet_at,pet_x,8,ms)),y=166+lroundf(boot_tween(pet_at,pet_y,8,ms));lv_area_t target={x,y,x+72,y+91};lv_draw_image(s_layer,&pet,&target);
    const int wave_at[]={1140,1290,1560,1860,2100,2400,2730,3000};const float wave_opacity[]={0,1,1,1,1,1,0,0},wave_angle[]={0,-30,20,-30,20,-30,0,0};
    float wave=boot_tween(wave_at,wave_opacity,8,ms),angle=boot_tween(wave_at,wave_angle,8,ms)*3.14159265f/180;
    if(wave>.01f){int hand_x=x+6-lroundf(cosf(angle)*12-sinf(angle)*6),hand_y=y+63-lroundf(sinf(angle)*12+cosf(angle)*6);rect(hand_x-6,hand_y-6,12,12,boot_color(0xa3dec0,wave),6,2);}
    const int spark_at[]={930,1440,2220,2760,3000};const float spark_opacity[]={0,1,1,0,0};float spark=boot_tween(spark_at,spark_opacity,5,ms);
    if(spark>.01f)for(int i=0;i<2;i++){int xx=i?191:52,yy=i?192:216;uint32_t c=boot_color(i?0x9474d3:0xff805c,spark);line(xx,yy-6,xx,yy+6,c,3);line(xx-6,yy,xx+6,yy,c,3);}
    boot_label(20,288,152,"POCKET COMPANION",&od_boot_caption,1,1);
    rect(174,292,46,3,0xe4e0d7,0,0);int width=46*ms/3000;if(width)rect(174,292,width,3,0xff805c,0,0);
}
static void status(uint32_t color,bool final) {
    char buf[16];
    if(final) strcpy(buf,"OFF DUTY / T-");
    else if(s_view.utc_us) clock_text(buf,(s_view.utc_us/1000000+28800)%86400/60);
    else strcpy(buf,"--:--");
    TXT(17,12,final?160:100,buf,&od_font_10,color);

    if(s_view.battery<0) strcpy(buf,"--%"); else snprintf(buf,sizeof(buf),"%d%%",s_view.battery);
    text(181,12,42,buf,&od_font_9,color,LV_TEXT_ALIGN_RIGHT);
}

static void buddy(int x,int y,int mood,int ms,bool compact) {
    if(compact){s_origin_x=x;s_origin_y=y;s_scale=s_scale_y=mood==6 || mood==7?.44f:.48f;}
    int bob=(int)(sinf(ms*(mood==7?.02f:.003f))*3);
    if(!compact && (s_view.early_done || s_view.time.phase==OD_DONE)) bob=-(int)(sinf((ms%700)*3.14159265f/700)*14);
    if(s_relax_mode<0)y+=bob;
    int gaze=(ms/6000)%3==1?2:(ms/6000)%3==2?-2:0;
    bool blink=ms%4700>=4300 && ms%4700<4450;
    bool wave=s_relax_mode<0 && ms%16000>=8800 && ms%16000<10200;
    uint32_t body=s_view.boot?0xa3dec0:od_themes[mood].pet,shade=s_view.boot?0x81cfa9:od_themes[mood].shade;
    rect(x+9,y+66,18,8,INK,4,0); rect(x+44,y+66,18,8,INK,4,0);
    rect(x,y,73,70,shade,27,2);rect(x+2,y+2,61,60,body,25,0);
    if(s_relax_mode==1 || s_relax_mode==5 || mood==0 || mood==6) { line(x+18,y+28,x+29,y+29,INK,2); line(x+45,y+29,x+56,y+28,INK,2); }
    else if(mood==7) {line(x+18,y+27,x+27,y+30,INK,3);line(x+46,y+30,x+55,y+27,INK,3);}
    else { rect(x+25+gaze,y+(blink?32:28),5,blink?2:9,INK,3,0); rect(x+43+gaze,y+(blink?32:28),5,blink||mood==2?2:9,INK,3,0); }
    if(mood==4) { rect(x+13,y+19,23,18,body,5,2); rect(x+40,y+19,23,18,body,5,2); line(x+36,y+27,x+40,y+27,INK,2); }
    if(mood==1) { line(x+16,y+17,x+28,y+22,INK,3); line(x+46,y+22,x+59,y+17,INK,3); }
    uint32_t cheek=s_view.boot?0xff9c90:od_themes[mood].seal;
    rect(x+14,y+40,12,5,cheek,3,0); rect(x+46,y+40,12,5,cheek,3,0);
    if(mood==0 || mood==5) rect(x+31,y+41,12,mood==5?17:12,INK,7,0);
    else if(mood==1) {lv_draw_arc_dsc_t d;lv_draw_arc_dsc_init(&d);d.center=(lv_point_t){x+38,y+51};d.radius=8;d.width=2;d.start_angle=180;d.end_angle=360;d.color=lv_color_hex(INK);arc(&d);}
    else {lv_draw_arc_dsc_t d;lv_draw_arc_dsc_init(&d);d.center=(lv_point_t){x+38,y+45};d.radius=8;d.width=2;d.start_angle=0;d.end_angle=180;d.color=lv_color_hex(INK);arc(&d);}
    if(mood==5) rect(x+40,y+54,5,11,0x6dc5e7,3,0);
    if(mood==6) { rect(x+16,y-15,40,8,od_themes[mood].seal,5,2); rect(x+22,y-13,28,3,od_themes[mood].paper,2,0); }
    if(mood==7) {
        const int points[][2]={{0,24},{3,6},{12,14},{18,0},{24,15},{29,7},{29,24}};
        lv_draw_triangle_dsc_t d;lv_draw_triangle_dsc_init(&d);d.color=lv_color_hex(od_themes[mood].seal);
        for(int i=1;i<6;i++){d.p[0]=(lv_point_precise_t){x+21,y+6};d.p[1]=(lv_point_precise_t){x+21+points[i][0],y-18+points[i][1]};d.p[2]=(lv_point_precise_t){x+21+points[i+1][0],y-18+points[i+1][1]};for(int j=0;j<3;j++){d.p[j].x=scaled_x(d.p[j].x);d.p[j].y=scaled_y(d.p[j].y);}lv_draw_triangle(s_layer,&d);}
    }
    if(wave) {line(x-2,y+43,x-9,y+34+(int)(sinf(ms*.025f)*5),INK,3);rect(x-13,y+28+(int)(sinf(ms*.025f)*5),9,9,body,5,1);}
    if(mood==0 && s_relax_mode<0 && s_view.time.phase!=OD_BEFORE) TXT(x+66,y-6,30,"Zz",&od_font_14,INK);
    s_scale=s_scale_y=1.f;
}

static void home(void) {
    uint32_t bg=od_themes[s_view.mood].paper; rect(0,0,240,320,bg,0,0); status(INK,false);
    char weekday[32];const char *days[]={"周日","周一","周二","周三","周四","周五","周六"};
    int day=(s_view.utc_us/1000000+28800)/86400;int wd=(day+4)%7;
    snprintf(weekday,sizeof(weekday),"%s / %s",days[wd],!s_view.connected?"离线续时":wd==0 || wd==6?"休息日":"工作日");CENTER(84,12,90,weekday,&od_font_9,INK);
    TXT(17,40,210,s_view.overtime?"加班有终点。":"自由，正在靠近。",&od_font_14,INK);
    TXT(17,63,210,s_view.overtime?"本次加班还剩":s_view.time.phase==OD_BEFORE?"距离开工还有":"距离下班还有",&od_font_10,INK);
    int seconds=(int)((s_view.time.remaining_us+999999)/1000000);
    char buf[32]; snprintf(buf,sizeof(buf),"%02d:%02d:%02d",seconds/3600,seconds/60%60,seconds%60);
    TXT(17,83,210,s_view.time.phase==OD_WAIT?"--:--:--":buf,&od_digits_35,INK);
    rect(73,207,96,9,THEME.ticket,6,0); buddy(84,142,s_view.mood,s_view.animation_ms,false);
    rect(17,148,58,25,od_themes[s_view.mood].seal,5,0);
    const char *tag=s_mood[s_view.mood];
    CENTER(17,154,58,tag,&od_font_10,od_themes[s_view.mood].ink);
    line(182,133,182,145,THEME.accent,3); line(176,139,188,139,THEME.accent,3);
    const char *quote=s_quote[s_view.mood];
    CENTER(10,226,220,quote,&od_font_12,INK);
    TXT(17,253,165,s_view.overtime?"本次加班进度":"今日打工进度",&od_font_10,INK); snprintf(buf,sizeof(buf),"%d%%",s_view.time.percent); TXT(184,253,40,buf,&od_font_10,INK);
    rect(17,271,206,8,THEME.ticket,3,0); if(s_view.time.percent) rect(17,271,206*s_view.time.percent/100,8,THEME.progress,3,0);
    clock_text(buf,(s_view.time.start_utc+28800)%86400/60); strcat(buf," 开工"); TXT(17,284,80,buf,&od_font_9,INK);
    clock_text(buf,(s_view.time.end_utc+28800)%86400/60); strcat(buf," 自由"); text(143,284,80,buf,&od_font_9,INK,LV_TEXT_ALIGN_RIGHT);
    TXT(17,305,70,"↑ 换心情",&od_font_9,INK);CENTER(85,305,70,"↓ 摸鱼一下",&od_font_9,INK);text(170,305,53,"OK 设置",&od_font_9,INK,LV_TEXT_ALIGN_RIGHT);
}

static void ribbon(int x,int y,int w,int h,uint32_t bg,const char *label,int offset);
static void waiting(void) {
    uint32_t bg=PAPER,band=THEME.ticket,ink=INK,fg=INK;
    rect(0,0,240,320,bg,0,0);status(ink,false);
    char buf[64];snprintf(buf,sizeof(buf),"OFF DUTY CLUB / %s",s_wait_mood[s_view.mood]);TXT(17,42,206,buf,&od_font_8,ink);
    TXT(17,57,206,"本人目前",&od_wait_title,ink);TXT(17,88,206,"尚未营业。",&od_wait_title,ink);
    lv_draw_triangle_dsc_t triangle;lv_draw_triangle_dsc_init(&triangle);triangle.color=lv_color_hex(band);
    triangle.p[0]=(lv_point_precise_t){0,143};triangle.p[1]=(lv_point_precise_t){239,122};triangle.p[2]=(lv_point_precise_t){239,211};lv_draw_triangle(s_layer,&triangle);
    // Overlap the shared diagonal by one pixel to avoid an antialias seam.
    triangle.p[0]=(lv_point_precise_t){0,142};triangle.p[1]=(lv_point_precise_t){239,210};triangle.p[2]=(lv_point_precise_t){0,232};lv_draw_triangle(s_layer,&triangle);
    int sec=(int)((s_view.time.remaining_us+999999)/1000000);
    if(sec>=3600)snprintf(buf,sizeof(buf),"%02d:%02d:%02d",sec/3600,sec/60%60,sec%60);
    else snprintf(buf,sizeof(buf),"%02d:%02d",sec/60,sec%60);
    // Rotate the small clock layer only; the full diagonal band uses triangles.
    lv_area_t area={10,143,158,202};lv_layer_t *parent=s_layer,*clock_layer=lv_draw_layer_create(parent,LV_COLOR_FORMAT_RGB565,&area);
    if(clock_layer){s_layer=clock_layer;rect(10,143,149,60,band,0,0);TXT(17,sec>=3600?163:147,133,buf,sec>=3600?&od_digits_24:&od_digits_45,fg);s_layer=parent;
        lv_draw_image_dsc_t image;lv_draw_image_dsc_init(&image);image.src=clock_layer;image.rotation=3550;image.pivot=(lv_point_t){73,30};image.antialias=true;lv_draw_layer(parent,&image,&area);}
    area=(lv_area_t){19,202,152,222};lv_layer_t *unit=lv_draw_layer_create(parent,LV_COLOR_FORMAT_RGB565,&area);
    if(unit){s_layer=unit;rect(19,202,134,21,band,0,0);TXT(22,206,127,sec>=3600?"距离开工 · 时 / 分 / 秒":"距离开工 · 分 / 秒",&od_font_9,fg);s_layer=parent;
        lv_draw_image_dsc_t image;lv_draw_image_dsc_init(&image);image.src=unit;image.rotation=3550;image.pivot=(lv_point_t){67,10};image.antialias=true;lv_draw_layer(parent,&image,&area);}
    int next_min=(s_view.time.start_utc+28800)%86400/60;
    int charge=od_wait_percent(s_view.time.remaining_us,next_min,s_view.settings.wait_min);
    uint32_t original_ink=INK;INK=THEME.ink;s_origin_x=189;s_origin_y=161;s_scale=s_scale_y=.62f;
    int breathe=(int)lroundf(1-cosf(s_view.animation_ms*2*3.14159265f/2500));
    buddy(153,161-breathe,s_view.mood,0,false);INK=original_ink;
    TXT(17,239,206,s_wait_copy[s_view.mood],&od_font_11,ink);
    TXT(17,259,150,"开工前充能",&od_font_9,ink);snprintf(buf,sizeof(buf),"%d%%",charge);text(180,259,43,buf,&od_font_9,ink,LV_TEXT_ALIGN_RIGHT);
    for(int i=0;i<10;i++){lv_draw_rect_dsc_t block;lv_draw_rect_dsc_init(&block);block.bg_color=lv_color_hex(band);block.bg_opa=i<charge/10?LV_OPA_COVER:i==charge/10?166:46;block.border_width=0;
        lv_area_t block_area={17+i*21,275,17+i*21+17,280};lv_draw_rect(s_layer,&block,&block_area);}
    TXT(17,287,150,"准点开工",&od_font_10,ink);clock_text(buf,next_min);text(177,287,46,buf,&od_font_10,ink,LV_TEXT_ALIGN_RIGHT);
    TXT(20,306,100,"↑ 换心情",&od_font_9,ink);text(120,306,100,"OK 设置",&od_font_9,ink,LV_TEXT_ALIGN_RIGHT);
}
static void ribbon(int x,int y,int w,int h,uint32_t bg,const char *label,int offset) {
    lv_area_t area={x,y,x+w-1,y+h-1};
    lv_layer_t *parent=s_layer;
    lv_layer_t *strip=lv_draw_layer_create(parent,LV_COLOR_FORMAT_RGB565,&area);
    if(!strip)return;
    // Rotate only these small strips, keeping the 48 KB LVGL pool bounded.
    s_layer=strip;
    rect(x,y,w,h,bg,0,0);
    if(offset<0)CENTER(x,y+6,w,label,&od_font_10,INK);
    else TXT(x-offset,y+6,470,label,&od_font_10,INK);
    s_layer=parent;
    lv_draw_image_dsc_t d;lv_draw_image_dsc_init(&d);
    d.src=strip;d.rotation=3570;d.pivot=(lv_point_t){w/2,h/2};d.antialias=true;
    lv_draw_layer(parent,&d,&area);
}
static void final(void) {
    int sec=(s_view.time.remaining_us+999999)/1000000;
    uint32_t bg=sec<=10?THEME.seal:THEME.ticket, fg=INK, accent=THEME.accent;
    rect(0,0,240,320,bg,0,0);
    int drift=s_view.animation_ms/50%28;
    for(int x=135;x<280;x+=28) line(x+drift,0,x+drift-110,320,THEME.pet,2);
    char battery[8];if(s_view.battery<0)strcpy(battery,"--%");else snprintf(battery,sizeof(battery),"%d%%",s_view.battery>100?100:s_view.battery);
    TXT(19,17,160,s_view.overtime?"OVERTIME / T-":"OFF DUTY / T-",&od_font_9,fg);text(181,17,40,battery,&od_font_9,fg,LV_TEXT_ALIGN_RIGHT); TXT(19,51,150,"自由",&od_font_29,fg); TXT(19,81,190,"即将发射",&od_font_29,fg);
    line(181,89,210,60,accent,7); line(188,60,210,60,accent,7); line(210,60,210,82,accent,7);
    char buf[8]; snprintf(buf,sizeof(buf),"%d",sec);
    const lv_font_t *f=sec<=5?&od_digits_148:&od_digits_128;
    int pulse=s_view.utc_us%1000000/1000;
    int digit_y=114+(sec<=5?-12+(pulse<260?(int)(sinf(pulse*3.14159f/260)*-5):0):0);
    CENTER(23,digit_y+5,159,buf,f,THEME.shade);CENTER(19,digit_y,159,buf,f,fg);
    TXT(18,134,32,"T-",&od_font_10,accent); TXT(200,218,30,"秒",&od_font_12,fg);
    const char *stage=sec<=5?"自由发射 / LET'S GO":sec<=10?"撤退就绪 / STAND BY":sec<=30?"收包模式 / GET READY":"收尾模式 / WRAP UP";
    ribbon(19,235,124,24,THEME.seal,stage,-1);
    for(int i=0;i<12;i++) rect(19+i*17,265,14,5,i<(60-sec)/5?accent:THEME.pet,0,0);
    TXT(19,278,202,sec<=5?"把时间，还给自己。":sec<=10?"准备，离开工位。":sec<=30?"包已收好，快乐即将出站。":"存好今天，把快乐带走。",&od_font_10,fg);
    int offset=s_view.animation_ms/(sec<=10?14:30)%200;
    ribbon(-5,294,250,30,PAPER,"下班倒数 / GET READY TO GO / 下班倒数 / GET READY TO GO",offset);
}

static uint32_t confetti_random(uint32_t seed,int i) {
    uint32_t value=seed+(uint32_t)(i+1)*0x9e3779b9u;value^=value>>16;value*=0x85ebca6bu;value^=value>>13;return value;
}
static bool confetti_position(int age,int i,uint32_t seed,int *x,int *y) {
    uint32_t random=confetti_random(seed,i);int delay=random%681;
    if(age<delay)return false;
    int sway=(age/24+(random>>16)%68)%68;
    *x=18+(random>>8)%190+(sway<34?sway:68-sway)-17;
    *y=-16+(age-delay)*336/1200;
    return *y+18>=0 && *y<320;
}
static void confetti(void) {
    if(s_view.confetti_ms<0)return;
    for(int i=0;i<10;i++) {
        int x,y;if(!confetti_position(s_view.confetti_ms,i,s_view.confetti_seed,&x,&y))continue;
        uint32_t random=confetti_random(s_view.confetti_seed,i);
        uint32_t color=(const uint32_t[]){0xff805c,0xe6b735,0x4f9d83,0x59a9e6,0x9474d3}[(random>>20)%5];
        if(random%3==0) {line(x,y,x+4,y+9,color,3);line(x+4,y+9,x+2,y+13,color,2);}
        else if(random%3==1) {
            lv_draw_triangle_dsc_t d;lv_draw_triangle_dsc_init(&d);d.color=lv_color_hex(color);
            d.p[0]=(lv_point_precise_t){x,y};d.p[1]=(lv_point_precise_t){x+8,y+2};d.p[2]=(lv_point_precise_t){x+2,y+11};lv_draw_triangle(s_layer,&d);
        } else {line(x,y,x-3,y+10,color,4);line(x-3,y+10,x+2,y+12,color,2);}
    }
}
static void done(void) {
    int mood=s_view.mood;uint32_t ink=od_themes[mood].ink,mint=od_themes[mood].ticket,original_ink=INK;INK=ink;
    rect(0,0,240,320,PAPER,0,0);
    char buf[24],clock[6];clock_text(clock,(s_view.utc_us/1000000+28800)%86400/60);
    snprintf(buf,sizeof(buf),"OFF DUTY / %s",s_view.utc_us?clock:"--:--");TXT(17,12,168,buf,&od_font_10,ink);
    snprintf(buf,sizeof(buf),s_view.battery<0?"--%%":"%d%%",s_view.battery);text(181,12,42,buf,&od_font_9,ink,LV_TEXT_ALIGN_RIGHT);
    rect(17,44,206,210,mint,0,0);
    for(int i=0;i<9;i++) {
        lv_draw_triangle_dsc_t d;lv_draw_triangle_dsc_init(&d);d.color=lv_color_hex(mint);
        int x=17+(i?4+12*(i-1):0)*206/100,xx=17+(4+12*i)*206/100,tip=17+(i?10+12*(i-1):0)*206/100;
        d.p[0]=(lv_point_precise_t){x,253};d.p[1]=(lv_point_precise_t){xx,253};d.p[2]=(lv_point_precise_t){tip,261};lv_draw_triangle(s_layer,&d);
    }
    lv_draw_label_dsc_t label;lv_draw_label_dsc_init(&label);label.font=&od_font_10;label.color=lv_color_hex(ink);label.text=s_view.overtime?"OVERTIME COMPLETE":"MISSION COMPLETE";label.letter_space=2;
    lv_area_t label_area={33,60,206,60+od_font_10.line_height-1};lv_draw_label(s_layer,&label,&label_area);
    TXT(33,82,174,s_view.overtime?"本次加班":"今日打工",&od_font_29,ink);TXT(33,115,174,"到此为止。",&od_font_29,ink);
    for(int x=33;x<207;x+=5)line(x,163,x+2,163,ink,1);
    TXT(33,171,84,"工作已收尾",&od_font_11,ink);TXT(33,186,84,"快乐待领取",&od_font_11,ink);
#ifdef OD_STAMP_REFERENCE
    lv_area_t area={130,169,214,238};lv_layer_t *parent=s_layer;
    lv_layer_t *stamp=lv_draw_layer_create(parent,LV_COLOR_FORMAT_RGB565,&area);
    if(stamp) {
        s_layer=stamp;rect(130,169,85,70,mint,0,0);
        for(int y=-33;y<=33;y++) {
            int width=(int)(41*sqrtf(1-y*y/(33.0f*33.0f)));
            rect(172-width,204+y,2*width+1,1,od_themes[mood].seal,0,0);
        }
        for(int i=0;i<32;i++) {
            float a=i*6.2831853f/32,b=(i+1)*6.2831853f/32;
            int x=172+(int)(cosf(a)*41),y=204+(int)(sinf(a)*33),xx=172+(int)(cosf(b)*41),yy=204+(int)(sinf(b)*33);
            line(x,y,xx,yy,ink,2);
        }
        CENTER(134,188,77,"已下班",&od_font_21,ink);CENTER(134,214,77,"APPROVED",&od_font_9,ink);
        s_layer=parent;lv_draw_image_dsc_t d;lv_draw_image_dsc_init(&d);d.src=stamp;d.rotation=3470;d.pivot=(lv_point_t){42,35};d.antialias=true;lv_draw_layer(parent,&d,&area);
    }
#else
    const lv_image_dsc_t *images[]={&od_stamp_0,&od_stamp_1,&od_stamp_2,&od_stamp_3,&od_stamp_4,&od_stamp_5,&od_stamp_6,&od_stamp_7};
    lv_draw_image_dsc_t image;lv_draw_image_dsc_init(&image);image.src=images[mood];
    lv_area_t stamp_area={123,160,222,249};lv_draw_image(s_layer,&image,&stamp_area);
#endif
    buddy(37,mood==6 || mood==7?210:204,mood,s_view.animation_ms,true);
    CENTER(17,275,206,"签收快乐，不签收加班。",&od_font_11,ink);
    TXT(17,299,100,"↑↓ 放彩带",&od_font_9,ink);text(153,299,70,"OK 下班",&od_font_9,ink,LV_TEXT_ALIGN_RIGHT);
    confetti();
    INK=original_ink;
}

static void hold(void) {
    int remaining=s_view.hold_remaining_ms;
    rect(0,0,240,320,THEME.ticket,0,0); status(INK,true);
    rect(17,12,160,15,THEME.ticket,0,0);TXT(17,12,160,"EARLY EXIT / HOLD",&od_font_10,INK);
    TXT(19,47,202,"提前下班",&od_font_29,INK);
    TXT(19,82,202,"即将发射",&od_font_29,INK);
    char digit[2]={(char)('0'+(remaining+999)/1000),0};
    CENTER(23,126,202,digit,&od_digits_128,THEME.shade);CENTER(19,121,202,digit,&od_digits_128,INK);
    for(int i=0;i<OD_HOLD_MS/1000;i++) rect(19+i*42,258,34,5,(OD_HOLD_MS-remaining)>i*1000?THEME.seal:THEME.pet,3,0);
    CENTER(19,278,202,"继续按住下键",&od_font_12,INK);
    CENTER(19,301,202,"松手取消 / 归零直接下班",&od_font_9,INK);
}
static void early_done(void) {
    rect(0,0,240,320,THEME.ticket,0,0);status(INK,false);
    TXT(17,49,206,"EARLY EXIT / 偷跑成功",&od_font_10,INK);
    TXT(17,82,206,"一键下班！",&od_font_29,INK);
    TXT(17,118,206,"自由先行。",&od_font_29,INK);
    CENTER(17,164,206,"今天的快乐，提前签收。",&od_font_12,INK);
    buddy(84,211,s_view.mood,s_view.animation_ms,false);
    CENTER(17,293,206,"OK 返回下班首页",&od_font_11,INK);
    CENTER(17,308,206,"↑ ↓ 彩带 / 长按 OK 设置",&od_font_9,INK);
    confetti();
}
static void off_home(void) {
    rect(0,0,240,320,PAPER,0,0); status(INK,false);
    TXT(17,43,206,"OFF DUTY / LIFE ON",&od_font_10,INK);
    TXT(17,66,223,"已下班，好好生活。",&od_font_23,INK);
    TXT(17,106,206,"自由时间已开启",&od_font_10,INK);
    int sec=s_view.free_us/1000000;if(sec<0)sec=0;
    char buf[16];snprintf(buf,sizeof(buf),"%02d:%02d:%02d",sec/3600,sec/60%60,sec%60);
    TXT(17,124,206,buf,&od_digits_35,INK);
    buddy(84,186,s_view.mood,s_view.animation_ms,false);
    rect(17,194,58,23,THEME.ticket,6,1);CENTER(17,200,58,"下班模式",&od_font_10,INK);
    CENTER(17,266,206,"工作暂停，快乐继续。",&od_font_12,INK);
    line(17,287,223,287,THEME.ticket,1);
    TXT(17,300,70,"↑ 上班 / 加班",&od_font_9,INK);CENTER(85,300,70,"↓ 放松一下",&od_font_9,INK);text(170,300,53,"OK 设置",&od_font_9,INK,LV_TEXT_ALIGN_RIGHT);
}
static void work_choice(void) {
    rect(0,0,240,320,PAPER,0,0);status(INK,false);
    TXT(17,44,206,"BACK TO WORK / 选择模式",&od_font_10,INK);
    TXT(17,69,206,"这次怎么上班？",&od_font_24,INK);
    for(int i=0;i<2;i++) {
        int y=126+i*75;
        rect(17,y,206,62,i==s_view.work_option?THEME.ticket:PAPER,10,1);
        TXT(29,y+10,182,i?"加个班":"继续上班",&od_font_21,INK);
        TXT(29,y+39,182,i?"独立倒计时 · 不改日常作息":"调节下班时间 · 同步到设置",&od_font_10,INK);
    }
    CENTER(17,285,206,"↑ ↓ 选择 · OK 确认",&od_font_11,INK);
    CENTER(17,306,206,"长按 OK 取消",&od_font_9,INK);
}
static void resume_picker(void) {
    rect(0,0,240,320,PAPER,0,0);status(INK,false);
    TXT(17,44,206,s_view.overtime_picker?"OVERTIME / EXTRA ROUND":"BACK TO WORK",&od_font_10,INK);
    TXT(17,69,206,s_view.overtime_picker?"给加班定个点":"设置下班时间",&od_font_29,INK);
    TXT(17,112,206,s_view.overtime_picker?"只记这一段，不改变日常作息。":"重新上班前，先定好自由时刻。",&od_font_10,INK);
    rect(17,148,206,76,THEME.ticket,12,1);
    char clock[8];clock_text(clock,s_view.overtime_picker?s_view.overtime_min:s_view.resume_end_min);
    CENTER(17,s_view.overtime_picker?157:165,206,clock,&od_digits_35,INK);
    if(s_view.overtime_picker) {
        char end[8],label[32];clock_text(end,(s_view.utc_us/1000000+28800+s_view.overtime_min*60)%86400/60);
        snprintf(label,sizeof(label),"%s %s 结束",((s_view.utc_us/1000000+28800)%86400+s_view.overtime_min*60)>=86400?"次日":"预计",end);
        CENTER(17,205,206,label,&od_font_10,INK);
    }
    CENTER(17,238,206,s_view.notice==2?"上下班时间不能相同":s_view.notice==1?"保存失败，请重试":s_view.overtime_picker?"加班时长 · 每次调整半小时":"每次调整 1 分钟 · 同步到设置",&od_font_10,INK);
    rect(17,269,206,25,THEME.ticket,8,0);
    CENTER(17,275,206,s_view.overtime_picker?"OK 开始本次加班":"OK 保存并上班",&od_font_11,INK);
    CENTER(17,306,206,"↑ ↓ 调整 / 长按 OK 取消",&od_font_9,INK);
}
static void break_footer(bool after_work) {
    int age=s_view.effect_ms<0?0:s_view.effect_ms>3000?3000:s_view.effect_ms;
    rect(17,289,206,3,THEME.ticket,0,0);rect(17,289,206*(3000-age)/3000,3,INK,0,0);
    TXT(17,299,140,after_work?"3 秒后回到下班首页":"3 秒后回到上班首页",&od_font_9,INK);
    text(157,299,66,after_work?"↓ 再放松一下":"↓ 再摸鱼一下",&od_font_9,INK,LV_TEXT_ALIGN_RIGHT);
}
static void relax(void) {
    const char *titles[]={"拉伸一下。","沙发，接住我。","下班小舞步。","快乐先开饭。","脑袋放个假。","眯一下也行。"};
    const char *quotes[]={"伸个懒腰，把班味抖掉。","今日已下线，有事明天说。","不为 KPI，只为开心。","工作已收尾，胃口刚上线。","待办飘走，快乐留下。","今日电量，留给自己。"};
    int mode=s_view.relax,t=s_view.effect_ms<0?0:s_view.effect_ms;uint32_t paper=PAPER;
    rect(0,0,240,320,paper,0,0);status(INK,false);
    TXT(17,44,206,"OFF DUTY / JUST CHILL",&od_font_10,INK);
    TXT(17,70,206,titles[mode],&od_font_24,INK);
    rect(75,232,90,7,THEME.ticket,5,0);
    if(mode==1) {rect(50,207,140,36,THEME.shade,10,2);rect(52,209,136,26,THEME.pet,10,0);}
    if(mode==5) rect(60,214,120,28,THEME.ticket,12,2);
    int motion_t=mode==0 && t>2800?2800:mode==2 && t>2750?2750:t;
    float wave=(1.f-cosf((motion_t%(mode==2?550:mode==3?600:1400))*6.2831853f/(mode==2?550:mode==3?600:1400)))/2;
    const lv_image_dsc_t *normal[]={&od_pet_0,&od_pet_1,&od_pet_2,&od_pet_3,&od_pet_4,&od_pet_5,&od_pet_6,&od_pet_7};
    const lv_image_dsc_t *resting[]={&od_pet_rest_0,&od_pet_rest_1,&od_pet_rest_2,&od_pet_rest_3,&od_pet_rest_4,&od_pet_rest_5,&od_pet_rest_6,&od_pet_rest_7};
    lv_draw_image_dsc_t image;lv_draw_image_dsc_init(&image);image.src=(mode==1 || mode==5?resting:normal)[s_view.mood];image.pivot=(lv_point_t){36,53};image.antialias=true;
    int dy=0,angle=0;
    if(mode==0) {dy=-(int)(7*wave);image.scale_x=(int)(256*(1-.06f*wave));image.scale_y=(int)(256*(1+.13f*wave));}
    if(mode==1) {
        float phase=t<750?t/750.f:t<1200?(t-750)/450.f:(t-1200)/1800.f;
        dy=(int)(t<750?-23+24*phase:t<1200?1-6*phase:-5+3*phase);
        angle=(int)(t<750?23*phase:t<1200?23-7*phase:16+4*phase);
    }
    if(mode==2) {angle=(int)(-10+20*wave);dy=-2-(int)(6*wave);}
    if(mode==3) {dy=-10+(int)(5*wave);image.scale_x=(int)(256*(1+.03f*wave));image.scale_y=(int)(256*(1-.06f*wave));}
    if(mode==4) {float breath=sinf(t*3.14159265f/3000);dy=-(int)(8*breath);image.scale_x=image.scale_y=(int)(256*(1+.04f*breath));}
    if(mode==5) {float settle=t<600?t/600.f:t>2550?(3000-t)/450.f:1;angle=(int)(-12*settle);dy=(int)(3*settle)-3;image.scale_x=image.scale_y=(int)(256*(1+.025f*sinf(t*3.14159265f/3000)*settle));}
    image.rotation=(angle*10+3600)%3600;lv_area_t target={84,139+dy,156,230+dy};lv_draw_image(s_layer,&image,&target);
    if(mode==0) {line(69,184-(int)(20*wave),69,203-(int)(20*wave),INK,4);line(170,184-(int)(20*wave),170,203-(int)(20*wave),INK,4);}
    if(mode==2) {int dy=(int)(8*sinf(t*3.14159265f/1000));TXT(49,151-dy,28,"♪",&od_font_24,THEME.accent);TXT(160,151-dy,30,"♫",&od_font_24,THEME.accent);}
    if(mode==3) {rect(96,210,49,31,THEME.seal,16,2);rect(103,202,35,13,PAPER,7,2);}
    if(mode==4) {
        int dy=8-t*41/3000;
        if(t>250 && t<2900) {rect(47,191+dy,16,16,paper,8,2);rect(166,198+dy,10,10,THEME.ticket,5,0);rect(143,160+dy,8,8,THEME.pet,4,0);}
    }
    if(mode==5 && t>250 && t<2900)TXT(168,150-t*33/3000,30,"Zz",&od_font_14,THEME.accent);
    CENTER(17,262,206,quotes[mode],&od_font_12,INK);break_footer(true);
}
static void egg(void) {
    int e=s_view.egg, ms=s_view.effect_ms<0?0:s_view.effect_ms;
    rect(0,0,240,320,PAPER,0,0);status(INK,false);
    TXT(17,44,206,"OFF DUTY / TAKE A BREAK",&od_font_10,INK);
    TXT(17,70,206,s_eggs[e],&od_font_24,INK);
    if(e==0) { rect(35,204,170,21,THEME.ticket,8,0); line(165,140,165,190,INK,2); line(165,190,158,196,INK,2); int x=104+(int)(sinf(ms*.004f)*28); rect(x,172,44,26,THEME.seal,15,2); line(x-12,174,x,186,INK,3); line(x-12,198,x,186,INK,3); rect(x+31,179,3,3,INK,2,0); }
    if(e==1) { rect(77,175,66,50,THEME.ticket,12,2); rect(140,184,21,23,PAPER,8,3); rect(83,178,54,9,0x96603b,4,0); for(int i=0;i<3;i++){int x=91+i*16+(int)(sinf(ms*.003f+i)*3); line(x,164,x+3,141,0xa59c94,2);} }
    if(e==2) { for(int i=0;i<3;i++) rect(120-45+i*15,182-45+i*15,90-i*30,90-i*30,PAPER,45,1); float a=ms*.002f; line(120,182,120+cosf(a)*44,182+sinf(a)*44,THEME.progress,3); rect(136,160,6,6,THEME.accent,3,0); }
    if(e==3) { rect(159,143,30,79,THEME.seal,3,2); int x=51+ms*108/3000; rect(x,168,35,42,THEME.pet,15,2); rect(x+9,182,4,6,INK,2,0); rect(x+23,182,4,6,INK,2,0); }
    if(e==4) { rect(51,154,131,56,PAPER,8,3); rect(182,169,7,26,INK,2,0); for(int i=0;i<4;i++) rect(59+i*29,162,23,40,(ms/350%5)>i?THEME.pet:THEME.ticket,3,0); }
    CENTER(17,262,206,s_egg_quote[e],&od_font_12,INK);break_footer(false);
}

static void menu(void) {
    char value[40]; rect(0,0,240,320,PAPER,0,0); status(INK,false);
    TXT(17,39,140,"我的小工位",&od_font_14,INK);
    text(163,43,60,"v" OD_APP_VERSION,&od_font_9,INK,LV_TEXT_ALIGN_RIGHT);
    if(s_view.menu==2) {
        if(!s_view.portal){CENTER(17,142,206,"正在准备连接…",&od_font_12,INK);return;}
        CENTER(17,66,206,s_view.phone_connected?"手机已连接热点":"扫码连接设备热点",&od_font_12,INK);
        char payload[96];
        snprintf(payload,sizeof(payload),"WIFI:T:WPA;S:%s;P:%s;;",s_view.ap_name,s_view.ap_password);
        if(strcmp(payload,s_qr_payload)) {
            uint8_t scratch[sizeof(s_qr)];
            if(qrcodegen_encodeText(payload,scratch,s_qr,qrcodegen_Ecc_MEDIUM,1,5,qrcodegen_Mask_AUTO,true))strcpy(s_qr_payload,payload);
            else {s_qr_payload[0]=0;memset(s_qr,0,sizeof(s_qr));}
        }
        if(!s_qr_payload[0]){CENTER(17,142,206,"连接码生成失败，请重试",&od_font_12,INK);return;}
        int size=qrcodegen_getSize(s_qr),scale=123/(size+8),width=(size+8)*scale,left=(240-width)/2;
        rect(left,86,width,width,0xffffff,0,0);
        for(int y=0;y<size;y++)for(int x=0;x<size;x++)if(qrcodegen_getModule(s_qr,x,y))rect(left+(x+4)*scale,86+(y+4)*scale,scale,scale,0,0,0);
        CENTER(17,218,206,s_view.ap_name,&od_font_10,INK);
        rect(17,238,206,30,THEME.ticket,8,1);
        CENTER(22,246,196,"http://192.168.4.1",&od_font_12,INK);
        CENTER(17,276,206,s_view.phone_connected?(s_view.connected?"Wi-Fi 已连接 · 可调整设置":"浏览器打开地址，配网与设置"):"连上热点后，在浏览器打开地址",&od_font_10,INK);
        CENTER(17,298,206,"OK 关闭热点",&od_font_10,INK); return;
    }
    int first=s_view.selected/5*5;
    for(int i=first;i<first+5 && i<OD_MENU_ROWS;i++) {
        int y=78+(i-first)*38;
        if(i==s_view.selected) rect(12,y-4,216,34,s_view.edit?THEME.seal:THEME.ticket,8,1);
        TXT(20,y+7,108,s_menu[i],&od_font_12,INK); value[0]=0;
        switch(i) {
        case 0:clock_text(value,s_view.settings.start_min);break;
        case 1:clock_text(value,s_view.settings.end_min);break;
        case 2:clock_text(value,s_view.settings.wait_min);break;
        case 3:snprintf(value,sizeof(value),"%d%%",s_view.settings.brightness);break;
        case 4:snprintf(value,sizeof(value),"%d%%",s_view.settings.volume);break;
        case 5:strcpy(value,(const char *[]){"常亮","1分钟","2分钟","5分钟","10分钟","30分钟"}[od_sleep_choice(&s_view.settings)]);break;
        case 6:strcpy(value,s_view.settings.random_eggs?"开启":"关闭");break;
        case 7:strcpy(value,s_view.connected?"已连接":"配网");break;
        case 8:strcpy(value,s_view.connected?"同步":"离线");break;
        case 9:strcpy(value,s_view.synced?(s_view.phone_time?"手机校准":s_view.connected?"网络校准":"离线续时"):"等待校准");break;
        case 10:strcpy(value,s_view.edit?"确认删除？":"需确认");break;
        default:break;
        }
        text(126,y+7,92,value,&od_font_12,INK,LV_TEXT_ALIGN_RIGHT);
    }
    CENTER(8,278,224,s_view.notice==1?"保存失败，请重试":s_view.notice==2?"上下班时间不能相同":s_view.notice==3?"网络启动失败，请重启":s_view.edit?"↑↓ 调整 · OK 保存":"↑↓ 选择 · OK 编辑",&od_font_11,INK);
    CENTER(8,303,224,"长按 OK 返回",&od_font_11,INK);
}

static void welcome(void) {
    od_setup_t stage=od_setup_state(s_view.connected,s_view.synced,s_view.plan_ready);
    bool plan=stage==OD_NEED_PLAN;
    rect(0,0,240,320,PAPER,0,0);status(INK,false);
    TXT(17,43,206,"HELLO / 自由准备中",&od_font_10,INK);
    TXT(17,66,122,plan?"把自由":"先校准",&od_font_23,INK);
    TXT(17,94,122,plan?"定个点。":"时间。",&od_font_23,INK);
    buddy(149,82,s_view.mood,s_view.animation_ms,false);
    TXT(17,163,206,plan?"设置作息，开启你的下班倒数。":stage==OD_NEED_TIME?"正在对时，也可使用手机校时。":"连接热点，用手机校准时间。",&od_font_10,INK);
    for(int i=0;i<2;i++) {
        int y=187+i*35;bool ready=i==0?s_view.synced:s_view.plan_ready;
        rect(17,y,206,29,ready?THEME.ticket:PAPER,7,0);
        TXT(26,y+8,148,i==0?"01 / 网络或手机校时":"02 / 设置上下班时间",&od_font_10,INK);
        text(175,y+8,40,ready?"已完成":"待完成",&od_font_10,INK,LV_TEXT_ALIGN_RIGHT);
    }
    rect(17,269,206,28,INK,8,0);
    CENTER(17,275,206,plan?"OK 设置作息":"OK 联网 / 手机校时",&od_font_12,PAPER);
    CENTER(8,301,224,"↑ 网络设置 / ↓ 作息设置",&od_font_10,INK);
}

static void draw(lv_event_t *e) {
    s_layer=lv_event_get_layer(e);
    INK=THEME.ink;
#ifdef OD_PET_REFERENCE
    s_relax_mode=s_view.relax;buddy(84,157,s_view.mood,0,false);s_relax_mode=-1;return;
#endif
    if(s_view.boot) {INK=0x293534;boot();}
    else if(s_view.hold_remaining_ms>0) hold();
    else if(s_view.menu) menu();
    else if(od_setup_state(s_view.connected,s_view.synced,s_view.plan_ready)!=OD_READY) welcome();
    else if(s_view.work_choice) work_choice();
    else if(s_view.resume_picker) resume_picker();
    else if(s_view.off_home) {if(s_view.relax>=0)relax();else off_home();}
    else if(s_view.early_done) early_done();
    else if(s_view.time.phase==OD_FINAL) final();
    else if(s_view.time.phase==OD_DONE) done();
    else if(s_view.time.phase==OD_BEFORE) waiting();
    else {if(s_view.egg>=0)egg();else home();}
    if(s_view.feedback && !s_view.boot) {
        const char *messages[]={"","设置已保存","保存失败，请重试","手机校准完成"};
        rect(8,274,224,46,PAPER,0,0);
        rect(17,277,206,28,PAPER,8,1);
        CENTER(17,284,206,messages[s_view.feedback],&od_font_12,s_view.feedback==2?0x914123:INK);
    }
}
lv_obj_t *od_ui_create(void) {
    lv_obj_t *scr=lv_obj_create(NULL); lv_obj_remove_style_all(scr);
    lv_obj_set_size(scr,240,320); lv_obj_remove_flag(scr,LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(scr,draw,LV_EVENT_DRAW_MAIN,NULL); lv_screen_load(scr); return scr;
}
static bool stamp_visible(const od_view_t *v) {
    return !v->boot && v->time.phase==OD_DONE && !v->menu && !v->off_home && !v->resume_picker && !v->work_choice && !v->early_done && !v->hold_remaining_ms && od_setup_state(v->connected,v->synced,v->plan_ready)==OD_READY;
}
static bool waiting_visible(const od_view_t *v) {
    return !v->boot && v->time.phase==OD_BEFORE && !v->menu && !v->off_home && !v->resume_picker && !v->work_choice && !v->early_done && !v->hold_remaining_ms && !v->feedback && od_setup_state(v->connected,v->synced,v->plan_ready)==OD_READY;
}
static int waiting_breathe(int ms) {return (int)lroundf(1-cosf(ms*2*3.14159265f/2500));}
void od_ui_update(lv_obj_t *screen,const od_view_t *view) {
    bool same_wait=waiting_visible(&s_view) && waiting_visible(view) && s_view.mood==view->mood && s_view.battery==view->battery && s_view.utc_us/60000000==view->utc_us/60000000 && s_view.time.start_utc==view->time.start_utc && s_view.settings.start_min==view->settings.start_min && s_view.settings.wait_min==view->settings.wait_min;
    bool wait_pet_changed=waiting_breathe(s_view.animation_ms)!=waiting_breathe(view->animation_ms);
    bool wait_clock_changed=(s_view.time.remaining_us+999999)/1000000!=(view->time.remaining_us+999999)/1000000;
    bool wait_progress_changed=od_wait_percent(s_view.time.remaining_us,s_view.settings.start_min,s_view.settings.wait_min)!=od_wait_percent(view->time.remaining_us,view->settings.start_min,view->settings.wait_min);
    bool same_stamp=stamp_visible(&s_view) && stamp_visible(view) && s_view.feedback==view->feedback && s_view.overtime==view->overtime && !s_view.work_choice && !view->work_choice && s_view.mood==view->mood && s_view.battery==view->battery && s_view.utc_us/60000000==view->utc_us/60000000;
    int old_confetti_ms=s_view.confetti_ms;
    uint32_t old_confetti_seed=s_view.confetti_seed;
    bool same_effect=!s_view.boot && !view->boot && !s_view.menu && !view->menu && !s_view.resume_picker && !view->resume_picker && s_view.feedback==view->feedback && s_view.overtime==view->overtime && !s_view.work_choice && !view->work_choice && s_view.mood==view->mood && s_view.battery==view->battery && s_view.utc_us/60000000==view->utc_us/60000000 && s_view.effect_ms<=view->effect_ms && od_setup_state(view->connected,view->synced,view->plan_ready)==OD_READY && ((s_view.off_home && view->off_home && s_view.relax>=0 && s_view.relax==view->relax) || (!s_view.off_home && !view->off_home && s_view.time.phase==OD_WORK && view->time.phase==OD_WORK && s_view.egg>=0 && s_view.egg==view->egg));
    s_view=*view;
    if(same_stamp) {
        if(s_view.feedback){lv_area_t feedback_area={8,274,231,319};lv_obj_invalidate_area(screen,&feedback_area);}
        lv_area_t pet_area={29,194,89,249};lv_obj_invalidate_area(screen,&pet_area);
        // Include old positions so falling fragments never leave trails over text.
        lv_area_t area={239,319,0,0};bool visible=false;
        for(int frame=0;frame<2;frame++)for(int i=0;i<10;i++) {
            int x,y;if(!confetti_position(frame?s_view.confetti_ms:old_confetti_ms,i,frame?s_view.confetti_seed:old_confetti_seed,&x,&y))continue;
            if(x-6<area.x1)area.x1=x-6;
            if(x+12>area.x2)area.x2=x+12;
            if(y-3<area.y1)area.y1=y-3;
            if(y+19>area.y2)area.y2=y+19;
            visible=true;
        }
        if(visible)lv_obj_invalidate_area(screen,&area);
    } else if(same_effect) {
        // Keep transformed-image clipping aligned with the 40-row LCD buffer.
        lv_area_t area={0,80,239,319};lv_obj_invalidate_area(screen,&area);
    } else if(same_wait) {
        // Preserve rotated-clock clipping on the 40-row LCD buffer.
        if(wait_pet_changed || wait_clock_changed){lv_area_t area={0,120,239,239};lv_obj_invalidate_area(screen,&area);}
        if(wait_progress_changed){lv_area_t area={17,259,222,280};lv_obj_invalidate_area(screen,&area);}
    } else lv_obj_invalidate(screen);
}
