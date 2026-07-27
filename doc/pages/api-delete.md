[← README](../../README.md) · [API index](api.md)

# API: `/api/delete`

Removes a file from LittleFS by absolute path. It only accepts paths that
start with `/lfs1` (a defense against path traversal).

## Endpoint

| Method | URL           | Body                            | Response |
|--------|---------------|---------------------------------|----------|
| POST   | `/api/delete` | `{"path":"/lfs1/.../file"}` | `{"ok":true,"path":"..."}` or an error |

## Example with `curl`

```bash
# Command
$ curl -X POST http://embarcados.local/api/delete \
    -H "Content-Type: application/json" \
    -d '{"path":"/lfs1/guzman.jpeg"}'

# Expected response
{"ok":true,"path":"/lfs1/guzman.jpeg"}
```

## Two ways to call this endpoint

Zephyr's HTTP server treats every client the same, it is only HTTP. This
endpoint is triggered by two kinds of client:

**1. External client** (`curl`, Postman, a mobile app, another firmware on
the network), hitting the URL directly. That is the `curl` example above.

**2. Browser loading the page served by the Pico W**, the user opens
`http://embarcados.local/`, the browser downloads `/`, `/style.css` and
`/app.js`, and `app.js` fires the POST through `fetch()`. Trigger:
clicking the Delete button on a file row in the table, with a `confirm()`
safety prompt before sending. Full lifecycle in [api.md](api.md).

**Both paths converge** on the `HTTP_RESOURCE_DEFINE` registered in
[`web_app.c`](../../app/src/web_app.c), the single entry point into the
firmware. The section below describes that common route, from the URL
match down to the handler returning the response.

## Request path

**1. The URL `/api/delete` matches the registered resource** -
[web_app.c:305](../../app/src/web_app.c#L305):
```c
HTTP_RESOURCE_DEFINE(delete_resource, web_service, "/api/delete", &delete_detail);
```

**2. Handler at [web_app.c:848](../../app/src/web_app.c#L848)**, copies
the body into a local buffer, parses `path` with `json_obj_parse`,
validates that it starts with `/lfs1` (rejecting arbitrary absolute paths),
and calls `fs_unlink(path)`.

**3. The server sends** `{"ok":true,"path":"..."}` to the client.

---

[← Previous: api-upload](api-upload.md) · [API index](api.md) · [Next: api-download →](api-download.md)
