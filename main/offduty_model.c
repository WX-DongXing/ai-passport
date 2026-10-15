#include "offduty_model.h"
#include <string.h>
#include <stdlib.h>

void od_settings_default(od_settings_t *s) {
    *s = (od_settings_t){540, 1080, 1200, 300, 75, 50, 0, 1, 480, 1, 0, 0};
}
void od_wait_link(od_settings_t *s) {
    int rest=(s->start_min+1440-s->end_min)%1440;
    s->wait_min=rest<60?s->end_min:(s->start_min+1380)%1440;
}
int od_wait_percent(int64_t remaining_us,int start_min,int wait_min) {
    if(start_min<0 || start_min>=1440 || wait_min<0 || wait_min>=1440)return 0;
    int64_t span=(start_min+1440-wait_min)%1440*INT64_C(60000000);
    if(!span || remaining_us>=span)return 0;
    if(remaining_us<=0)return 100;
    return (int)((span-remaining_us)*100/span);
}
int od_wait_step(const od_settings_t *s,int direction) {
    int candidate=s->wait_min;
    for(int i=0;i<1440/OD_TIME_STEP_MIN;i++) {
        candidate=od_shift_step(candidate,direction);
        int after_end=(candidate+1440-s->end_min)%1440,rest=(s->start_min+1440-s->end_min)%1440;
        if(after_end<rest)return candidate;
    }
    return s->wait_min;
}
int od_boot_age(int64_t started_us,int64_t now_us) {
    if(now_us<started_us)return 0;
    int64_t age=(now_us-started_us)/1000;
    return age>=OD_BOOT_MS?-1:(int)age;
}

bool od_settings_valid(const od_settings_t *s) {
    return s->wait_min < 1440 && s->start_min < 1440 && s->end_min < 1440 && s->start_min != s->end_min &&
        (s->wait_min+1440-s->end_min)%1440 < (s->start_min+1440-s->end_min)%1440 &&
        s->sleep_min < 1440 && s->idle_sec >= 30 && s->idle_sec <= 3600 &&
        s->brightness >= 10 && s->brightness <= 100 && s->volume <= 100 &&
        s->sleep_mode <= 2 && s->random_eggs <= 1 && s->theme < OD_THEME_COUNT && s->orientation <= 1 && s->landscape_style < OD_LANDSCAPE_STYLES;
}

bool od_settings_blob(const void *blob,size_t size,od_settings_t *settings) {
    if(!blob || (size!=sizeof(*settings) && size!=offsetof(od_settings_t,wait_min) && size!=offsetof(od_settings_t,theme) && size!=offsetof(od_settings_t,orientation)))return false;
    od_settings_t value;od_settings_default(&value);memcpy(&value,blob,size);
    od_settings_t linked=value;od_wait_link(&linked);
    if(value.wait_min>=1440 || !od_settings_valid(&linked))return false;
    if(size==offsetof(od_settings_t,wait_min) || !od_settings_valid(&value))value.wait_min=linked.wait_min;
    if(!od_settings_valid(&value))return false;
    *settings=value;return true;
}
int64_t od_wait_boundary(int64_t end,int wait_min) {
    if(end<=0 || wait_min<0 || wait_min>=1440)return 0;
    int64_t boundary=(end+28800)/86400*86400-28800+wait_min*60;
    return boundary<end?boundary+86400:boundary;
}
static const uint16_t sleep_seconds[]={0,60,120,300,600,1800};
int od_sleep_choice(const od_settings_t *s) {
    if(s->sleep_mode!=1)return 0;
    for(int i=1;i<6;i++)if(s->idle_sec<=sleep_seconds[i])return i;
    return 5;
}
void od_sleep_set(od_settings_t *s,int choice) {
    if(choice<0 || choice>=6)return;
    s->sleep_mode=choice?1:0;
    s->idle_sec=choice?sleep_seconds[choice]:300;
}
void od_sleep_migrate(od_settings_t *s) {
    // Preserve the blob layout; round old idle delays up, retire scheduled sleep.
    od_sleep_set(s,od_sleep_choice(s));
}

int od_shift_step(int minute,int direction) {
    int next=direction>0?(minute/OD_TIME_STEP_MIN+1)*OD_TIME_STEP_MIN:((minute+OD_TIME_STEP_MIN-1)/OD_TIME_STEP_MIN-1)*OD_TIME_STEP_MIN;
    return (next+1440)%1440;
}

bool od_control_form(const char *body,bool *forget,const char *token) {
    const char *prefix="action=sync&token=";*forget=false;
    if(!strncmp(body,"action=forget&token=",20)){prefix="action=forget&token=";*forget=true;}
    size_t n=strlen(prefix);
    return strlen(token)==32 && strlen(body)==n+32 && !memcmp(body,prefix,n) && !strcmp(body+n,token);
}

od_time_t od_time_at(int64_t utc_us, const od_settings_t *s) {
    if (utc_us < INT64_C(1735689600000000) || !od_settings_valid(s)) return (od_time_t){.phase=OD_WAIT};
    int64_t utc = utc_us / 1000000;
    int64_t day = (utc + 28800) / 86400;
    int64_t start = day * 86400 - 28800 + s->start_min * 60;
    int64_t duration = ((s->end_min + 1440 - s->start_min) % 1440) * 60;
    // Keep yesterday's completed overnight shift until tonight's next start.
    if (utc < start)
        start -= 86400;
    od_time_t t=od_time_between(utc_us,start,start+duration);
    int64_t next_start=start+86400,wait=od_wait_boundary(t.end_utc,s->wait_min);
    if(t.phase==OD_DONE && wait<next_start && utc>=wait && (wait!=t.end_utc || utc>t.end_utc))return od_time_between(utc_us,next_start,next_start+duration);
    return t;
}

od_time_t od_time_keep_finished(int64_t utc_us,od_time_t current,od_time_t previous,bool finished,int wait_min) {
    if(!finished || previous.end_utc<=previous.start_utc || !utc_us)return current;
    // A changed daily plan can enter waiting before the old completion deadline.
    if(current.phase==OD_BEFORE && current.start_utc!=previous.start_utc) {
        int64_t planned_wait=od_wait_boundary(current.end_utc-86400,wait_min);
        if(planned_wait && utc_us>=planned_wait*1000000)return current;
    }
    int64_t wait=od_wait_boundary(previous.end_utc,wait_min);
    if(current.start_utc>previous.end_utc && (utc_us>=current.start_utc*1000000 || (wait && utc_us>=wait*1000000)))return current;
    return previous;
}
int od_overtime_step(int minutes,int direction) {
    int next=minutes+(direction>0?30:-30);
    return next<30?30:next>OD_OVERTIME_MAX_MIN?OD_OVERTIME_MAX_MIN:next;
}
bool od_overtime_valid(int64_t start,int64_t end) {
    return start>=INT64_C(1735689600) && end<INT64_C(4102444800) && end>start && end-start<=OD_OVERTIME_MAX_MIN*60;
}
bool od_overtime_applies(int64_t utc_us,int64_t start,int64_t end,int64_t scheduled_start,int wait_min) {
    int64_t wait=od_wait_boundary(end,wait_min),cutoff=wait && wait<scheduled_start?wait:scheduled_start;
    return od_overtime_valid(start,end) && (!utc_us || scheduled_start<end || utc_us<cutoff*1000000);
}
od_time_t od_time_between(int64_t utc_us,int64_t start,int64_t end) {
    od_time_t t={.phase=OD_WAIT};
    if(utc_us<INT64_C(1735689600000000) || end<=start)return t;
    int64_t duration=end-start;
    t.start_utc = start;
    t.end_utc = end;
    t.remaining_us = t.end_utc * 1000000 - utc_us;
    if (utc_us < start * 1000000) { t.phase = OD_BEFORE; t.remaining_us = start * 1000000 - utc_us; }
    else if (t.remaining_us <= 0) { t.phase = OD_DONE; t.remaining_us = 0; t.percent = 100; }
    else {
        t.phase = t.remaining_us <= 60000000 ? OD_FINAL : OD_WORK;
        t.percent = (int)(((utc_us - start * 1000000) * 100) /
                          (duration * 1000000));
    }
    return t;
}

static int hex(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

int od_confetti_age(int64_t started_us,int64_t now_us) {
    if(started_us<0 || now_us<started_us || now_us-started_us>=2000000)return -1;
    return (int)((now_us-started_us)/1000);
}

static bool decode(const char *p, size_t len, char *out, size_t cap) {
    size_t n = 0;
    for (size_t i = 0; i < len; i++) {
        unsigned char c = (unsigned char)p[i];
        if (c == '%') {
            if (i + 2 >= len || hex(p[i+1]) < 0 || hex(p[i+2]) < 0) return false;
            c = (unsigned char)(hex(p[i+1]) * 16 + hex(p[i+2])); i += 2;
        } else if (c == '+') c = ' ';
        if (c == 0 || c < 32 || c == 127 || n + 1 >= cap) return false;
        out[n++] = (char)c;
    }
    out[n] = 0;
    return true;
}

bool od_settings_form(const char *body,od_settings_t *settings,char token[33]) {
    const char *names[]={"start","end","brightness","volume","screen_off","eggs","token","wait","theme","orientation","landscape_style"};
    unsigned seen=0;unsigned values[11]={[8]=1};token[0]=0;
    if(!body || strlen(body)>512) return false;
    for(const char *p=body;*p;) {
        const char *end=strchr(p,'&');if(!end)end=p+strlen(p);
        const char *eq=memchr(p,'=',end-p);if(!eq)return false;
        unsigned field=0;for(;field<11;field++)if(strlen(names[field])==(size_t)(eq-p) && !memcmp(names[field],p,eq-p))break;
        char value[33];if(field==11 || (seen&(1u<<field)) || !decode(eq+1,end-eq-1,value,sizeof(value)))return false;
        seen|=1u<<field;
        if(field==6) {if(strlen(value)!=32)return false;strcpy(token,value);}
        else if(field==0 || field==1 || field==7) {
            if(strlen(value)!=5 || value[2]!=':' || value[0]<'0'||value[0]>'2'||value[1]<'0'||value[1]>'9'||value[3]<'0'||value[3]>'5'||value[4]<'0'||value[4]>'9')return false;
            unsigned h=(value[0]-'0')*10+value[1]-'0';if(h>23)return false;
            values[field]=h*60+(value[3]-'0')*10+value[4]-'0';
        } else {
            if(!value[0] || strlen(value)>4)return false;
            for(const char *c=value;*c;c++)if(*c<'0'||*c>'9')return false;
            values[field]=(unsigned)strtoul(value,NULL,10);
        }
        p=*end?end+1:end;
    }
    if(values[0]%OD_TIME_STEP_MIN || values[1]%OD_TIME_STEP_MIN || values[7]%OD_TIME_STEP_MIN)return false;
    if(((seen&255)!=255) || values[8]>=OD_THEME_COUNT || values[9]>1 || values[10]>=OD_LANDSCAPE_STYLES || values[2]<10 || values[2]>100 || values[3]>100 || values[5]>1)return false;
    int choice=0;for(;choice<6;choice++)if(values[4]==sleep_seconds[choice])break;
    if(choice==6)return false;
    od_settings_t candidate;od_settings_default(&candidate);
    candidate.theme=values[8];candidate.orientation=values[9];candidate.landscape_style=values[10];candidate.wait_min=values[7];candidate.start_min=values[0];candidate.end_min=values[1];candidate.brightness=values[2];candidate.volume=values[3];candidate.random_eggs=values[5];
    od_sleep_set(&candidate,choice);
    if(!od_settings_valid(&candidate))return false;
    *settings=candidate;return true;
}

bool od_form_parse(const char *body, char ssid[33], char password[65], char token[33]) {
    unsigned seen = 0;
    ssid[0] = password[0] = token[0] = 0;
    if (!body || strlen(body) > 512) return false;
    for (const char *p = body; *p;) {
        const char *end = strchr(p, '&');
        if (!end) end = p + strlen(p);
        const char *eq = memchr(p, '=', (size_t)(end-p));
        if (!eq) return false;
        size_t keylen = (size_t)(eq-p);
        unsigned bit; char *dest; size_t cap;
        if (keylen == 4 && memcmp(p,"ssid",4) == 0) { bit=1; dest=ssid; cap=33; }
        else if (keylen == 8 && memcmp(p,"password",8) == 0) { bit=2; dest=password; cap=65; }
        else if (keylen == 5 && memcmp(p,"token",5) == 0) { bit=4; dest=token; cap=33; }
        else return false;
        if ((seen & bit) || !decode(eq+1, (size_t)(end-eq-1), dest, cap)) return false;
        seen |= bit;
        p = *end ? end+1 : end;
    }
    size_t len = strlen(password);
    if (seen != 7 || !ssid[0] || strlen(token) != 32 || (len && len < 8)) return false;
    if (len == 64) for (size_t i=0; i<len; i++) if (hex(password[i]) < 0) return false;
    return true;
}

int od_next_egg(int previous, uint32_t random) {
    if (previous < 0 || previous >= 5) return (int)(random % 5);
    return (previous + 1 + (int)(random % 4)) % 5;
}
int od_next_relax(int previous,uint32_t random) {
    if(previous<0 || previous>=6)return (int)(random%6);
    return (previous+1+(int)(random%5))%6;
}
int od_break_age(int64_t started_us,int64_t now_us) {
    if(started_us<0 || now_us<started_us || now_us-started_us>=3000000)return -1;
    return (int)((now_us-started_us)/1000);
}

bool od_press_is_short(int64_t started_us,int64_t released_us) {
    return started_us>=0 && released_us>=started_us && released_us-started_us<500000;
}
int od_hold_remaining(int64_t started_us,int64_t now_us,bool still_down) {
    if(started_us<0 || !still_down || now_us<started_us) return -1;
    int64_t elapsed=now_us-started_us;
    return elapsed>=OD_HOLD_MS*INT64_C(1000)?0:(int)((OD_HOLD_MS*INT64_C(1000)-elapsed+999)/1000);
}

bool od_sleep_due(const od_settings_t *s, int64_t idle_us, int local_min,
                  od_phase_t phase, bool woke_this_day) {
    (void)local_min;(void)woke_this_day;
    if (phase == OD_FINAL || s->sleep_mode == 0) return false;
    if (s->sleep_mode == 1) return idle_us >= s->idle_sec * INT64_C(1000000);
    return false;
}

od_setup_t od_setup_state(bool connected,bool synced,bool plan_ready) {
    if(!synced) return connected?OD_NEED_TIME:OD_NEED_WIFI;
    return plan_ready?OD_READY:OD_NEED_PLAN;
}

int64_t od_free_time(int64_t now_us,int64_t scheduled_end,int64_t early_end,int64_t early_at_us) {
    int64_t since=early_end==scheduled_end?early_at_us:scheduled_end*1000000;
    return now_us>since?now_us-since:0;
}

int64_t od_resume_end(int64_t utc_us,int minute) {
    if(utc_us<INT64_C(1735689600000000) || minute<0 || minute>=1440)return 0;
    int64_t day=(utc_us/1000000+28800)/86400;
    int64_t end=day*86400-28800+minute*60;
    if(end*1000000<=utc_us)end+=86400;
    return end;
}

bool od_resume_applies(int64_t utc_us,int64_t start,int64_t end,od_time_t scheduled) {
    if(end<=start)return false;
    if(!utc_us || utc_us<end*1000000 || scheduled.start_utc<=start)return true;
    return scheduled.phase!=OD_BEFORE && utc_us<scheduled.start_utc*1000000;
}
int64_t od_clock_utc(int64_t utc_anchor,int64_t mono_anchor,int64_t now) {
    if(utc_anchor<INT64_C(1735689600000000) || mono_anchor<0 || now<mono_anchor)return 0;
    return utc_anchor+now-mono_anchor;
}
bool od_phone_time_form(const char *body,const char *token,int64_t *utc_us) {
    if(!body || !token || strlen(token)!=32 || strlen(body)!=56 || strncmp(body,"utc=",4) ||
       strncmp(body+17,"&token=",7) || strcmp(body+24,token))return false;
    int64_t ms=0;
    for(int i=4;i<17;i++){if(body[i]<'0' || body[i]>'9')return false;ms=ms*10+body[i]-'0';}
    if(ms<INT64_C(1735689600000) || ms>=INT64_C(4102444800000))return false;
    *utc_us=ms*1000;return true;
}
bool od_completion_due(const od_time_t *time,int64_t last_end) {
    return time->phase==OD_DONE && time->end_utc>last_end;
}
