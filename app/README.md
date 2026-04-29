# app/

Aplicação principal do firmware (`WebinarEmbarcados2026`). Para visão
geral do projeto, build, testes, endpoints REST e arquitetura, veja o
[README do projeto](../README.md) e a documentação em
[`doc/pages/`](../doc/pages/).

```
app/
├── inc/                ← headers internos da aplicação
├── src/                ← código C (main, setup, network_app, web_app)
├── www/                ← interface web embutida (gzip via CMake)
├── boards/             ← Device Tree overlay da placa
├── CMakeLists.txt
├── prj.conf            ← Kconfig base do firmware
├── debug.conf          ← Overlay de debug
└── VERSION             ← Versão (gera app_version.h)
```
