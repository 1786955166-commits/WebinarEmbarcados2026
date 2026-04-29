[← README](../../README.md) · [Arquitetura](architecture.md)

# API REST do firmware

Documentação detalhada de cada endpoint HTTP exposto pelo Pico W, mostrando
como uma requisição flui da página `index.html` até o handler em C que
controla o hardware (e o caminho de volta).

> Para a visão geral da arquitetura (dois clientes, STATIC vs DYNAMIC,
> como os `.gz.inc` viram resources em flash) com diagramas, veja
> [`architecture.md`](architecture.md). Este doc foca no contrato REST
> de cada endpoint individual.

## Princípio geral

O firmware roda o servidor HTTP do Zephyr (`CONFIG_HTTP_SERVER`) na porta 80.
Cada endpoint é um **contrato REST**, qualquer cliente HTTP pode chamar
diretamente: o browser via `index.html`, `curl` da linha de comando,
Postman, app mobile, outro firmware na rede.

A página servida em `/` é apenas **um cliente** desse API, a única
peculiaridade é que o próprio Pico W também serve essa página, gzipada,
diretamente da flash.

## Lifecycle da página (entry comum a quase tudo)

Toda interação que parte do browser passa por essa mesma sequência de
carregamento:

```
1. Usuário abre http://embarcados.local/
2. Browser GET /              → index_html_resource → index.html (gzip, da flash)
3. Browser GET /style.css     → style_css_resource  → style.css (gzip, da flash)
4. Browser GET /app.js        → app_js_resource     → app.js (gzip, da flash)
5. app.js executa → DOMContentLoaded handler dispara as chamadas iniciais
   (fetchInfo, fetchTime, fetchRelays, fetchRgb, fetchBuzzerStatus, fetchFiles)
   e registra event listeners nos botões / sliders
```

A partir do passo 5 cada endpoint tem seu próprio caminho. Os docs abaixo
começam o trace nesse ponto onde a chamada concreta é disparada, pode ser
uma das chamadas automáticas iniciais ou uma interação do usuário.

### Exemplo via `curl`, pegar a página inicial

```bash
# Comando
$ curl --compressed http://embarcados.local/

# Resposta esperada
<!DOCTYPE html>
<html lang="pt-BR">
<head>
    <meta charset="UTF-8">
    <title>ZPHR-PICO-0001</title>
    <link rel="stylesheet" href="/style.css">
</head>
<body>
    ...
    <script src="app.js"></script>
</body>
</html>
```

`--compressed` faz o curl negociar gzip e descomprimir, sem isso você
recebe o blob binário direto da flash. As linhas `<link>` e `<script>` são
**a única coisa** que diz ao browser pra buscar os outros 2 arquivos.

### Exemplo via `curl` seguindo a rota, reproduzir o que o browser faz

O browser, ao parsear o HTML acima, dispara automaticamente mais 2 GETs.
Você pode replicar manualmente a mesma sequência:

```bash
$ curl --compressed http://embarcados.local/         # GET /          → index.html
$ curl --compressed http://embarcados.local/style.css # GET /style.css → style.css
$ curl --compressed http://embarcados.local/app.js   # GET /app.js    → app.js
```

3 GETs independentes, não há mágica. Cada URL bate em um
`HTTP_RESOURCE_DEFINE` distinto em [`web_app.c`](../../app/src/web_app.c#L295-L297):

```c
HTTP_RESOURCE_DEFINE(index_html_resource, web_service, "/",          &index_html_detail);
HTTP_RESOURCE_DEFINE(app_js_resource,     web_service, "/app.js",    &app_js_detail);
HTTP_RESOURCE_DEFINE(style_css_resource,  web_service, "/style.css", &style_css_detail);
```

Inspecionar os headers confirma que o conteúdo vem gzipado da flash:
```bash
# Comando
$ curl -v http://embarcados.local/style.css 2>&1 | grep -i "content-"

# Resposta esperada
< Content-Type: text/css
< Content-Encoding: gzip
< Transfer-Encoding: chunked
```

## Endpoints documentados

| Doc | Endpoint | Disparado por |
|---|---|---|
| [api-info.md](api-info.md) | `/api/info` | Auto-fetch no `DOMContentLoaded` |
| [api-time.md](api-time.md) | `/api/time` | Auto-fetch + polling 1 s |
| [api-relays.md](api-relays.md) | `/api/relays` | Botões Turn ON/OFF de cada canal |
| [api-rgb.md](api-rgb.md) | `/api/rgb` | Sliders RGB + botões Apply/OFF |
| [api-buzzer.md](api-buzzer.md) | `/api/buzzer` | Botões Play/Stop |
| [api-files.md](api-files.md) | `/api/files` | Refresh + navegação de pastas |
| [api-upload.md](api-upload.md) | `/api/upload` | `<input type="file">` + botão Upload |
| [api-delete.md](api-delete.md) | `/api/delete` | Botão Delete em cada linha |
| [api-download.md](api-download.md) | `/download/*` | Link `<a download>` (sem JS) |

## Convenção dos docs

Cada arquivo segue a mesma estrutura:

1. **Tabela do endpoint**, método(s), URL, body, resposta
2. **Exemplo via `curl`**, comando + resposta esperada
3. **Caminho da requisição**, passo a passo numerado do match da URL
   com o `HTTP_RESOURCE_DEFINE` até o handler em C devolver a resposta

---

[← README](../../README.md) · [Próximo: api-info →](api-info.md)
