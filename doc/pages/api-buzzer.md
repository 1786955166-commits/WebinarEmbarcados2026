[← README](../../README.md) · [API index](api.md)

# API: `/api/buzzer`

Plays and stops RTTTL ringtones on the passive buzzer (GP6) through PWM.
It uses the [`gzm/ringtone`](../../lib/ringtone/) library (a custom RTTTL
player).

## Endpoint

| Method | URL           | Body                              | Response |
|--------|---------------|-----------------------------------|----------|
| GET    | `/api/buzzer` | -                                 | `{"playing":true/false}` |
| POST   | `/api/buzzer` | `{"action":"play","rtttl":"..."}` | `{"ok":true,"playing":true}` |
| POST   | `/api/buzzer` | `{"action":"stop"}`               | `{"ok":true,"playing":false}` |

## Examples with `curl`

Play Mario:
```bash
# Command
$ curl -X POST http://embarcados.local/api/buzzer \
    -H "Content-Type: application/json" \
    -d '{"action":"play","rtttl":"Mario:d=4,o=5,b=100:16e6,16e6,32p,8e6,16c6,8e6,8g6,8p,8g"}'

# Expected response
{"ok":true,"playing":true}
```

Stop:
```bash
# Command
$ curl -X POST http://embarcados.local/api/buzzer -d '{"action":"stop"}'

# Expected response
{"ok":true,"playing":false}
```

Status:
```bash
# Command
$ curl http://embarcados.local/api/buzzer

# Expected response
{"playing":false}
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
clicking the Play button after pasting an RTTTL string into the textarea
(or picking a preset), or clicking Stop. Full lifecycle in
[api.md](api.md).

**Both paths converge** on the `HTTP_RESOURCE_DEFINE` registered in
[`web_app.c`](../../app/src/web_app.c), the single entry point into the
firmware. The section below describes that common route, from the URL
match down to the handler returning the response.

## Request path

**1. The URL `/api/buzzer` matches the registered resource** -
[web_app.c:302](../../app/src/web_app.c#L302):
```c
HTTP_RESOURCE_DEFINE(buzzer_resource, web_service, "/api/buzzer", &buzzer_detail);
```

**2. Handler at [web_app.c:570](../../app/src/web_app.c#L570)**, copies
the body into a local buffer (needed because `JSON_TOK_STRING` modifies it
in place), parses it, validates the RTTTL string and calls
`ringtone_play_custom(rtttl)`. The library spawns a dedicated thread that
plays the melody through PWM.

**3. The server sends** `{"ok":true,"playing":true}` to the client.

---

[← Previous: api-rgb](api-rgb.md) · [API index](api.md) · [Next: api-files →](api-files.md)
