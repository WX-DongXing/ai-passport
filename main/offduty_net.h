#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
#include "offduty_model.h"

typedef struct {
    bool connected, synced, portal, failed, phone_connected, connecting;
    int64_t utc_us, sync_age_us;
    int portal_seconds;
    char ap_name[24], ap_password[17];
} od_network_t;
typedef enum { OD_NET_SETUP, OD_NET_CLOSE, OD_NET_SYNC, OD_NET_FORGET } od_net_action_t;
esp_err_t od_net_init(void);
bool od_net_action(od_net_action_t action);
void od_net_snapshot(od_network_t *out);
void od_net_settings_snapshot(const od_settings_t *settings, bool saved);
bool od_net_take_settings(od_settings_t *settings);
