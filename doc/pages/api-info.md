[← README](../../README.md) · [API index](api.md)

# API: `/api/info`

Returns firmware metadata: version (generated from the
[`app/VERSION`](../../app/VERSION) file), author, site and contact. It
populates the `info-bar` at the top of the page.

## Endpoint

| Method | URL         | Body | Response |
|--------|-------------|------|----------|
| GET    | `/api/info` | -    | `{"firmware":"...","author":"...","site":"...","contact":"..."}` |

## Example with `curl`

```bash
# Command
$ curl http://embarcados.local/api/info

# Expected response
{"firmware":"1.0.0","author":"Jorge Guzman","site":"gzm-emb.com","contact":"jorge@gzm-emb.com"}
```

## Two ways to call this endpoint

Zephyr's HTTP server treats every client the same, it is only HTTP. This
endpoint is triggered by two kinds of client:

**1. External client** (`curl`, Postman, a mobile app, another firmware on
the network), hitting the URL directly. That is the `curl` example above.

**2. Browser loading the page served by the Pico W**, the user opens
`http://embarcados.local/`, the browser downloads `/`, `/style.css` and
`/app.js`, and `app.js` fires the GET through `fetch()`. Trigger:
auto-fetch on `DOMContentLoaded`. Full lifecycle in [api.md](api.md).

**Both paths converge** on the `HTTP_RESOURCE_DEFINE` registered in
[`web_app.c`](../../app/src/web_app.c), the single entry point into the
firmware. The section below describes that common route, from the URL
match down to the handler returning the response.

## Request path

**1. The URL `/api/info` matches the registered resource** -
[web_app.c:298](../../app/src/web_app.c#L298):
```c
HTTP_RESOURCE_DEFINE(info_resource, web_service, "/api/info", &info_detail);
```

**2. `info_detail` is a STATIC resource**, with no dynamic handler, the
JSON is assembled at compile time -
[web_app.c:165-169](../../app/src/web_app.c#L165-L169):
```c
static const char info_json[] =
    "{\"firmware\":\"" APP_VERSION_STRING "\","
    "\"author\":\""    WEB_INFO_AUTHOR     "\","
    /* ... */
```

**3. The kernel returns the blob straight from flash**, with no CPU spent
on formatting.

---

[API index](api.md) · [Next: api-time →](api-time.md)
