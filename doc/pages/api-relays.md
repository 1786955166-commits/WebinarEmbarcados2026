[← README](../../README.md) · [Índice de APIs](api.md)

# API: `/api/relays`

Controla os 8 relés (CH1..CH8) do módulo Waveshare Pico-Relay-B,
mapeados em GP14..GP21.

## Endpoint

| Método | URL           | Body                          | Resposta |
|--------|---------------|-------------------------------|----------|
| GET    | `/api/relays` | -                             | Estado dos 8 canais |
| POST   | `/api/relays` | `{"ch":N,"state":true/false}` | Estado atualizado dos 8 canais |

## Exemplos via `curl`

Ligar canal 1:
```bash
# Comando
$ curl -X POST http://embarcados.local/api/relays \
    -H "Content-Type: application/json" \
    -d '{"ch":1,"state":true}'

# Resposta esperada
{"relays":[true,false,false,false,false,false,false,false]}
```

Desligar canal 1:
```bash
# Comando
$ curl -X POST http://embarcados.local/api/relays -d '{"ch":1,"state":false}'

# Resposta esperada
{"relays":[false,false,false,false,false,false,false,false]}
```

Estado atual:
```bash
# Comando
$ curl http://embarcados.local/api/relays

# Resposta esperada
{"relays":[false,false,false,false,false,false,false,false]}
```

## Dois caminhos para chamar este endpoint

O servidor HTTP do Zephyr trata todo cliente igual, é só HTTP. Esse
endpoint é disparado por dois tipos de cliente:

**1. Cliente externo** (`curl`, Postman, app mobile, outro firmware na
rede), bate direto na URL. É o caso dos exemplos `curl` mostrados acima.

**2. Browser carregando a página servida pelo Pico W**, usuário abre
`http://embarcados.local/`, browser baixa `/`, `/style.css` e `/app.js`,
e o `app.js` dispara o GET/POST via `fetch()`. Trigger: clique nos
botões Turn ON/OFF de cada canal no card "Relay Control". Lifecycle
completo em [api.md](api.md).

**Os dois caminhos convergem** no `HTTP_RESOURCE_DEFINE` registrado em
[`web_app.c`](../../app/src/web_app.c), ponto único de entrada no
firmware. A seção abaixo descreve esse trajeto comum, do match da URL
até o handler devolver a resposta.

## Caminho da requisição

**1. URL `/api/relays` casa com o resource registrado** -
[web_app.c:300](../../app/src/web_app.c#L300):
```c
HTTP_RESOURCE_DEFINE(relay_resource, web_service, "/api/relays", &relay_detail);
```

**2. Handler em [web_app.c:473](../../app/src/web_app.c#L473)**, parseia
o JSON, toma o semáforo `relay_lock`, chama `gpio_pin_set_dt(&relays[idx], 1)`
(driver GPIO genérico do Zephyr) e devolve o estado atualizado dos 8 canais.

**3. Servidor envia** o JSON ao cliente.

---

[← Anterior: api-time](api-time.md) · [Índice de APIs](api.md) · [Próximo: api-rgb →](api-rgb.md)
