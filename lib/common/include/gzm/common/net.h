#ifndef GZM_COMMON_NET_H
#define GZM_COMMON_NET_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Buffer sizes */

#define GZM_NET_LEN_IPV4_ADDR        (16)  /* "255.255.255.255\0" */
#define GZM_NET_LEN_MAC_ADDR         (18)  /* "AA:BB:CC:DD:EE:FF\0" */
#define GZM_NET_SIZE_HOST            (64)
#define GZM_NET_SIZE_HOST_PARAM      (30)
#define GZM_NET_SIZE_SNTP            (100)
#define GZM_NET_SIZE_MQTT_TOPIC      (128)

/* Types */

/**
 * @brief IPv4 network configuration (static or DHCP).
 *
 * Used by any gzm library that brings up an IP interface, wifi_mgr,
 * eth_mgr, and persistence layers that store the last known config.
 */
struct gzm_net_config {
	bool dhcp_enable;
	char ip_address[GZM_NET_LEN_IPV4_ADDR];
	char netmask[GZM_NET_LEN_IPV4_ADDR];
	char gateway[GZM_NET_LEN_IPV4_ADDR];
	char dns[GZM_NET_LEN_IPV4_ADDR];
	char dns2[GZM_NET_LEN_IPV4_ADDR];
	char mac_addr[GZM_NET_LEN_MAC_ADDR];
	char sntp[GZM_NET_SIZE_SNTP];
	char sntp2[GZM_NET_SIZE_SNTP];
};

/**
 * @brief Target endpoint (host + port) for outgoing client
 * connections (HTTP, MQTT, SNTP client mode, ...).
 */
struct gzm_net_endpoint {
	char host[GZM_NET_SIZE_HOST];
	uint16_t port;
};

#ifdef __cplusplus
}
#endif

#endif /* GZM_COMMON_NET_H */
