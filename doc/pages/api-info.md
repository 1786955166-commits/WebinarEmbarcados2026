[← README](../../README.md) · [Índice de APIs](api.md)

# API: `/api/info`

Retorna metadados do firmware, versão (gerada do arquivo
[`app/VERSION`](../../app/VERSION)), autor, site e contato. Popula o
`info-bar` no topo da página.

## Endpoint

| Método | URL         | Body | Resposta |
|--------|-------------|------|----------|
| GET    | `/api/info` | -    | `{"firmware":"...","author":"...","site":"...","contact":"..."}` |

## Exemplo via `curl`

```bash
# Comando
$ curl http://embarcados.local/api/info

# Resposta esperada
{"firmware":"1.0.0","author":"Jorge Guzman","site":"gzm-emb.com","contact":"jorge@gzm-emb.com"}
```

## Dois caminhos para chamar este endpoint

O servidor HTTP do Zephyr trata todo cliente igual, é só HTTP. Esse
endpoint é disparado por dois tipos de cliente:

**1. Cliente externo** (`curl`, Postman, app mobile, outro firmware na
rede), bate direto na URL. É o caso do exemplo `curl` mostrado acima.

**2. Browser carregando a página servida pelo Pico W**, usuário abre
`http://embarcados.local/`, browser baixa `/`, `/style.css` e `/app.js`,
e o `app.js` dispara o GET via `fetch()`. Trigger: auto-fetch no
`DOMContentLoaded`. Lifecycle completo em [api.md](api.md).

**Os dois caminhos convergem** no `HTTP_RESOURCE_DEFINE` registrado em
[`web_app.c`](../../app/src/web_app.c), ponto único de entrada no
firmware. A seção abaixo descreve esse trajeto comum, do match da URL
até o handler devolver a resposta.

## Caminho da requisição

**1. URL `/api/info` casa com o resource registrado** -
[web_app.c:298](../../app/src/web_app.c#L298):
```c
HTTP_RESOURCE_DEFINE(info_resource, web_service, "/api/info", &info_detail);
```

**2. `info_detail` é um STATIC resource**, sem handler dinâmico, o JSON
é montado em compile time -
[web_app.c:165-169](../../app/src/web_app.c#L165-L169):
```c
static const char info_json[] =
    "{\"firmware\":\"" APP_VERSION_STRING "\","
    "\"author\":\""    WEB_INFO_AUTHOR     "\","
    /* ... */
```

**3. Kernel devolve o blob direto da flash**, sem CPU usado em formatação.

---

[Índice de APIs](api.md) · [Próximo: api-time →](api-time.md)
