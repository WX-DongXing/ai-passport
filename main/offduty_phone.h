#pragma once
#include "offduty_model.h"
#include "offduty_theme.h"
#include <stdio.h>
#include <string.h>

#define OD_WIFI_LIMIT 16
typedef struct { char ssid[33]; int rssi; bool secured; } od_wifi_choice_t;
typedef struct { od_wifi_choice_t choices[OD_WIFI_LIMIT]; unsigned count; bool scanning, failed; } od_wifi_list_t;
typedef bool (*od_phone_emit_t)(void *ctx, const char *html);
enum { OD_PHONE_UNSAVED, OD_PHONE_SAVING, OD_PHONE_SAVED, OD_PHONE_SAVE_FAILED };
#define OD_PHONE_CLOCK_SCRIPT "const clock=document.getElementById('phone-clock');document.getElementById('phone-clock-button').disabled=false;\nconst a=document.querySelector('[name=start]'),b=document.querySelector('[name=end]'),w=document.querySelector('[name=wait]');\nconst minute=v=>Number(v.slice(0,2))*60+Number(v.slice(3)),span=(x,y)=>(y-x+1440)%1440;\nfunction validWait(){w.setCustomValidity(span(minute(b.value),minute(w.value))<span(minute(b.value),minute(a.value))?'':'等待开工需在下班后、下次上班前');}\nfunction linkWait(){const start=minute(a.value),end=minute(b.value),m=span(end,start)<60?end:(start+1380)%1440;w.value=String(Math.floor(m/60)).padStart(2,'0')+':'+String(m%60).padStart(2,'0');validWait();}\na.addEventListener('change',linkWait);b.addEventListener('change',linkWait);w.addEventListener('change',validWait);\nconst status=document.getElementById('device-status');let pending=null,reading=false;\nfunction message(form,text,kind){let el=form.querySelector('.feedback');if(!el){el=document.createElement('p');el.className='feedback';el.setAttribute('role','status');el.setAttribute('aria-live','polite');form.append(el);}el.textContent=text;el.dataset.kind=kind;el.setAttribute('aria-busy',String(kind==='loading'));}\nfunction finish(text,kind){if(!pending)return;message(pending.form,text,kind);pending.form.querySelectorAll('button').forEach(x=>x.disabled=false);pending=null;}\nasync function request(url,options={}){const r=await fetch(url,{cache:'no-store',signal:AbortSignal.timeout(8000),...options});if(!r.ok)throw Error();return r;}\nasync function readStatus(){if(reading)return;reading=true;try{const r=await request('/status'),doc=new DOMParser().parseFromString(await r.text(),'text/html'),state=doc.body.dataset;status.querySelector('strong').textContent=doc.querySelector('strong').textContent;status.querySelector('p').textContent=doc.querySelector('p').textContent;status.dataset.reachable='true';if(pending&&pending.accepted){const path=pending.path;if(path==='/settings'&&state.saved==='2')finish('偏好已保存 ✓ 下次开机继续使用。','success');else if(path==='/settings'&&state.saved==='3')finish('保存失败，请重试。','error');else if(path==='/wifi'&&state.wifiSaved==='3'&&r.headers.get('X-Wifi-Revision')!==pending.revision)finish('Wi-Fi 已连接，但配置未保存。请重试。','error');else if(path==='/wifi'&&state.connected==='1'&&state.wifiSaved==='2'&&r.headers.get('X-Wifi-Revision')!==pending.revision)finish('Wi-Fi 已连接，网络配置已保存 ✓','success');else if(path==='/wifi'&&state.failed==='1'&&r.headers.get('X-Wifi-Revision')!==pending.revision)finish('连接未成功，请检查密码或重试。','error');else if((path==='/time'||path==='/control')&&r.headers.get('X-Clock-Revision')!==pending.revision&&state.synced==='1')finish('时间已校准 ✓','success');else if(Date.now()-pending.started>30000)finish('暂未确认结果，请查看设备状态后重试。','error');}}catch(e){status.querySelector('strong').textContent='暂时无法读取设备';status.querySelector('p').textContent='请保持连接设备热点，再重试。';status.dataset.reachable='false';if(pending&&Date.now()-pending.started>30000)finish('连接中断，未确认结果。请重新连接设备热点。','error');}finally{reading=false;}}\ndocument.querySelectorAll('[data-mode]').forEach(button=>button.addEventListener('click',()=>{document.querySelectorAll('[data-mode]').forEach(x=>x.setAttribute('aria-pressed',String(x===button)));document.querySelectorAll('[data-panel]').forEach(x=>x.hidden=x.dataset.panel!==button.dataset.mode);}));\ndocument.addEventListener('submit',async event=>{const form=event.target;if(form.tagName!=='FORM')return;event.preventDefault();if(pending){message(form,'另一个操作正在处理，请稍候。','error');return;}const path=form.getAttribute('action');pending={form,path,started:Date.now(),accepted:false,revision:null};form.querySelectorAll('button').forEach(x=>x.disabled=true);message(form,path==='/settings'?'正在保存…':path==='/wifi'?'正在连接 Wi-Fi…':path==='/scan'?'正在扫描…':'正在校准…','loading');try{if(path==='/time'||path==='/control'||path==='/wifi'){const baseline=await request('/status');pending.revision=baseline.headers.get(path==='/wifi'?'X-Wifi-Revision':'X-Clock-Revision');if(pending.revision===null)throw Error();}if(form===clock)form.elements.utc.value=Date.now();await request(path,{method:'POST',body:new URLSearchParams(new FormData(form))});pending.accepted=true;if(path==='/scan'){let doc;do{await new Promise(resolve=>setTimeout(resolve,1500));const r=await request('/');doc=new DOMParser().parseFromString(await r.text(),'text/html');if(Date.now()-pending.started>30000)throw Error();}while(doc.querySelector('[action=\"/scan\"] button').disabled);const fresh=doc.querySelector('[data-panel=online]'),current=document.querySelector('[data-panel=online]');const hasNetworks=doc.querySelector('[name=ssid]')?.tagName==='SELECT';current.replaceChildren(...fresh.childNodes);pending.form=current.querySelector('form');finish(hasNetworks?'扫描完成 ✓ 请选择附近网络。':'未找到网络，请重试或手动添加。','success');}else if(path==='/control'&&form.elements.action.value==='forget')finish('忘记网络请求已提交，热点即将关闭。','success');else await readStatus();}catch(e){finish('操作未确认，请检查热点连接、输入内容后重试。','error');}});\nreadStatus();setInterval(readStatus,3000);\ndocument.addEventListener('click',event=>{const button=event.target.closest('[data-password]');if(!button)return;const input=document.getElementById(button.dataset.password);if(!input)return;const show=input.type==='password';input.type=show?'text':'password';button.textContent=show?'隐藏':'显示';button.setAttribute('aria-label',show?'隐藏密码':'显示密码');button.setAttribute('aria-pressed',String(show));});\n"
#define OD_PHONE_CLOCK_HASH "sha256-rSjwfnp+pUFTfjbuUZ5DJhC67JPjW3kN0xJHGyWhJ54="
static const char od_phone_status_frame[]="<iframe class=status-frame src=/status title='设备连接与保存状态，每三秒自动更新'></iframe>";

static inline bool od_phone_status(od_phone_emit_t emit, void *ctx, bool connected,
                                  bool synced, bool connecting, bool failed, bool phone_time, unsigned wifi_save_result, unsigned save_result) {
    const char *saved[]={"尚未设置，请保存下方偏好。","正在保存到设备…","设置已保存 ✓ 下次开机继续使用。","保存失败，请返回设置重新保存。"};
    char html[1536];
    if(save_result>OD_PHONE_SAVE_FAILED || wifi_save_result>OD_PHONE_SAVE_FAILED)return false;
    snprintf(html,sizeof(html),"<!doctype html><html lang=zh-CN><head><meta charset=utf-8><meta name=viewport content='width=device-width,initial-scale=1'><title>设备状态</title><style>"
        "*{box-sizing:border-box}body{margin:0;color:#241b34;font:13px/1.6 system-ui;background:#c2ead4;border:1px solid #241b34;border-radius:16px;padding:16px 18px;min-height:0}strong{font-size:15px;display:block}p{margin:8px 0}small{font-size:11px;color:#6b6176}.error{color:#914123}</style></head><body data-connected=%u data-synced=%u data-failed=%u data-saved=%u data-wifi-saved=%u>"
        "<strong>%s</strong><p>%s</p><p class='%s' role=status>%s</p><small>状态每 3 秒自动更新 · 编辑内容不会丢失</small></body></html>",
        connected,synced,failed,save_result,wifi_save_result,
        connected?(wifi_save_result==OD_PHONE_SAVE_FAILED?"Wi-Fi 已连接 · 配置未保存":wifi_save_result==OD_PHONE_SAVING?"Wi-Fi 已连接 · 正在保存配置":"Wi-Fi 已连接 ✓"):synced?"离线运行 ✓":failed?"连接未成功，设备会继续重试":connecting?"正在连接 Wi-Fi…":"等待连接 Wi-Fi",
        wifi_save_result==OD_PHONE_SAVE_FAILED?"网络配置保存失败，请重新连接保存。":synced?(phone_time?(connected?"手机时间已校准，等待网络复核。":"手机时间已校准，离线继续计时。"):connected?"网络时间已校准，准备准点下班。":"时间已校准，离线继续计时。"):(connected?"正在网络对时，也可用手机校时。":"时间待校准，可用下方手机校时。"),
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
    "main{max-width:520px;margin:auto;padding:20px 16px 28px}a{color:var(--purple);text-underline-offset:4px}"
    ".brand{display:flex;justify-content:space-between;align-items:center;font:11px/1.5 ui-monospace,monospace;letter-spacing:2px;margin-bottom:16px}"
    ".dot{display:inline-block;background:#d47b59;width:8px;height:8px;border-radius:50%;margin-right:7px}"
    "h1{font-size:clamp(28px,7vw,36px);line-height:1.2;letter-spacing:-1px;margin:6px 0 12px}h2{font-size:21px;line-height:1.4;margin:0}"
    "p{margin:10px 0}.muted,small{color:#6b6176;font-size:13px;line-height:1.7}.hero{position:relative;padding-right:90px;margin-bottom:16px}"
    ".pet{position:absolute;right:4px;top:15px;width:68px;height:72px;background:#dd815c;border:2px solid var(--ink);border-radius:26px 26px 22px 22px;transform:rotate(-8deg);box-shadow:5px 5px 0 #efca9b}"
    ".pet:before{content:'';position:absolute;top:27px;left:20px;width:5px;height:5px;border-radius:50%;background:var(--ink);box-shadow:28px 0 0 var(--ink)}"
    ".pet:after{content:'';position:absolute;width:17px;height:9px;border:2px solid var(--ink);border-top:0;border-radius:0 0 12px 12px;left:23px;top:41px}"
    ".status-frame{width:100%;height:174px;display:block;border:0;margin-bottom:26px;border-radius:16px}"
    ".card{background:#fffdf5;border:1px solid #d8cfbc;border-radius:18px;padding:16px;margin:14px 0;box-shadow:0 4px 0 #ede5d2}"
    ".section-head{display:flex;gap:10px;align-items:center;margin-bottom:6px}.index{background:var(--purple);color:var(--cream);border-radius:50%;width:28px;height:28px;display:grid;place-items:center;font:12px ui-monospace,monospace;flex-shrink:0}"
    "label{display:block;font-size:14px;font-weight:600;margin-top:14px}input,button,select{font:inherit;line-height:1.5;width:100%;min-width:0;min-height:44px;border:1px solid #bdb3a6;border-radius:12px;padding:9px 10px;background:#fffdf8;color:var(--ink)}"
    "input,select{margin-top:7px}input[type=hidden]{display:none}button{cursor:pointer;font-weight:650;background:var(--purple);color:var(--cream);border-color:var(--purple);margin-top:14px}"
    "button:disabled{opacity:.45;cursor:default}button.secondary{background:var(--cream);color:var(--ink);border-color:#bdb3a6}"
    "button.danger{background:#fff1e8;color:#914123;border-color:#deb8a2}input:focus-visible,select:focus-visible,button:focus-visible,a:focus-visible,summary:focus-visible{outline:3px solid #dba657;outline-offset:3px}"
    ".grid{display:grid;grid-template-columns:minmax(0,1fr) minmax(0,1fr);gap:12px}.unit{float:right;color:#786b87;font-weight:400}"
    ".password-field{display:flex;gap:8px;align-items:center;margin-top:7px}.password-field input{flex:1;margin:0;min-width:0}.password-field button{flex:0 0 60px;width:60px;min-height:44px;margin:0;padding:8px 4px;font-size:13px}"
    "details{border-top:1px dashed #d8cfbc;margin-top:20px;padding-top:16px}summary{cursor:pointer;font-size:14px;color:var(--purple)}"
    ".subhead{font-size:13px;color:var(--purple);font-weight:600;margin:22px 0 -4px}.note{font-size:12px;line-height:1.7;color:#6b6176;text-align:left;margin-top:20px;padding:14px 4px 0;border-top:1px dashed #d8cfbc}.note p{margin:0 0 5px}.note .signature{display:flex;gap:7px;align-items:center;margin-top:12px;font-size:11px;color:#8b7e88}.note .signature:before{content:'';width:5px;height:5px;flex-shrink:0;border-radius:50%;background:#d47b59}"
    "[hidden]{display:none!important}.status-frame{display:block;width:100%;height:160px;border:0}.status-box{padding:12px 14px;border:1px solid #83a794;border-radius:14px;background:var(--mint);font-size:13px}.status-box p{margin:3px 0 0}.mode-switch{display:flex;gap:8px;margin-top:16px}.mode-switch button{margin:0;flex:1;padding:10px 4px;font-size:13px;background:transparent;color:var(--ink);border-color:#bdb3a6}.mode-switch button[aria-pressed=true]{background:var(--purple);color:var(--cream);border-color:var(--purple)}.mode-note{font-size:12px;color:#6b6176;margin:8px 0}.feedback:empty{display:none}.feedback{font-size:13px;margin:10px 0 0;overflow-wrap:anywhere}.feedback[data-kind=error]{color:#914123}.feedback[data-kind=success]{color:#376a51}.feedback[data-kind=loading]:before{content:'';display:inline-block;width:12px;height:12px;border:2px solid #d4c9e8;border-top-color:var(--purple);border-radius:50%;margin-right:7px;vertical-align:-2px;animation:spin .8s linear infinite}@keyframes spin{to{transform:rotate(360deg)}}.grid>label{min-width:0}input[type=time]{display:block;appearance:none;-webkit-appearance:none;inline-size:100%;max-inline-size:100%;min-inline-size:0;text-align:center}input::-webkit-date-and-time-value{text-align:center;min-width:0}input::-webkit-datetime-edit{padding:0}.hero{min-height:104px}.status-box[data-reachable=false]{background:#fff1e8;border-color:#deb8a2}@media(prefers-reduced-motion:reduce){.feedback[data-kind=loading]:before{animation:none}}"
    "@media(max-width:350px){main{padding:20px 14px}.card{padding:16px}.grid{gap:8px}.hero{padding-right:78px}}"
    ".brand .version{display:block;text-align:right;font:10px/1.4 ui-monospace,monospace;letter-spacing:0;color:#6b6176}"
    "</style></head><body><main><div class=brand><span><i class=dot></i>OFF DUTY CLUB</span><span>我的小工位<small class=version>v" OD_APP_VERSION "</small></span></div>";

static inline bool od_phone_time_select(od_phone_emit_t emit,void *ctx,const char *name,unsigned selected) {
    char html[192];snprintf(html,sizeof(html),"<select name=%s required>",name);if(!emit(ctx,html))return false;
    if(selected%OD_TIME_STEP_MIN){snprintf(html,sizeof(html),"<option value='%02u:%02u' selected disabled>%02u:%02u · 请选择 15 分钟刻度</option>",selected/60,selected%60,selected/60,selected%60);if(!emit(ctx,html))return false;}
    for(unsigned m=0;m<1440;m+=OD_TIME_STEP_MIN){snprintf(html,sizeof(html),"<option value='%02u:%02u' %s>%02u:%02u</option>",m/60,m%60,m==selected?"selected":"",m/60,m%60);if(!emit(ctx,html))return false;}
    return emit(ctx,"</select>");
}

static inline bool od_phone_page(od_phone_emit_t emit, void *ctx, const od_settings_t *settings,
                                const od_wifi_list_t *wifi, const char *token) {
    char html[1024];
#define SEND(s) do { if(!emit(ctx,(s)))return false; } while(0)
    SEND(od_phone_head);
    SEND("<header class=hero><p class=muted>把工位，调成喜欢的样子。</p><h1>准点下班。<br>快乐先行。</h1><div class=pet aria-hidden=true></div></header>");
    SEND("<aside id=device-status class=status-box role=status><strong>正在读取设备状态…</strong><p>请保持连接设备热点。</p></aside><div class=mode-switch aria-label=校时方式><button type=button data-mode=offline aria-pressed=true>离线 · 手机校时</button><button type=button data-mode=online aria-pressed=false>联网 · Wi-Fi 对时</button></div><p class=mode-note>两种方式任选其一，设置作息无需联网。</p><div data-panel=offline>");
    snprintf(html,sizeof(html),"<section class=card><div class=section-head><span class=index>↗</span><h2>手机校时</h2></div><p class=muted>无需设备联网。请先确认手机日期和时间准确，校准后即可继续倒计时。</p><form id=phone-clock method=post action=/time><input type=hidden name=utc><input type=hidden name=token value='%s'><button id=phone-clock-button disabled>使用手机当前时间 ✓</button></form><noscript><p class=muted>请启用浏览器 JavaScript 后重新打开页面。</p></noscript></section>",token);SEND(html);

    SEND("</div><div data-panel=online hidden><section class=card><div class=section-head><span class=index>↗</span><h2>Wi-Fi 联网</h2></div><p class=muted>选择附近的 2.4 GHz Wi-Fi，连接后自动校准时间。</p>");
    snprintf(html,sizeof(html),"<form method=post action=/scan><input type=hidden name=action value=scan><input type=hidden name=token value='%s'><button class=secondary %s>%s</button></form>",token,wifi->scanning?"disabled":"",wifi->scanning?"正在扫描附近网络…":"↻ 重新扫描附近网络");SEND(html);
    if(wifi->failed)SEND("<p class=muted role=status>暂时无法扫描，设备可能正在连接。请稍后重试。</p>");
    else if(!wifi->count)SEND(wifi->scanning?"<p class=muted role=status>正在扫描，请稍后刷新设备状态。</p>":"<p class=muted role=status>未发现可连接网络，可重新扫描或手动添加。</p>");
    if(wifi->count) {
        SEND("<form method=post action=/wifi><label>附近的网络<select name=ssid required><option value='' selected disabled>请选择 Wi-Fi</option>");
        for(unsigned i=0;i<wifi->count;i++) {
            char ssid[193];od_phone_escape(wifi->choices[i].ssid,ssid);
            snprintf(html,sizeof(html),"<option value=\"%s\">%s</option>",ssid,ssid);SEND(html);
        }
        SEND("</select></label><label for=wifi-password>Wi-Fi 密码</label><div class=password-field><input id=wifi-password name=password type=password maxlength=64 autocomplete=new-password placeholder='开放网络可留空'><button type=button class=secondary data-password=wifi-password aria-controls=wifi-password aria-label=显示密码 aria-pressed=false>显示</button></div>");
        snprintf(html,sizeof(html),"<input type=hidden name=token value='%s'><button>连接，并自动对时 ↗</button></form>",token);SEND(html);
    }
    SEND("<details><summary>没找到？手动添加隐藏网络</summary><form method=post action=/wifi><label>网络名称<input name=ssid maxlength=32 required autocomplete=off placeholder='输入 2.4 GHz 网络名称'></label><label for=hidden-password>Wi-Fi 密码</label><div class=password-field><input id=hidden-password name=password type=password maxlength=64 autocomplete=new-password placeholder='开放网络可留空'><button type=button class=secondary data-password=hidden-password aria-controls=hidden-password aria-label=显示密码 aria-pressed=false>显示</button></div>");
    snprintf(html,sizeof(html),"<input type=hidden name=token value='%s'><button>连接此网络 ↗</button></form></details><p class=muted>支持开放、WPA / WPA2 个人网络；不支持 5 GHz 或企业认证。</p></section>",token);SEND(html);
    SEND("</div><section class=card><div class=section-head><span class=index>◷</span><h2>我的下班计划</h2></div><p class=muted>日常作息和设备偏好，一次设置好。</p><form method=post action=/settings>");
    snprintf(html,sizeof(html),"<input type=hidden name=token value='%s'><div class=grid><label>上班时间",token);SEND(html);
    if(!od_phone_time_select(emit,ctx,"start",settings->start_min))return false;
    SEND("</label><label>下班时间");if(!od_phone_time_select(emit,ctx,"end",settings->end_min))return false;
    SEND("</label></div><p class=muted>每 15 分钟一格，支持跨夜班次。</p><label>准备时间");
    if(!od_phone_time_select(emit,ctx,"wait",settings->wait_min))return false;
    SEND("</label><p class=muted>默认提前 1 小时，随作息调整。</p>");
    snprintf(html,sizeof(html),"<p class=subhead>屏幕与声音</p><div class=grid><label>屏幕亮度<span class=unit>%%</span><input name=brightness type=number min=10 max=100 value=%u required></label><label>声音大小<span class=unit>%%</span><input name=volume type=number min=0 max=100 value=%u required></label></div><p class=muted>亮度 10–100%%；音量为 0 时静音。</p>",settings->brightness,settings->volume);SEND(html);
    SEND("<label>主题配色<select name=theme>");
    for(int i=0;i<OD_THEME_COUNT;i++){snprintf(html,sizeof(html),"<option value=%d %s>%s</option>",i,settings->theme==i?"selected":"",od_theme_name(i));SEND(html);}
    SEND("</select></label><p class=muted>撞色、彩印、果冻，选一套喜欢的。开机和校时页保持原样。</p>");
    SEND("<label>熄屏方式<select name=screen_off>");
    const char *modes[]={"常亮","1分钟","2分钟","5分钟","10分钟","30分钟"};
    const unsigned delays[]={0,60,120,300,600,1800};
    for(int i=0;i<6;i++){snprintf(html,sizeof(html),"<option value=%u %s>%s</option>",delays[i],od_sleep_choice(settings)==i?"selected":"",modes[i]);SEND(html);}
    snprintf(html,sizeof(html),"</select></label><p class=muted>闲置达到所选时长后熄屏；按任意键唤醒，下班前一分钟保持亮屏。</p><label>随机彩蛋<select name=eggs><option value=1 %s>开启 · 偶尔来点惊喜</option><option value=0 %s>关闭 · 安静等下班</option></select></label><button>保存我的偏好 ✓</button></form></section>",settings->random_eggs?"selected":"",settings->random_eggs?"":"selected");SEND(html);
    snprintf(html,sizeof(html),"<section class=card data-panel=online hidden><div class=section-head><span class=index>↻</span><h2>网络管理</h2></div><p class=muted>连接 Wi-Fi 后会自动对时，也可以手动发起。</p><form method=post action=/control><input type=hidden name=action value=sync><input type=hidden name=token value='%s'><button class=secondary>立即校准时间</button></form><details><summary>更换网络 / 忘记已保存的网络</summary><p class=muted>忘记后将关闭热点；再次连接请在设备设置中打开热点。</p><form method=post action=/control><input type=hidden name=action value=forget><input type=hidden name=token value='%s'><button class=danger>确认忘记网络并关闭热点</button></form></details></section>",token,token);SEND(html);
    SEND("<script>" OD_PHONE_CLOCK_SCRIPT "</script>");
    SEND("<footer class=note><p>热点开放 5 分钟，可在设备设置重开。</p><p class=signature>OFF DUTY CLUB · 快乐准时签收</p></footer></main></body></html>");
#undef SEND
    return true;
}
