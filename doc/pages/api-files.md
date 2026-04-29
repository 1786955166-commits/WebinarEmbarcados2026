[← README](../../README.md) · [Índice de APIs](api.md)

# API: `/api/files`

Lista o conteúdo de um diretório no LittleFS (mountado em `/lfs1`) e
retorna estatísticas de armazenamento (total / used / pct).

## Endpoint

| Método | URL                       | Body | Resposta |
|--------|---------------------------|------|----------|
| GET    | `/api/files`              | -    | Conteúdo de `/lfs1` |
| GET    | `/api/files?dir=/lfs1/x`  | -    | Conteúdo de subdiretório |

## Exemplo via `curl`

```bash
# Comando
$ curl http://embarcados.local/api/files

# Resposta esperada
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

## Dois caminhos para chamar este endpoint

O servidor HTTP do Zephyr trata todo cliente igual, é só HTTP. Esse
endpoint é disparado por dois tipos de cliente:

**1. Cliente externo** (`curl`, Postman, app mobile, outro firmware na
rede), bate direto na URL. É o caso do exemplo `curl` mostrado acima.

**2. Browser carregando a página servida pelo Pico W**, usuário abre
`http://embarcados.local/`, browser baixa `/`, `/style.css` e `/app.js`,
e o `app.js` dispara o GET via `fetch()`. Trigger: auto-fetch no
`DOMContentLoaded`, clique em Refresh, ou navegação por subpastas no
breadcrumb. Lifecycle completo em [api.md](api.md).

**Os dois caminhos convergem** no `HTTP_RESOURCE_DEFINE` registrado em
[`web_app.c`](../../app/src/web_app.c), ponto único de entrada no
firmware. A seção abaixo descreve esse trajeto comum, do match da URL
até o handler devolver a resposta.

## Caminho da requisição

**1. URL `/api/files` casa com o resource registrado** -
[web_app.c:303](../../app/src/web_app.c#L303):
```c
HTTP_RESOURCE_DEFINE(files_resource, web_service, "/api/files", &files_detail);
```

**2. Handler em [web_app.c:665](../../app/src/web_app.c#L665)** -
extrai o `?dir=...` da query string, chama `fs_statvfs(dir, &stat)`
para popular total/used/pct, depois `fs_opendir` + `fs_readdir` em loop
pra montar a lista de arquivos. Devolve JSON único com tudo.

**3. Servidor envia** o JSON ao cliente.

---

[← Anterior: api-buzzer](api-buzzer.md) · [Índice de APIs](api.md) · [Próximo: api-upload →](api-upload.md)
