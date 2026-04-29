[← README](../../README.md) · [Índice de APIs](api.md)

# Arquitetura do servidor HTTP

O firmware expõe um servidor HTTP único na porta 80, baseado no
`CONFIG_HTTP_SERVER` do Zephyr. Esse servidor atende dois tipos de
cliente que convergem no mesmo ponto e, depois disso, despacha para
dois tipos de resource (STATIC ou DYNAMIC) dependendo da URL.

## 1. Quem chama: dois clientes, um único handler

![Caminho da requisição: dois clientes, um único handler](../images/slide_http_two_paths.png)

**Caminho 1, via página web**, usuário abre `http://embarcados.local/`,
o browser faz `GET /`, `GET /style.css`, `GET /app.js`, recebe os
arquivos gzipados direto da flash, e o `app.js` rodando no browser
dispara `fetch('/api/info')`, `fetch('/api/relays')`, etc.

**Caminho 2, via API direta**, qualquer cliente HTTP (`curl`, Postman,
app mobile, outro firmware na rede) bate direto na URL, sem nunca
abrir o browser nem carregar a página.

A página servida em `/` é apenas **mais um cliente** desse API REST,
a única peculiaridade é que o próprio Pico W também serve essa página
da flash. Os dois caminhos chegam no mesmo `HTTP_RESOURCE_DEFINE` em
[`web_app.c`](../../app/src/web_app.c#L295-L306), o servidor não
diferencia, é só HTTP.

## 2. Para onde vai: STATIC vs DYNAMIC resource

Quando a URL casa com um `HTTP_RESOURCE_DEFINE`, o servidor decide
entre dois tipos de resource:

![Dispatch da URL: STATIC vs DYNAMIC](../images/slide_http_dispatch.png)

**STATIC RESOURCE**, o `detail` aponta para um byte array em flash, o
kernel só faz `memcpy` dos bytes pro socket TCP, sem CPU custom.
São 4 rotas:

| URL          | Array em flash       | Origem                      |
|--------------|----------------------|-----------------------------|
| `/`          | `index_html_gz[]`    | `index.html.gz.inc`         |
| `/style.css` | `style_css_gz[]`     | `style.css.gz.inc`          |
| `/app.js`    | `app_js_gz[]`        | `app.js.gz.inc`             |
| `/api/info`  | `info_json[]`        | string montada em compile time |

Os 3 primeiros vêm dos arquivos `.gz.inc`, gerados pelo CMake em build
time via `generate_inc_file_for_target`, que pega
[`app/www/*`](../../app/www/),
gzipa e produz um `.inc` com bytes em hex. O `.inc` é incluído no
[`web_app.c`](../../app/src/web_app.c#L153) virando array C que vai pra
flash junto com o resto do firmware.

**DYNAMIC RESOURCE**, o `detail` aponta para uma função callback que
o kernel chama a cada request. A função decide o que fazer: parsear
JSON, tomar lock, chamar driver, montar resposta. São 8 rotas:

| URL            | Handler em [`web_app.c`](../../app/src/web_app.c) |
|----------------|--------------------------------|
| `/api/time`    | `web_app_time_handler()`       |
| `/api/relays`  | `web_app_relay_handler()`      |
| `/api/rgb`     | `web_app_rgb_handler()`        |
| `/api/buzzer`  | `web_app_buzzer_handler()`     |
| `/api/files`   | `web_app_files_handler()`      |
| `/api/upload`  | `web_app_upload_handler()`     |
| `/api/delete`  | `web_app_delete_handler()`     |
| `/download/*`  | `web_app_download_handler()`   |

## 3. Quando usar STATIC vs DYNAMIC

A regra é: **se a resposta depende de algo que pode mudar entre boots
ou entre requests, é dynamic**. Senão, é static.

| Endpoint     | Por que é dynamic                                                     |
|--------------|-----------------------------------------------------------------------|
| `/api/time`  | RTC avança 1 s, valor muda continuamente                              |
| `/api/relays`| GET lê estado dos GPIOs · POST recebe JSON, aciona GPIO, devolve estado |
| `/api/rgb`   | GET lê cor atual · POST recebe RGB, aplica no WS2812                  |
| `/api/buzzer`| POST recebe RTTTL no body, dispara thread de PWM                      |
| `/api/files` | Lista LittleFS (muda quando upload/delete) + estatísticas total/used  |
| `/api/upload`| Recebe arquivo multipart em chunks, vai gravando                      |
| `/api/delete`| Recebe path no body, chama `fs_unlink`                                |
| `/download/*`| Lê arquivo do FS, faz stream em chunks de 1024 bytes                  |

Por contraste, `/api/info` é static (firmware version + autor são
constantes), assim como o HTML/CSS/JS.

## 4. O que o código declara

Os tipos no Zephyr deixam clara a escolha:

```c
// STATIC: tudo decidido em compile time
struct http_resource_detail_static info_detail = {
    .static_data     = info_json,
    .static_data_len = sizeof(info_json) - 1,
};

// DYNAMIC: callback chamado a cada request
struct http_resource_detail_dynamic relay_detail = {
    .cb          = web_app_relay_handler,
    .data_buffer = ...,
};

// Ambos são registrados pela mesma macro
HTTP_RESOURCE_DEFINE(info_resource,  web_service, "/api/info",   &info_detail);
HTTP_RESOURCE_DEFINE(relay_resource, web_service, "/api/relays", &relay_detail);
```

**Custo:** dynamic gasta CPU (parse JSON, lock, driver call) e RAM
(buffers). Static é praticamente grátis, só o kernel TCP enviando
bytes da flash.

---

[← README](../../README.md) · [Índice de APIs](api.md)
