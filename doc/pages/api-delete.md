[← README](../../README.md) · [Índice de APIs](api.md)

# API: `/api/delete`

Remove um arquivo do LittleFS por path absoluto. Aceita apenas paths
que começam com `/lfs1` (defesa contra path traversal).

## Endpoint

| Método | URL           | Body                            | Resposta |
|--------|---------------|---------------------------------|----------|
| POST   | `/api/delete` | `{"path":"/lfs1/.../arquivo"}` | `{"ok":true,"path":"..."}` ou erro |

## Exemplo via `curl`

```bash
# Comando
$ curl -X POST http://embarcados.local/api/delete \
    -H "Content-Type: application/json" \
    -d '{"path":"/lfs1/guzman.jpeg"}'

# Resposta esperada
{"ok":true,"path":"/lfs1/guzman.jpeg"}
```

## Dois caminhos para chamar este endpoint

O servidor HTTP do Zephyr trata todo cliente igual, é só HTTP. Esse
endpoint é disparado por dois tipos de cliente:

**1. Cliente externo** (`curl`, Postman, app mobile, outro firmware na
rede), bate direto na URL. É o caso do exemplo `curl` mostrado acima.

**2. Browser carregando a página servida pelo Pico W**, usuário abre
`http://embarcados.local/`, browser baixa `/`, `/style.css` e `/app.js`,
e o `app.js` dispara o POST via `fetch()`. Trigger: clique no botão
Delete na linha de um arquivo da tabela, com `confirm()` de segurança
antes de enviar. Lifecycle completo em [api.md](api.md).

**Os dois caminhos convergem** no `HTTP_RESOURCE_DEFINE` registrado em
[`web_app.c`](../../app/src/web_app.c), ponto único de entrada no
firmware. A seção abaixo descreve esse trajeto comum, do match da URL
até o handler devolver a resposta.

## Caminho da requisição

**1. URL `/api/delete` casa com o resource registrado** -
[web_app.c:305](../../app/src/web_app.c#L305):
```c
HTTP_RESOURCE_DEFINE(delete_resource, web_service, "/api/delete", &delete_detail);
```

**2. Handler em [web_app.c:848](../../app/src/web_app.c#L848)**, copia
o body pra buffer local, parseia `path` via `json_obj_parse`, valida
que começa com `/lfs1` (rejeita paths absolutos arbitrários), chama
`fs_unlink(path)`.

**3. Servidor envia** `{"ok":true,"path":"..."}` ao cliente.

---

[← Anterior: api-upload](api-upload.md) · [Índice de APIs](api.md) · [Próximo: api-download →](api-download.md)
