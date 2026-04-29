#include <gzm/wifi_mgr.h>

#include <string.h>
#include <stdio.h>

#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/dhcpv4.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/sntp.h>
#include <arpa/inet.h>

#include <gzm/common/net.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(wifi_mgr, CONFIG_LOG_DEFAULT_LEVEL);

static void wifi_mgr_on_connection_event(struct net_mgmt_event_callback *cb,
                                      uint64_t mgmt_event,
                                      struct net_if *iface);
static void wifi_mgr_on_ipv4_obtained(struct net_mgmt_event_callback *cb,
                                   uint64_t mgmt_event,
                                   struct net_if *iface);

#define WIFI_MGR_EVENT_QUEUE_DEPTH 8

static struct net_if *g_iface = NULL;
static struct net_mgmt_event_callback g_wifi_cb;
static struct net_mgmt_event_callback g_ipv4_cb;
static K_SEM_DEFINE(g_wifi_sem, 0, 1);
static K_SEM_DEFINE(g_wifi_sem_ipv4, 0, 1);

K_MSGQ_DEFINE(g_event_q, sizeof(struct wifi_mgr_event_data),
	      WIFI_MGR_EVENT_QUEUE_DEPTH, 4);

/* Push an event onto the queue. If the queue is full, drop the oldest
 * to make room, newer state is more useful than older state.
 */
static void wifi_mgr_post_event(const struct wifi_mgr_event_data *evt)
{
	int ret = k_msgq_put(&g_event_q, evt, K_NO_WAIT);

	if (ret == -ENOMSG) {
		struct wifi_mgr_event_data discard;

		(void)k_msgq_get(&g_event_q, &discard, K_NO_WAIT);
		(void)k_msgq_put(&g_event_q, evt, K_NO_WAIT);
	}
}

static void wifi_mgr_on_connection_event(struct net_mgmt_event_callback *cb,
                                      uint64_t mgmt_event,
                                      struct net_if *iface)
{
	const struct wifi_status *status = (const struct wifi_status *)cb->info;

	if (mgmt_event == NET_EVENT_WIFI_CONNECT_RESULT) {
		if (status->status) {
			LOG_ERR("Connection request failed (%d)", status->status);
		} else {
			struct wifi_mgr_event_data evt = {
				.type = WIFI_MGR_EVENT_CONNECTED,
			};

			LOG_INF("Connected");
			k_sem_give(&g_wifi_sem);
			wifi_mgr_post_event(&evt);
		}
	} else if (mgmt_event == NET_EVENT_WIFI_DISCONNECT_RESULT) {
		if (status->status) {
			LOG_ERR("Disconnection request failed (%d)", status->status);
		} else {
			struct wifi_mgr_event_data evt = {
				.type = WIFI_MGR_EVENT_DISCONNECTED,
			};

			LOG_INF("Disconnected");
			k_sem_take(&g_wifi_sem, K_NO_WAIT);
			wifi_mgr_post_event(&evt);
		}
	}
}

static void wifi_mgr_on_ipv4_obtained(struct net_mgmt_event_callback *cb,
                                   uint64_t mgmt_event,
                                   struct net_if *iface)
{
	if (mgmt_event == NET_EVENT_IPV4_ADDR_ADD) {
		struct wifi_mgr_event_data evt = {
			.type = WIFI_MGR_EVENT_IP_OBTAINED,
		};

		k_sem_give(&g_wifi_sem_ipv4);

		if (iface && iface->config.ip.ipv4) {
			zsock_inet_ntop(AF_INET,
				&iface->config.ip.ipv4->unicast[0].ipv4.address.in_addr,
				evt.ip, sizeof(evt.ip));
			zsock_inet_ntop(AF_INET,
				&iface->config.ip.ipv4->unicast[0].netmask,
				evt.netmask, sizeof(evt.netmask));
			zsock_inet_ntop(AF_INET, &iface->config.ip.ipv4->gw,
				evt.gw, sizeof(evt.gw));
		}

		wifi_mgr_post_event(&evt);
	}
}

int wifi_mgr_event_get(struct wifi_mgr_event_data *evt, k_timeout_t timeout)
{
	if (evt == NULL) {
		return -EINVAL;
	}
	return k_msgq_get(&g_event_q, evt, timeout);
}

void wifi_mgr_event_drain(void)
{
	k_msgq_purge(&g_event_q);
}

#ifdef CONFIG_GZM_WIFI_MGR_TEST_HOOKS
void wifi_mgr_test_post_event(const struct wifi_mgr_event_data *evt)
{
	if (evt) {
		wifi_mgr_post_event(evt);
	}
}
#endif

int wifi_mgr_init(struct net_if *iface)
{
	if (iface == NULL) {
		LOG_ERR("wifi_mgr_init: NULL net_if");
		return -EINVAL;
	}

	g_iface = iface;

	net_mgmt_init_event_callback(&g_wifi_cb,
	                             wifi_mgr_on_connection_event,
	                             NET_EVENT_WIFI_CONNECT_RESULT | NET_EVENT_WIFI_DISCONNECT_RESULT);
	net_mgmt_init_event_callback(&g_ipv4_cb,
	                             wifi_mgr_on_ipv4_obtained,
	                             NET_EVENT_IPV4_ADDR_ADD);

	net_mgmt_add_event_callback(&g_wifi_cb);
	net_mgmt_add_event_callback(&g_ipv4_cb);

	return 0;
}

int wifi_mgr_connect(char *ssid, char *psk)
{
	int ret = 0;
	struct wifi_connect_req_params params;

	if (g_iface == NULL) {
		LOG_ERR("wifi_mgr not initialized");
		return -ENODEV;
	}

	k_sem_reset(&g_wifi_sem);

	memset(&params, 0, sizeof(params));
	params.ssid = (const uint8_t *)ssid;
	params.ssid_length = strlen(ssid);
	params.psk = (const uint8_t *)psk;
	params.psk_length = strlen(psk);
	params.security = WIFI_SECURITY_TYPE_PSK;
	params.band = WIFI_FREQ_BAND_UNKNOWN;
	params.channel = WIFI_CHANNEL_ANY;
	params.mfp = WIFI_MFP_OPTIONAL;

	LOG_INF("Connecting to WiFi SSID: %s", ssid);

	ret = net_mgmt(NET_REQUEST_WIFI_CONNECT, g_iface, &params, sizeof(params));
	if (ret < 0) {
		LOG_ERR("WiFi connect request failed (%d)", ret);
		return ret;
	}

	ret = k_sem_take(&g_wifi_sem, K_SECONDS(30));
	if (ret == -EAGAIN) {
		LOG_ERR("WiFi connection timeout");
		return -ETIMEDOUT;
	}

	return 0;
}

int wifi_mgr_wait_for_ip(void)
{
	int ret = 0;
	char ip_addr[NET_IPV4_ADDR_LEN];
	char gw_addr[NET_IPV4_ADDR_LEN];
	struct wifi_iface_status status;

	if (g_iface == NULL) {
		LOG_ERR("wifi_mgr not initialized");
		return -ENODEV;
	}

	LOG_INF("Starting DHCP client...");
	net_dhcpv4_start(g_iface);

	LOG_INF("Waiting for IP address...");

	ret = k_sem_take(&g_wifi_sem_ipv4, K_SECONDS(30));
	if (ret == -EAGAIN) {
		LOG_ERR("IP address timeout");
		return -ETIMEDOUT;
	}

	if (net_mgmt(NET_REQUEST_WIFI_IFACE_STATUS, g_iface, &status,
	             sizeof(struct wifi_iface_status))) {
		LOG_ERR("WiFi status request failed");
	}

	memset(ip_addr, 0, sizeof(ip_addr));
	if (zsock_inet_ntop(AF_INET,
	                    &g_iface->config.ip.ipv4->unicast[0].ipv4.address.in_addr,
	                    ip_addr, sizeof(ip_addr)) == NULL) {
		LOG_ERR("Could not convert IP address to string");
	}

	memset(gw_addr, 0, sizeof(gw_addr));
	if (zsock_inet_ntop(AF_INET, &g_iface->config.ip.ipv4->gw,
	                    gw_addr, sizeof(gw_addr)) == NULL) {
		LOG_ERR("Could not convert gateway address to string");
	}

	LOG_INF("WiFi status:");
	if (status.state >= WIFI_STATE_ASSOCIATED) {
		LOG_INF("  SSID: %-32s", status.ssid);
		LOG_INF("  Band: %s", wifi_band_txt(status.band));
		LOG_INF("  Channel: %d", status.channel);
		LOG_INF("  Security: %s", wifi_security_txt(status.security));
		LOG_INF("  IP address: %s", ip_addr);
		LOG_INF("  Gateway: %s", gw_addr);
	}

	return 0;
}

int wifi_mgr_disconnect(void)
{
	if (g_iface == NULL) {
		LOG_ERR("wifi_mgr not initialized");
		return -ENODEV;
	}

	return net_mgmt(NET_REQUEST_WIFI_DISCONNECT, g_iface, NULL, 0);
}

uint32_t wifi_mgr_sntp(const char *server_addr, uint16_t port)
{
	int ret = 0;
	uint32_t timestamp = 0;
	struct sntp_ctx ctx;
	struct sockaddr_in addr;
	struct sntp_time sntp_time;
	struct net_if_ipv4 *ipv4 = NULL;

	if (g_iface == NULL) {
		LOG_ERR("SNTP: wifi_mgr not initialized");
		return 0;
	}

	ipv4 = g_iface->config.ip.ipv4;
	if (!ipv4 || ipv4->unicast[0].ipv4.address.in_addr.s_addr == 0) {
		LOG_ERR("SNTP: No valid IP address");
		return 0;
	}

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);

	if (zsock_inet_pton(AF_INET, server_addr, &addr.sin_addr) <= 0) {
		LOG_ERR("SNTP: Invalid server address: %s", server_addr);
		return 0;
	}

	ret = sntp_init(&ctx, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));
	if (ret < 0) {
		LOG_ERR("SNTP: Failed to init context: %d", ret);
		return 0;
	}

	LOG_INF("SNTP: Requesting time from %s...", server_addr);

	ret = sntp_query(&ctx, 10 * MSEC_PER_SEC, &sntp_time);
	if (ret < 0) {
		LOG_ERR("SNTP: Request failed: %d", ret);
		sntp_close(&ctx);
		return 0;
	}

	LOG_INF("SNTP: Success! Time: %u (high), %u (low)",
	        (uint32_t)(sntp_time.seconds >> 32),
	        (uint32_t)sntp_time.seconds);

	timestamp = (uint32_t)sntp_time.seconds;

	sntp_close(&ctx);
	return timestamp;
}

int wifi_mgr_get_status(struct gzm_wifi_status *connection)
{
	int ret = 0;
	struct wifi_iface_status status;

	if (!connection) {
		return -EINVAL;
	}

	if (g_iface == NULL) {
		LOG_ERR("wifi_mgr not initialized");
		return -ENODEV;
	}

	/* Initialize structure */
	memset(connection, 0, sizeof(struct gzm_wifi_status));

	/* Get WiFi status */
	ret = net_mgmt(NET_REQUEST_WIFI_IFACE_STATUS, g_iface, &status,
	               sizeof(struct wifi_iface_status));

	/* Check if connected */
	connection->is_connected = (ret == 0 && status.state >= WIFI_STATE_ASSOCIATED);

	if (!connection->is_connected) {
		return 0; /* Not an error, just not connected */
	}

	/* Fill WiFi information */
	connection->rssi = status.rssi;
	snprintf(connection->ssid, GZM_WIFI_SIZE_SSID, "%s", status.ssid);

	/* MAC Address (BSSID) */
	snprintf(connection->mac, GZM_NET_LEN_MAC_ADDR,
	         "%02X:%02X:%02X:%02X:%02X:%02X",
	         status.bssid[0], status.bssid[1], status.bssid[2],
	         status.bssid[3], status.bssid[4], status.bssid[5]);

	/* Check if we have IP configuration */
	if (g_iface->config.ip.ipv4 &&
	    g_iface->config.ip.ipv4->unicast[0].ipv4.address.in_addr.s_addr != 0) {

		/* DHCP status - assume enabled if we have an IP */
		connection->net.dhcp_enable = true;

		/* IP Address */
		if (zsock_inet_ntop(AF_INET,
		                    &g_iface->config.ip.ipv4->unicast[0].ipv4.address.in_addr,
		                    connection->net.ip_address,
		                    GZM_NET_LEN_IPV4_ADDR) == NULL) {
			LOG_WRN("Could not convert IP address");
		}

		/* Netmask */
		if (zsock_inet_ntop(AF_INET,
		                    &g_iface->config.ip.ipv4->unicast[0].netmask,
		                    connection->net.netmask,
		                    GZM_NET_LEN_IPV4_ADDR) == NULL) {
			LOG_WRN("Could not convert netmask");
		}

		/* Gateway */
		if (zsock_inet_ntop(AF_INET, &g_iface->config.ip.ipv4->gw,
		                    connection->net.gateway,
		                    GZM_NET_LEN_IPV4_ADDR) == NULL) {
			LOG_WRN("Could not convert gateway");
		}

		/* DNS - Zephyr doesn't expose DNS config easily, set to N/A */
		snprintf(connection->net.dns, GZM_NET_LEN_IPV4_ADDR, "N/A");
		snprintf(connection->net.dns2, GZM_NET_LEN_IPV4_ADDR, "N/A");

		/* SNTP - Set to N/A */
		snprintf(connection->net.sntp, GZM_NET_SIZE_SNTP, "N/A");
		snprintf(connection->net.sntp2, GZM_NET_SIZE_SNTP, "N/A");
	}

	return 0;
}
