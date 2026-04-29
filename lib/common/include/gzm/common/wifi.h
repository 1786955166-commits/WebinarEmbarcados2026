#ifndef GZM_COMMON_WIFI_H
#define GZM_COMMON_WIFI_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <gzm/common/net.h>

#ifdef __cplusplus
extern "C" {
#endif

#define GZM_WIFI_SIZE_SSID           (33)  /* 32 bytes + NUL */
#define GZM_WIFI_SIZE_PASSWORD       (64)
#define GZM_WIFI_SIZE_SEC_FLAGS      (50)
#define GZM_WIFI_MAX_SCAN_LIST       (15)

/**
 * @brief AP credentials (SSID + PSK).
 */
struct gzm_wifi_credentials {
	char ssid[GZM_WIFI_SIZE_SSID];
	char password[GZM_WIFI_SIZE_PASSWORD];
};

/**
 * @brief Full WiFi configuration: credentials + network config.
 */
struct gzm_wifi_settings {
	struct gzm_wifi_credentials access;
	struct gzm_net_config net;
};

/**
 * @brief Current WiFi link state (reported by wifi_mgr_get_status).
 */
struct gzm_wifi_status {
	struct gzm_net_config net;
	char mac[GZM_NET_LEN_MAC_ADDR];
	char ssid[GZM_WIFI_SIZE_SSID];
	int8_t rssi;
	bool is_connected;
};

/**
 * @brief Information about one AP discovered during a scan.
 */
struct gzm_wifi_scan_entry {
	int8_t rssi;
	uint16_t frequency;
	char bssid[GZM_NET_LEN_MAC_ADDR];
	char security[GZM_WIFI_SIZE_SEC_FLAGS];
	char ssid[GZM_WIFI_SIZE_SSID];
};

/**
 * @brief Result of a WiFi scan.
 */
struct gzm_wifi_scan_result {
	struct gzm_wifi_scan_entry entries[GZM_WIFI_MAX_SCAN_LIST];
	uint16_t count;
};

/**
 * @brief Fill a gzm_wifi_settings struct with placeholder defaults.
 *
 * Useful on first-boot when no settings have been persisted yet -
 * the struct is valid (non-random) so the rest of the stack can run
 * in a well-defined state until the user provides real credentials.
 */
static inline void gzm_wifi_settings_init(struct gzm_wifi_settings *settings)
{
	memset(settings, 0, sizeof(*settings));

	snprintf(settings->access.ssid, GZM_WIFI_SIZE_SSID, "%s",
		 "@MyNetwork");
	snprintf(settings->access.password, GZM_WIFI_SIZE_PASSWORD, "%s",
		 "MyPassword");

	settings->net.dhcp_enable = true;
	snprintf(settings->net.ip_address, GZM_NET_LEN_IPV4_ADDR, "%s",
		 "0.0.0.0");
	snprintf(settings->net.netmask, GZM_NET_LEN_IPV4_ADDR, "%s",
		 "0.0.0.0");
	snprintf(settings->net.gateway, GZM_NET_LEN_IPV4_ADDR, "%s",
		 "0.0.0.0");
	snprintf(settings->net.dns, GZM_NET_LEN_IPV4_ADDR, "%s",
		 "0.0.0.0");
	snprintf(settings->net.dns2, GZM_NET_LEN_IPV4_ADDR, "%s",
		 "0.0.0.0");
	snprintf(settings->net.sntp, GZM_NET_SIZE_SNTP, "%s",
		 "pool.ntp.org");
	snprintf(settings->net.sntp2, GZM_NET_SIZE_SNTP, "%s",
		 "1.pool.ntp.org");
}

#ifdef __cplusplus
}
#endif

#endif /* GZM_COMMON_WIFI_H */
