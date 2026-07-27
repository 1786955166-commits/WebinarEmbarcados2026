[← README](../../README.md) · [API index](api.md)

# API: `/download/*`

Streams a file from LittleFS back to the HTTP client. The path in the `*`
is relative to the mount point (`/lfs1`). It sets `Content-Type`
automatically based on the file extension.

## Endpoint

| Method | URL                | Body | Response |
|--------|--------------------|------|----------|
| GET    | `/download/<file>` | -    | Binary (chunked, with the appropriate Content-Type) |

## Example with `curl`

```bash
# Command
$ curl -O http://embarcados.local/download/guzman.jpeg

# Expected response
  % Total    % Received % Xferd  Average Speed   Time    Time     Time  Current
                                 Dload  Upload   Total   Spent    Left  Speed
100 21886  100 21886    0     0   142k      0 --:--:-- --:--:-- --:--:--  143k
```

Checking the downloaded file:
```bash
# Command
$ file guzman.jpeg

# Expected response
guzman.jpeg: JPEG image data, JFIF standard 1.01, ...
```

## Two ways to call this endpoint

Zephyr's HTTP server treats every client the same, it is only HTTP. This
endpoint is triggered by two kinds of client:

**1. External client** (`curl`, Postman, a mobile app, another firmware on
the network), hitting the URL directly. That is the `curl` example above.

**2. Browser loading the page served by the Pico W**, the user opens
`http://embarcados.local/`, the browser downloads `/`, `/style.css` and
`/app.js`, and the table rendered by `app.js` shows an `<a download>` link
on each row. Trigger: clicking the Download link, which **the browser
handles natively** (no JS involved), issuing a GET on the URL and opening
the "Save As" dialog with the file name. Full lifecycle in
[api.md](api.md).

> There is also a "View" button on the same row that does a `fetch()` on
> the same endpoint through JS and shows the content inline in a modal.
> That case goes through JS, but reaches the same handler.

**Both paths converge** on the `HTTP_RESOURCE_DEFINE` registered in
[`web_app.c`](../../app/src/web_app.c), the single entry point into the
firmware. The section below describes that common route, from the URL
match down to the handler returning the response.

## Request path

**1. The URL `/download/...` matches the wildcard resource** -
[web_app.c:304](../../app/src/web_app.c#L304):
```c
HTTP_RESOURCE_DEFINE(download_resource, web_service, "/download/*", &download_detail);
```

(The `*` is captured by Zephyr when `CONFIG_HTTP_SERVER_RESOURCE_WILDCARD=y`.)

**2. Handler at [web_app.c:761](../../app/src/web_app.c#L761)**, extracts
the path from the URL (after `/download`), URL-decodes it, prepends
`/lfs1` and calls `fs_open`. It sets the `Content-Type` based on the
extension through `web_app_content_type_from_path` (image/jpeg,
application/pdf, and so on). It reads in 1024-byte chunks with `fs_read`,
handing each chunk back to the HTTP server kernel until EOF.

Synchronization: the handler uses a `download_lock` semaphore plus a
`download_owner` (the client pointer) to serialize access, so only one
download is active at a time and other clients wait.

**3. The client receives** the full stream and writes it to a local file.

---

[← Previous: api-delete](api-delete.md) · [API index](api.md) · [README](../../README.md)
