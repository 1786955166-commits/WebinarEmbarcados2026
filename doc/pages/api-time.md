[← README](../../README.md) · [Índice de APIs](api.md)

# API: `/api/time`

Retorna a data/hora atual do RTC já convertida para o timezone local
(UTC-3 / BRT, configurado em
[`network_app.c`](../../app/src/network_app.c#L18)).

## Endpoint

| Método | URL         | Body | Resposta |
|--------|-------------|------|----------|
| GET    | `/api/time` | -    | `{"datetime":"YYYY-MM-DD HH:MM:SS"}` |

## Exemplo via `curl`

```bash
# Comando
$ curl http://embarcados.local/api/time

# Resposta esperada
{"datetime":"2026-04-29 14:23:12"}
```

## Dois caminhos para chamar este endpoint

O servidor HTTP do Zephyr trata todo cliente igual, é só HTTP. Esse
endpoint é disparado por dois tipos de cliente:

**1. Cliente externo** (`curl`, Postman, app mobile, outro firmware na
rede), bate direto na URL. É o caso do exemplo `curl` mostrado acima.

**2. Browser carregando a página servida pelo Pico W**, usuário abre
`http://embarcados.local/`, browser baixa `/`, `/style.css` e `/app.js`,
e o `app.js` dispara o GET via `fetch()`. Trigger: auto-fetch no
`DOMContentLoaded` + polling de 1 s via `setInterval`. Lifecycle
completo em [api.md](api.md).

**Os dois caminhos convergem** no `HTTP_RESOURCE_DEFINE` registrado em
[`web_app.c`](../../app/src/web_app.c), ponto único de entrada no
firmware. A seção abaixo descreve esse trajeto comum, do match da URL
até o handler devolver a resposta.

## Caminho da requisição

**1. URL `/api/time` casa com o resource registrado** -
[web_app.c:299](../../app/src/web_app.c#L299):
```c
HTTP_RESOURCE_DEFINE(time_resource, web_service, "/api/time", &time_detail);
```

**2. Handler em [web_app.c:1082](../../app/src/web_app.c#L1082)** -
chama `rtc_mgr_get_local()` (UTC + offset configurado) e formata:
```c
rtc_mgr_get_local(&t);
snprintf(resp_buf, sizeof(resp_buf),
    "{\"datetime\":\"%04d-%02d-%02d %02d:%02d:%02d\"}",
    t.tm_year, t.tm_mon, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);
```

**3. Servidor envia** o JSON ao cliente.

---

[← Anterior: api-info](api-info.md) · [Índice de APIs](api.md) · [Próximo: api-relays →](api-relays.md)
