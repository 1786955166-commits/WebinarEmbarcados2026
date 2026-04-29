# Fórum de Sistemas Embarcados e IoT 2026

> Palestra: **Zephyr RTOS: Segundos Passos, Construindo uma Aplicação IoT Completa com Raspberry Pi Pico W**
> Palestrante: **Jorge Guzman**

Esta palestra é a **continuação** do material apresentado em 2025
([Zephyr RTOS: Primeiros Passos](https://github.com/JorgeGzm/EmbarcadosIot2025)).
Enquanto a edição anterior cobriu instalação do ambiente, primeiro projeto e
fundamentos de DTS/Kconfig, esta edição mergulha em uma aplicação IoT completa:
servidor HTTP embarcado, controle de periféricos via API REST e sistema de arquivos
LittleFS.

## 🛠️ Hardware Utilizado

Este projeto foi desenvolvido e testado com:

* **PC:** Ubuntu 22.04 / 24.04
* **Placa:** [Raspberry Pi Pico W](https://www.raspberrypi.com/products/raspberry-pi-pico/)
  * MCU: RP2040 (dual-core Cortex-M0+ @ 133 MHz)
  * 264 KB SRAM, 2 MB Flash QSPI
  * WiFi: Infineon CYW43439 (driver `airoc`)
* **Módulo de expansão:** [Waveshare Pico-Relay-B](https://www.waveshare.com/pico-relay-b.htm)
  * 8 canais de relés (GP14–GP21)
  * LED RGB WS2812 endereçável (GP13)
  * Buzzer passivo (GP6)
* **Zephyr RTOS:** v4.4.0
* **Zephyr SDK:** v1.0.1

## 1. Guia de Instalação

Para preparar o ambiente Zephyr, siga o
[Getting Started Guide](https://docs.zephyrproject.org/latest/develop/getting_started/index.html).
Documentos oficiais úteis ao longo do projeto:

- [Boards Supported](https://docs.zephyrproject.org/latest/boards/index.html), placas com BSP upstream
- [Device Tree bindings](https://docs.zephyrproject.org/latest/build/dts/api/bindings.html), referência dos drivers
- [example-application](https://github.com/zephyrproject-rtos/example-application/), template para novos projetos
- [Board Porting Guide](https://docs.zephyrproject.org/latest/hardware/porting/board_porting.html), como portar hardware custom

### 1.1 Blobs Binários (WiFi e RP2040)

A placa Pico W exige blobs proprietários do firmware CYW43 e bibliotecas HAL
do RP2040. Devem ser baixados antes do primeiro build:

```bash
$ west blobs fetch hal_infineon
$ west blobs fetch hal_rpi_pico
```

## 2. Estrutura do Projeto

```
WebinarEmbarcados2026/
├── app/                                ← Aplicação principal
│   ├── boards/
│   │   └── dev_rpi_pico_rp2040_w.overlay   ← Device Tree overlay
│   ├── inc/
│   │   ├── setup.h                     ← Bootstrap (sanity/middleware/db/tasks)
│   │   ├── network_app.h               ← Bring-up de rede em state machine
│   │   └── web_app.h                   ← Aplicação web (handlers REST)
│   ├── src/
│   │   ├── main.c                      ← 3 linhas: setup_init() + wdt_feed loop
│   │   ├── setup.c                     ← Orquestração das fases de inicialização
│   │   ├── network_app.c               ← Thread de rede com state machine
│   │   └── web_app.c                   ← Endpoints HTTP + recursos estáticos
│   ├── www/                            ← Interface web embutida (gzip + chunks)
│   │   ├── index.html
│   │   ├── style.css
│   │   └── app.js
│   ├── CMakeLists.txt
│   ├── prj.conf                        ← Configuração base do firmware
│   ├── debug.conf                      ← Overlay de debug adicional
│   └── VERSION                         ← Versão do firmware (Zephyr gera app_version.h)
├── lib/                                ← Bibliotecas customizadas (gzm/)
│   ├── common/                         ← Tipos compartilhados (net, wifi)
│   ├── wifi_mgr/                       ← WiFi + DHCP + SNTP + queue de eventos
│   ├── rtc_mgr/                        ← Real-time clock + timezone
│   ├── fs_mgr/                         ← Filesystem LittleFS wrapper
│   └── ringtone/                       ← Player RTTTL via PWM
├── boards/raspberrypi/dev_rpi_pico/    ← Board custom out-of-tree
└── .vscode/                            ← tasks.json, launch.json, settings.json
```

## 3. Build Manual

Antes de compilar, ative o ambiente virtual e configure as variáveis do Zephyr:

```bash
$ source ~/zephyrproject/.venv/bin/activate
$ export ZEPHYR_BASE=~/zephyrproject/zephyr
$ export ZEPHYR_SDK_INSTALL_DIR=~/zephyr-sdk-1.0.1
```

### 3.1 Build Otimizado (Produção)

Build padrão usando apenas `prj.conf`, recomendado para deploy:

```bash
$ west build -b dev_rpi_pico/rp2040/w -p -s app -d app/build \
    -- -DBOARD_ROOT=${PWD} \
       -DZEPHYR_EXTRA_MODULES=${PWD}/lib \
       -DDTC_OVERLAY_FILE=${PWD}/app/boards/dev_rpi_pico_rp2040_w.overlay

-- Zephyr version: 4.4.0 (/home/jaga/zephyrproject/zephyr)
[1358/1358] Linking C executable zephyr/zephyr.elf
Memory region         Used Size  Region Size  %age Used
      BOOT_FLASH:         256 B        256 B    100.00%
           FLASH:      733808 B    2031360 B     36.12%
             RAM:      174272 B       264 KB     64.46%
        IDT_LIST:           0 B        32 KB      0.00%
Wrote 1468416 bytes to zephyr.uf2
```

### 3.2 Build com Debug Adicional

O `prj.conf` já mantém ativos os símbolos básicos para diagnóstico de crash
(`CONFIG_DEBUG=y`, `CONFIG_EXCEPTION_DEBUG=y`, `CONFIG_FAULT_DUMP=2`,
`CONFIG_EXTRA_EXCEPTION_INFO=y`). O `app/debug.conf` adiciona
`CONFIG_DEBUG_THREAD_INFO=y`, que melhora stack traces e habilita inspeção
via GDB / Cortex-Debug:

```bash
$ west build -b dev_rpi_pico/rp2040/w -p -s app -d app/build \
    -- -DBOARD_ROOT=${PWD} \
       -DZEPHYR_EXTRA_MODULES=${PWD}/lib \
       -DEXTRA_CONF_FILE="debug.conf" \
       -DDTC_OVERLAY_FILE=${PWD}/app/boards/dev_rpi_pico_rp2040_w.overlay
```

Diferença mínima neste projeto (~72 bytes em FLASH) porque os símbolos
críticos já estão no build padrão. Em projetos sem `CONFIG_DEBUG=y` no
`prj.conf`, a diferença típica é 50-60 KB de FLASH.

### 3.3 Gravação no Hardware

```bash
$ west flash -d app/build
```

A flashing por padrão usa o picotool do BOOTSEL (modo de boot do RP2040, segure
o botão BOOTSEL e conecte o USB). Também é possível gravar copiando manualmente
o arquivo `app/build/zephyr/zephyr.uf2` para o volume `RPI-RP2` montado.

### 3.4 Análise de Memória

```bash
$ west build -d app/build -t rom_report   # uso de FLASH por símbolo/módulo
$ west build -d app/build -t ram_report   # uso de RAM por símbolo/módulo
```

## 4. Build e Debug Utilizando o VSCode

Os arquivos de configuração do VSCode estão em `.vscode/`:

```
.vscode/
├── launch.json      ← Perfis de debug (Cortex-Debug + OpenOCD/J-Link)
├── settings.json    ← Variáveis de ambiente, IntelliSense, paths
└── tasks.json       ← Tasks de build e flash
```

Tasks principais (`Ctrl+Shift+B`):

| Task                       | Equivalente                                |
|----------------------------|--------------------------------------------|
| `app: build`               | Build otimizado (seção 3.1)                |
| `app: build (debug.conf)`  | Build com `debug.conf` (seção 3.2)         |
| `app: flash`               | `west flash`                               |
| `app: rom_report`          | Uso de FLASH por símbolo                   |
| `app: ram_report`          | Uso de RAM por símbolo                     |

## 5. Funcionalidades da Aplicação

O firmware expõe um servidor HTTP único na porta 80, atendendo dois
tipos de cliente (página web + API direta) que convergem no
`HTTP_RESOURCE_DEFINE` e despacham entre resources STATIC (byte arrays
em flash, vindos dos `.gz.inc`) e DYNAMIC (handlers em `web_app.c`).

> Detalhes da arquitetura, com diagramas, em
> [`doc/pages/architecture.md`](doc/pages/architecture.md).

### 5.1 Endpoints REST

Cada endpoint tem uma página dedicada em [`doc/pages/`](doc/pages/) com exemplo `curl`
e trace até o handler em C.

| Endpoint                     | Método    | Descrição                                                | Detalhes                          |
|------------------------------|-----------|----------------------------------------------------------|-----------------------------------|
| `/`, `/style.css`, `/app.js` | GET       | Recursos estáticos (servidos da flash, gzip + chunked)   | -                                 |
| `/api/info`                  | GET       | Versão do firmware, autor, contato                       | [doc/pages/api-info.md](doc/pages/api-info.md)         |
| `/api/time`                  | GET       | Hora atual já com timezone local (UTC-3)                 | [doc/pages/api-time.md](doc/pages/api-time.md)         |
| `/api/relays`                | GET/POST  | Estado dos 8 relés / acionamento individual              | [doc/pages/api-relays.md](doc/pages/api-relays.md)     |
| `/api/rgb`                   | GET/POST  | Cor atual / set RGB do WS2812                            | [doc/pages/api-rgb.md](doc/pages/api-rgb.md)           |
| `/api/buzzer`                | GET/POST  | Status / play/stop de ringtone RTTTL                     | [doc/pages/api-buzzer.md](doc/pages/api-buzzer.md)     |
| `/api/files`                 | GET       | Listagem de diretório + estatísticas (total/used/pct)    | [doc/pages/api-files.md](doc/pages/api-files.md)       |
| `/api/upload`                | POST      | Upload multipart de arquivo                              | [doc/pages/api-upload.md](doc/pages/api-upload.md)     |
| `/api/delete`                | POST      | Remoção de arquivo do LittleFS                           | [doc/pages/api-delete.md](doc/pages/api-delete.md)     |
| `/download/*`                | GET       | Download de arquivo (chunked, Content-Type por extensão) | [doc/pages/api-download.md](doc/pages/api-download.md) |

> Todos os exemplos usam `embarcados.local` (mDNS). Se sua rede não resolve,
> troque pelo IP direto que aparece no log do device (ex: `192.168.15.8`).

### 5.2 Subsistemas Internos

- **WiFi + DHCP + mDNS**, `embarcados.local`, gerenciado via `gzm/wifi_mgr`
- **State machine de rede**, `network_app` orquestra connect/IP/SNTP/web em thread dedicada com retry automático em caso de queda do WiFi (loop `OFFLINE → CONNECTING → RUNNING`)
- **Queue de eventos WiFi**, `wifi_mgr` produz eventos com payload (IP, gateway, netmask) consumidos pela state machine via `k_msgq`
- **Watchdog hardware**, timeout de 5s, alimentado pelo loop principal a cada 300ms (`setup_wdt_feed`)
- **Reset cause diagnostics**, no boot, `setup_init_sanity` lê `hwinfo_get_reset_cause()` e classifica em LOG_INF (PIN/SOFTWARE/POR/DEBUG) ou LOG_ERR (WATCHDOG, BROWNOUT, CPU_LOCKUP, etc.)
- **Bootstrap em fases**, `setup.c` orquestra `sanity → middleware → database → tasks` com handler central de erro fatal
- **RTC + timezone**, RTC sempre em UTC, timezone (-3 BRT) aplicada via `rtc_mgr_get_local()` no momento da consulta
- **SNTP**, sincronização de horário via `time.google.com` (UDP/123)
- **LittleFS**, sistema de arquivos persistente em `/lfs1` (256 KB)
- **Shell**, UART (USB CDC) e Telnet (porta 23), com comandos de filesystem, RTC, ringtone

### 5.3 Hardware controlado

- **8 relés**, GP14-GP21 via aliases DTS `relay1`..`relay8`
- **LED RGB WS2812**, GP13 via PIO + driver `led_strip`
- **Buzzer passivo**, GP6 via PWM, alias `pwm_buzzer0`

## 6. Referências

A placa custom `dev_rpi_pico` em
[boards/raspberrypi/dev_rpi_pico/](boards/raspberrypi/dev_rpi_pico/) é
derivada do
[BSP raspberrypi/rpi_pico](https://github.com/zephyrproject-rtos/zephyr/tree/main/boards/raspberrypi/rpi_pico)
(upstream da família Pico).

Após compilar, o Zephyr gera arquivos úteis para depurar configuração:

* [`app/build/zephyr/zephyr.dts`](app/build/zephyr/zephyr.dts), DT final
  após merge de `.dts` + `.dtsi` + overlays
* [`app/build/zephyr/include/generated/zephyr/autoconf.h`](app/build/zephyr/include/generated/zephyr/autoconf.h),
  todas as macros `CONFIG_*` ativas após merge de `prj.conf` + `debug.conf`
* [`app/build/zephyr/zephyr.map`](app/build/zephyr/zephyr.map), mapa de
  memória, tamanho/localização de funções e símbolos

## 7. RTTTL

[RTTTL](https://en.wikipedia.org/wiki/Ring_Tone_Text_Transfer_Language)
(Ring Tone Text Transfer Language) é um formato em texto criado pela Nokia
para descrever ringtones monofônicos. Cada string segue o padrão:

```
<nome>:<controle>:<notas>
```

- **nome**, identificador (até 10 caracteres)
- **controle**, defaults: `d=` duração, `o=` oitava, `b=` BPM (ex.: `d=4,o=5,b=100`)
- **notas**, sequência separada por vírgula no formato `[duração]nota[oitava][.]`,
  onde `.` indica nota pontuada (1.5×) e `p` é pausa

Exemplo (Mario theme, abreviado):

```
Mario:d=4,o=5,b=100:16e6,16e6,32p,8e6,16c6,8e6,8g6
```

Este projeto reproduz RTTTL no buzzer passivo (GP6, PWM) através da
biblioteca [`lib/ringtone/`](lib/ringtone/), com três formas de uso:

**1. Shell (UART/USB CDC ou Telnet porta 23):**

```
uart:~$ ringtone test                       # toca tema do Mario embutido
uart:~$ ringtone alarm 1                    # alarmes pré-definidos
uart:~$ ringtone play "Beep:d=4,o=5,b=100:c,e,g"
uart:~$ ringtone stop
uart:~$ ringtone status
```

**2. REST API** (`/api/buzzer`, ver [doc/pages/api-buzzer.md](doc/pages/api-buzzer.md)):

```bash
$ curl -X POST http://embarcados.local/api/buzzer \
    -H "Content-Type: application/json" \
    -d '{"action":"play","rtttl":"Beep:d=4,o=5,b=100:c,e,g"}'
$ curl -X POST http://embarcados.local/api/buzzer -d '{"action":"stop"}'
```

**3. C API** (incluindo `<gzm/ringtone.h>`):

```c
ringtone_play_custom("Beep:d=4,o=5,b=100:c,e,g");
ringtone_play_notification(RINGTONE_ALARM1);
ringtone_stop();
```

Coleções de strings RTTTL prontas:
[picaxe.com/rtttl-ringtones-for-tune](http://www.picaxe.com/rtttl-ringtones-for-tune/),
[mines.lumpylumpy.com/Electronics/Computers/Software/Cpp/MFC/RingTones.php](https://mines.lumpylumpy.com/Electronics/Computers/Software/Cpp/MFC/RingTones.php).

## 8. Firmware DebugProbe

A [Raspberry Pi Debug Probe](https://www.raspberrypi.com/products/debug-probe/) é
um adaptador USB→SWD/UART baseado no RP2040 que implementa o protocolo
**CMSIS-DAP**, permitindo flash + debug via OpenOCD sem depender do modo BOOTSEL.
Também é possível transformar um Pico comum em probe gravando o firmware
[`debugprobe_on_pico.uf2`](https://github.com/raspberrypi/debugprobe/releases/tag/debugprobe-v2.3.0)
(use `debugprobe.uf2` para a placa oficial).

**Procedimento de flash do probe:**

1. Pressione BOOTSEL no Pico/Probe e conecte o USB, monte o volume `RPI-RP2`
2. Copie o `.uf2` correspondente, o dispositivo reinicia automaticamente como
   CMSIS-DAP (`lsusb` deve mostrar `2e8a:000c Raspberry Pi Debug Probe`)

## 9. Shell via Telnet

```bash
telnet embarcados.local
# ou
ncat embarcados.local 23
```
