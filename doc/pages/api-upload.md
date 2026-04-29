[← README](../../README.md) · [Índice de APIs](api.md)

# API: `/api/upload`

Recebe um arquivo via POST multipart e grava no LittleFS, no diretório
indicado pelo campo `dir` do form.

## Endpoint

| Método | URL           | Body | Resposta |
|--------|---------------|------|----------|
| POST   | `/api/upload` | `multipart/form-data` com campos `dir` (string) e `file` (binário) | `{"ok":true,"file":"/lfs1/.../arquivo"}` |

## Exemplo via `curl`

Upload de uma foto pro `/lfs1`:
```bash
# Comando
$ curl -X POST http://embarcados.local/api/upload \
    -F "dir=/lfs1" \
    -F "file=@guzman.jpeg"

# Resposta esperada
{"ok":true,"file":"/lfs1/guzman.jpeg"}
```

## Dois caminhos para chamar este endpoint

O servidor HTTP do Zephyr trata todo cliente igual, é só HTTP. Esse
endpoint é disparado por dois tipos de cliente:

**1. Cliente externo** (`curl`, Postman, app mobile, outro firmware na
rede), bate direto na URL. É o caso do exemplo `curl` mostrado acima.

**2. Browser carregando a página servida pelo Pico W**, usuário abre
`http://embarcados.local/`, browser baixa `/`, `/style.css` e `/app.js`,
e o `app.js` dispara o POST via `fetch()` com um `FormData`. Trigger:
seleção de arquivo no `<input type="file">` + clique em Upload no card
"Upload File". Lifecycle completo em [api.md](api.md).

**Os dois caminhos convergem** no `HTTP_RESOURCE_DEFINE` registrado em
[`web_app.c`](../../app/src/web_app.c), ponto único de entrada no
firmware. A seção abaixo descreve esse trajeto comum, do match da URL
até o handler devolver a resposta.

## Caminho da requisição

**1. URL `/api/upload` casa com o resource registrado** -
[web_app.c:306](../../app/src/web_app.c#L306):
```c
HTTP_RESOURCE_DEFINE(upload_resource, web_service, "/api/upload", &upload_detail);
```

**2. Handler em [web_app.c:908](../../app/src/web_app.c#L908)**, recebe
o body em chunks (POST grande não cabe num buffer único do servidor),
detecta o boundary do multipart, extrai os headers `name="dir"` e
`name="file"; filename="..."`, vai gravando os bytes em `fs_write`
conforme chegam, e no último chunk trunca o trailing boundary com
`fs_truncate`.

**3. Servidor envia** `{"ok":true,"file":"/lfs1/.../arquivo"}` ao cliente.

---

[← Anterior: api-files](api-files.md) · [Índice de APIs](api.md) · [Próximo: api-delete →](api-delete.md)
