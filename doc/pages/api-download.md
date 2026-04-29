[← README](../../README.md) · [Índice de APIs](api.md)

# API: `/download/*`

Faz streaming de um arquivo do LittleFS de volta pro cliente HTTP. O
path no `*` é relativo ao mount point (`/lfs1`). Define `Content-Type`
automaticamente com base na extensão.

## Endpoint

| Método | URL                   | Body | Resposta |
|--------|-----------------------|------|----------|
| GET    | `/download/<arquivo>` | -    | Binário (chunked, com Content-Type apropriado) |

## Exemplo via `curl`

```bash
# Comando
$ curl -O http://embarcados.local/download/guzman.jpeg

# Resposta esperada
  % Total    % Received % Xferd  Average Speed   Time    Time     Time  Current
                                 Dload  Upload   Total   Spent    Left  Speed
100 21886  100 21886    0     0   142k      0 --:--:-- --:--:-- --:--:--  143k
```

Verificar o arquivo baixado:
```bash
# Comando
$ file guzman.jpeg

# Resposta esperada
guzman.jpeg: JPEG image data, JFIF standard 1.01, ...
```

## Dois caminhos para chamar este endpoint

O servidor HTTP do Zephyr trata todo cliente igual, é só HTTP. Esse
endpoint é disparado por dois tipos de cliente:

**1. Cliente externo** (`curl`, Postman, app mobile, outro firmware na
rede), bate direto na URL. É o caso do exemplo `curl` mostrado acima.

**2. Browser carregando a página servida pelo Pico W**, usuário abre
`http://embarcados.local/`, browser baixa `/`, `/style.css` e `/app.js`,
e a tabela renderizada por `app.js` mostra um link `<a download>` em
cada linha. Trigger: clique no link Download, **o browser trata
nativamente** (sem JS envolvido), faz GET na URL e abre o "Save As" com
o nome do arquivo. Lifecycle completo em [api.md](api.md).

> Há também um botão "View" na mesma linha que faz `fetch()` no mesmo
> endpoint via JS e mostra inline numa modal. Esse caso passa pelo JS,
> mas chega no mesmo handler.

**Os dois caminhos convergem** no `HTTP_RESOURCE_DEFINE` registrado em
[`web_app.c`](../../app/src/web_app.c), ponto único de entrada no
firmware. A seção abaixo descreve esse trajeto comum, do match da URL
até o handler devolver a resposta.

## Caminho da requisição

**1. URL `/download/...` casa com o resource wildcard** -
[web_app.c:304](../../app/src/web_app.c#L304):
```c
HTTP_RESOURCE_DEFINE(download_resource, web_service, "/download/*", &download_detail);
```

(O `*` é capturado pelo Zephyr quando `CONFIG_HTTP_SERVER_RESOURCE_WILDCARD=y`.)

**2. Handler em [web_app.c:761](../../app/src/web_app.c#L761)**, extrai
o path da URL (após `/download`), faz URL-decode, prepende `/lfs1` e
chama `fs_open`. Define o `Content-Type` baseado na extensão via
`web_app_content_type_from_path` (image/jpeg, application/pdf, etc.).
Lê em chunks de 1024 bytes via `fs_read`, devolvendo cada chunk
ao kernel HTTP server até EOF.

Sincronização: o handler usa um semáforo `download_lock` mais um
`download_owner` (ponteiro do client) pra serializar, só um download
ativo por vez, e diferentes clients aguardam.

**3. Cliente recebe** o stream completo, escreve em arquivo local.

---

[← Anterior: api-delete](api-delete.md) · [Índice de APIs](api.md) · [README](../../README.md)
