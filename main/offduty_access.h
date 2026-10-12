#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#ifdef ESP_PLATFORM
#include "lwip/sockets.h"
#else
#include <sys/socket.h>
#include <netinet/in.h>
#endif

// The dual-stack HTTP server reports IPv4 connections as mapped IPv6 addresses.
static inline bool od_ap_address(const struct sockaddr *addr,socklen_t len,uint32_t ap) {
    if(len>=sizeof(struct sockaddr_in) && addr->sa_family==AF_INET)
        return ((const struct sockaddr_in *)addr)->sin_addr.s_addr==ap;
    if(len>=sizeof(struct sockaddr_in6) && addr->sa_family==AF_INET6) {
        const struct in6_addr *ip=&((const struct sockaddr_in6 *)addr)->sin6_addr;
        return IN6_IS_ADDR_V4MAPPED(ip) && !memcmp(&ip->s6_addr[12],&ap,sizeof(ap));
    }
    return false;
}
