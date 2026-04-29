[← README](../../README.md) · [Índice de APIs](api.md)

# API: `/api/buzzer`

Toca/para ringtones no formato RTTTL no buzzer passivo (GP6) via PWM.
Usa a lib [`gzm/ringtone`](../../lib/ringtone/) (player RTTTL custom).

## Endpoint

| Método | URL           | Body                              | Resposta |
|--------|---------------|-----------------------------------|----------|
| GET    | `/api/buzzer` | -                                 | `{"playing":true/false}` |
| POST   | `/api/buzzer` | `{"action":"play","rtttl":"..."}` | `{"ok":true,"playing":true}` |
| POST   | `/api/buzzer` | `{"action":"stop"}`               | `{"ok":true,"playing":false}` |

## Exemplos via `curl`

Tocar Mario:
```bash
# Comando
$ curl -X POST http://embarcados.local/api/buzzer \
    -H "Content-Type: application/json" \
    -d '{"action":"play","rtttl":"Mario:d=4,o=5,b=100:16e6,16e6,32p,8e6,16c6,8e6,8g6,8p,8g"}'

# Resposta esperada
{"ok":true,"playing":true}
```

Parar:
```bash
# Comando
$ curl -X POST http://embarcados.local/api/buzzer -d '{"action":"stop"}'

# Resposta esperada
{"ok":true,"playing":false}
```

Status:
```bash
# Comando
$ curl http://embarcados.local/api/buzzer

# Resposta esperada
{"playing":false}
```

## Dois caminhos para chamar este endpoint

O servidor HTTP do Zephyr trata todo cliente igual, é só HTTP. Esse
endpoint é disparado por dois tipos de cliente:

**1. Cliente externo** (`curl`, Postman, app mobile, outro firmware na
rede), bate direto na URL. É o caso dos exemplos `curl` mostrados acima.

**2. Browser carregando a página servida pelo Pico W**, usuário abre
`http://embarcados.local/`, browser baixa `/`, `/style.css` e `/app.js`,
e o `app.js` dispara o GET/POST via `fetch()`. Trigger: clique no botão
Play após colar um RTTTL no textarea (ou selecionar um preset),
ou clique em Stop. Lifecycle completo em [api.md](api.md).

**Os dois caminhos convergem** no `HTTP_RESOURCE_DEFINE` registrado em
[`web_app.c`](../../app/src/web_app.c), ponto único de entrada no
firmware. A seção abaixo descreve esse trajeto comum, do match da URL
até o handler devolver a resposta.

## Caminho da requisição

**1. URL `/api/buzzer` casa com o resource registrado** -
[web_app.c:302](../../app/src/web_app.c#L302):
```c
HTTP_RESOURCE_DEFINE(buzzer_resource, web_service, "/api/buzzer", &buzzer_detail);
```

**2. Handler em [web_app.c:570](../../app/src/web_app.c#L570)**, copia
o body pra um buffer local (necessário porque `JSON_TOK_STRING` modifica
in-place), parseia, valida o RTTTL e chama `ringtone_play_custom(rtttl)`.
A lib spawn uma thread dedicada que toca a melodia via PWM.

**3. Servidor envia** `{"ok":true,"playing":true}` ao cliente.

---

[← Anterior: api-rgb](api-rgb.md) · [Índice de APIs](api.md) · [Próximo: api-files →](api-files.md)
