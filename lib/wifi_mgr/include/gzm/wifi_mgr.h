#ifndef GZM_WIFI_MGR_H
#define GZM_WIFI_MGR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#include <zephyr/kernel.h>
#include <zephyr/net/net_if.h>

#include <gzm/common/net.h>
#include <gzm/common/wifi.h>

/**
 * @brief WiFi connectivity events emitted by wifi_mgr to subscribers.
 */
enum wifi_mgr_event {
	WIFI_MGR_EVENT_CONNECTED,    /**< WiFi associated successfully */
	WIFI_MGR_EVENT_DISCONNECTED, /**< WiFi association lost */
	WIFI_MGR_EVENT_IP_OBTAINED,  /**< IPv4 address obtained via DHCP */
};

/**
 * @brief Payload carried by each wifi_mgr event.
 *
 * The fields populated depend on the event type:
 *   - WIFI_MGR_EVENT_CONNECTED:    none currently (could carry SSID/RSSI)
 *   - WIFI_MGR_EVENT_DISCONNECTED: none
 *   - WIFI_MGR_EVENT_IP_OBTAINED:  ip / netmask / gw
 */
struct wifi_mgr_event_data {
	enum wifi_mgr_event type;
	char ip[GZM_NET_LEN_IPV4_ADDR];
	char netmask[GZM_NET_LEN_IPV4_ADDR];
	char gw[GZM_NET_LEN_IPV4_ADDR];
};

/**
 * @brief Initialize WiFi management subsystem.
 *
 * Stores the network interface used by all subsequent calls and registers
 * network management callbacks for WiFi connection and IPv4 address events.
 * The caller resolves the interface (e.g. via net_if_get_default()) so the
 * library stays portable on systems with multiple interfaces or non-default
 * network stacks.
 *
 * @param iface Network interface to drive. Must not be NULL.
 * @return 0 on success, negative errno on failure.
 */
int wifi_mgr_init(struct net_if *iface);

/**
 * @brief Connect to a WiFi network
 *
 * @param ssid SSID of the WiFi network
 * @param psk Pre-shared key (password)
 * @return 0 on success, negative error code on failure
 */
int wifi_mgr_connect(char *ssid, char *psk);

/**
 * @brief Wait for IP address to be obtained via DHCP
 *
 * Starts DHCP client and blocks until an IP address is obtained
 * or a 30-second timeout occurs.
 *
 * @return 0 on success, -ETIMEDOUT on timeout
 */
int wifi_mgr_wait_for_ip(void);

/**
 * @brief Disconnect from the current WiFi network
 *
 * @return 0 on success, negative error code on failure
 */
int wifi_mgr_disconnect(void);

/**
 * @brief Get NTP time from server
 *
 * @param server_addr NTP server IP address string
 * @param port NTP server port (usually 123)
 * @return Unix timestamp on success, 0 on failure
 */
uint32_t wifi_mgr_sntp(const char *server_addr, uint16_t port);

/**
 * @brief Get current WiFi connection status
 *
 * @param connection Pointer to structure to fill with connection info
 * @return 0 on success, negative error code on failure
 */
int wifi_mgr_get_status(struct gzm_wifi_status *connection);

/**
 * @brief Wait for the next wifi event from the internal queue.
 *
 * Blocking consumer-side API. Producer is the wifi_mgr's own net_mgmt
 * callbacks. The queue holds up to 8 pending events; if the consumer
 * cannot keep up, the oldest event is dropped to make room.
 *
 * @param evt     Output buffer filled with the next event.
 * @param timeout K_FOREVER, K_NO_WAIT, or a duration.
 * @return 0 on success, -EAGAIN on timeout, -ENOMSG if returned without
 *         a message (e.g. K_NO_WAIT and queue was empty).
 */
int wifi_mgr_event_get(struct wifi_mgr_event_data *evt, k_timeout_t timeout);

/**
 * @brief Discard any pending events without blocking.
 *
 * Useful when transitioning to a state where stale events should not
 * affect behaviour (e.g. before retrying a connect).
 */
void wifi_mgr_event_drain(void);

#ifdef CONFIG_GZM_WIFI_MGR_TEST_HOOKS
/**
 * @brief Test-only: inject a synthetic event into the queue.
 *
 * Bypasses the net_mgmt callbacks so ztest can drive the queue
 * deterministically (without a real WiFi driver). Available only when
 * CONFIG_GZM_WIFI_MGR_TEST_HOOKS=y. Do not use from production code.
 *
 * @param evt Event to push.
 */
void wifi_mgr_test_post_event(const struct wifi_mgr_event_data *evt);
#endif

#ifdef __cplusplus
}
#endif

#endif /* GZM_WIFI_MGR_H */
