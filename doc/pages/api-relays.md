[← README](../../README.md) · [API index](api.md)

# API: `/api/relays`

Controls the 8 relays (CH1..CH8) of the Waveshare Pico-Relay-B module,
mapped to GP14..GP21.

## Endpoint

| Method | URL           | Body                          | Response |
|--------|---------------|-------------------------------|----------|
| GET    | `/api/relays` | -                             | State of the 8 channels |
| POST   | `/api/relays` | `{"ch":N,"state":true/false}` | Updated state of the 8 channels |

## Examples with `curl`

Turn channel 1 on:
```bash
# Command
$ curl -X POST http://embarcados.local/api/relays \
    -H "Content-Type: application/json" \
    -d '{"ch":1,"state":true}'

# Expected response
{"relays":[true,false,false,false,false,false,false,false]}
```

Turn channel 1 off:
```bash
# Command
$ curl -X POST http://embarcados.local/api/relays -d '{"ch":1,"state":false}'

# Expected response
{"relays":[false,false,false,false,false,false,false,false]}
```

Current state:
```bash
# Command
$ curl http://embarcados.local/api/relays

# Expected response
{"relays":[false,false,false,false,false,false,false,false]}
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
clicking the Turn ON/OFF buttons of each channel in the "Relay Control"
card. Full lifecycle in [api.md](api.md).

**Both paths converge** on the `HTTP_RESOURCE_DEFINE` registered in
[`web_app.c`](../../app/src/web_app.c), the single entry point into the
firmware. The section below describes that common route, from the URL
match down to the handler returning the response.

## Request path

**1. The URL `/api/relays` matches the registered resource** -
[web_app.c:300](../../app/src/web_app.c#L300):
```c
HTTP_RESOURCE_DEFINE(relay_resource, web_service, "/api/relays", &relay_detail);
```

**2. Handler at [web_app.c:473](../../app/src/web_app.c#L473)**, parses
the JSON, takes the `relay_lock` semaphore, calls
`gpio_pin_set_dt(&relays[idx], 1)` (Zephyr's generic GPIO driver) and
returns the updated state of the 8 channels.

**3. The server sends** the JSON to the client.

---

[← Previous: api-time](api-time.md) · [API index](api.md) · [Next: api-rgb →](api-rgb.md)
