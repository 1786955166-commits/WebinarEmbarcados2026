[← README](../../README.md) · [API index](api.md)

# API: `/api/time`

Returns the current RTC date/time already converted to the local timezone
(UTC-3 / BRT, configured in
[`network_app.c`](../../app/src/network_app.c#L18)).

## Endpoint

| Method | URL         | Body | Response |
|--------|-------------|------|----------|
| GET    | `/api/time` | -    | `{"datetime":"YYYY-MM-DD HH:MM:SS"}` |

## Example with `curl`

```bash
# Command
$ curl http://embarcados.local/api/time

# Expected response
{"datetime":"2026-04-29 14:23:12"}
```

## Two ways to call this endpoint

Zephyr's HTTP server treats every client the same, it is only HTTP. This
endpoint is triggered by two kinds of client:

**1. External client** (`curl`, Postman, a mobile app, another firmware on
the network), hitting the URL directly. That is the `curl` example above.

**2. Browser loading the page served by the Pico W**, the user opens
`http://embarcados.local/`, the browser downloads `/`, `/style.css` and
`/app.js`, and `app.js` fires the GET through `fetch()`. Trigger:
auto-fetch on `DOMContentLoaded` + 1 s polling through `setInterval`.
Full lifecycle in [api.md](api.md).

**Both paths converge** on the `HTTP_RESOURCE_DEFINE` registered in
[`web_app.c`](../../app/src/web_app.c), the single entry point into the
firmware. The section below describes that common route, from the URL
match down to the handler returning the response.

## Request path

**1. The URL `/api/time` matches the registered resource** -
[web_app.c:299](../../app/src/web_app.c#L299):
```c
HTTP_RESOURCE_DEFINE(time_resource, web_service, "/api/time", &time_detail);
```

**2. Handler at [web_app.c:1082](../../app/src/web_app.c#L1082)** -
calls `rtc_mgr_get_local()` (UTC + the configured offset) and formats it:
```c
rtc_mgr_get_local(&t);
snprintf(resp_buf, sizeof(resp_buf),
    "{\"datetime\":\"%04d-%02d-%02d %02d:%02d:%02d\"}",
    t.tm_year, t.tm_mon, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);
```

**3. The server sends** the JSON to the client.

---

[← Previous: api-info](api-info.md) · [API index](api.md) · [Next: api-relays →](api-relays.md)
