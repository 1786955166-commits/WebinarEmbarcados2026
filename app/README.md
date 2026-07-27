# app/

Main firmware application (`WebinarEmbarcados2026`). For the project
overview, build, tests, REST endpoints and architecture, see the
[project README](../README.md) and the documentation in
[`doc/pages/`](../doc/pages/).

```
app/
├── inc/                ← internal application headers
├── src/                ← C code (main, setup, network_app, web_app)
├── www/                ← embedded web interface (gzipped via CMake)
├── boards/             ← board Device Tree overlay
├── CMakeLists.txt
├── prj.conf            ← firmware base Kconfig
├── debug.conf          ← debug overlay
└── VERSION             ← version (generates app_version.h)
```
