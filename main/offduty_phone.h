#pragma once
#include "offduty_model.h"
#include <stdio.h>
#include <string.h>

#define OD_WIFI_LIMIT 16
typedef struct { char ssid[33]; int rssi; bool secured; } od_wifi_choice_t;
typedef struct { od_wifi_choice_t choices[OD_WIFI_LIMIT]; unsigned count; bool scanning, failed; } od_wifi_list_t;
typedef bool (*od_phone_emit_t)(void *ctx, const char *html);
enum { OD_PHONE_UNSAVED, OD_PHONE_SAVING, OD_PHONE_SAVED, OD_PHONE_SAVE_FAILED };
static const char od_phone_status_frame[]="<iframe class=status-frame src=/status title='设备连接与保存状态，每三秒自动更新'></iframe>";

static inline bool od_phone_status(od_phone_emit_t emit, void *ctx, bool connected,
                                  bool synced, bool connecting, bool failed, unsigned save_result) {
    const char *saved[]={"尚未设置，请保存下方偏好。","正在保存到设备…","设置已保存 ✓ 下次开机继续使用。","保存失败，请返回设置重新保存。"};
    char html[1536];
    if(save_result>OD_PHONE_SAVE_FAILED)return false;
    snprintf(html,sizeof(html),"<!doctype html><html lang=zh-CN><head><meta charset=utf-8><meta name=viewport content='width=device-width,initial-scale=1'><title>设备状态</title><style>"
        "*{box-sizing:border-box}body{margin:0;color:#241b34;font:13px/1.6 system-ui;background:#c2ead4;border:1px solid #241b34;border-radius:16px;padding:16px 18px;min-height:100vh}strong{font-size:15px;display:block}p{margin:8px 0}small{font-size:11px;color:#6b6176}.error{color:#914123}</style></head><body>"
        "<strong>%s</strong><p>%s</p><p class='%s' role=status>%s</p><small>状态每 3 秒自动更新 · 编辑内容不会丢失</small></body></html>",
        connected?"Wi-Fi 已连接 ✓":failed?"连接未成功，设备会继续重试":connecting?"正在连接 Wi-Fi…":"等待连接 Wi-Fi",
        synced?(connected?"时间已校准，准备准点下班。":"时间已校准，离线继续计时。"):(connected?"正在自动校准时间…":"联网后自动校准时间。"),
        save_result==OD_PHONE_SAVE_FAILED?"error":"",saved[save_result]);
    return emit(ctx,html);
}

static inline bool od_phone_scan_form(const char *body, const char *token) {
    return strlen(token)==32 && strlen(body)==50 &&
        !strncmp(body,"action=scan&token=",18) && !strcmp(body+18,token);
}

/* Scan results arrive strongest first; keep one entry per selectable network. */
static inline void od_wifi_add(od_wifi_list_t *list, const char *ssid, int rssi, bool secured) {
    if(!ssid[0] || strlen(ssid)>32 || list->count>=OD_WIFI_LIMIT)return;
    for(unsigned i=0;i<list->count;i++)if(!strcmp(list->choices[i].ssid,ssid))return;
    od_wifi_choice_t *choice=&list->choices[list->count++];
    strcpy(choice->ssid,ssid);choice->rssi=rssi;choice->secured=secured;
}
static inline void od_phone_escape(const char *src, char out[193]) {
    char *p=out;
    for(unsigned i=0;src[i] && i<32;i++) {
        const char *escaped=NULL;
        switch(src[i]){case '&':escaped="&amp;";break;case '<':escaped="&lt;";break;
            case '>':escaped="&gt;";break;case '"':escaped="&quot;";break;case '\'':escaped="&#39;";break;}
        if(escaped){size_t n=strlen(escaped);memcpy(p,escaped,n);p+=n;}else *p++=src[i];
    }
    *p=0;
}
static const char od_phone_head[]=
    "<!doctype html><html lang=zh-CN><head><meta charset=utf-8><meta name=viewport content='width=device-width,initial-scale=1'>"
    "<title>下班俱乐部 · 我的小工位</title><style>"
    ":root{color-scheme:light;--ink:#241b34;--cream:#fff9e6;--mint:#c2ead4;--purple:#432b87}"
    "*{box-sizing:border-box}body{margin:0;background:var(--cream);color:var(--ink);font:16px/1.6 system-ui,-apple-system,sans-serif}"
    "main{max-width:520px;margin:auto;padding:28px 20px 36px}a{color:var(--purple);text-underline-offset:4px}"
    ".brand{display:flex;justify-content:space-between;align-items:center;font:11px/1.5 ui-monospace,monospace;letter-spacing:2px;margin-bottom:24px}"
    ".dot{display:inline-block;background:#d47b59;width:8px;height:8px;border-radius:50%;margin-right:7px}"
    "h1{font-size:clamp(32px,8vw,42px);line-height:1.2;letter-spacing:-1px;margin:6px 0 12px}h2{font-size:21px;line-height:1.4;margin:0}"
    "p{margin:10px 0}.muted,small{color:#6b6176;font-size:13px;line-height:1.7}.hero{position:relative;padding-right:90px;margin-bottom:24px}"
    ".pet{position:absolute;right:4px;top:15px;width:68px;height:72px;background:#dd815c;border:2px solid var(--ink);border-radius:26px 26px 22px 22px;transform:rotate(-8deg);box-shadow:5px 5px 0 #efca9b}"
    ".pet:before{content:'';position:absolute;top:27px;left:20px;width:5px;height:5px;border-radius:50%;background:var(--ink);box-shadow:28px 0 0 var(--ink)}"
    ".pet:after{content:'';position:absolute;width:17px;height:9px;border:2px solid var(--ink);border-top:0;border-radius:0 0 12px 12px;left:23px;top:41px}"
    ".status-frame{width:100%;height:174px;display:block;border:0;margin-bottom:26px;border-radius:16px}"
    ".card{background:#fffdf5;border:1px solid #d8cfbc;border-radius:20px;padding:20px;margin:18px 0;box-shadow:0 4px 0 #ede5d2}"
    ".section-head{display:flex;gap:10px;align-items:center;margin-bottom:6px}.index{background:var(--purple);color:var(--cream);border-radius:50%;width:28px;height:28px;display:grid;place-items:center;font:12px ui-monospace,monospace;flex-shrink:0}"
    "label{display:block;font-size:14px;font-weight:600;margin-top:18px}input,button,select{font:inherit;line-height:1.5;width:100%;min-width:0;min-height:48px;border:1px solid #bdb3a6;border-radius:12px;padding:11px 12px;background:#fffdf8;color:var(--ink)}"
    "input,select{margin-top:7px}input[type=hidden]{display:none}button{cursor:pointer;font-weight:650;background:var(--purple);color:var(--cream);border-color:var(--purple);margin-top:18px}"
    "button:disabled{opacity:.45;cursor:default}button.secondary{background:var(--cream);color:var(--ink);border-color:#bdb3a6}"
    "button.danger{background:#fff1e8;color:#914123;border-color:#deb8a2}input:focus-visible,select:focus-visible,button:focus-visible,a:focus-visible,summary:focus-visible{outline:3px solid #dba657;outline-offset:3px}"
    ".grid{display:grid;grid-template-columns:minmax(0,1fr) minmax(0,1fr);gap:14px}.unit{float:right;color:#786b87;font-weight:400}"
    "details{border-top:1px dashed #d8cfbc;margin-top:20px;padding-top:16px}summary{cursor:pointer;font-size:14px;color:var(--purple)}"
    ".subhead{font-size:13px;color:var(--purple);font-weight:600;margin:22px 0 -4px}.note{font-size:12px;color:#786c76;text-align:center;margin-top:28px}"
    "@media(max-width:350px){main{padding:20px 14px}.card{padding:16px}.grid{gap:8px}.hero{padding-right:78px}}"
    "</style></head><body><main><div class=brand><span><i class=dot></i>OFF DUTY CLUB</span><span>我的小工位</span></div>";

static inline bool od_phone_page(od_phone_emit_t emit, void *ctx, const od_settings_t *settings,
                                const od_wifi_list_t *wifi, const char *token) {
    char html[1024];
#define SEND(s) do { if(!emit(ctx,(s)))return false; } while(0)
    SEND(od_phone_head);
    SEND("<header class=hero><p class=muted>把工位，调成喜欢的样子。</p><h1>准点下班。<br>快乐先行。</h1><div class=pet aria-hidden=true></div></header>");
    SEND(od_phone_status_frame);
    SEND("<section class=card><div class=section-head><span class=index>01</span><h2>让小工位联网</h2></div><p class=muted>选择附近的 2.4 GHz Wi-Fi，连接后自动校准时间。</p>");
    snprintf(html,sizeof(html),"<form method=post action=/scan><input type=hidden name=action value=scan><input type=hidden name=token value='%s'><button class=secondary %s>%s</button></form>",token,wifi->scanning?"disabled":"",wifi->scanning?"正在扫描附近网络…":"↻ 重新扫描附近网络");SEND(html);
    if(wifi->failed)SEND("<p class=muted role=status>暂时无法扫描，设备可能正在连接。请稍后重试。</p>");
    else if(!wifi->count)SEND(wifi->scanning?"<p class=muted role=status>正在扫描，请稍后刷新设备状态。</p>":"<p class=muted role=status>未发现可连接网络，可重新扫描或手动添加。</p>");
    if(wifi->count) {
        SEND("<form method=post action=/wifi><label>附近的网络<select name=ssid required><option value='' selected disabled>请选择 Wi-Fi</option>");
        for(unsigned i=0;i<wifi->count;i++) {
            char ssid[193];od_phone_escape(wifi->choices[i].ssid,ssid);
            snprintf(html,sizeof(html),"<option value=\"%s\">%s · %s · %s</option>",ssid,ssid,wifi->choices[i].rssi>=-55?"信号强":wifi->choices[i].rssi>=-70?"信号中":"信号弱",wifi->choices[i].secured?"加密":"开放");SEND(html);
        }
        SEND("</select></label><label>Wi-Fi 密码<input name=password type=password maxlength=64 autocomplete=new-password placeholder='开放网络可留空'></label>");
        snprintf(html,sizeof(html),"<input type=hidden name=token value='%s'><button>连接，并自动对时 ↗</button></form>",token);SEND(html);
    }
    SEND("<details><summary>没找到？手动添加隐藏网络</summary><form method=post action=/wifi><label>网络名称<input name=ssid maxlength=32 required autocomplete=off placeholder='输入 2.4 GHz 网络名称'></label><label>Wi-Fi 密码<input name=password type=password maxlength=64 autocomplete=new-password placeholder='开放网络可留空'></label>");
    snprintf(html,sizeof(html),"<input type=hidden name=token value='%s'><button>连接此网络 ↗</button></form></details><p class=muted>支持开放、WPA / WPA2 个人网络；不支持 5 GHz 或企业认证。</p></section>",token);SEND(html);
    SEND("<section class=card><div class=section-head><span class=index>02</span><h2>我的下班计划</h2></div><p class=muted>日常作息和设备偏好，一次设置好。</p><form method=post action=/settings>");
    snprintf(html,sizeof(html),"<input type=hidden name=token value='%s'><div class=grid><label>上班时间<input name=start type=time step=1800 value='%02u:%02u' required></label><label>下班时间<input name=end type=time step=1800 value='%02u:%02u' required></label></div><p class=muted>按半小时选择，也支持跨午夜班次。</p>",token,settings->start_min/60,settings->start_min%60,settings->end_min/60,settings->end_min%60);SEND(html);
    snprintf(html,sizeof(html),"<p class=subhead>屏幕与声音</p><div class=grid><label>屏幕亮度<span class=unit>%%</span><input name=brightness type=number min=10 max=100 value=%u required></label><label>声音大小<span class=unit>%%</span><input name=volume type=number min=0 max=100 value=%u required></label></div><p class=muted>亮度 10–100%%；音量为 0 时静音。</p>",settings->brightness,settings->volume);SEND(html);
    SEND("<label>熄屏方式<select name=screen_off>");
    const char *modes[]={"常亮","1分钟","2分钟","5分钟","10分钟","30分钟"};
    const unsigned delays[]={0,60,120,300,600,1800};
    for(int i=0;i<6;i++){snprintf(html,sizeof(html),"<option value=%u %s>%s</option>",delays[i],od_sleep_choice(settings)==i?"selected":"",modes[i]);SEND(html);}
    snprintf(html,sizeof(html),"</select></label><p class=muted>闲置达到所选时长后熄屏；按任意键唤醒，下班前一分钟保持亮屏。</p><label>随机彩蛋<select name=eggs><option value=1 %s>开启 · 偶尔来点惊喜</option><option value=0 %s>关闭 · 安静等下班</option></select></label><button>保存我的偏好 ✓</button></form></section>",settings->random_eggs?"selected":"",settings->random_eggs?"":"selected");SEND(html);
    snprintf(html,sizeof(html),"<section class=card><div class=section-head><span class=index>03</span><h2>网络管理</h2></div><p class=muted>连接 Wi-Fi 后会自动对时，也可以手动发起。</p><form method=post action=/control><input type=hidden name=action value=sync><input type=hidden name=token value='%s'><button class=secondary>立即校准时间</button></form><details><summary>更换网络 / 忘记已保存的网络</summary><p class=muted>忘记后将关闭热点；再次连接请在设备设置中打开热点。</p><form method=post action=/control><input type=hidden name=action value=forget><input type=hidden name=token value='%s'><button class=danger>确认忘记网络并关闭热点</button></form></details></section>",token,token);SEND(html);
    SEND("<footer class=note>热点五分钟后关闭，可从设备设置重新打开。<br>密码仅发往当前设备，不回显、不上传云端。<br>OFF DUTY CLUB · 今天的快乐，准时签收。</footer></main></body></html>");
#undef SEND
    return true;
}
