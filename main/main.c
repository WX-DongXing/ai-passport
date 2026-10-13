#include "offduty_model.h"
#include "offduty_net.h"
#include "offduty_ui.h"
#include "bsp_audio.h"
#include "bsp_battery.h"
#include "bsp_button.h"
#include "bsp_display.h"
#include "bsp_i2c.h"
#include "bsp_pins.h"
#include "key_guard.h"
#include "esp_log.h"
#include "esp_random.h"
#include "esp_sleep.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "nvs.h"
#include <math.h>
#include <string.h>

typedef struct { bsp_btn_t btn; bsp_btn_ev_t event; int64_t at_us; } input_event_t;
typedef struct { int kind; uint8_t volume; } sound_t;
static QueueHandle_t s_input_queue,s_sound_queue;
static volatile bool s_input_ready;
static key_guard_t s_key_guard;
static od_view_t s_view;
static od_settings_t s_saved;
static nvs_handle_t s_nvs;
static bool s_have_nvs,s_asleep,s_woke_this_day,s_portal_seen;
static int64_t s_activity,s_egg_end,s_next_egg,s_day=-1,s_celebrated=-1,s_tone_end=-1;
static int64_t s_feedback_at=-1;
static unsigned s_phone_revision;
static int s_last_egg=-1,s_last_relax=-1,s_last_second=-1;
static int64_t s_down_since=-1,s_down_pressed=-1,s_early_end=-1;
static int64_t s_early_at,s_confetti_at=-1,s_resume_start=-1,s_resume_end=-1;
static lv_obj_t *s_screen;
static int64_t s_overtime[2];
static bool store_overtime(int64_t start,int64_t end) {
    int64_t session[]={start,end};
    if(!s_have_nvs || nvs_set_blob(s_nvs,"overtime",session,sizeof(session))!=ESP_OK || nvs_commit(s_nvs)!=ESP_OK) {s_view.notice=1;return false;}
    memcpy(s_overtime,session,sizeof(session));s_view.notice=0;return true;
}

// Callback runs on esp_timer: consume boot-held key, then enqueue only.
static void on_key(bsp_btn_t btn,bsp_btn_ev_t ev,void *user) {
    (void)user;
    if(key_guard_consume(&s_key_guard,ev==BSP_BTN_RELEASE,esp_timer_get_time())) return;
    if(!s_input_ready || !s_input_queue) return;
    input_event_t input={btn,ev,esp_timer_get_time()}; (void)xQueueSend(s_input_queue,&input,0);
}
static void play(int kind) {
    if(!s_sound_queue || !s_view.settings.volume) return;
    sound_t sound={kind,s_view.settings.volume}; (void)xQueueSend(s_sound_queue,&sound,0);
}
// All codec calls and bounded PCM writes have a single owner outside the UI task.
static void audio_worker(void *arg) {
    (void)arg; sound_t sound; int16_t pcm[256];
    bsp_audio_sleep();
    for(;;) {
        if(xQueueReceive(s_sound_queue,&sound,portMAX_DELAY)!=pdTRUE) continue;
        if(bsp_audio_wake()!=ESP_OK || bsp_audio_set_format(16000,16,1)!=ESP_OK) continue;
        bsp_audio_set_volume(sound.volume);
        int notes=sound.kind==3?4:1;
        for(int note=0;note<notes;note++) {
            int samples=sound.kind==0?640:1920;
            float freq=sound.kind==3?(float[]){523,659,784,1047}[note]:sound.kind==2?1047:sound.kind==1?784:440;
            for(int pos=0;pos<samples;pos+=256) {
                int n=samples-pos<256?samples-pos:256;
                for(int i=0;i<n;i++) {
                    int p=pos+i; float envelope=fminf(1.f,fminf(p/80.f,(samples-p)/160.f));
                    pcm[i]=(int16_t)(sinf(6.2831853f*freq*p/16000)*5000*envelope);
                }
                if(bsp_audio_write(pcm,n*sizeof(int16_t))!=ESP_OK) break;
            }
        }
        bsp_audio_sleep();
    }
}
static void save(void) {
    if(!od_settings_valid(&s_view.settings)) { s_view.notice=2; return; }
    if(!s_have_nvs || nvs_set_blob(s_nvs,"settings",&s_view.settings,sizeof(s_view.settings))!=ESP_OK || (s_view.selected<2 && nvs_set_u8(s_nvs,"plan",1)!=ESP_OK) || nvs_set_u8(s_nvs,"waitv",1)!=ESP_OK || nvs_commit(s_nvs)!=ESP_OK) { s_view.notice=1; return; }
    if(s_view.selected<2) s_view.plan_ready=true;
    if(s_view.selected<2){s_resume_start=s_resume_end=-1;}
    s_saved=s_view.settings; s_view.edit=0; s_view.notice=0;
}
static void close_menu(void) {
    s_view.settings=s_saved; s_view.menu=0; s_view.edit=0; s_view.notice=0;
    if(s_view.portal) od_net_action(OD_NET_CLOSE);
}
static void adjust(int delta) {
    od_settings_t *s=&s_view.settings; int row=s_view.selected;
    if(row==0) s->start_min=od_shift_step(s->start_min,delta);
    if(row==1) s->end_min=od_shift_step(s->end_min,delta);
    if(row==3) {int v=s->brightness+delta*5; s->brightness=v<10?10:v>100?100:v;}
    if(row==4) {int v=s->volume+delta*5; s->volume=v<0?0:v>100?100:v; play(0);}
    if(row==5) od_sleep_set(s,(od_sleep_choice(s)+6+delta)%6);
    if(row==6) s->random_eggs=!s->random_eggs;
    if(row==2)s->wait_min=od_wait_step(s,delta);
    if(row<2)od_wait_link(s);
    s_view.notice=0;
}
static void early_exit(void) {
    if(s_view.overtime) {
        int64_t end=s_view.utc_us/1000000;
        if(end<=s_overtime[0])end=s_overtime[0]+1;
        if(!store_overtime(s_overtime[0],end)){s_down_since=s_down_pressed=-1;s_view.hold_remaining_ms=0;s_view.menu=1;return;}
        s_view.time.end_utc=end;s_celebrated=end;
    }
    s_early_end=s_view.time.end_utc;s_early_at=s_view.utc_us;s_view.early_done=true;s_view.off_home=false;s_down_since=-1;s_confetti_at=esp_timer_get_time();s_view.confetti_seed=esp_random();play(3);
}
static void input(const input_event_t *in,int64_t now) {
    bsp_btn_ev_t event=in->event;
    if(in->btn==BSP_BTN_DOWN && event==BSP_BTN_CLICK) return;
    if(in->btn==BSP_BTN_DOWN && event==BSP_BTN_RELEASE) {
        if(s_down_since>=0 && od_hold_remaining(s_down_since,in->at_us,true)==0 && s_view.time.phase==OD_WORK) {early_exit();s_down_pressed=-1;return;}
        bool short_press=od_press_is_short(s_down_pressed,in->at_us);
        s_down_pressed=-1;s_down_since=-1;s_view.hold_remaining_ms=0;
        if(!short_press) return;
        event=BSP_BTN_CLICK;
    }
    if(in->btn==BSP_BTN_DOWN && event==BSP_BTN_PRESS) {
        if(s_asleep) {s_asleep=false;s_woke_this_day=true;s_activity=now;s_down_pressed=-1;return;}
        s_down_pressed=in->at_us;
        if(!s_asleep && !s_view.menu && !s_view.resume_picker && !s_view.work_choice && !s_view.early_done && s_view.time.phase==OD_WORK) {s_down_since=in->at_us;s_view.egg=-1;s_activity=now;}
        return;
    }
    if(event!=BSP_BTN_CLICK && !(event==BSP_BTN_LONG && in->btn==BSP_BTN_OK)) return;
    s_activity=now;
    if(s_asleep) {s_asleep=false; s_woke_this_day=true; return;}
    if(event==BSP_BTN_LONG) {
        if(s_view.resume_picker || s_view.work_choice){s_view.resume_picker=false;s_view.work_choice=false;s_view.off_home=true;return;}
        if(s_view.menu)close_menu();
        else {s_view.menu=1;s_view.selected=0;s_view.egg=-1;}
        return;
    }
    if(in->btn==BSP_BTN_OK){s_view.egg=-1;s_view.relax=-1;s_egg_end=now;}
    if(!s_view.menu && od_setup_state(s_view.connected,s_view.synced,s_view.plan_ready)!=OD_READY) {
        if(in->btn==BSP_BTN_UP) {s_view.menu=1;s_view.selected=7;}
        else if(in->btn==BSP_BTN_DOWN) {s_view.menu=1;s_view.selected=0;}
        else if(!s_view.synced) {if(od_net_action(OD_NET_SETUP)){s_portal_seen=false;s_view.selected=0;s_view.menu=2;}else{s_view.menu=1;s_view.selected=7;s_view.notice=3;}}
        else {s_view.menu=1;s_view.selected=0;}
        return;
    }
    if(s_view.work_choice && !s_view.menu) {
        if(in->btn!=BSP_BTN_OK)s_view.work_option=1-s_view.work_option;
        else {
            int current=(s_view.utc_us/1000000+28800)%86400/60;
            s_view.resume_end_min=current<s_saved.end_min?s_saved.end_min:od_shift_step(current,1);
            s_view.overtime_min=30;s_view.overtime_picker=s_view.work_option==1;
            s_view.work_choice=false;s_view.resume_picker=true;s_view.notice=0;
        }
        return;
    }
    if(s_view.resume_picker && !s_view.menu) {
        if(in->btn==BSP_BTN_OK) {
            if(s_view.overtime_picker) {
                int64_t start=s_view.utc_us/1000000,end=start+s_view.overtime_min*60;
                if(!od_overtime_valid(start,end) || !store_overtime(start,end))return;
                s_view.overtime=true;
            } else {
                od_settings_t previous=s_view.settings;int selected=s_view.selected;
                s_view.settings.end_min=s_view.resume_end_min;od_wait_link(&s_view.settings);
                if(!od_settings_valid(&s_view.settings)){s_view.settings=previous;s_view.notice=2;return;}
                int64_t cleared[]={0,0};
                if(!s_have_nvs || nvs_set_blob(s_nvs,"overtime",cleared,sizeof(cleared))!=ESP_OK){s_view.settings=previous;s_view.notice=1;return;}
                s_view.selected=1;save();s_view.selected=selected;
                if(s_view.notice){s_view.settings=previous;return;}
                memset(s_overtime,0,sizeof(s_overtime));
                s_resume_start=s_view.utc_us/1000000;s_resume_end=od_resume_end(s_view.utc_us,s_view.resume_end_min);
                s_view.overtime=false;
            }
            s_early_end=-1;s_view.early_done=false;s_view.off_home=false;s_view.resume_picker=false;s_confetti_at=-1;
            s_view.egg=-1;s_view.relax=-1;s_egg_end=now;s_next_egg=now+900000000;
            s_view.time=s_view.overtime?od_time_between(s_view.utc_us,s_overtime[0],s_overtime[1]):od_time_between(s_view.utc_us,s_resume_start,s_resume_end);
        } else {if(s_view.overtime_picker)s_view.overtime_min=od_overtime_step(s_view.overtime_min,in->btn==BSP_BTN_UP?1:-1);else s_view.resume_end_min=od_shift_step(s_view.resume_end_min,in->btn==BSP_BTN_UP?1:-1);s_view.notice=0;}
        return;
    }
    if(s_view.time.phase==OD_FINAL && !s_view.early_done) return;
    if(!s_view.menu && !s_view.off_home && (s_view.early_done || s_view.time.phase==OD_DONE)) {
        if(in->btn==BSP_BTN_OK)s_view.off_home=true;
        else {s_confetti_at=now;s_view.confetti_seed=esp_random();play(1);}
        return;
    }
    if((s_view.early_done || s_view.off_home) && !s_view.menu) {
        if(in->btn==BSP_BTN_UP) {
            s_view.work_choice=true;s_view.work_option=0;s_view.egg=-1;s_view.relax=-1;s_view.notice=0;
        }
        else if(in->btn==BSP_BTN_OK) {if(s_view.off_home) {s_view.menu=1;s_view.selected=0;} else s_view.off_home=true;}
        else if(in->btn==BSP_BTN_DOWN && s_view.off_home) {s_view.relax=s_last_relax=od_next_relax(s_last_relax,esp_random());s_view.egg=-1;s_egg_end=now+3000000;play(1);}
        return;
    }
    if(!s_view.menu && s_view.time.phase==OD_DONE && in->btn==BSP_BTN_OK) {s_view.off_home=true;return;}
    if(s_view.menu==2) {if(in->btn==BSP_BTN_OK) {od_net_action(OD_NET_CLOSE);s_view.menu=1;} return;}
    if(s_view.menu) {
        if(in->btn!=BSP_BTN_OK) {
            if(s_view.edit && s_view.selected<7) adjust(in->btn==BSP_BTN_UP?1:-1);
            else if(!s_view.edit) s_view.selected=(s_view.selected+OD_MENU_ROWS+(in->btn==BSP_BTN_UP?-1:1))%OD_MENU_ROWS;
            else s_view.edit=0;
        } else if(s_view.selected<7) { if(s_view.edit) save(); else s_view.edit=1; }
        else if(s_view.selected==7) { if(od_net_action(OD_NET_SETUP)) {s_portal_seen=false;s_view.selected=0;s_view.menu=2;} else s_view.notice=3; }
        else if(s_view.selected==8) {if(!od_net_action(OD_NET_SYNC)) s_view.notice=3;}
        else if(s_view.selected==10) {
            if(!s_view.edit) s_view.edit=1;
            else {if(!od_net_action(OD_NET_FORGET)) s_view.notice=3; s_view.edit=0;}
        } else if(s_view.selected==11) close_menu();
    } else if(in->btn==BSP_BTN_OK) {s_view.menu=1; s_view.egg=-1;}
    else if(in->btn==BSP_BTN_UP) {s_view.mood=(s_view.mood+1)%8; play(0);}
    else if(s_view.time.phase==OD_WORK) {
        s_view.egg=s_last_egg=od_next_egg(s_last_egg,esp_random()); s_egg_end=now+3000000; s_next_egg=now+900000000; play(1);
    }
}
void app_main(void) {
    const uint32_t wake_causes=esp_sleep_get_wakeup_causes();
    if(wake_causes & (1u<<ESP_SLEEP_WAKEUP_GPIO)) key_guard_arm(&s_key_guard,esp_timer_get_time(),10000000);
    od_settings_default(&s_view.settings); s_saved=s_view.settings;
    s_view.boot=true;
    s_view.mood=2; s_view.egg=-1; s_view.relax=-1; s_view.battery=-1;
    if(bsp_i2c_init()!=ESP_OK || bsp_display_init()!=ESP_OK || !bsp_lvgl_init()) {ESP_LOGE("offduty","Display initialization failed");return;}
    if(!bsp_lvgl_lock(1000)) return;
    s_screen=od_ui_create(); od_ui_update(s_screen,&s_view); bsp_lvgl_unlock();
    bsp_display_backlight(s_view.settings.brightness);
    int64_t boot_started=esp_timer_get_time();
    ESP_LOGI("offduty","Boot splash started");
    while((s_view.boot_ms=od_boot_age(boot_started,esp_timer_get_time()))>=0) {
        if(bsp_lvgl_lock(100)){od_ui_update(s_screen,&s_view);bsp_lvgl_unlock();}
        vTaskDelay(pdMS_TO_TICKS(20));
    }
    s_view.boot=false;
    ESP_LOGI("offduty","Boot splash completed in %lld ms",(long long)((esp_timer_get_time()-boot_started)/1000));
    if(bsp_lvgl_lock(100)){od_ui_update(s_screen,&s_view);bsp_lvgl_unlock();}
    esp_err_t net_err=od_net_init();
    if(net_err!=ESP_OK) {s_view.notice=3;ESP_LOGE("offduty","Network initialization failed: %s",esp_err_to_name(net_err));}
    s_have_nvs=nvs_open("offduty",NVS_READWRITE,&s_nvs)==ESP_OK;
    if(s_have_nvs) {
        uint8_t plan=0;s_view.plan_ready=nvs_get_u8(s_nvs,"plan",&plan)==ESP_OK && plan==1;
        size_t n=sizeof(s_saved); od_settings_t saved;
        if(nvs_get_blob(s_nvs,"settings",&saved,&n)==ESP_OK && od_settings_blob(&saved,n,&saved)) {uint8_t waitv=0;if(nvs_get_u8(s_nvs,"waitv",&waitv)!=ESP_OK || waitv!=1)od_wait_link(&saved);od_sleep_migrate(&saved);s_view.settings=s_saved=saved;}
        else s_view.plan_ready=false;
        n=sizeof(s_overtime);
        if(nvs_get_blob(s_nvs,"overtime",s_overtime,&n)!=ESP_OK || n!=sizeof(s_overtime) || !od_overtime_valid(s_overtime[0],s_overtime[1]))memset(s_overtime,0,sizeof(s_overtime));
    }
    s_input_queue=xQueueCreate(12,sizeof(input_event_t));
    key_guard_arm(&s_key_guard,esp_timer_get_time(),10000000);
    if(s_input_queue && bsp_button_init(on_key,NULL)==ESP_OK) {
        key_guard_sync_released(&s_key_guard,bsp_button_read_mv()>=BSP_BTN_MV_RELEASED_MIN); s_input_ready=true;
    } else {ESP_LOGE("offduty","Button initialization failed");return;}
    if(bsp_audio_init()==ESP_OK) {
        s_sound_queue=xQueueCreate(4,sizeof(sound_t));
        if(s_sound_queue && xTaskCreate(audio_worker,"offduty_sound",3072,NULL,3,NULL)!=pdPASS) {vQueueDelete(s_sound_queue);s_sound_queue=NULL;}
    }
    bool battery_ok=bsp_battery_init()==ESP_OK;
    s_activity=esp_timer_get_time(); s_next_egg=s_activity+900000000;
    int64_t next_battery=0; int previous_brightness=-1;
    for(;;) {
        od_settings_t remote;
        if(od_net_take_settings(&remote)) {
            if((remote.start_min!=s_saved.start_min || remote.end_min!=s_saved.end_min) && remote.wait_min==s_saved.wait_min)od_wait_link(&remote);
            s_view.settings=remote;int selected=s_view.selected;s_view.selected=0;save();s_view.selected=selected;
            s_view.feedback=s_view.notice?2:1;s_feedback_at=esp_timer_get_time();
            s_asleep=false;s_activity=s_feedback_at;
        }
        od_net_settings_snapshot(&s_saved,s_view.plan_ready && s_view.notice!=1 && s_view.notice!=2);
        int64_t now=esp_timer_get_time(); s_view.confetti_ms=od_confetti_age(s_confetti_at,now); od_network_t net; od_net_snapshot(&net);
        if(net.phone_revision!=s_phone_revision){s_phone_revision=net.phone_revision;s_view.feedback=3;s_feedback_at=now;s_asleep=false;s_activity=now;}
        if(od_break_age(s_feedback_at,now)<0)s_view.feedback=0;
        od_time_t previous=s_view.time;bool finished=s_view.early_done || previous.phase==OD_DONE;
        s_view.utc_us=net.utc_us; od_time_t scheduled=od_time_at(s_view.plan_ready?net.utc_us:0,&s_saved);
        if(s_resume_end>0 && net.utc_us>=s_resume_end*1000000 && scheduled.start_utc>s_resume_start && net.utc_us>=scheduled.start_utc*1000000)s_resume_start=s_resume_end=-1;
        s_view.time=s_resume_end>0?od_time_between(net.utc_us,s_resume_start,s_resume_end):scheduled;
        if(od_overtime_valid(s_overtime[0],s_overtime[1]) && !od_overtime_applies(net.utc_us,s_overtime[0],s_overtime[1],scheduled.start_utc,s_saved.wait_min)) {
            if(!store_overtime(0,0)){memset(s_overtime,0,sizeof(s_overtime));s_view.menu=1;}
        }
        s_view.overtime=od_overtime_valid(s_overtime[0],s_overtime[1]);
        if(s_view.overtime)s_view.time=od_time_between(net.utc_us,s_overtime[0],s_overtime[1]);
        s_view.time=od_time_keep_finished(net.utc_us,s_view.time,previous,finished,s_saved.wait_min);

        s_view.early_done=s_early_end==s_view.time.end_utc && (s_view.time.phase==OD_WORK || s_view.time.phase==OD_FINAL);
        if(!s_view.early_done && s_view.time.phase!=OD_DONE) s_view.off_home=false;
        s_view.free_us=od_free_time(net.utc_us,s_view.time.end_utc,s_early_end,s_early_at);
        s_view.connected=net.connected; s_view.synced=net.synced;s_view.phone_time=net.phone_time; s_view.portal=net.portal;
        s_view.phone_connected=net.phone_connected;
        strcpy(s_view.ap_name,net.ap_name); strcpy(s_view.ap_password,net.ap_password);
        if(s_view.menu==2 && !net.portal && net.failed) {s_view.menu=1; s_view.notice=3;}
        else if(s_view.menu==2 && !net.portal && s_portal_seen) s_view.menu=1;
        if(s_view.menu==2 && net.portal) s_portal_seen=true;
        if(net.utc_us) {
            int64_t day=(net.utc_us/1000000+28800)/86400;
            if(day!=s_day) {s_day=day;s_woke_this_day=false;}
        }
        input_event_t in;
        if(xQueueReceive(s_input_queue,&in,pdMS_TO_TICKS(40))==pdTRUE) input(&in,now);
        now=esp_timer_get_time();
        if(s_down_since>=0 && s_view.time.phase==OD_WORK && !s_view.menu && !s_view.resume_picker && !s_view.work_choice) {
            static const int ranges[][2]=BSP_BTN_MV_TABLE;
            int mv=bsp_button_read_mv();
            int remaining=od_hold_remaining(s_down_since,now,mv>=ranges[BSP_BTN_DOWN][0] && mv<ranges[BSP_BTN_DOWN][1]);
            s_view.hold_remaining_ms=now-s_down_since>=500000?remaining:0;
            if(remaining<0) {s_down_since=-1;s_view.hold_remaining_ms=0;}
            else if(remaining==0) early_exit();
        } else s_view.hold_remaining_ms=0;
        if(s_view.time.phase==OD_FINAL && !s_view.early_done) {
            if(s_view.menu) close_menu();
            s_view.egg=-1; s_asleep=false; s_down_since=-1;
            s_activity=now; s_woke_this_day=true;
            int sec=(s_view.time.remaining_us+999999)/1000000;
            if(s_tone_end!=s_view.time.end_utc) {s_tone_end=s_view.time.end_utc;s_last_second=-1;}
            if(s_view.time.end_utc>s_celebrated && (s_last_second<0 || sec<s_last_second)) {if(sec<=10) play(sec<=5?2:1); s_last_second=sec;}
        }
        if(od_completion_due(&s_view.time,s_celebrated)) {s_celebrated=s_view.time.end_utc;s_confetti_at=now;s_view.confetti_seed=esp_random();play(3);}
        s_view.effect_ms=od_break_age(s_egg_end-3000000,now);
        if(s_view.effect_ms<0) {s_view.egg=-1;s_view.relax=-1;}
        if(!s_view.off_home || s_view.menu || s_view.resume_picker || s_view.work_choice || s_asleep)s_view.relax=-1;
        if(s_view.settings.random_eggs && now>=s_next_egg && s_view.time.phase==OD_WORK && !s_view.menu && !s_view.resume_picker && !s_view.work_choice && !s_asleep && !s_view.early_done && s_down_since<0) {
            s_view.egg=s_last_egg=od_next_egg(s_last_egg,esp_random()); s_egg_end=now+3000000;s_next_egg=now+900000000;
        }
        int local_min=net.utc_us?(net.utc_us/1000000+28800)%86400/60:-1;
        if(s_view.plan_ready && s_view.synced && !s_view.menu && s_down_since<0 && od_sleep_due(&s_view.settings,now-s_activity,local_min,s_view.time.phase,s_woke_this_day)) s_asleep=true;
        int brightness=s_asleep?0:s_view.time.phase==OD_FINAL && s_view.settings.brightness<50?50:s_view.settings.brightness;
        if(previous_brightness!=brightness) {bsp_display_backlight(brightness);previous_brightness=brightness;}
        if(now>=next_battery) {s_view.battery=battery_ok?bsp_battery_soc():-1;next_battery=now+30000000;}
        s_view.animation_ms=now/1000;
        if(!s_asleep && bsp_lvgl_lock(100)) {od_ui_update(s_screen,&s_view);bsp_lvgl_unlock();}
    }
}
