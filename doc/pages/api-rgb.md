[← README](../../README.md) · [API index](api.md)

# API: `/api/rgb`

Controls the color of the addressable WS2812 LED (GP13) through the PIO
and Zephyr's `led_strip` driver.

## Endpoint

| Method | URL        | Body                          | Response |
|--------|------------|-------------------------------|----------|
| GET    | `/api/rgb` | -                             | `{"r":N,"g":N,"b":N}` |
| POST   | `/api/rgb` | `{"r":0-255,"g":...,"b":...}` | Applied color |

## Examples with `curl`

Saturated red:
```bash
# Command
$ curl -X POST http://embarcados.local/api/rgb \
    -H "Content-Type: application/json" \
    -d '{"r":255,"g":0,"b":0}'

# Expected response
{"r":255,"g":0,"b":0}
```

Turn it off:
```bash
# Command
$ curl -X POST http://embarcados.local/api/rgb -d '{"r":0,"g":0,"b":0}'

# Expected response
{"r":0,"g":0,"b":0}
```

## Two ways to call this endpoint

Zephyr's HTTP server treats every client the same, it is only HTTP. This
endpoint is triggered by two kinds of client:

**1. External client** (`curl`, Postman, a mobile app, another firmware on
the network), hitting the URL directly. That is the case of the `curl`
examples above.

**2. Browser loading the page served by the Pico W**, the user opens
`http://embarcados.local/`, the browser downloads `/`, `/style.css` and
`/app.js`, and `app.js` fires the GET/POST through `fetch()`. Trigger:
clicking the Apply (or OFF) button after adjusting the R/G/B sliders in
the "RGB LED" card. Full lifecycle in [api.md](api.md).

**Both paths converge** on the `HTTP_RESOURCE_DEFINE` registered in
[`web_app.c`](../../app/src/web_app.c), the single entry point into the
firmware. The section below describes that common route, from the URL
match down to the handler returning the response.

## Request path

**1. The URL `/api/rgb` matches the registered resource** -
[web_app.c:301](../../app/src/web_app.c#L301):
```c
HTTP_RESOURCE_DEFINE(rgb_resource, web_service, "/api/rgb", &rgb_detail);
```

**2. Handler at [web_app.c:522](../../app/src/web_app.c#L522)** -
parses the JSON, takes the `rgb_lock` semaphore, updates the `led_rgb`
struct and calls `led_strip_update_rgb(strip_dev, &rgb_pixel, 1)`, and the
driver pushes the bits out to the WS2812 through the PIO.

**3. The server sends** the JSON with the applied color.

---

[← Previous: api-relays](api-relays.md) · [API index](api.md) · [Next: api-buzzer →](api-buzzer.md)
