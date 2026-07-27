[← README](../../README.md) · [Architecture](architecture.md)

# Firmware REST API

Detailed documentation of every HTTP endpoint exposed by the Pico W,
showing how a request flows from the `index.html` page down to the C
handler that drives the hardware (and the way back).

> For the architecture overview (two clients, STATIC vs DYNAMIC, how the
> `.gz.inc` files become resources in flash) with diagrams, see
> [`architecture.md`](architecture.md). This doc focuses on the REST
> contract of each individual endpoint.

## General principle

The firmware runs Zephyr's HTTP server (`CONFIG_HTTP_SERVER`) on port 80.
Every endpoint is a **REST contract**, and any HTTP client can call it
directly: the browser through `index.html`, `curl` from the command line,
Postman, a mobile app, another firmware on the network.

The page served at `/` is just **one client** of this API, the only
peculiarity being that the Pico W itself also serves that page, gzipped,
straight from flash.

## Page lifecycle (the entry point common to almost everything)

Every interaction that starts in the browser goes through this same
loading sequence:

```
1. User opens http://embarcados.local/
2. Browser GET /              → index_html_resource → index.html (gzip, from flash)
3. Browser GET /style.css     → style_css_resource  → style.css (gzip, from flash)
4. Browser GET /app.js        → app_js_resource     → app.js (gzip, from flash)
5. app.js runs → the DOMContentLoaded handler fires the initial calls
   (fetchInfo, fetchTime, fetchRelays, fetchRgb, fetchBuzzerStatus, fetchFiles)
   and registers event listeners on the buttons / sliders
```

From step 5 on, each endpoint has its own path. The docs below start the
trace at that point where the concrete call is fired, which may be one of
the initial automatic calls or a user interaction.

### Example with `curl`, fetching the home page

```bash
# Command
$ curl --compressed http://embarcados.local/

# Expected response
<!DOCTYPE html>
<html lang="pt-BR">
<head>
    <meta charset="UTF-8">
    <title>ZPHR-PICO-0001</title>
    <link rel="stylesheet" href="/style.css">
</head>
<body>
    ...
    <script src="app.js"></script>
</body>
</html>
```

`--compressed` makes curl negotiate gzip and decompress it, without it you
get the binary blob straight from flash. The `<link>` and `<script>` lines
are **the only thing** telling the browser to fetch the other 2 files.

### Example with `curl` following the route, reproducing what the browser does

The browser, when parsing the HTML above, automatically fires 2 more GETs.
You can replicate the same sequence by hand:

```bash
$ curl --compressed http://embarcados.local/         # GET /          → index.html
$ curl --compressed http://embarcados.local/style.css # GET /style.css → style.css
$ curl --compressed http://embarcados.local/app.js   # GET /app.js    → app.js
```

3 independent GETs, there is no magic. Each URL hits a distinct
`HTTP_RESOURCE_DEFINE` in [`web_app.c`](../../app/src/web_app.c#L295-L297):

```c
HTTP_RESOURCE_DEFINE(index_html_resource, web_service, "/",          &index_html_detail);
HTTP_RESOURCE_DEFINE(app_js_resource,     web_service, "/app.js",    &app_js_detail);
HTTP_RESOURCE_DEFINE(style_css_resource,  web_service, "/style.css", &style_css_detail);
```

Inspecting the headers confirms the content comes gzipped from flash:
```bash
# Command
$ curl -v http://embarcados.local/style.css 2>&1 | grep -i "content-"

# Expected response
< Content-Type: text/css
< Content-Encoding: gzip
< Transfer-Encoding: chunked
```

## Documented endpoints

| Doc | Endpoint | Triggered by |
|---|---|---|
| [api-info.md](api-info.md) | `/api/info` | Auto-fetch on `DOMContentLoaded` |
| [api-time.md](api-time.md) | `/api/time` | Auto-fetch + 1 s polling |
| [api-relays.md](api-relays.md) | `/api/relays` | Turn ON/OFF buttons on each channel |
| [api-rgb.md](api-rgb.md) | `/api/rgb` | RGB sliders + Apply/OFF buttons |
| [api-buzzer.md](api-buzzer.md) | `/api/buzzer` | Play/Stop buttons |
| [api-files.md](api-files.md) | `/api/files` | Refresh + folder navigation |
| [api-upload.md](api-upload.md) | `/api/upload` | `<input type="file">` + Upload button |
| [api-delete.md](api-delete.md) | `/api/delete` | Delete button on each row |
| [api-download.md](api-download.md) | `/download/*` | `<a download>` link (no JS) |

## Doc convention

Every file follows the same structure:

1. **Endpoint table**, method(s), URL, body, response
2. **Example with `curl`**, command + expected response
3. **Request path**, numbered step by step from the URL matching the
   `HTTP_RESOURCE_DEFINE` down to the C handler returning the response

---

[← README](../../README.md) · [Next: api-info →](api-info.md)
