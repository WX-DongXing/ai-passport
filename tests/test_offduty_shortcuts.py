"""Exercise the real application input dispatcher with isolated storage/radio stubs."""
from pathlib import Path
import os
import re
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
source = (ROOT / "main/main.c").read_text()
header = (ROOT / "main/offduty_ui.h").read_text()
view = re.search(r"typedef struct \{.*?\} od_view_t;", header, re.S).group()
menu = re.search(r"enum \{ OD_MENU_.*?\};", header, re.S).group()
toggle = source[source.index("static void toggle_orientation("):source.index("static void close_menu(")]
dispatch = source[source.index("static void input("):source.index("void app_main(")]
harness = r'''
#include <assert.h>
#include <string.h>
#include "offduty_model.h"
#include "bsp_button.h"
#include "offduty_net.h"
VIEW
MENU
typedef struct {bsp_btn_t btn;bsp_btn_ev_t event;int64_t at_us;} input_event_t;
static od_view_t s_view;
static od_settings_t s_saved,written;
static bool s_have_nvs=true,s_asleep,s_woke_this_day,s_portal_seen;
static int s_nvs,storage_fail,writes,radio_calls,s_last_egg=-1,s_last_relax=-1;
static int64_t s_activity,s_egg_end,s_next_egg,s_feedback_at,s_down_since=-1,s_down_pressed=-1;
static int64_t s_early_end=-1,s_early_at,s_confetti_at=-1,s_resume_start=-1,s_resume_end=-1,s_overtime[2];
static int nvs_set_blob(int handle,const char *key,const void *data,size_t size){
    (void)handle;(void)key;assert(size==sizeof(written));written=*(const od_settings_t *)data;writes++;return storage_fail;
}
static int nvs_set_u8(int handle,const char *key,int value){(void)handle;(void)key;(void)value;return 0;}
static int nvs_commit(int handle){(void)handle;return storage_fail;}
bool od_net_action(od_net_action_t action){(void)action;radio_calls++;return true;}
static unsigned esp_random(void){return 1;}
static void play(int kind){(void)kind;}
static void early_exit(void){assert(0);}
static bool store_overtime(int64_t start,int64_t end){(void)start;(void)end;return true;}
static void save(void){assert(0 && "Shortcut must not save the current edit");}
static void close_menu(void){s_view.menu=0;s_view.edit=0;s_view.settings=s_saved;}
static void adjust(int delta){(void)delta;assert(0);}
TOGGLE
DISPATCH
static void reset(void){
    memset(&s_view,0,sizeof(s_view));od_settings_default(&s_saved);s_view.settings=s_saved;
    s_view.connected=s_view.synced=s_view.plan_ready=true;s_view.egg=s_view.relax=-1;
    s_asleep=false;storage_fail=0;writes=radio_calls=0;s_down_since=s_down_pressed=-1;
}
static void press(bsp_btn_t button,bsp_btn_ev_t event){input_event_t in={button,event,1000000};input(&in,1000000);}
int main(void){
    for(int stage=0;stage<12;stage++){
        reset();s_view.time.phase=stage%5;s_view.off_home=stage==5;s_view.egg=stage==6?2:-1;
        s_view.relax=stage==7?2:-1;s_view.work_choice=stage==8;s_view.resume_picker=stage==9;
        s_view.menu=stage==10?1:stage==11?2:0;s_view.time.end_utc=5000;s_view.mood=4;
        press(BSP_BTN_UP,BSP_BTN_LONG);assert(s_view.settings.orientation==1 && s_saved.orientation==1 && writes==1);
        assert(s_view.time.end_utc==5000 && s_view.mood==4 && radio_calls==0);
        press(BSP_BTN_UP,BSP_BTN_RELEASE);assert(writes==1 && s_view.mood==4);
        if(s_view.menu!=1){press(BSP_BTN_OK,BSP_BTN_LONG);assert(s_view.menu==1 && s_view.selected==OD_MENU_ORIENTATION && !s_view.work_choice && !s_view.resume_picker && s_view.egg==-1 && s_view.relax==-1);}
    }
    reset();s_view.menu=1;s_view.edit=1;s_view.selected=OD_MENU_THEME;s_view.settings.theme=2;s_view.settings.end_min=1110;
    press(BSP_BTN_UP,BSP_BTN_LONG);assert(written.orientation==1 && written.theme==1 && written.end_min==1080);
    assert(s_view.edit==1 && s_view.settings.theme==2 && s_view.settings.end_min==1110);
    press(BSP_BTN_OK,BSP_BTN_LONG);assert(s_view.menu==0 && s_view.settings.theme==1 && s_view.settings.end_min==1080 && s_view.settings.orientation==1);
    reset();storage_fail=1;press(BSP_BTN_UP,BSP_BTN_LONG);assert(s_view.settings.orientation==0 && s_saved.orientation==0 && s_view.feedback==2);
    reset();s_asleep=true;press(BSP_BTN_UP,BSP_BTN_LONG);assert(!s_asleep && writes==0);
    reset();s_view.menu=1;s_view.selected=OD_MENU_FORGET;s_view.edit=1;press(BSP_BTN_UP,BSP_BTN_LONG);assert(radio_calls==0 && s_view.selected==OD_MENU_FORGET);
    return 0;
}
'''
harness = harness.replace("VIEW", view).replace("MENU\n", menu + "\n").replace("TOGGLE", toggle).replace("DISPATCH", dispatch)
with tempfile.TemporaryDirectory(prefix="offduty-shortcuts-") as temp:
    c = Path(temp) / "test.c"
    c.write_text(harness)
    binary = Path(temp) / "test"
    subprocess.run(shlex.split(os.environ.get("CC", "cc")) + ["-std=c11", "-Wall", "-Wextra", "-Wno-unused-function", "-Wno-unused-variable", "-I" + str(ROOT / "main"), "-I" + str(ROOT / "components/bsp/include"), "-I" + str(ROOT / "tests/bsp_stubs"), str(c), str(ROOT / "main/offduty_model.c"), "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
print("Application global shortcuts, edit isolation and save failure: PASS")
