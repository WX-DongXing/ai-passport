#include "offduty_model.h"
#include "offduty_access.h"
#include "offduty_phone.h"
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

static char phone_html[24000];
static bool capture_phone(void *ctx,const char *html) {
    (void)ctx;
    assert(strlen(phone_html)+strlen(html)<sizeof(phone_html));
    strcat(phone_html,html);return true;
}
static bool fail_phone(void *ctx,const char *html) { (void)ctx;(void)html;return false; }

int main(void) {
    assert(od_boot_age(1000000,999999)==0);
    assert(od_boot_age(1000000,1000000)==0);
    assert(od_boot_age(1000000,3999999)==2999);
    assert(od_boot_age(1000000,4000000)==-1);
    const char *scan_token="0123456789abcdef0123456789abcdef";
    assert(od_phone_scan_form("action=scan&token=0123456789abcdef0123456789abcdef",scan_token));
    assert(!od_phone_scan_form("action=scan&token=0123456789abcdef0123456789abcdee",scan_token));
    assert(!od_phone_scan_form("action=scan&token=0123456789abcdef0123456789abcdef&x=1",scan_token));
    assert(!od_phone_scan_form("action=forget&token=0123456789abcdef0123456789abcdef",scan_token));
    assert(!od_phone_scan_form("action=scan&token=",""));
    od_wifi_list_t networks={0};
    od_wifi_add(&networks,"Coffee & <\"'>",-42,true);
    od_wifi_add(&networks,"Coffee & <\"'>",-70,true);
    od_wifi_add(&networks,"",-30,false);
    assert(networks.count==1 && networks.choices[0].rssi==-42);
    char escaped[193];od_phone_escape("Coffee & <\"'>",escaped);
    assert(!strcmp(escaped,"Coffee &amp; &lt;&quot;&#39;&gt;"));
    od_phone_escape("\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"",escaped);
    assert(strlen(escaped)==192);
    for(int i=0;i<20;i++){char name[33];snprintf(name,sizeof(name),"Test %d",i);od_wifi_add(&networks,name,-60,false);}
    assert(networks.count==OD_WIFI_LIMIT);
    od_settings_t phone_settings;od_settings_default(&phone_settings);
    assert(od_phone_page(capture_phone,NULL,&phone_settings,&networks,scan_token));
    assert(strstr(phone_html,"value=\"Coffee &amp; &lt;&quot;&#39;&gt;\""));
    assert(!strstr(phone_html,"value=\"Coffee & <"));
    assert(!strstr(phone_html,"信号强") && !strstr(phone_html,"信号中") && !strstr(phone_html,"信号弱"));
    assert(strstr(phone_html,"action=/time") && strstr(phone_html,"Date.now()") && strstr(phone_html,"id=phone-clock-button disabled"));
    assert(strstr(phone_html,"name=start type=time step=60") && strstr(phone_html,"name=end type=time step=60") && strstr(phone_html,"name=wait type=time step=60"));
    assert(!strstr(phone_html,"半小时") && strstr(phone_html,"默认提前 1 小时"));
    assert(strstr(phone_html,"action=/scan") && strstr(phone_html,"action=/settings"));
    assert(strstr(phone_html,"data-password=wifi-password") && strstr(phone_html,"data-password=hidden-password"));
    assert(strstr(phone_html,"name=eggs") && strstr(phone_html,"name=screen_off") && !strstr(phone_html,"name=idle"));
    assert(strstr(phone_html,"确认忘记网络并关闭热点</button></form></details></section>"));
    assert(strstr(phone_html,"</main></body></html>"));
    assert(!od_phone_page(fail_phone,NULL,&phone_settings,&networks,scan_token));
    phone_html[0]=0;networks=(od_wifi_list_t){.failed=true};
    assert(od_phone_page(capture_phone,NULL,&phone_settings,&networks,scan_token));
    assert(strstr(phone_html,"暂时无法扫描") && strstr(phone_html,"手动添加隐藏网络"));
    assert(!strstr(phone_html,"附近的网络<select"));

    assert(strstr(phone_html,"id=device-status") && strstr(phone_html,"data-panel=online hidden"));
    assert(strstr(phone_html,"getAttribute('action')"));
    phone_html[0]=0;
    assert(od_phone_status(capture_phone,NULL,true,true,false,false,false,OD_PHONE_SAVED,OD_PHONE_SAVED));
    assert(strstr(phone_html,"Wi-Fi 已连接") && strstr(phone_html,"设置已保存"));
    assert(strstr(phone_html,"data-connected=1 data-synced=1 data-failed=0 data-saved=2 data-wifi-saved=2"));
    phone_html[0]=0;
    assert(od_phone_status(capture_phone,NULL,false,false,true,false,false,OD_PHONE_SAVED,OD_PHONE_SAVING));
    assert(strstr(phone_html,"正在连接") && strstr(phone_html,"正在保存"));
    phone_html[0]=0;
    assert(od_phone_status(capture_phone,NULL,false,true,true,true,false,OD_PHONE_SAVED,OD_PHONE_SAVE_FAILED));
    assert(strstr(phone_html,"离线运行") && strstr(phone_html,"保存失败") && strstr(phone_html,"离线继续计时"));
    assert(!od_phone_status(capture_phone,NULL,true,true,false,false,false,OD_PHONE_SAVED,99));
    phone_html[0]=0;
    assert(od_phone_status(capture_phone,NULL,false,true,false,false,true,OD_PHONE_SAVED,OD_PHONE_SAVED));
    assert(strstr(phone_html,"手机时间已校准") && strstr(phone_html,"离线继续计时"));
    assert(strstr(od_phone_head,"我的小工位"));
    assert(strstr(od_phone_head,"class=version>v" OD_APP_VERSION));
    phone_html[0]=0;
    assert(od_phone_status(capture_phone,NULL,true,true,false,false,false,OD_PHONE_SAVE_FAILED,OD_PHONE_SAVED));
    assert(strstr(phone_html,"配置未保存") && strstr(phone_html,"网络配置保存失败") && strstr(phone_html,"data-wifi-saved=3"));
    assert(!od_phone_status(capture_phone,NULL,true,true,false,false,false,99,OD_PHONE_SAVED));
    assert(strstr(phone_html,"data-saved=2"));
    assert(strstr(od_phone_head,"step=1800")==NULL);
    int64_t phone_utc=0;
    assert(od_phone_time_form("utc=1791790920123&token=0123456789abcdef0123456789abcdef",scan_token,&phone_utc));
    assert(phone_utc==INT64_C(1791790920123000));
    const char *bad_times[]={"utc=1791790920&token=0123456789abcdef0123456789abcdef",
        "utc=1791790920123000&token=0123456789abcdef0123456789abcdef",
        "utc=1791790920123&token=0123456789abcdef0123456789abcdee",
        "utc=179179092012x&token=0123456789abcdef0123456789abcdef",
        "utc=1735689599999&token=0123456789abcdef0123456789abcdef",
        "utc=4102444800000&token=0123456789abcdef0123456789abcdef",
        "utc=1791790920123&token=0123456789abcdef0123456789abcdef&utc=1791790920123"};
    for(unsigned i=0;i<sizeof(bad_times)/sizeof(bad_times[0]);i++)assert(!od_phone_time_form(bad_times[i],scan_token,&phone_utc));
    assert(!od_phone_time_form(NULL,scan_token,&phone_utc));
    assert(od_clock_utc(phone_utc,1000000,61000000)==phone_utc+60000000);
    assert(od_clock_utc(phone_utc,1000000,1000000)==phone_utc);
    assert(!od_clock_utc(phone_utc,1000000,0));
    assert(!od_clock_utc(0,0,60000000));
    od_time_t completion={.phase=OD_DONE,.end_utc=2000};
    assert(od_completion_due(&completion,-1));
    assert(!od_completion_due(&completion,2000));
    completion.end_utc=1999;assert(!od_completion_due(&completion,2000));
    completion.end_utc=2001;assert(od_completion_due(&completion,2000));
    completion.phase=OD_WORK;assert(!od_completion_due(&completion,-1));
    uint32_t ap=inet_addr("192.168.4.1");
    struct sockaddr_in v4={.sin_family=AF_INET,.sin_addr={.s_addr=ap}};
    assert(od_ap_address((struct sockaddr *)&v4,sizeof(v4),ap));
    v4.sin_addr.s_addr=inet_addr("192.168.1.1");
    assert(!od_ap_address((struct sockaddr *)&v4,sizeof(v4),ap));
    struct sockaddr_in6 v6={.sin6_family=AF_INET6};
    assert(inet_pton(AF_INET6,"::ffff:192.168.4.1",&v6.sin6_addr)==1);
    assert(od_ap_address((struct sockaddr *)&v6,sizeof(v6),ap));
    assert(!od_ap_address((struct sockaddr *)&v6,sizeof(v4),ap));
    assert(inet_pton(AF_INET6,"::ffff:192.168.1.1",&v6.sin6_addr)==1);
    assert(!od_ap_address((struct sockaddr *)&v6,sizeof(v6),ap));
    assert(inet_pton(AF_INET6,"2001:db8::c0a8:401",&v6.sin6_addr)==1);
    assert(!od_ap_address((struct sockaddr *)&v6,sizeof(v6),ap));
    assert(od_setup_state(false,false,false)==OD_NEED_WIFI);
    assert(od_setup_state(true,false,false)==OD_NEED_TIME);
    assert(od_setup_state(true,true,false)==OD_NEED_PLAN);
    assert(od_setup_state(false,true,true)==OD_READY);
    assert(od_setup_state(false,false,true)==OD_NEED_WIFI);
    assert(od_free_time(900000000,1000,1000,800000000)==100000000);
    assert(od_free_time(1100000000,1000,1000,800000000)==300000000);
    assert(od_free_time(1100000000,1000,-1,0)==100000000);
    assert(od_free_time(900000000,1000,-1,0)==0);
    od_settings_t s; od_settings_default(&s); assert(od_settings_valid(&s));
    od_settings_t migrated;s.wait_min=120;assert(od_settings_blob(&s,sizeof(s),&migrated) && migrated.wait_min==120);
    assert(od_settings_blob(&s,offsetof(od_settings_t,wait_min),&migrated) && migrated.wait_min==480 && migrated.volume==s.volume);
    assert(!od_settings_blob(&s,1,&migrated));s.wait_min=480;
    assert(s.wait_min==480);
    for(int start=0;start<1440;start+=30)for(int end=0;end<1440;end+=30)if(start!=end) {
        od_settings_t plan=s;plan.start_min=start;plan.end_min=end;od_wait_link(&plan);
        assert(od_settings_valid(&plan));
        int rest=(start+1440-end)%1440;
        assert(plan.wait_min==(rest<60?end:(start+1380)%1440));
        plan.wait_min=od_wait_step(&plan,1);assert(od_settings_valid(&plan));
        plan.wait_min=od_wait_step(&plan,-1);assert(od_settings_valid(&plan));
        plan.wait_min=start;assert(!od_settings_valid(&plan));
    }
    od_settings_t night=s;night.start_min=1320;night.end_min=360;night.wait_min=0;
    assert(od_settings_blob(&night,sizeof(night),&migrated) && migrated.wait_min==1260);
    od_wait_link(&night);assert(night.wait_min==1260 && od_settings_valid(&night));
    night.wait_min=1380;assert(!od_settings_valid(&night));
    assert(od_shift_step(540,1)==541 && od_shift_step(540,-1)==539);
    assert(od_shift_step(557,1)==558 && od_shift_step(557,-1)==556);
    assert(od_shift_step(1439,1)==0 && od_shift_step(0,-1)==1439);
    char settings_token[33];od_settings_t remote;
    bool forget;
    assert(od_control_form("action=sync&token=0123456789abcdef0123456789abcdef",&forget,"0123456789abcdef0123456789abcdef") && !forget);
    assert(od_control_form("action=forget&token=0123456789abcdef0123456789abcdef",&forget,"0123456789abcdef0123456789abcdef") && forget);
    assert(!od_control_form("action=forget&token=0123456789abcdef0123456789abcdee",&forget,"0123456789abcdef0123456789abcdef"));
    assert(!od_control_form("action=sync&token=0123456789abcdef0123456789abcdef&action=forget",&forget,"0123456789abcdef0123456789abcdef"));
    const char *form="start=09%3A00&end=18%3A00&brightness=75&volume=50&screen_off=0&eggs=1&wait=08%3A00&token=0123456789abcdef0123456789abcdef";
    assert(od_settings_form(form,&remote,settings_token) && !memcmp(&s,&remote,sizeof(s)));
    assert(!od_settings_form("start=09%3A00&end=18%3A00&brightness=75&volume=50&screen_off=0&eggs=1&wait=09%3A00&token=0123456789abcdef0123456789abcdef",&remote,settings_token));
    assert(!od_settings_form("start=22%3A00&end=06%3A00&brightness=75&volume=50&screen_off=0&eggs=1&wait=23%3A00&token=0123456789abcdef0123456789abcdef",&remote,settings_token));
    assert(!od_settings_form("start=09%3A00&end=09%3A00&brightness=75&volume=50&screen_off=0&eggs=1&wait=08%3A00&token=0123456789abcdef0123456789abcdef",&remote,settings_token));
    assert(!od_settings_form("start=24%3A00&end=18%3A00&brightness=75&volume=50&screen_off=0&eggs=1&wait=08%3A00&token=0123456789abcdef0123456789abcdef",&remote,settings_token));
    assert(!od_settings_form("start=09%3A00&end=18%3A00&brightness=256&volume=35&screen_off=0&eggs=1&wait=08%3A00&token=0123456789abcdef0123456789abcdef",&remote,settings_token));
    assert(!od_settings_form("start=09%3A00&start=10%3A00",&remote,settings_token));
    assert(od_settings_form("start=09%3A01&end=18%3A07&brightness=75&volume=50&screen_off=0&eggs=1&wait=08%3A06&token=0123456789abcdef0123456789abcdef",&remote,settings_token));
    assert(remote.start_min==541 && remote.end_min==1087 && remote.wait_min==486);
    od_settings_t existing=s;existing.brightness=80;existing.volume=35;
    assert(od_settings_blob(&existing,sizeof(existing),&remote) && remote.brightness==80 && remote.volume==35);
    assert(od_overtime_step(30,-1)==30 && od_overtime_step(30,1)==60);
    assert(od_overtime_step(480,1)==480 && od_overtime_step(480,-1)==450);
    assert(od_overtime_valid(1791799200,1791799200+1800));
    assert(!od_overtime_valid(0,1800) && !od_overtime_valid(1791799200,1791799200));
    assert(!od_overtime_valid(1791799200,1791799200+481*60));
    assert(od_overtime_applies(0,1791799200,1791801000,0,0));
    assert(od_overtime_applies(INT64_C(1791801000)*1000000,1791799200,1791801000,1791766800,0));
    assert(!od_overtime_applies(INT64_C(1791801000)*1000000,1791799200,1791801000,1791801000,0));
    assert(!od_overtime_applies(INT64_C(1791853200)*1000000,1791799200,1791801000,1791853200,0));
    assert(od_time_between(0,1791799200,1791801000).phase==OD_WAIT);
    const int64_t resume_now=INT64_C(1791799200)*1000000; /* 18:00 CST */
    od_time_t completed=od_time_between(resume_now,resume_now/1000000-9*3600,resume_now/1000000);
    assert(od_time_at(resume_now+14*3600*INT64_C(1000000)-1,&s).phase==OD_DONE);
    assert(od_time_at(resume_now+14*3600*INT64_C(1000000),&s).phase==OD_BEFORE);
    od_wait_link(&night);
    int64_t night_start=resume_now+4*3600*INT64_C(1000000);
    assert(od_time_at(night_start+8*3600*INT64_C(1000000),&night).phase==OD_DONE);
    assert(od_time_at(night_start+23*3600*INT64_C(1000000),&night).phase==OD_BEFORE);
    assert(od_time_at(night_start+24*3600*INT64_C(1000000),&night).phase==OD_WORK);
    od_settings_t short_rest=s;short_rest.end_min=510;od_wait_link(&short_rest);
    assert(short_rest.wait_min==510 && od_settings_valid(&short_rest));
    od_time_t tomorrow=od_time_between(resume_now+6*3600*INT64_C(1000000),completed.start_utc+86400,completed.end_utc+86400);
    assert(tomorrow.phase==OD_BEFORE);
    assert(od_wait_boundary(completed.end_utc,0)==completed.end_utc+6*3600);
    assert(od_wait_boundary(completed.end_utc,120)==completed.end_utc+8*3600);
    assert(od_time_keep_finished(resume_now+6*3600*INT64_C(1000000),tomorrow,completed,true,0).phase==OD_BEFORE);
    od_settings_t wait_plan;od_settings_default(&wait_plan);wait_plan.wait_min=120;
    assert(od_time_at(resume_now+6*3600*INT64_C(1000000),&wait_plan).phase==OD_DONE);
    assert(od_time_at(resume_now+8*3600*INT64_C(1000000),&wait_plan).phase==OD_BEFORE);
    assert(od_time_at(resume_now+15*3600*INT64_C(1000000),&wait_plan).phase==OD_WORK);
    wait_plan.wait_min=1200;
    assert(od_time_at(resume_now+2*3600*INT64_C(1000000),&wait_plan).phase==OD_BEFORE);

    assert(od_time_keep_finished(resume_now+6*3600*INT64_C(1000000),tomorrow,completed,true,120).phase==OD_DONE);
    assert(od_time_keep_finished((completed.start_utc+86400)*INT64_C(1000000),tomorrow,completed,true,0).start_utc==tomorrow.start_utc);
    od_time_t extended=od_time_between(resume_now,completed.start_utc,completed.end_utc+1800);
    assert(extended.phase==OD_WORK);
    assert(od_time_keep_finished(resume_now,extended,completed,true,0).phase==OD_DONE);
    assert(od_time_keep_finished(resume_now,extended,completed,false,0).phase==OD_WORK);
    od_settings_t next_plan=s;next_plan.end_min=1110;od_wait_link(&next_plan);
    int64_t next_wait=(completed.start_utc+86400-3600)*INT64_C(1000000);
    assert(od_resume_applies(resume_now,resume_now/1000000,resume_now/1000000+1800,od_time_at(resume_now,&next_plan)));
    assert(od_resume_applies(next_wait-1,resume_now/1000000,resume_now/1000000+1800,od_time_at(next_wait-1,&next_plan)));
    assert(!od_resume_applies(next_wait,resume_now/1000000,resume_now/1000000+1800,od_time_at(next_wait,&next_plan)));
    assert(!od_resume_applies(next_wait+3600*INT64_C(1000000),resume_now/1000000,resume_now/1000000+1800,od_time_at(next_wait+3600*INT64_C(1000000),&next_plan)));
    assert(od_resume_end(resume_now,1110)==resume_now/1000000+1800);
    assert(od_resume_end(resume_now,1080)==resume_now/1000000+86400);
    assert(od_resume_end(resume_now,0)==resume_now/1000000+21600);
    assert(od_resume_end(resume_now,1440)==0);
    assert(od_resume_end(0,1080)==0);
    od_time_t resumed=od_time_between(resume_now,resume_now/1000000,od_resume_end(resume_now,1110));
    assert(resumed.phase==OD_WORK && resumed.percent==0 && resumed.remaining_us==1800000000);
    assert(od_time_between(resume_now+1800000000-1,resumed.start_utc,resumed.end_utc).percent==99);
    assert(od_time_between(resume_now+1800000000,resumed.start_utc,resumed.end_utc).phase==OD_DONE);
    od_time_t rewind_before=od_time_between((completed.start_utc-1800)*INT64_C(1000000),completed.start_utc,completed.end_utc);
    assert(od_time_keep_finished((completed.start_utc-1800)*INT64_C(1000000),rewind_before,completed,true,480).phase==OD_DONE);
    assert(od_wait_percent(INT64_C(1080000000),1020,960)==70);
    assert(od_wait_percent(INT64_C(1080000000),30,1410)==70);
    assert(od_wait_percent(INT64_C(3600000000),540,480)==0);
    assert(od_wait_percent(1,540,480)==99);
    assert(od_wait_percent(0,540,480)==100);
    assert(od_wait_percent(-1,540,480)==100);
    assert(od_wait_percent(0,540,540)==0);
    assert(od_wait_percent(0,-1,480)==0);
    od_settings_t changed_plan;od_settings_default(&changed_plan);
    changed_plan.start_min=1020;changed_plan.end_min=1080;od_wait_link(&changed_plan);
    const int64_t changed_now=INT64_C(1791763200)+8*3600+42*60; /* 16:42 CST */
    od_time_t waiting_changed=od_time_at(changed_now*1000000,&changed_plan);
    assert(waiting_changed.phase==OD_BEFORE && waiting_changed.remaining_us==18*60*INT64_C(1000000));
    assert(od_time_keep_finished(changed_now*1000000,waiting_changed,completed,true,changed_plan.wait_min).phase==OD_BEFORE);
    assert(od_time_at((changed_now-42*60)*1000000,&changed_plan).phase==OD_BEFORE); /* 16:00 */
    assert(od_time_at((changed_now+18*60)*1000000,&changed_plan).phase==OD_WORK); /* 17:00 */
    const int64_t day = INT64_C(1791763200); /* 2026-10-12 00:00 UTC */
    assert(od_time_at(0,&s).phase == OD_WAIT);
    assert(od_time_at((day+0)*1000000,&s).phase == OD_BEFORE); /* 08:00 CST */
    assert(od_time_at((day+35940)*1000000-1,&s).phase == OD_WORK);
    assert(od_time_at((day+35940)*1000000,&s).phase == OD_FINAL);
    assert(od_time_at((day+36000)*1000000,&s).phase == OD_DONE);
    assert(od_time_at((day+35700)*1000000,&s).percent==99); /* five minutes left */
    assert(od_time_at((day+36000)*1000000-1,&s).percent==99);
    assert(od_time_at((day+36000)*1000000,&s).percent==100);

    assert(od_time_at((day+86400)*1000000,&s).phase == OD_BEFORE);
    s.start_min=1320; s.end_min=360;
    od_time_t t=od_time_at((day+17*3600)*1000000,&s); /* next day 01:00 CST */
    assert(t.phase==OD_WORK && t.remaining_us==5*3600*INT64_C(1000000));
    assert(t.percent==37);
    assert(od_time_at((day+22*3600)*1000000,&s).phase==OD_DONE);
    s.end_min=s.start_min; assert(!od_settings_valid(&s)); od_settings_default(&s);
    char ssid[33], pw[65], token[33];
    assert(od_press_is_short(100,500099));
    assert(!od_press_is_short(100,500100));
    assert(!od_press_is_short(-1,100));
    assert(!od_press_is_short(100,99));
    assert(!od_press_is_short(100,3000100));
    assert(od_hold_remaining(100,100,true)==5000);
    assert(od_hold_remaining(100,2000100,true)==3000);
    assert(od_hold_remaining(100,5000099,true)==1);
    assert(od_hold_remaining(100,3000100,true)==2000);
    assert(od_hold_remaining(100,5000100,true)==0);
    assert(od_hold_remaining(100,5000100,false)==-1);
    assert(od_hold_remaining(100,99,true)==-1);
    assert(od_confetti_age(-1,100)==-1);
    assert(od_confetti_age(100,99)==-1);
    assert(od_confetti_age(100,100)==0);
    assert(od_confetti_age(100,1999100)==1999);
    assert(od_confetti_age(100,2000100)==-1);
    assert(od_break_age(-1,100)==-1);
    assert(od_break_age(100,99)==-1);
    assert(od_break_age(100,100)==0);
    assert(od_break_age(100,3000099)==2999);
    assert(od_break_age(100,3000100)==-1);
    for(int p=-1;p<6;p++)for(uint32_t r=0;r<100;r++){int effect=od_next_relax(p,r);assert(effect>=0 && effect<6 && effect!=p);}
    const char *valid="ssid=My+WiFi&password=test%26word&token=0123456789abcdef0123456789abcdef";
    assert(od_form_parse(valid,ssid,pw,token)); assert(strcmp(ssid,"My WiFi")==0);
    assert(strcmp(pw,"test&word")==0);
    assert(!od_form_parse("ssid=x&ssid=y&password=12345678&token=0123456789abcdef0123456789abcdef",ssid,pw,token));
    assert(!od_form_parse("ssid=%00&password=&token=0123456789abcdef0123456789abcdef",ssid,pw,token));
    assert(!od_form_parse("ssid=x%GG&password=&token=0123456789abcdef0123456789abcdef",ssid,pw,token));
    assert(!od_form_parse("ssid=x&password=short&token=0123456789abcdef0123456789abcdef",ssid,pw,token));
    for (int p=0;p<5;p++) for(uint32_t r=0;r<20;r++) {
        int e=od_next_egg(p,r); assert(e>=0 && e<5 && e!=p);
    }
    od_settings_default(&s);
    assert(s.start_min==540 && s.end_min==1080 && s.brightness==75 && s.volume==50 && s.sleep_mode==0);
    s.end_min=1110;assert(od_settings_valid(&s) && od_time_at(resume_now,&s).end_utc==od_resume_end(resume_now,1110));
    s.end_min=s.start_min;assert(!od_settings_valid(&s));od_settings_default(&s);
    const int delays[]={0,60,120,300,600,1800};
    for(int i=0;i<6;i++) {
        od_sleep_set(&s,i);assert(od_sleep_choice(&s)==i && od_settings_valid(&s));
        int64_t limit=delays[i]*INT64_C(1000000);
        assert(!od_sleep_due(&s,limit-1,1200,OD_WORK,false));
        assert(od_sleep_due(&s,limit,1200,OD_WORK,false)==(i!=0));
        assert(!od_sleep_due(&s,limit,1200,OD_FINAL,false));
        char body[256];snprintf(body,sizeof(body),"start=09%%3A00&end=18%%3A00&brightness=75&volume=50&screen_off=%d&eggs=1&wait=08%%3A00&token=0123456789abcdef0123456789abcdef",delays[i]);
        assert(od_settings_form(body,&remote,settings_token) && od_sleep_choice(&remote)==i);
    }
    assert(!od_settings_form("start=09%3A00&end=18%3A00&brightness=75&volume=50&screen_off=61&eggs=1&wait=08%3A00&token=0123456789abcdef0123456789abcdef",&remote,settings_token));
    s.sleep_mode=2;od_sleep_migrate(&s);assert(s.sleep_mode==0 && s.start_min==540 && s.brightness==75);
    s.sleep_mode=1;s.idle_sec=180;od_sleep_migrate(&s);assert(s.idle_sec==300);
    puts("Off-duty timing, settings, provisioning boundary and egg tests: PASS");
}
