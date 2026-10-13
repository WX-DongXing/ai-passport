#include "offduty_net.h"
#include "offduty_model.h"
#include "offduty_access.h"
#include "offduty_phone.h"
#include "esp_event.h"
#include "esp_http_server.h"
#include "esp_netif.h"
#include "esp_netif_sntp.h"
#include "esp_random.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "lwip/sockets.h"
#include "nvs.h"
#include "nvs_flash.h"
#include <stdio.h>
#include <string.h>

typedef struct { char ssid[33], password[65]; } credentials_t;
typedef enum { CMD_SETUP,CMD_CLOSE,CMD_SYNC,CMD_FORGET,CMD_CREDENTIALS,CMD_IP,CMD_LOST,CMD_SCAN,CMD_PHONE_TIME } command_id_t;
typedef struct { command_id_t id; credentials_t credentials; int64_t utc_us,mono_us; } command_t;
static QueueHandle_t s_queue;
static bool s_ready;
static portMUX_TYPE s_mux=portMUX_INITIALIZER_UNLOCKED;
static od_network_t s_state;
static int64_t s_utc_anchor,s_mono_anchor,s_portal_end;
static credentials_t s_saved,s_candidate;
static bool s_trying,s_sntp;
static httpd_handle_t s_server;
static esp_netif_t *s_ap_netif;
static char s_token[33];
static nvs_handle_t s_nvs;
static od_settings_t s_settings,s_pending_settings;
static bool s_settings_pending,s_settings_saved;
static od_wifi_list_t s_wifi;
static unsigned s_save_result,s_connection_revision;

void od_net_settings_snapshot(const od_settings_t *settings,bool saved) {
    portENTER_CRITICAL(&s_mux);s_settings=*settings;s_settings_saved=saved;
    if(s_save_result==OD_PHONE_SAVING && !s_settings_pending)s_save_result=saved?OD_PHONE_SAVED:OD_PHONE_SAVE_FAILED;
    portEXIT_CRITICAL(&s_mux);
}
bool od_net_take_settings(od_settings_t *settings) {
    portENTER_CRITICAL(&s_mux);bool pending=s_settings_pending;
    if(pending){*settings=s_pending_settings;s_settings_pending=false;}
    portEXIT_CRITICAL(&s_mux);return pending;
}

void od_net_snapshot(od_network_t *out) {
    portENTER_CRITICAL(&s_mux);
    int64_t now=esp_timer_get_time();
    *out=s_state;
    out->utc_us=s_state.synced?od_clock_utc(s_utc_anchor,s_mono_anchor,now):0;
    out->sync_age_us=s_state.synced?now-s_mono_anchor:0;
    out->portal_seconds=s_state.portal?(int)((s_portal_end-now+999999)/1000000):0;
    portEXIT_CRITICAL(&s_mux);
}
static void set_failed(bool failed) { portENTER_CRITICAL(&s_mux); s_state.failed=failed; portEXIT_CRITICAL(&s_mux); }
static void synced(struct timeval *tv) {
    int64_t utc=(int64_t)tv->tv_sec*1000000+tv->tv_usec;
    if(utc<INT64_C(1735689600000000)) return;
    portENTER_CRITICAL(&s_mux);
    s_utc_anchor=utc; s_mono_anchor=esp_timer_get_time(); s_state.synced=true;s_state.phone_time=false;
    portEXIT_CRITICAL(&s_mux);
}
static void event(void *arg,esp_event_base_t base,int32_t id,void *data) {
    (void)arg; (void)data;
    if(base==WIFI_EVENT && (id==WIFI_EVENT_AP_STACONNECTED || id==WIFI_EVENT_AP_STADISCONNECTED)) {
        portENTER_CRITICAL(&s_mux);s_state.phone_connected=id==WIFI_EVENT_AP_STACONNECTED;portEXIT_CRITICAL(&s_mux);
        return;
    }
    command_t cmd={0};
    if(base==IP_EVENT && id==IP_EVENT_STA_GOT_IP) cmd.id=CMD_IP;
    else if(base==WIFI_EVENT && id==WIFI_EVENT_STA_DISCONNECTED) cmd.id=CMD_LOST;
    else return;
    (void)xQueueSend(s_queue,&cmd,0);
}
bool od_net_action(od_net_action_t action) {
    if(!s_ready) return false;
    if(action==OD_NET_SETUP) set_failed(false);
    command_t cmd={.id=(command_id_t)action};
    return xQueueSend(s_queue,&cmd,0)==pdTRUE;
}
// Accept configuration only on the private AP interface, never on the office LAN.
static bool local_request(httpd_req_t *req) {
    struct sockaddr_storage addr={0}; socklen_t len=sizeof(addr);esp_netif_ip_info_t ip;
    return esp_netif_get_ip_info(s_ap_netif,&ip)==ESP_OK && ip.ip.addr!=0 &&
        getsockname(httpd_req_to_sockfd(req),(struct sockaddr *)&addr,&len)==0 &&
        od_ap_address((struct sockaddr *)&addr,len,ip.ip.addr);
}
static bool phone_emit(void *ctx,const char *html) {
    return httpd_resp_sendstr_chunk(ctx,html)==ESP_OK;
}
static void phone_headers(httpd_req_t *req) {
    httpd_resp_set_type(req,"text/html; charset=utf-8");
    httpd_resp_set_hdr(req,"Cache-Control","no-store");
    httpd_resp_set_hdr(req,"Content-Security-Policy","default-src 'none'; style-src 'unsafe-inline'; form-action 'self'; connect-src 'self'; frame-src 'self'; script-src '" OD_PHONE_CLOCK_HASH "'; frame-ancestors 'none'");
}
static esp_err_t notice(httpd_req_t *req,const char *message,bool refresh) {
    phone_headers(req);
    if(refresh)httpd_resp_set_hdr(req,"Refresh","3;url=/");
    if(!phone_emit(req,od_phone_head))return ESP_FAIL;
    if(!phone_emit(req,"<section class=card><h1>我的小工位。</h1><p>") ||
       !phone_emit(req,message) || !phone_emit(req,"</p></section>") || !phone_emit(req,od_phone_status_frame) ||
       !phone_emit(req,"<a href=/>返回设置与连接状态 ↗</a></main></body></html>"))return ESP_FAIL;
    return httpd_resp_send_chunk(req,NULL,0);
}
static esp_err_t page(httpd_req_t *req) {
    if(!local_request(req))return httpd_resp_send_err(req,HTTPD_403_FORBIDDEN,"AP only");
    od_settings_t settings;od_wifi_list_t wifi;
    portENTER_CRITICAL(&s_mux);settings=s_settings;wifi=s_wifi;portEXIT_CRITICAL(&s_mux);
    phone_headers(req);
    if(!od_phone_page(phone_emit,req,&settings,&wifi,s_token))return ESP_FAIL;
    return httpd_resp_send_chunk(req,NULL,0);
}
static esp_err_t status_page(httpd_req_t *req) {
    if(!local_request(req))return httpd_resp_send_err(req,HTTPD_403_FORBIDDEN,"AP only");
    od_network_t net;od_net_snapshot(&net);unsigned result;
    portENTER_CRITICAL(&s_mux);
    int64_t revision=s_mono_anchor;unsigned connection_revision=s_connection_revision;
    result=s_save_result?s_save_result:(s_settings_saved?OD_PHONE_SAVED:OD_PHONE_UNSAVED);
    portEXIT_CRITICAL(&s_mux);
    phone_headers(req);
    char clock_revision[24];snprintf(clock_revision,sizeof(clock_revision),"%lld",(long long)revision);
    httpd_resp_set_hdr(req,"X-Clock-Revision",clock_revision);
    char wifi_revision[16];snprintf(wifi_revision,sizeof(wifi_revision),"%u",connection_revision);
    httpd_resp_set_hdr(req,"X-Wifi-Revision",wifi_revision);
    httpd_resp_set_hdr(req,"Content-Security-Policy","default-src 'none'; style-src 'unsafe-inline'; frame-ancestors 'self'");
    httpd_resp_set_hdr(req,"Refresh","3");
    if(!od_phone_status(phone_emit,req,net.connected,net.synced,net.connecting,net.failed,net.phone_time,result))return ESP_FAIL;
    return httpd_resp_send_chunk(req,NULL,0);
}
static esp_err_t scan_request(httpd_req_t *req) {
    if(!local_request(req))return httpd_resp_send_err(req,HTTPD_403_FORBIDDEN,"AP only");
    if(req->content_len!=50)return httpd_resp_send_err(req,HTTPD_400_BAD_REQUEST,"Invalid form");
    char body[51];int read=0;
    while(read<50){int n=httpd_req_recv(req,body+read,50-read);if(n<=0)return httpd_resp_send_err(req,HTTPD_400_BAD_REQUEST,"Incomplete form");read+=n;}
    body[read]=0;
    if(!od_phone_scan_form(body,s_token))return httpd_resp_send_err(req,HTTPD_400_BAD_REQUEST,"Invalid token");
    portENTER_CRITICAL(&s_mux);bool busy=s_wifi.scanning;s_wifi.scanning=true;portEXIT_CRITICAL(&s_mux);
    if(!busy) {
        command_t cmd={.id=CMD_SCAN};
        if(xQueueSend(s_queue,&cmd,0)!=pdTRUE){portENTER_CRITICAL(&s_mux);s_wifi.scanning=false;portEXIT_CRITICAL(&s_mux);return httpd_resp_send_err(req,HTTPD_500_INTERNAL_SERVER_ERROR,"Busy; retry");}
    }
    return notice(req,"正在扫描附近网络，稍后自动返回。",true);
}
static esp_err_t submit(httpd_req_t *req) {
    if(!local_request(req)) return httpd_resp_send_err(req,HTTPD_403_FORBIDDEN,"AP only");
    if(req->content_len<=0 || req->content_len>512) return httpd_resp_send_err(req,HTTPD_400_BAD_REQUEST,"Invalid length");
    char body[513],token[33]; int read=0;
    while(read<req->content_len) {
        int n=httpd_req_recv(req,body+read,req->content_len-read);
        if(n<=0) return httpd_resp_send_err(req,HTTPD_400_BAD_REQUEST,"Incomplete form");
        read+=n;
    }
    body[read]=0; command_t cmd={.id=CMD_CREDENTIALS};
    if(!od_form_parse(body,cmd.credentials.ssid,cmd.credentials.password,token) || strcmp(token,s_token))
        return httpd_resp_send_err(req,HTTPD_400_BAD_REQUEST,"Invalid form; refresh and retry");
    if(xQueueSend(s_queue,&cmd,0)!=pdTRUE) return httpd_resp_send_err(req,HTTPD_500_INTERNAL_SERVER_ERROR,"Busy; retry");
    memset(body,0,sizeof(body)); memset(&cmd,0,sizeof(cmd));
    return notice(req,"正在连接所选网络。下方状态自动更新，连接后可返回设置调整偏好。",false);
}
static esp_err_t submit_settings(httpd_req_t *req) {
    if(!local_request(req))return httpd_resp_send_err(req,HTTPD_403_FORBIDDEN,"AP only");
    if(req->content_len<=0 || req->content_len>512)return httpd_resp_send_err(req,HTTPD_400_BAD_REQUEST,"Invalid length");
    char body[513],token[33];int read=0;od_settings_t settings;
    while(read<req->content_len){int n=httpd_req_recv(req,body+read,req->content_len-read);if(n<=0)return httpd_resp_send_err(req,HTTPD_400_BAD_REQUEST,"Incomplete form");read+=n;}
    body[read]=0;
    if(!od_settings_form(body,&settings,token) || strcmp(token,s_token)) {
        httpd_resp_set_status(req,"400 Bad Request");
        return notice(req,"保存未提交：请检查时间和数值范围；若页面已过期，请返回设置重新提交。",false);
    }
    portENTER_CRITICAL(&s_mux);
    bool busy=s_settings_pending;if(!busy){s_pending_settings=settings;s_settings_pending=true;s_settings_saved=false;s_save_result=OD_PHONE_SAVING;}
    portEXIT_CRITICAL(&s_mux);
    if(busy){httpd_resp_set_status(req,"503 Service Unavailable");return notice(req,"上一份设置仍在保存，请稍后重试。",false);}
    return notice(req,"正在保存你的偏好。下方会自动显示设备保存结果。",false);
}
static esp_err_t control(httpd_req_t *req) {
    if(!local_request(req))return httpd_resp_send_err(req,HTTPD_403_FORBIDDEN,"AP only");
    if(req->content_len<=0 || req->content_len>=96)return httpd_resp_send_err(req,HTTPD_400_BAD_REQUEST,"Invalid length");
    char body[96];int read=0;
    while(read<req->content_len){int n=httpd_req_recv(req,body+read,req->content_len-read);if(n<=0)return httpd_resp_send_err(req,HTTPD_400_BAD_REQUEST,"Incomplete form");read+=n;}
    body[read]=0;bool forget=false;
    if(!od_control_form(body,&forget,s_token))return httpd_resp_send_err(req,HTTPD_400_BAD_REQUEST,"Invalid form");
    if(!od_net_action(forget?OD_NET_FORGET:OD_NET_SYNC))return httpd_resp_send_err(req,HTTPD_500_INTERNAL_SERVER_ERROR,"Busy; retry");
    return notice(req,forget?"忘记网络请求已提交，请查看设备状态。热点将关闭。":"对时请求已提交，请返回设置刷新查看结果。",false);
}
static esp_err_t phone_time(httpd_req_t *req) {
    if(!local_request(req))return httpd_resp_send_err(req,HTTPD_403_FORBIDDEN,"AP only");
    if(req->content_len!=56)return httpd_resp_send_err(req,HTTPD_400_BAD_REQUEST,"Invalid length");
    char body[57];int read=0;command_t cmd={.id=CMD_PHONE_TIME};
    while(read<56){int n=httpd_req_recv(req,body+read,56-read);if(n<=0)return httpd_resp_send_err(req,HTTPD_400_BAD_REQUEST,"Incomplete form");read+=n;}
    body[read]=0;
    if(!od_phone_time_form(body,s_token,&cmd.utc_us))return httpd_resp_send_err(req,HTTPD_400_BAD_REQUEST,"Invalid time or token; refresh and retry");
    cmd.mono_us=esp_timer_get_time();
    if(xQueueSend(s_queue,&cmd,0)!=pdTRUE)return httpd_resp_send_err(req,HTTPD_500_INTERNAL_SERVER_ERROR,"Busy; retry");
    return notice(req,"手机校时已提交，下方状态会自动确认。校准后可关闭热点，离线继续倒计时。",false);
}
static void close_portal(void) {
    if(s_server) { httpd_stop(s_server); s_server=NULL; }
    portENTER_CRITICAL(&s_mux); s_state.portal=false;s_state.phone_connected=false;s_wifi.scanning=false; memset(s_state.ap_password,0,sizeof(s_state.ap_password)); portEXIT_CRITICAL(&s_mux);
    memset(s_token,0,sizeof(s_token));
    if(esp_wifi_set_mode(WIFI_MODE_STA)!=ESP_OK) set_failed(true);
}
static void setup(void) {
    close_portal();
    wifi_config_t ap={0}; uint8_t mac[6];
    if(esp_wifi_get_mac(WIFI_IF_STA,mac)!=ESP_OK) { set_failed(true); return; }
    char name[24],pass[17]; snprintf(name,sizeof(name),"OFFDUTY-%02X%02X",mac[4],mac[5]);
    snprintf(pass,sizeof(pass),"%08lx%08lx",(unsigned long)esp_random(),(unsigned long)esp_random());
    snprintf(s_token,sizeof(s_token),"%08lx%08lx%08lx%08lx",(unsigned long)esp_random(),(unsigned long)esp_random(),(unsigned long)esp_random(),(unsigned long)esp_random());
    memcpy(ap.ap.ssid,name,strlen(name)); ap.ap.ssid_len=strlen(name);
    memcpy(ap.ap.password,pass,strlen(pass)); ap.ap.channel=1; ap.ap.max_connection=1; ap.ap.authmode=WIFI_AUTH_WPA2_PSK;
    if(esp_wifi_set_mode(WIFI_MODE_APSTA)!=ESP_OK || esp_wifi_set_config(WIFI_IF_AP,&ap)!=ESP_OK) { close_portal(); set_failed(true); return; }
    httpd_config_t config=HTTPD_DEFAULT_CONFIG(); config.stack_size=6144; config.max_open_sockets=2; config.lru_purge_enable=true;
    config.recv_wait_timeout=3; config.send_wait_timeout=3;
    if(httpd_start(&s_server,&config)!=ESP_OK) { close_portal(); set_failed(true); return; }
    httpd_uri_t get={.uri="/",.method=HTTP_GET,.handler=page},post={.uri="/wifi",.method=HTTP_POST,.handler=submit},settings_post={.uri="/settings",.method=HTTP_POST,.handler=submit_settings},control_post={.uri="/control",.method=HTTP_POST,.handler=control},scan_post={.uri="/scan",.method=HTTP_POST,.handler=scan_request},status_get={.uri="/status",.method=HTTP_GET,.handler=status_page},time_post={.uri="/time",.method=HTTP_POST,.handler=phone_time};
    if(httpd_register_uri_handler(s_server,&get)!=ESP_OK || httpd_register_uri_handler(s_server,&post)!=ESP_OK || httpd_register_uri_handler(s_server,&settings_post)!=ESP_OK || httpd_register_uri_handler(s_server,&control_post)!=ESP_OK || httpd_register_uri_handler(s_server,&scan_post)!=ESP_OK || httpd_register_uri_handler(s_server,&status_get)!=ESP_OK || httpd_register_uri_handler(s_server,&time_post)!=ESP_OK) { close_portal(); set_failed(true); return; }
    portENTER_CRITICAL(&s_mux);
    strcpy(s_state.ap_name,name); strcpy(s_state.ap_password,pass); s_state.portal=true; s_state.failed=false; memset(&s_wifi,0,sizeof(s_wifi));s_wifi.scanning=true; s_portal_end=esp_timer_get_time()+300000000;
    portEXIT_CRITICAL(&s_mux);
}
static void scan_wifi(void) {
    od_wifi_list_t found={0};
    od_network_t state;od_net_snapshot(&state);
    if(!state.portal)return;
    // The network worker owns scans; never interrupt an in-flight connection.
    esp_err_t err=s_trying?ESP_ERR_WIFI_STATE:esp_wifi_scan_start(NULL,true);
    if(err==ESP_OK) {
        uint16_t count=0;
        err=esp_wifi_scan_get_ap_num(&count);
        wifi_ap_record_t ap;
        for(unsigned i=0;err==ESP_OK && i<count && found.count<OD_WIFI_LIMIT;i++) {
            err=esp_wifi_scan_get_ap_record(&ap);
            ap.ssid[32]=0;
            if(err==ESP_OK && (ap.authmode==WIFI_AUTH_OPEN || ap.authmode==WIFI_AUTH_WPA_PSK ||
                              ap.authmode==WIFI_AUTH_WPA2_PSK || ap.authmode==WIFI_AUTH_WPA_WPA2_PSK || ap.authmode==WIFI_AUTH_WPA2_WPA3_PSK))
                od_wifi_add(&found,(const char *)ap.ssid,ap.rssi,ap.authmode!=WIFI_AUTH_OPEN);
        }
    }
    // Free remaining driver records, including failures and results above our cap.
    esp_wifi_clear_ap_list();
    found.failed=err!=ESP_OK;
    portENTER_CRITICAL(&s_mux);
    if(found.failed){s_wifi.failed=true;s_wifi.scanning=false;}else s_wifi=found;
    portEXIT_CRITICAL(&s_mux);
}
static bool connect_wifi(const credentials_t *c) {
    wifi_config_t config={0};
    memcpy(config.sta.ssid,c->ssid,strlen(c->ssid)); memcpy(config.sta.password,c->password,strlen(c->password));
    config.sta.threshold.authmode=c->password[0]?WIFI_AUTH_WPA_PSK:WIFI_AUTH_OPEN;
    config.sta.pmf_cfg.capable=true; config.sta.pmf_cfg.required=false;
    return esp_wifi_set_config(WIFI_IF_STA,&config)==ESP_OK && esp_wifi_connect()==ESP_OK;
}
static void worker(void *arg) {
    (void)arg; int64_t retry=0; command_t cmd;
    for(;;) {
        if(xQueueReceive(s_queue,&cmd,pdMS_TO_TICKS(1000))==pdTRUE) {
            switch(cmd.id) {
            case CMD_PHONE_TIME:
                portENTER_CRITICAL(&s_mux);
                s_utc_anchor=cmd.utc_us;s_mono_anchor=cmd.mono_us;s_state.synced=true;s_state.phone_time=true;s_state.phone_revision++;
                portEXIT_CRITICAL(&s_mux);break;
            case CMD_SETUP: setup(); scan_wifi(); break;
            case CMD_SCAN: scan_wifi(); break;
            case CMD_CLOSE: close_portal(); break;
            case CMD_SYNC: if(s_sntp && esp_netif_sntp_start()!=ESP_OK) set_failed(true); break;
            case CMD_FORGET:
                if(nvs_erase_key(s_nvs,"wifi")!=ESP_OK && s_saved.ssid[0]) { set_failed(true); break; }
                if(nvs_commit(s_nvs)!=ESP_OK) { set_failed(true); break; }
                memset(&s_saved,0,sizeof(s_saved)); memset(&s_candidate,0,sizeof(s_candidate)); s_trying=false;
                esp_wifi_disconnect(); close_portal();
                portENTER_CRITICAL(&s_mux); s_state.connected=false; portEXIT_CRITICAL(&s_mux); break;
            case CMD_CREDENTIALS:
                s_candidate=cmd.credentials; s_trying=true;
                portENTER_CRITICAL(&s_mux);s_connection_revision++;s_state.connecting=true;s_state.connected=false;s_state.failed=false;portEXIT_CRITICAL(&s_mux);
                esp_wifi_disconnect();
                set_failed(!connect_wifi(&s_candidate)); retry=esp_timer_get_time()+10000000; break;
            case CMD_IP: {
                wifi_ap_record_t ap;
                if(esp_wifi_sta_get_ap_info(&ap)!=ESP_OK || (s_trying && strcmp((const char *)ap.ssid,s_candidate.ssid))) break;
                portENTER_CRITICAL(&s_mux); s_state.connected=true; s_state.connecting=false;s_state.failed=false; portEXIT_CRITICAL(&s_mux);
                if(s_trying) {
                    if(nvs_set_blob(s_nvs,"wifi",&s_candidate,sizeof(s_candidate))==ESP_OK && nvs_commit(s_nvs)==ESP_OK) s_saved=s_candidate;
                    else set_failed(true);
                    s_trying=false; memset(&s_candidate,0,sizeof(s_candidate));
                }
                if(!s_sntp) {
                    esp_sntp_config_t config=ESP_NETIF_SNTP_DEFAULT_CONFIG_MULTIPLE(2,ESP_SNTP_SERVER_LIST("ntp.aliyun.com","time.cloudflare.com"));
                    config.sync_cb=synced; config.wait_for_sync=false;
                    s_sntp=esp_netif_sntp_init(&config)==ESP_OK; if(!s_sntp) set_failed(true);
                } else if(esp_netif_sntp_start()!=ESP_OK) set_failed(true);
                break;
            }
            case CMD_LOST:
                portENTER_CRITICAL(&s_mux); s_state.connected=false; portEXIT_CRITICAL(&s_mux);
                retry=esp_timer_get_time()+10000000; break;
            }
            memset(&cmd,0,sizeof(cmd));
        }
        od_network_t state; od_net_snapshot(&state); int64_t now=esp_timer_get_time();
        if(state.portal && state.portal_seconds<=0) close_portal();
        if(!state.connected && now>=retry && (s_trying || s_saved.ssid[0])) {
            if(s_trying)set_failed(true);
            if(!connect_wifi(s_trying?&s_candidate:&s_saved))set_failed(true);
            retry=now+10000000;
        }
    }
}
esp_err_t od_net_init(void) {
    od_settings_default(&s_settings);
    esp_err_t err=nvs_flash_init(); if(err!=ESP_OK) return err;
    if((err=nvs_open("offduty",NVS_READWRITE,&s_nvs))!=ESP_OK) return err;
    size_t size=sizeof(s_saved);
    if(nvs_get_blob(s_nvs,"wifi",&s_saved,&size)!=ESP_OK || size!=sizeof(s_saved) || !memchr(s_saved.ssid,0,33) || !memchr(s_saved.password,0,65)) memset(&s_saved,0,sizeof(s_saved));
    if((err=esp_netif_init())!=ESP_OK) return err;
    if((err=esp_event_loop_create_default())!=ESP_OK) return err;
    if(!esp_netif_create_default_wifi_sta() || !(s_ap_netif=esp_netif_create_default_wifi_ap())) return ESP_ERR_NO_MEM;
    s_queue=xQueueCreate(12,sizeof(command_t)); if(!s_queue) return ESP_ERR_NO_MEM;
    wifi_init_config_t config=WIFI_INIT_CONFIG_DEFAULT();
    if((err=esp_wifi_init(&config))!=ESP_OK) return err;
    if((err=esp_wifi_set_storage(WIFI_STORAGE_RAM))!=ESP_OK) return err;
    if((err=esp_event_handler_register(WIFI_EVENT,ESP_EVENT_ANY_ID,event,NULL))!=ESP_OK) return err;
    if((err=esp_event_handler_register(IP_EVENT,IP_EVENT_STA_GOT_IP,event,NULL))!=ESP_OK) return err;
    if((err=esp_wifi_set_mode(WIFI_MODE_STA))!=ESP_OK) return err;
    if((err=esp_wifi_start())!=ESP_OK) return err;
    if(xTaskCreate(worker,"offduty_net",4096,NULL,4,NULL)!=pdPASS) return ESP_ERR_NO_MEM;
    s_ready=true;
    return ESP_OK;
}
