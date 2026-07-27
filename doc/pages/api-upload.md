[← README](../../README.md) · [API index](api.md)

# API: `/api/upload`

Receives a file through a multipart POST and writes it to LittleFS, in the
directory given by the form's `dir` field.

## Endpoint

| Method | URL           | Body | Response |
|--------|---------------|------|----------|
| POST   | `/api/upload` | `multipart/form-data` with the fields `dir` (string) and `file` (binary) | `{"ok":true,"file":"/lfs1/.../file"}` |

## Example with `curl`

Uploading a photo to `/lfs1`:
```bash
# Command
$ curl -X POST http://embarcados.local/api/upload \
    -F "dir=/lfs1" \
    -F "file=@guzman.jpeg"

# Expected response
{"ok":true,"file":"/lfs1/guzman.jpeg"}
```

## Two ways to call this endpoint

Zephyr's HTTP server treats every client the same, it is only HTTP. This
endpoint is triggered by two kinds of client:

**1. External client** (`curl`, Postman, a mobile app, another firmware on
the network), hitting the URL directly. That is the `curl` example above.

**2. Browser loading the page served by the Pico W**, the user opens
`http://embarcados.local/`, the browser downloads `/`, `/style.css` and
`/app.js`, and `app.js` fires the POST through `fetch()` with a
`FormData`. Trigger: picking a file in the `<input type="file">` and
clicking Upload in the "Upload File" card. Full lifecycle in
[api.md](api.md).

**Both paths converge** on the `HTTP_RESOURCE_DEFINE` registered in
[`web_app.c`](../../app/src/web_app.c), the single entry point into the
firmware. The section below describes that common route, from the URL
match down to the handler returning the response.

## Request path

**1. The URL `/api/upload` matches the registered resource** -
[web_app.c:306](../../app/src/web_app.c#L306):
```c
HTTP_RESOURCE_DEFINE(upload_resource, web_service, "/api/upload", &upload_detail);
```

**2. Handler at [web_app.c:908](../../app/src/web_app.c#L908)**, receives
the body in chunks (a large POST does not fit in a single server buffer),
detects the multipart boundary, extracts the `name="dir"` and
`name="file"; filename="..."` headers, writes the bytes out with `fs_write`
as they arrive, and on the last chunk truncates the trailing boundary with
`fs_truncate`.

**3. The server sends** `{"ok":true,"file":"/lfs1/.../file"}` to the client.

---

[← Previous: api-files](api-files.md) · [API index](api.md) · [Next: api-delete →](api-delete.md)
