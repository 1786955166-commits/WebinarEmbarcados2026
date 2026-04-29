#define _GNU_SOURCE
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/http/server.h>
#include <zephyr/net/http/service.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/data/json.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/led_strip.h>
#include <zephyr/fs/fs.h>

#include <app_version.h>

#include "web_app.h"
#include <gzm/ringtone.h>
#include <gzm/rtc_mgr.h>

LOG_MODULE_REGISTER(web_app, CONFIG_LOG_DEFAULT_LEVEL);

#define NUM_RELAYS           8
#define BUZZER_RTTTL_MAX_LEN 512
#define FILES_RESP_BUF_SIZE  2048
#define DOWNLOAD_CHUNK_SIZE  1024
#define UPLOAD_BUF_SIZE      2048

#define WEB_INFO_AUTHOR  "Jorge Guzman"
#define WEB_INFO_SITE    "gzm-emb.com"
#define WEB_INFO_CONTACT "jorge@gzm-emb.com"

#define SET_JSON_RESPONSE(ctx, buf, len) \
	do {                                 \
		(ctx)->body = (uint8_t *)(buf);  \
		(ctx)->body_len = (len);         \
		(ctx)->final_chunk = true;       \
	} while (0)

struct relay_cmd {
	int ch;
	bool state;
};

static const struct json_obj_descr relay_cmd_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct relay_cmd, ch,    JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct relay_cmd, state, JSON_TOK_TRUE),
};

struct rgb_cmd {
	int r;
	int g;
	int b;
};

static const struct json_obj_descr rgb_cmd_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct rgb_cmd, r, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct rgb_cmd, g, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct rgb_cmd, b, JSON_TOK_NUMBER),
};

struct buzzer_cmd {
	const char *rtttl;
	const char *action;
};

static const struct json_obj_descr buzzer_cmd_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct buzzer_cmd, rtttl,  JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct buzzer_cmd, action, JSON_TOK_STRING),
};

struct delete_cmd {
	const char *path;
};

static const struct json_obj_descr delete_cmd_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct delete_cmd, path, JSON_TOK_STRING),
};

struct upload_ctx {
	struct fs_file_t file;
	bool file_open;
	bool header_parsed;
	char filepath[160];
	uint8_t buf[UPLOAD_BUF_SIZE];
	size_t buf_len;
	char boundary[80];
	size_t boundary_len;
};

static int   web_app_build_relays_json(char *buf, size_t buf_size);
static size_t web_app_url_decode(const char *src, size_t src_len,
				    char *dst, size_t dst_size);
static bool  web_app_validate_rtttl(const char *str);
static const char *web_app_content_type_from_path(const char *path);
static const uint8_t *web_app_find_header_end(const uint8_t *data, size_t len);
static bool  web_app_extract_filename(const uint8_t *header_start, size_t header_len,
					 char *fname, size_t fname_size);
static bool  web_app_extract_dir(const uint8_t *data, size_t len,
				    char *dir, size_t dir_size);
static int web_app_relay_handler(struct http_client_ctx *client,
				    enum http_transaction_status status,
				    const struct http_request_ctx *request_ctx,
				    struct http_response_ctx *response_ctx,
				    void *user_data);
static int web_app_rgb_handler(struct http_client_ctx *client,
				  enum http_transaction_status status,
				  const struct http_request_ctx *request_ctx,
				  struct http_response_ctx *response_ctx,
				  void *user_data);
static int web_app_buzzer_handler(struct http_client_ctx *client,
				     enum http_transaction_status status,
				     const struct http_request_ctx *request_ctx,
				     struct http_response_ctx *response_ctx,
				     void *user_data);
static int web_app_files_handler(struct http_client_ctx *client,
				    enum http_transaction_status status,
				    const struct http_request_ctx *request_ctx,
				    struct http_response_ctx *response_ctx,
				    void *user_data);
static int web_app_download_handler(struct http_client_ctx *client,
				       enum http_transaction_status status,
				       const struct http_request_ctx *request_ctx,
				       struct http_response_ctx *response_ctx,
				       void *user_data);
static int web_app_delete_handler(struct http_client_ctx *client,
				     enum http_transaction_status status,
				     const struct http_request_ctx *request_ctx,
				     struct http_response_ctx *response_ctx,
				     void *user_data);
static int web_app_upload_handler(struct http_client_ctx *client,
				     enum http_transaction_status status,
				     const struct http_request_ctx *request_ctx,
				     struct http_response_ctx *response_ctx,
				     void *user_data);
static int web_app_time_handler(struct http_client_ctx *client,
				    enum http_transaction_status status,
				    const struct http_request_ctx *request_ctx,
				    struct http_response_ctx *response_ctx,
				    void *user_data);

static K_SEM_DEFINE(relay_lock,    1, 1);
static K_SEM_DEFINE(rgb_lock,      1, 1);
static K_SEM_DEFINE(buzzer_lock,   1, 1);
static K_SEM_DEFINE(download_lock, 1, 1);

static bool relay_state[NUM_RELAYS];
static struct led_rgb rgb_pixel;
static char buzzer_rtttl_buf[BUZZER_RTTTL_MAX_LEN + 1];
static struct upload_ctx upload;
static struct http_client_ctx *download_owner;

static const uint8_t index_html_gz[] = {
#include "index.html.gz.inc"
};

static const uint8_t app_js_gz[] = {
#include "app.js.gz.inc"
};

static const uint8_t style_css_gz[] = {
#include "style.css.gz.inc"
};

static const char info_json[] =
	"{\"firmware\":\"" APP_VERSION_STRING "\","
	"\"author\":\""    WEB_INFO_AUTHOR     "\","
	"\"site\":\""      WEB_INFO_SITE       "\","
	"\"contact\":\""   WEB_INFO_CONTACT    "\"}";

static const struct gpio_dt_spec relays[NUM_RELAYS] = {
	GPIO_DT_SPEC_GET(DT_ALIAS(relay1), gpios),
	GPIO_DT_SPEC_GET(DT_ALIAS(relay2), gpios),
	GPIO_DT_SPEC_GET(DT_ALIAS(relay3), gpios),
	GPIO_DT_SPEC_GET(DT_ALIAS(relay4), gpios),
	GPIO_DT_SPEC_GET(DT_ALIAS(relay5), gpios),
	GPIO_DT_SPEC_GET(DT_ALIAS(relay6), gpios),
	GPIO_DT_SPEC_GET(DT_ALIAS(relay7), gpios),
	GPIO_DT_SPEC_GET(DT_ALIAS(relay8), gpios),
};

static const struct device *const strip_dev = DEVICE_DT_GET(DT_ALIAS(led_strip));

static uint16_t http_port = 80;
HTTP_SERVICE_DEFINE(web_service, NULL, &http_port, CONFIG_HTTP_SERVER_MAX_CLIENTS,
		    10, NULL, NULL, NULL);

static struct http_resource_detail_static index_html_detail = {
	.common = {
		.type = HTTP_RESOURCE_TYPE_STATIC,
		.bitmask_of_supported_http_methods = BIT(HTTP_GET),
		.content_encoding = "gzip",
		.content_type = "text/html",
	},
	.static_data = index_html_gz,
	.static_data_len = sizeof(index_html_gz),
};

static struct http_resource_detail_static app_js_detail = {
	.common = {
		.type = HTTP_RESOURCE_TYPE_STATIC,
		.bitmask_of_supported_http_methods = BIT(HTTP_GET),
		.content_encoding = "gzip",
		.content_type = "text/javascript",
	},
	.static_data = app_js_gz,
	.static_data_len = sizeof(app_js_gz),
};

static struct http_resource_detail_static style_css_detail = {
	.common = {
		.type = HTTP_RESOURCE_TYPE_STATIC,
		.bitmask_of_supported_http_methods = BIT(HTTP_GET),
		.content_encoding = "gzip",
		.content_type = "text/css",
	},
	.static_data = style_css_gz,
	.static_data_len = sizeof(style_css_gz),
};

static struct http_resource_detail_static info_detail = {
	.common = {
		.type = HTTP_RESOURCE_TYPE_STATIC,
		.bitmask_of_supported_http_methods = BIT(HTTP_GET),
		.content_type = "application/json",
	},
	.static_data = info_json,
	.static_data_len = sizeof(info_json) - 1,
};

static struct http_resource_detail_dynamic relay_detail = {
	.common = {
		.type = HTTP_RESOURCE_TYPE_DYNAMIC,
		.bitmask_of_supported_http_methods = BIT(HTTP_GET) | BIT(HTTP_POST),
	},
	.cb = web_app_relay_handler,
};

static struct http_resource_detail_dynamic rgb_detail = {
	.common = {
		.type = HTTP_RESOURCE_TYPE_DYNAMIC,
		.bitmask_of_supported_http_methods = BIT(HTTP_GET) | BIT(HTTP_POST),
	},
	.cb = web_app_rgb_handler,
};

static struct http_resource_detail_dynamic buzzer_detail = {
	.common = {
		.type = HTTP_RESOURCE_TYPE_DYNAMIC,
		.bitmask_of_supported_http_methods = BIT(HTTP_GET) | BIT(HTTP_POST),
	},
	.cb = web_app_buzzer_handler,
};

static struct http_resource_detail_dynamic files_detail = {
	.common = {
		.type = HTTP_RESOURCE_TYPE_DYNAMIC,
		.bitmask_of_supported_http_methods = BIT(HTTP_GET),
	},
	.cb = web_app_files_handler,
};

static struct http_resource_detail_dynamic download_detail = {
	.common = {
		.type = HTTP_RESOURCE_TYPE_DYNAMIC,
		.bitmask_of_supported_http_methods = BIT(HTTP_GET),
	},
	.cb = web_app_download_handler,
};

static struct http_resource_detail_dynamic delete_detail = {
	.common = {
		.type = HTTP_RESOURCE_TYPE_DYNAMIC,
		.bitmask_of_supported_http_methods = BIT(HTTP_POST),
	},
	.cb = web_app_delete_handler,
};

static struct http_resource_detail_dynamic upload_detail = {
	.common = {
		.type = HTTP_RESOURCE_TYPE_DYNAMIC,
		.bitmask_of_supported_http_methods = BIT(HTTP_POST),
	},
	.cb = web_app_upload_handler,
};

static struct http_resource_detail_dynamic time_detail = {
	.common = {
		.type = HTTP_RESOURCE_TYPE_DYNAMIC,
		.bitmask_of_supported_http_methods = BIT(HTTP_GET),
	},
	.cb = web_app_time_handler,
};

HTTP_RESOURCE_DEFINE(index_html_resource, web_service, "/",           &index_html_detail);
HTTP_RESOURCE_DEFINE(app_js_resource,     web_service, "/app.js",     &app_js_detail);
HTTP_RESOURCE_DEFINE(style_css_resource,  web_service, "/style.css",  &style_css_detail);
HTTP_RESOURCE_DEFINE(info_resource,       web_service, "/api/info",   &info_detail);
HTTP_RESOURCE_DEFINE(time_resource,       web_service, "/api/time",   &time_detail);
HTTP_RESOURCE_DEFINE(relay_resource,      web_service, "/api/relays", &relay_detail);
HTTP_RESOURCE_DEFINE(rgb_resource,        web_service, "/api/rgb",    &rgb_detail);
HTTP_RESOURCE_DEFINE(buzzer_resource,     web_service, "/api/buzzer", &buzzer_detail);
HTTP_RESOURCE_DEFINE(files_resource,      web_service, "/api/files",  &files_detail);
HTTP_RESOURCE_DEFINE(download_resource,   web_service, "/download/*", &download_detail);
HTTP_RESOURCE_DEFINE(delete_resource,     web_service, "/api/delete", &delete_detail);
HTTP_RESOURCE_DEFINE(upload_resource,     web_service, "/api/upload", &upload_detail);

HTTP_SERVER_CONTENT_TYPE(json, "application/json");
HTTP_SERVER_CONTENT_TYPE(txt,  "text/plain");
HTTP_SERVER_CONTENT_TYPE(csv,  "text/csv");
HTTP_SERVER_CONTENT_TYPE(bin,  "application/octet-stream");

static int web_app_build_relays_json(char *buf, size_t buf_size)
{
	int i;
	int off;

	off = snprintf(buf, buf_size, "{\"relays\":[");
	for (i = 0; i < NUM_RELAYS; i++) {
		off += snprintf(buf + off, buf_size - off, "%s%s",
				i > 0 ? "," : "",
				relay_state[i] ? "true" : "false");
	}
	off += snprintf(buf + off, buf_size - off, "]}");
	return off;
}

static size_t web_app_url_decode(const char *src, size_t src_len,
				    char *dst, size_t dst_size)
{
	char hex[3];
	char *endp;
	unsigned long val;
	size_t di;
	size_t si;

	di = 0;
	for (si = 0; si < src_len && di < dst_size - 1; si++) {
		if (src[si] == '%' && si + 2 < src_len) {
			hex[0] = src[si + 1];
			hex[1] = src[si + 2];
			hex[2] = '\0';
			val = strtoul(hex, &endp, 16);
			if (endp == &hex[2]) {
				dst[di++] = (char)val;
				si += 2;
				continue;
			}
		}
		dst[di++] = (src[si] == '+') ? ' ' : src[si];
	}
	dst[di] = '\0';
	return di;
}

static bool web_app_validate_rtttl(const char *str)
{
	const char *first_colon;
	const char *notes;
	const char *second_colon;

	first_colon = strchr(str, ':');
	if (!first_colon || first_colon == str) {
		return false;
	}
	second_colon = strchr(first_colon + 1, ':');
	if (!second_colon) {
		return false;
	}
	notes = second_colon + 1;
	while (*notes == ' ') {
		notes++;
	}
	return *notes != '\0';
}

static const char *web_app_content_type_from_path(const char *path)
{
	char ext[8];
	const char *dot;
	size_t i;

	dot = strrchr(path, '.');
	if (!dot || strlen(dot + 1) >= sizeof(ext)) {
		return "application/octet-stream";
	}
	dot++;
	for (i = 0; dot[i] && i < sizeof(ext) - 1; i++) {
		ext[i] = (dot[i] >= 'A' && dot[i] <= 'Z') ? dot[i] + 32 : dot[i];
	}
	ext[i] = '\0';

	if (strcmp(ext, "jpg")  == 0 || strcmp(ext, "jpeg") == 0) return "image/jpeg";
	if (strcmp(ext, "png")  == 0) return "image/png";
	if (strcmp(ext, "gif")  == 0) return "image/gif";
	if (strcmp(ext, "bmp")  == 0) return "image/bmp";
	if (strcmp(ext, "svg")  == 0) return "image/svg+xml";
	if (strcmp(ext, "pdf")  == 0) return "application/pdf";
	if (strcmp(ext, "txt")  == 0 || strcmp(ext, "log")  == 0) return "text/plain";
	if (strcmp(ext, "json") == 0) return "application/json";
	if (strcmp(ext, "csv")  == 0) return "text/csv";
	if (strcmp(ext, "html") == 0 || strcmp(ext, "htm")  == 0) return "text/html";
	if (strcmp(ext, "css")  == 0) return "text/css";
	if (strcmp(ext, "js")   == 0) return "application/javascript";
	if (strcmp(ext, "xml")  == 0) return "application/xml";
	return "application/octet-stream";
}

static const uint8_t *web_app_find_header_end(const uint8_t *data, size_t len)
{
	size_t i;

	for (i = 0; i + 3 < len; i++) {
		if (data[i] == '\r' && data[i + 1] == '\n' &&
		    data[i + 2] == '\r' && data[i + 3] == '\n') {
			return &data[i + 4];
		}
	}
	return NULL;
}

static bool web_app_extract_filename(const uint8_t *header_start, size_t header_len,
					char *fname, size_t fname_size)
{
	const char *end;
	const char *fn;

	fn = memmem(header_start, header_len, "filename=\"", 10);
	if (!fn) {
		return false;
	}
	fn += 10;
	end = memchr(fn, '"', header_len - (fn - (const char *)header_start));
	if (!end || (size_t)(end - fn) >= fname_size) {
		return false;
	}
	memcpy(fname, fn, end - fn);
	fname[end - fn] = '\0';
	return true;
}

static bool web_app_extract_dir(const uint8_t *data, size_t len,
				   char *dir, size_t dir_size)
{
	const char *name_dir;
	const char *val_end;
	const uint8_t *val_start;
	size_t vlen;

	/* Format: Content-Disposition: form-data; name="dir"\r\n\r\nVALUE\r\n */
	name_dir = memmem(data, len, "name=\"dir\"", 10);
	if (!name_dir) {
		return false;
	}
	val_start = web_app_find_header_end((const uint8_t *)name_dir,
					       len - (name_dir - (const char *)data));
	if (!val_start) {
		return false;
	}
	val_end = memchr(val_start, '\r', len - (val_start - data));
	if (!val_end) {
		return false;
	}
	vlen = val_end - (const char *)val_start;
	if (vlen >= dir_size) {
		vlen = dir_size - 1;
	}
	memcpy(dir, val_start, vlen);
	dir[vlen] = '\0';
	return true;
}

static int web_app_relay_handler(struct http_client_ctx *client,
				    enum http_transaction_status status,
				    const struct http_request_ctx *request_ctx,
				    struct http_response_ctx *response_ctx,
				    void *user_data)
{
	static char resp_buf[128];
	int idx;
	int len;
	int ret;
	struct relay_cmd cmd;

	if (status == HTTP_SERVER_TRANSACTION_ABORTED) {
		return 0;
	}

	if (client->method == HTTP_GET) {
		if (status == HTTP_SERVER_REQUEST_DATA_FINAL) {
			len = web_app_build_relays_json(resp_buf, sizeof(resp_buf));
			SET_JSON_RESPONSE(response_ctx, resp_buf, len);
		}
		return 0;
	}

	/* POST: skip calls without body (e.g. FINAL fired on headers with data_len=0) */
	if (status != HTTP_SERVER_REQUEST_DATA_FINAL || request_ctx->data_len == 0) {
		return 0;
	}

	cmd = (struct relay_cmd){0};
	LOG_DBG("relay POST body: '%.*s'", (int)request_ctx->data_len, request_ctx->data);
	ret = json_obj_parse((char *)request_ctx->data, request_ctx->data_len,
			     relay_cmd_descr, ARRAY_SIZE(relay_cmd_descr), &cmd);
	k_sem_take(&relay_lock, K_FOREVER);
	if (ret > 0 && cmd.ch >= 1 && cmd.ch <= NUM_RELAYS) {
		idx = cmd.ch - 1;
		relay_state[idx] = cmd.state;
		ret = gpio_pin_set_dt(&relays[idx], relay_state[idx] ? 1 : 0);
		LOG_INF("Relay CH%d (idx=%d) %s (gpio=%d)", cmd.ch, idx,
			relay_state[idx] ? "ON" : "OFF", ret);
	} else {
		LOG_WRN("relay JSON parse failed: ret=%d ch=%d", ret, cmd.ch);
	}
	len = web_app_build_relays_json(resp_buf, sizeof(resp_buf));
	k_sem_give(&relay_lock);
	SET_JSON_RESPONSE(response_ctx, resp_buf, len);
	return 0;
}

static int web_app_rgb_handler(struct http_client_ctx *client,
				  enum http_transaction_status status,
				  const struct http_request_ctx *request_ctx,
				  struct http_response_ctx *response_ctx,
				  void *user_data)
{
	static char resp_buf[48];
	int len;
	int ret;
	struct rgb_cmd cmd;

	if (status == HTTP_SERVER_TRANSACTION_ABORTED) {
		return 0;
	}

	if (client->method == HTTP_GET) {
		if (status == HTTP_SERVER_REQUEST_DATA_FINAL) {
			len = snprintf(resp_buf, sizeof(resp_buf),
				       "{\"r\":%u,\"g\":%u,\"b\":%u}",
				       rgb_pixel.r, rgb_pixel.g, rgb_pixel.b);
			SET_JSON_RESPONSE(response_ctx, resp_buf, len);
		}
		return 0;
	}

	/* POST: skip calls without body (e.g. FINAL fired on headers with data_len=0) */
	if (status != HTTP_SERVER_REQUEST_DATA_FINAL || request_ctx->data_len == 0) {
		return 0;
	}

	cmd = (struct rgb_cmd){0};
	ret = json_obj_parse((char *)request_ctx->data, request_ctx->data_len,
			     rgb_cmd_descr, ARRAY_SIZE(rgb_cmd_descr), &cmd);
	k_sem_take(&rgb_lock, K_FOREVER);
	if (ret > 0) {
		rgb_pixel.r = (uint8_t)(cmd.r & 0xFF);
		rgb_pixel.g = (uint8_t)(cmd.g & 0xFF);
		rgb_pixel.b = (uint8_t)(cmd.b & 0xFF);
		led_strip_update_rgb(strip_dev, &rgb_pixel, 1);
		LOG_INF("RGB set to (%u, %u, %u)", rgb_pixel.r, rgb_pixel.g, rgb_pixel.b);
	}
	len = snprintf(resp_buf, sizeof(resp_buf), "{\"r\":%u,\"g\":%u,\"b\":%u}",
		       rgb_pixel.r, rgb_pixel.g, rgb_pixel.b);
	k_sem_give(&rgb_lock);
	SET_JSON_RESPONSE(response_ctx, resp_buf, len);
	return 0;
}

static int web_app_buzzer_handler(struct http_client_ctx *client,
				     enum http_transaction_status status,
				     const struct http_request_ctx *request_ctx,
				     struct http_response_ctx *response_ctx,
				     void *user_data)
{
	static char resp_buf[128];
	char body_buf[BUZZER_RTTTL_MAX_LEN + 64];
	int len;
	int ret;
	size_t rtttl_len;
	struct buzzer_cmd cmd;

	if (status == HTTP_SERVER_TRANSACTION_ABORTED) {
		return 0;
	}

	if (client->method == HTTP_GET) {
		if (status == HTTP_SERVER_REQUEST_DATA_FINAL) {
			len = snprintf(resp_buf, sizeof(resp_buf),
				       "{\"playing\":%s}",
				       ringtone_is_playing() ? "true" : "false");
			SET_JSON_RESPONSE(response_ctx, resp_buf, len);
		}
		return 0;
	}

	/* POST: skip calls without body (e.g. FINAL fired on headers with data_len=0) */
	if (status != HTTP_SERVER_REQUEST_DATA_FINAL || request_ctx->data_len == 0) {
		return 0;
	}

	if (request_ctx->data_len >= sizeof(body_buf)) {
		goto buzzer_error;
	}
	memcpy(body_buf, request_ctx->data, request_ctx->data_len);
	body_buf[request_ctx->data_len] = '\0';

	cmd = (struct buzzer_cmd){0};
	ret = json_obj_parse(body_buf, request_ctx->data_len, buzzer_cmd_descr,
			     ARRAY_SIZE(buzzer_cmd_descr), &cmd);
	if (ret < 0) {
		goto buzzer_error;
	}

	if (cmd.action && strcmp(cmd.action, "stop") == 0) {
		ringtone_stop();
		len = snprintf(resp_buf, sizeof(resp_buf), "{\"ok\":true,\"playing\":false}");
		SET_JSON_RESPONSE(response_ctx, resp_buf, len);
		return 0;
	}

	if (!cmd.rtttl || cmd.rtttl[0] == '\0') {
		goto buzzer_error;
	}

	rtttl_len = strlen(cmd.rtttl);
	if (rtttl_len > BUZZER_RTTTL_MAX_LEN) {
		goto buzzer_error;
	}

	k_sem_take(&buzzer_lock, K_FOREVER);
	memcpy(buzzer_rtttl_buf, cmd.rtttl, rtttl_len + 1);
	k_sem_give(&buzzer_lock);

	if (!web_app_validate_rtttl(buzzer_rtttl_buf)) {
		len = snprintf(resp_buf, sizeof(resp_buf),
			       "{\"ok\":false,\"error\":\"Invalid RTTTL format. "
			       "Expected: Name:d=N,o=N,b=N:notes\"}");
		SET_JSON_RESPONSE(response_ctx, resp_buf, len);
		response_ctx->status = HTTP_400_BAD_REQUEST;
		return 0;
	}

	ret = ringtone_play_custom(buzzer_rtttl_buf);
	if (ret) {
		len = snprintf(resp_buf, sizeof(resp_buf),
			       "{\"ok\":false,\"error\":\"Buzzer error: %d\"}", ret);
		SET_JSON_RESPONSE(response_ctx, resp_buf, len);
		return 0;
	}

	LOG_INF("Buzzer playing RTTTL via web");
	len = snprintf(resp_buf, sizeof(resp_buf), "{\"ok\":true,\"playing\":true}");
	SET_JSON_RESPONSE(response_ctx, resp_buf, len);
	return 0;

buzzer_error:
	len = snprintf(resp_buf, sizeof(resp_buf),
		       "{\"ok\":false,\"error\":\"Invalid request\"}");
	SET_JSON_RESPONSE(response_ctx, resp_buf, len);
	response_ctx->status = HTTP_400_BAD_REQUEST;
	return 0;
}

static int web_app_files_handler(struct http_client_ctx *client,
				    enum http_transaction_status status,
				    const struct http_request_ctx *request_ctx,
				    struct http_response_ctx *response_ctx,
				    void *user_data)
{
	static char resp_buf[FILES_RESP_BUF_SIZE];
	bool first;
	char dir_path[128];
	int needed;
	int off;
	int ret;
	size_t len;
	const char *dir_param;
	const char *end;
	const char *q;
	const char *type_str;
	const char *url;
	struct fs_dir_t dirp;
	struct fs_dirent entry;
	struct fs_statvfs stat;
	uint32_t total_bytes = 0;
	uint32_t used_bytes = 0;
	uint32_t used_pct = 0;

	if (status != HTTP_SERVER_REQUEST_DATA_FINAL) {
		return 0;
	}

	strncpy(dir_path, "/lfs1", sizeof(dir_path));
	url = client->url_buffer;
	q = strchr(url, '?');

	if (q) {
		dir_param = strstr(q, "dir=");
		if (dir_param) {
			dir_param += 4;
			end = strchr(dir_param, '&');
			len = end ? (size_t)(end - dir_param) : strlen(dir_param);
			web_app_url_decode(dir_param, len, dir_path, sizeof(dir_path));
		}
	}

	if (fs_statvfs(dir_path, &stat) == 0) {
		uint64_t total64 = (uint64_t)stat.f_frsize * stat.f_blocks;
		uint64_t free64 = (uint64_t)stat.f_frsize * stat.f_bfree;
		uint64_t used64 = total64 - free64;

		total_bytes = (uint32_t)total64;
		used_bytes = (uint32_t)used64;
		if (total64 > 0) {
			used_pct = (uint32_t)((used64 * 100) / total64);
		}
	}

	off = snprintf(resp_buf, sizeof(resp_buf),
		"{\"dir\":\"%s\",\"total_bytes\":%u,\"used_bytes\":%u,\"used_pct\":%u,\"files\":[",
		dir_path, total_bytes, used_bytes, used_pct);

	fs_dir_t_init(&dirp);
	ret = fs_opendir(&dirp, dir_path);
	if (ret < 0) {
		off = snprintf(resp_buf, sizeof(resp_buf),
			"{\"dir\":\"%s\",\"total_bytes\":%u,\"used_bytes\":%u,\"used_pct\":%u,"
			"\"error\":%d,\"files\":[]}",
			dir_path, total_bytes, used_bytes, used_pct, ret);
		response_ctx->body = (uint8_t *)resp_buf;
		response_ctx->body_len = off;
		response_ctx->final_chunk = true;
		return 0;
	}

	first = true;
	while (fs_readdir(&dirp, &entry) == 0 && entry.name[0] != '\0') {
		type_str = (entry.type == FS_DIR_ENTRY_DIR) ? "dir" : "file";
		needed = snprintf(NULL, 0,
				  "%s{\"name\":\"%s\",\"size\":%zu,\"type\":\"%s\"}",
				  first ? "" : ",", entry.name, entry.size, type_str);
		if (off + needed + 3 > (int)sizeof(resp_buf)) {
			break;
		}
		off += snprintf(resp_buf + off, sizeof(resp_buf) - off,
				"%s{\"name\":\"%s\",\"size\":%zu,\"type\":\"%s\"}",
				first ? "" : ",", entry.name, entry.size, type_str);
		first = false;
	}

	fs_closedir(&dirp);
	off += snprintf(resp_buf + off, sizeof(resp_buf) - off, "]}");
	response_ctx->body = (uint8_t *)resp_buf;
	response_ctx->body_len = off;
	response_ctx->final_chunk = true;

	return 0;
}

static int web_app_download_handler(struct http_client_ctx *client,
				       enum http_transaction_status status,
				       const struct http_request_ctx *request_ctx,
				       struct http_response_ctx *response_ctx,
				       void *user_data)
{
	static struct fs_file_t file;
	static bool file_open;
	static uint8_t chunk[DOWNLOAD_CHUNK_SIZE];
	static char fpath[160];
	static struct http_header download_headers[1];
	int ret;
	ssize_t bytes;
	const char *err;
	const char *rel_path;
	const char *url;
	char decoded[128];

	if (status == HTTP_SERVER_TRANSACTION_ABORTED) {
		if (file_open && download_owner == client) {
			fs_close(&file);
			file_open = false;
			download_owner = NULL;
			k_sem_give(&download_lock);
		}
		return 0;
	}

	/* New download or different client: only open after request is fully received */
	if (!(file_open && download_owner == client)) {
		if (status != HTTP_SERVER_REQUEST_DATA_FINAL) {
			return 0;
		}
		k_sem_take(&download_lock, K_FOREVER);
		download_owner = client;

		url = client->url_buffer;
		rel_path = url + 9; /* skip "/download" prefix */
		web_app_url_decode(rel_path, strlen(rel_path), decoded, sizeof(decoded));
		snprintf(fpath, sizeof(fpath), "/lfs1%s", decoded);

		fs_file_t_init(&file);
		ret = fs_open(&file, fpath, FS_O_READ);
		if (ret < 0) {
			LOG_ERR("Download: failed to open %s: %d", fpath, ret);
			download_owner = NULL;
			k_sem_give(&download_lock);
			err = "{\"error\":\"file not found\"}";
			response_ctx->body = (const uint8_t *)err;
			response_ctx->body_len = strlen(err);
			response_ctx->final_chunk = true;
			response_ctx->status = HTTP_404_NOT_FOUND;
			return 0;
		}
		file_open = true;

		download_headers[0].name = "Content-Type";
		download_headers[0].value = web_app_content_type_from_path(fpath);
		response_ctx->headers = download_headers;
		response_ctx->header_count = 1;
	}

	bytes = fs_read(&file, chunk, sizeof(chunk));
	if (bytes <= 0) {
		fs_close(&file);
		file_open = false;
		download_owner = NULL;
		k_sem_give(&download_lock);
		response_ctx->body = NULL;
		response_ctx->body_len = 0;
		response_ctx->final_chunk = true;
		return 0;
	}

	response_ctx->body = chunk;
	response_ctx->body_len = bytes;
	response_ctx->final_chunk = (bytes < (ssize_t)sizeof(chunk));
	if (response_ctx->final_chunk) {
		fs_close(&file);
		file_open = false;
		download_owner = NULL;
		k_sem_give(&download_lock);
	}

	return 0;
}

static int web_app_delete_handler(struct http_client_ctx *client,
				     enum http_transaction_status status,
				     const struct http_request_ctx *request_ctx,
				     struct http_response_ctx *response_ctx,
				     void *user_data)
{
	static char resp_buf[128];
	char body_buf[256];
	int len;
	int ret;
	struct delete_cmd cmd;

	if (status == HTTP_SERVER_TRANSACTION_ABORTED) {
		return 0;
	}

	/* POST: skip calls without body (e.g. FINAL fired on headers with data_len=0) */
	if (status != HTTP_SERVER_REQUEST_DATA_FINAL || request_ctx->data_len == 0) {
		return 0;
	}

	if (request_ctx->data_len >= sizeof(body_buf)) {
		goto delete_error;
	}
	memcpy(body_buf, request_ctx->data, request_ctx->data_len);
	body_buf[request_ctx->data_len] = '\0';

	cmd = (struct delete_cmd){0};
	ret = json_obj_parse(body_buf, request_ctx->data_len, delete_cmd_descr,
			     ARRAY_SIZE(delete_cmd_descr), &cmd);
	if (ret < 0 || !cmd.path) {
		goto delete_error;
	}

	if (strncmp(cmd.path, "/lfs1", 5) != 0) {
		goto delete_error;
	}

	ret = fs_unlink(cmd.path);
	if (ret < 0) {
		len = snprintf(resp_buf, sizeof(resp_buf),
			       "{\"ok\":false,\"error\":%d}", ret);
		SET_JSON_RESPONSE(response_ctx, resp_buf, len);
		return 0;
	}

	LOG_INF("Deleted: %s", cmd.path);
	len = snprintf(resp_buf, sizeof(resp_buf),
		       "{\"ok\":true,\"path\":\"%s\"}", cmd.path);
	SET_JSON_RESPONSE(response_ctx, resp_buf, len);
	return 0;

delete_error:
	len = snprintf(resp_buf, sizeof(resp_buf),
		       "{\"ok\":false,\"error\":\"invalid request\"}");
	SET_JSON_RESPONSE(response_ctx, resp_buf, len);
	response_ctx->status = HTTP_400_BAD_REQUEST;
	return 0;
}

static int web_app_upload_handler(struct http_client_ctx *client,
				     enum http_transaction_status status,
				     const struct http_request_ctx *request_ctx,
				     struct http_response_ctx *response_ctx,
				     void *user_data)
{
	static char resp_buf[128];
	bool header_found;
	char dir[128];
	char filename[64];
	int len;
	int path_len;
	int ret;
	size_t blen;
	size_t copy_len;
	size_t header_len;
	size_t remaining;
	const char *eol;
	const char *file_part;
	const uint8_t *body;
	struct fs_dirent dirent;

	if (status == HTTP_SERVER_TRANSACTION_ABORTED) {
		if (upload.file_open) {
			fs_close(&upload.file);
			upload.file_open = false;
		}
		upload.header_parsed = false;
		upload.buf_len = 0;
		return 0;
	}

	/* Only react to body chunks. Ignores TRANSACTION_COMPLETE that fires
	 * after the response is sent (Zephyr 4.4+).
	 */
	if (status != HTTP_SERVER_REQUEST_DATA_MORE &&
	    status != HTTP_SERVER_REQUEST_DATA_FINAL) {
		return 0;
	}

	copy_len = request_ctx->data_len;
	if (copy_len + upload.buf_len > sizeof(upload.buf)) {
		if (upload.file_open && upload.buf_len > 0) {
			if (fs_write(&upload.file, upload.buf, upload.buf_len) < 0) {
				LOG_ERR("Upload: write flush failed");
			}
			upload.buf_len = 0;
		}
		if (copy_len > sizeof(upload.buf)) {
			copy_len = sizeof(upload.buf);
		}
	}

	memcpy(upload.buf + upload.buf_len, request_ctx->data, copy_len);
	upload.buf_len += copy_len;

	if (upload.boundary_len == 0 && upload.buf_len > 4) {
		eol = memchr(upload.buf, '\r', upload.buf_len);
		if (eol) {
			blen = eol - (const char *)upload.buf;
			if (blen > 2 && blen < sizeof(upload.boundary)) {
				memcpy(upload.boundary, upload.buf, blen);
				upload.boundary[blen] = '\0';
				upload.boundary_len = blen;
			}
		}
	}

	if (!upload.header_parsed) {
		memset(filename, 0, sizeof(filename));
		strncpy(dir, "/lfs1", sizeof(dir));

		file_part = memmem(upload.buf, upload.buf_len, "filename=\"", 10);
		if (!file_part) {
			if (status == HTTP_SERVER_REQUEST_DATA_FINAL) {
				goto upload_error;
			}
			return 0;
		}

		body = web_app_find_header_end((const uint8_t *)file_part,
						  upload.buf_len -
						  (file_part - (char *)upload.buf));
		if (!body) {
			if (status == HTTP_SERVER_REQUEST_DATA_FINAL) {
				goto upload_error;
			}
			return 0;
		}

		header_found = web_app_extract_filename((const uint8_t *)file_part,
							   body - (const uint8_t *)file_part,
							   filename, sizeof(filename));
		if (!header_found) {
			goto upload_error;
		}

		web_app_extract_dir(upload.buf, upload.buf_len, dir, sizeof(dir));

		path_len = snprintf(upload.filepath, sizeof(upload.filepath),
				    "%s/%s", dir, filename);
		if (path_len < 0 || (size_t)path_len >= sizeof(upload.filepath)) {
			LOG_ERR("File path too long");
			goto upload_error;
		}

		fs_file_t_init(&upload.file);
		ret = fs_open(&upload.file, upload.filepath, FS_O_CREATE | FS_O_WRITE);
		if (ret < 0) {
			LOG_ERR("Failed to open %s: %d", upload.filepath, ret);
			goto upload_error;
		}
		upload.file_open = true;
		upload.header_parsed = true;

		header_len = body - upload.buf;
		remaining = upload.buf_len - header_len;
		if (remaining > 0) {
			if (fs_write(&upload.file, body, remaining) < 0) {
				LOG_ERR("Upload: initial write failed");
			}
		}
		upload.buf_len = 0;

	} else if (upload.file_open) {
		if (fs_write(&upload.file, upload.buf, upload.buf_len) < 0) {
			LOG_ERR("Upload: write failed");
		}
		upload.buf_len = 0;
	}

	if (status == HTTP_SERVER_REQUEST_DATA_FINAL) {
		if (upload.file_open) {
			/* Truncate trailing boundary: file data ends with \r\n--boundary-- */
			fs_close(&upload.file);
			upload.file_open = false;

			if (fs_stat(upload.filepath, &dirent) == 0 &&
			    dirent.size > upload.boundary_len + 6) {
				fs_open(&upload.file, upload.filepath, FS_O_WRITE);
				fs_truncate(&upload.file,
					    dirent.size - upload.boundary_len - 6);
				fs_close(&upload.file);
			}

			LOG_INF("Upload complete: %s", upload.filepath);
		}

		len = snprintf(resp_buf, sizeof(resp_buf),
			       "{\"ok\":true,\"file\":\"%s\"}", upload.filepath);
		response_ctx->body = (uint8_t *)resp_buf;
		response_ctx->body_len = len;
		response_ctx->final_chunk = true;

		upload.header_parsed = false;
		upload.boundary_len = 0;
		upload.buf_len = 0;
	}

	return 0;

upload_error:
	upload.header_parsed = false;
	upload.boundary_len = 0;
	upload.buf_len = 0;

	len = snprintf(resp_buf, sizeof(resp_buf), "{\"ok\":false,\"error\":\"parse_failed\"}");
	response_ctx->body = (uint8_t *)resp_buf;
	response_ctx->body_len = len;
	response_ctx->final_chunk = true;
	response_ctx->status = HTTP_400_BAD_REQUEST;
	return 0;
}

static int web_app_time_handler(struct http_client_ctx *client,
				    enum http_transaction_status status,
				    const struct http_request_ctx *request_ctx,
				    struct http_response_ctx *response_ctx,
				    void *user_data)
{
	static char resp_buf[64];
	struct rtc_time t;
	int len;

	if (status != HTTP_SERVER_REQUEST_DATA_FINAL) {
		return 0;
	}

	if (rtc_mgr_get_local(&t) != 0) {
		len = snprintf(resp_buf, sizeof(resp_buf), "{\"datetime\":\"--\"}");
	} else {
		len = snprintf(resp_buf, sizeof(resp_buf),
			"{\"datetime\":\"%04d-%02d-%02d %02d:%02d:%02d\"}",
			t.tm_year, t.tm_mon, t.tm_mday,
			t.tm_hour, t.tm_min, t.tm_sec);
	}
	SET_JSON_RESPONSE(response_ctx, resp_buf, len);
	return 0;
}

/* 8. Exported functions */

int web_app_start(void)
{
	char addr_str[NET_IPV4_ADDR_LEN];
	int i;
	int ret;
	struct net_if *iface;
	struct net_if_ipv4 *ipv4;

	for (i = 0; i < NUM_RELAYS; i++) {
		if (!gpio_is_ready_dt(&relays[i])) {
			LOG_ERR("Relay CH%d GPIO not ready", i + 1);
			return -ENODEV;
		}
		ret = gpio_pin_configure_dt(&relays[i], GPIO_OUTPUT_INACTIVE);
		if (ret < 0) {
			LOG_ERR("Failed to configure relay CH%d GPIO: %d", i + 1, ret);
			return ret;
		}
	}

	LOG_INF("8 relay GPIOs initialized");

	if (!device_is_ready(strip_dev)) {
		LOG_WRN("WS2812 LED strip not ready");
	} else {
		led_strip_update_rgb(strip_dev, &rgb_pixel, 1);
		LOG_INF("WS2812 RGB LED initialized");
	}

	ret = http_server_start();
	if (ret < 0) {
		LOG_ERR("Failed to start HTTP server: %d", ret);
		return ret;
	}

	strncpy(addr_str, "<no ip>", sizeof(addr_str));
	iface = net_if_get_default();
	if (iface && iface->config.ip.ipv4) {
		ipv4 = iface->config.ip.ipv4;
		for (i = 0; i < NET_IF_MAX_IPV4_ADDR; i++) {
			if (ipv4->unicast[i].ipv4.is_used) {
				net_addr_ntop(AF_INET,
					      &ipv4->unicast[i].ipv4.address.in_addr,
					      addr_str, sizeof(addr_str));
				break;
			}
		}
	}

	LOG_INF("Web server ready - http://%s:%u", addr_str, http_port);
	return 0;
}
