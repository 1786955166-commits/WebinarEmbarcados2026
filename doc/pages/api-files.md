[← README](../../README.md) · [API index](api.md)

# API: `/api/files`

Lists the contents of a directory in LittleFS (mounted at `/lfs1`) and
returns storage statistics (total / used / pct).

## Endpoint

| Method | URL                       | Body | Response |
|--------|---------------------------|------|----------|
| GET    | `/api/files`              | -    | Contents of `/lfs1` |
| GET    | `/api/files?dir=/lfs1/x`  | -    | Contents of a subdirectory |

## Example with `curl`

```bash
# Command
$ curl http://embarcados.local/api/files

# Expected response
{
  "dir":"/lfs1",
  "total_bytes":65536,
  "used_bytes":21888,
  "used_pct":33,
  "files":[
    {"name":"guzman.jpeg","size":21886,"type":"file"}
  ]
}
```

## Two ways to call this endpoint

Zephyr's HTTP server treats every client the same, it is only HTTP. This
endpoint is triggered by two kinds of client:

**1. External client** (`curl`, Postman, a mobile app, another firmware on
the network), hitting the URL directly. That is the `curl` example above.

**2. Browser loading the page served by the Pico W**, the user opens
`http://embarcados.local/`, the browser downloads `/`, `/style.css` and
`/app.js`, and `app.js` fires the GET through `fetch()`. Trigger:
auto-fetch on `DOMContentLoaded`, clicking Refresh, or navigating through
subfolders in the breadcrumb. Full lifecycle in [api.md](api.md).

**Both paths converge** on the `HTTP_RESOURCE_DEFINE` registered in
[`web_app.c`](../../app/src/web_app.c), the single entry point into the
firmware. The section below describes that common route, from the URL
match down to the handler returning the response.

## Request path

**1. The URL `/api/files` matches the registered resource** -
[web_app.c:303](../../app/src/web_app.c#L303):
```c
HTTP_RESOURCE_DEFINE(files_resource, web_service, "/api/files", &files_detail);
```

**2. Handler at [web_app.c:665](../../app/src/web_app.c#L665)** -
extracts `?dir=...` from the query string, calls `fs_statvfs(dir, &stat)`
to populate total/used/pct, then loops over `fs_opendir` + `fs_readdir` to
build the file list. It returns a single JSON with everything.

**3. The server sends** the JSON to the client.

---

[← Previous: api-buzzer](api-buzzer.md) · [API index](api.md) · [Next: api-upload →](api-upload.md)
