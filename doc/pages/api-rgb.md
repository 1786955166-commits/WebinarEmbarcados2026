[← README](../../README.md) · [Índice de APIs](api.md)

# API: `/api/rgb`

Controla a cor do LED WS2812 endereçável (GP13) via PIO + driver
`led_strip` da Zephyr.

## Endpoint

| Método | URL        | Body                          | Resposta |
|--------|------------|-------------------------------|----------|
| GET    | `/api/rgb` | -                             | `{"r":N,"g":N,"b":N}` |
| POST   | `/api/rgb` | `{"r":0-255,"g":...,"b":...}` | Cor aplicada |

## Exemplos via `curl`

Vermelho saturado:
```bash
# Comando
$ curl -X POST http://embarcados.local/api/rgb \
    -H "Content-Type: application/json" \
    -d '{"r":255,"g":0,"b":0}'

# Resposta esperada
{"r":255,"g":0,"b":0}
```

Apagar:
```bash
# Comando
$ curl -X POST http://embarcados.local/api/rgb -d '{"r":0,"g":0,"b":0}'

# Resposta esperada
{"r":0,"g":0,"b":0}
```

## Dois caminhos para chamar este endpoint

O servidor HTTP do Zephyr trata todo cliente igual, é só HTTP. Esse
endpoint é disparado por dois tipos de cliente:

**1. Cliente externo** (`curl`, Postman, app mobile, outro firmware na
rede), bate direto na URL. É o caso dos exemplos `curl` mostrados acima.

**2. Browser carregando a página servida pelo Pico W**, usuário abre
`http://embarcados.local/`, browser baixa `/`, `/style.css` e `/app.js`,
e o `app.js` dispara o GET/POST via `fetch()`. Trigger: clique no botão
Apply (ou OFF) após ajustar os sliders R/G/B no card "RGB LED".
Lifecycle completo em [api.md](api.md).

**Os dois caminhos convergem** no `HTTP_RESOURCE_DEFINE` registrado em
[`web_app.c`](../../app/src/web_app.c), ponto único de entrada no
firmware. A seção abaixo descreve esse trajeto comum, do match da URL
até o handler devolver a resposta.

## Caminho da requisição

**1. URL `/api/rgb` casa com o resource registrado** -
[web_app.c:301](../../app/src/web_app.c#L301):
```c
HTTP_RESOURCE_DEFINE(rgb_resource, web_service, "/api/rgb", &rgb_detail);
```

**2. Handler em [web_app.c:522](../../app/src/web_app.c#L522)** -
parseia o JSON, toma o semáforo `rgb_lock`, atualiza a struct `led_rgb`
e chama `led_strip_update_rgb(strip_dev, &rgb_pixel, 1)`, o driver
empurra os bits via PIO até o WS2812.

**3. Servidor envia** o JSON com a cor aplicada.

---

[← Anterior: api-relays](api-relays.md) · [Índice de APIs](api.md) · [Próximo: api-buzzer →](api-buzzer.md)
