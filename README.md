# Embedded Systems and IoT Forum 2026

> Talk: **Zephyr RTOS: Second Steps, Building a Complete IoT Application with the Raspberry Pi Pico W**
> Speaker: **Jorge Guzman**

This talk is the **follow-up** to the material presented in 2025
([Zephyr RTOS: First Steps](https://github.com/JorgeGzm/EmbarcadosIot2025)).
While the previous edition covered environment setup, a first project and
DTS/Kconfig fundamentals, this edition dives into a complete IoT application:
embedded HTTP server, peripheral control through a REST API and a LittleFS
file system.

## 🛠️ Hardware Used

This project was developed and tested with:

* **PC:** Ubuntu 22.04 / 24.04
* **Board:** [Raspberry Pi Pico W](https://www.raspberrypi.com/products/raspberry-pi-pico/)
  * MCU: RP2040 (dual-core Cortex-M0+ @ 133 MHz)
  * 264 KB SRAM, 2 MB QSPI Flash
  * WiFi: Infineon CYW43439 (`airoc` driver)
* **Expansion module:** [Waveshare Pico-Relay-B](https://www.waveshare.com/pico-relay-b.htm)
  * 8 relay channels (GP14–GP21)
  * Addressable WS2812 RGB LED (GP13)
  * Passive buzzer (GP6)
* **Zephyr RTOS:** v4.4.0
* **Zephyr SDK:** v1.0.1

## 1. Installation Guide

To set up the Zephyr environment, follow the
[Getting Started Guide](https://docs.zephyrproject.org/latest/develop/getting_started/index.html).
Official documents that are useful throughout the project:

- [Boards Supported](https://docs.zephyrproject.org/latest/boards/index.html), boards with an upstream BSP
- [Device Tree bindings](https://docs.zephyrproject.org/latest/build/dts/api/bindings.html), driver reference
- [example-application](https://github.com/zephyrproject-rtos/example-application/), template for new projects
- [Board Porting Guide](https://docs.zephyrproject.org/latest/hardware/porting/board_porting.html), how to port custom hardware

### 1.1 Binary Blobs (WiFi and RP2040)

The Pico W board requires proprietary blobs for the CYW43 firmware and the
RP2040 HAL libraries. They must be downloaded before the first build:

```bash
$ west blobs fetch hal_infineon
$ west blobs fetch hal_rpi_pico
```

## 2. Project Structure

```
WebinarEmbarcados2026/
├── app/                                ← Main application
│   ├── boards/
│   │   └── dev_rpi_pico_rp2040_w.overlay   ← Device Tree overlay
│   ├── inc/
│   │   ├── setup.h                     ← Bootstrap (sanity/middleware/db/tasks)
│   │   ├── network_app.h               ← Network bring-up as a state machine
│   │   └── web_app.h                   ← Web application (REST handlers)
│   ├── src/
│   │   ├── main.c                      ← 3 lines: setup_init() + wdt_feed loop
│   │   ├── setup.c                     ← Orchestration of the init phases
│   │   ├── network_app.c               ← Network thread with a state machine
│   │   └── web_app.c                   ← HTTP endpoints + static resources
│   ├── www/                            ← Embedded web interface (gzip + chunks)
│   │   ├── index.html
│   │   ├── style.css
│   │   └── app.js
│   ├── CMakeLists.txt
│   ├── prj.conf                        ← Firmware base configuration
│   ├── debug.conf                      ← Additional debug overlay
│   └── VERSION                         ← Firmware version (Zephyr generates app_version.h)
├── lib/                                ← Custom libraries (gzm/)
│   ├── common/                         ← Shared types (net, wifi)
│   ├── wifi_mgr/                       ← WiFi + DHCP + SNTP + event queue
│   ├── rtc_mgr/                        ← Real-time clock + timezone
│   ├── fs_mgr/                         ← LittleFS file system wrapper
│   └── ringtone/                       ← RTTTL player through PWM
├── boards/raspberrypi/dev_rpi_pico/    ← Custom out-of-tree board
└── .vscode/                            ← tasks.json, launch.json, settings.json
```

## 3. Manual Build

Before compiling, activate the virtual environment and set the Zephyr
variables:

```bash
$ source ~/zephyrproject/.venv/bin/activate
$ export ZEPHYR_BASE=~/zephyrproject/zephyr
$ export ZEPHYR_SDK_INSTALL_DIR=~/zephyr-sdk-1.0.1
```

### 3.1 Optimized Build (Production)

Default build using only `prj.conf`, recommended for deployment:

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

### 3.2 Build with Additional Debug

`prj.conf` already keeps the basic crash diagnosis symbols enabled
(`CONFIG_DEBUG=y`, `CONFIG_EXCEPTION_DEBUG=y`, `CONFIG_FAULT_DUMP=2`,
`CONFIG_EXTRA_EXCEPTION_INFO=y`). `app/debug.conf` adds
`CONFIG_DEBUG_THREAD_INFO=y`, which improves stack traces and enables
inspection through GDB / Cortex-Debug:

```bash
$ west build -b dev_rpi_pico/rp2040/w -p -s app -d app/build \
    -- -DBOARD_ROOT=${PWD} \
       -DZEPHYR_EXTRA_MODULES=${PWD}/lib \
       -DEXTRA_CONF_FILE="debug.conf" \
       -DDTC_OVERLAY_FILE=${PWD}/app/boards/dev_rpi_pico_rp2040_w.overlay
```

The difference is minimal in this project (~72 bytes of FLASH) because the
critical symbols are already in the default build. In projects without
`CONFIG_DEBUG=y` in `prj.conf`, the typical difference is 50-60 KB of FLASH.

### 3.3 Flashing the Hardware

```bash
$ west flash -d app/build
```

Flashing uses picotool through BOOTSEL by default (the RP2040 boot mode, hold
the BOOTSEL button and plug in the USB). You can also flash by manually
copying the `app/build/zephyr/zephyr.uf2` file to the mounted `RPI-RP2`
volume.

### 3.4 Memory Analysis

```bash
$ west build -d app/build -t rom_report   # FLASH usage per symbol/module
$ west build -d app/build -t ram_report   # RAM usage per symbol/module
```

## 4. Build and Debug Using VSCode

The VSCode configuration files live in `.vscode/`:

```
.vscode/
├── launch.json      ← Debug profiles (Cortex-Debug + OpenOCD/J-Link)
├── settings.json    ← Environment variables, IntelliSense, paths
└── tasks.json       ← Build and flash tasks
```

Main tasks (`Ctrl+Shift+B`):

| Task                       | Equivalent                                 |
|----------------------------|--------------------------------------------|
| `app: build`               | Optimized build (section 3.1)              |
| `app: build (debug.conf)`  | Build with `debug.conf` (section 3.2)      |
| `app: flash`               | `west flash`                               |
| `app: rom_report`          | FLASH usage per symbol                     |
| `app: ram_report`          | RAM usage per symbol                       |

## 5. Application Features

The firmware exposes a single HTTP server on port 80, serving two kinds of
client (web page + direct API) that converge on `HTTP_RESOURCE_DEFINE` and
dispatch between STATIC resources (byte arrays in flash, coming from the
`.gz.inc` files) and DYNAMIC ones (handlers in `web_app.c`).

> Architecture details, with diagrams, in
> [`doc/pages/architecture.md`](doc/pages/architecture.md).

### 5.1 REST Endpoints

Each endpoint has a dedicated page in [`doc/pages/`](doc/pages/) with a `curl`
example and a trace down to the C handler.

| Endpoint                     | Method    | Description                                              | Details                           |
|------------------------------|-----------|----------------------------------------------------------|-----------------------------------|
| `/`, `/style.css`, `/app.js` | GET       | Static resources (served from flash, gzip + chunked)     | -                                 |
| `/api/info`                  | GET       | Firmware version, author, contact                        | [doc/pages/api-info.md](doc/pages/api-info.md)         |
| `/api/time`                  | GET       | Current time already in the local timezone (UTC-3)       | [doc/pages/api-time.md](doc/pages/api-time.md)         |
| `/api/relays`                | GET/POST  | State of the 8 relays / individual switching             | [doc/pages/api-relays.md](doc/pages/api-relays.md)     |
| `/api/rgb`                   | GET/POST  | Current color / set the WS2812 RGB                       | [doc/pages/api-rgb.md](doc/pages/api-rgb.md)           |
| `/api/buzzer`                | GET/POST  | Status / play and stop an RTTTL ringtone                 | [doc/pages/api-buzzer.md](doc/pages/api-buzzer.md)     |
| `/api/files`                 | GET       | Directory listing + statistics (total/used/pct)          | [doc/pages/api-files.md](doc/pages/api-files.md)       |
| `/api/upload`                | POST      | Multipart file upload                                    | [doc/pages/api-upload.md](doc/pages/api-upload.md)     |
| `/api/delete`                | POST      | File removal from LittleFS                               | [doc/pages/api-delete.md](doc/pages/api-delete.md)     |
| `/download/*`                | GET       | File download (chunked, Content-Type by extension)       | [doc/pages/api-download.md](doc/pages/api-download.md) |

> All examples use `embarcados.local` (mDNS). If your network does not
> resolve it, replace it with the direct IP shown in the device log
> (for example `192.168.15.8`).

### 5.2 Internal Subsystems

- **WiFi + DHCP + mDNS**, `embarcados.local`, managed through `gzm/wifi_mgr`
- **Network state machine**, `network_app` orchestrates connect/IP/SNTP/web in a dedicated thread with automatic retry whenever WiFi drops (loop `OFFLINE → CONNECTING → RUNNING`)
- **WiFi event queue**, `wifi_mgr` produces events carrying a payload (IP, gateway, netmask) consumed by the state machine through a `k_msgq`
- **Hardware watchdog**, 5 s timeout, fed by the main loop every 300 ms (`setup_wdt_feed`)
- **Reset cause diagnostics**, at boot `setup_init_sanity` reads `hwinfo_get_reset_cause()` and classifies it as LOG_INF (PIN/SOFTWARE/POR/DEBUG) or LOG_ERR (WATCHDOG, BROWNOUT, CPU_LOCKUP, and so on)
- **Phased bootstrap**, `setup.c` orchestrates `sanity → middleware → database → tasks` with a central fatal error handler
- **RTC + timezone**, the RTC always holds UTC, and the timezone (-3 BRT) is applied through `rtc_mgr_get_local()` at query time
- **SNTP**, time synchronization through `time.google.com` (UDP/123)
- **LittleFS**, persistent file system at `/lfs1` (256 KB)
- **Shell**, UART (USB CDC) and Telnet (port 23), with file system, RTC and ringtone commands

### 5.3 Controlled Hardware

- **8 relays**, GP14-GP21 through the DTS aliases `relay1`..`relay8`
- **WS2812 RGB LED**, GP13 through the PIO + `led_strip` driver
- **Passive buzzer**, GP6 through PWM, alias `pwm_buzzer0`

## 6. References

The custom board `dev_rpi_pico` in
[boards/raspberrypi/dev_rpi_pico/](boards/raspberrypi/dev_rpi_pico/) is
derived from the
[raspberrypi/rpi_pico BSP](https://github.com/zephyrproject-rtos/zephyr/tree/main/boards/raspberrypi/rpi_pico)
(upstream for the Pico family).

After compiling, Zephyr generates files that are useful for debugging the
configuration:

* [`app/build/zephyr/zephyr.dts`](app/build/zephyr/zephyr.dts), the final DT
  after merging `.dts` + `.dtsi` + overlays
* [`app/build/zephyr/include/generated/zephyr/autoconf.h`](app/build/zephyr/include/generated/zephyr/autoconf.h),
  every active `CONFIG_*` macro after merging `prj.conf` + `debug.conf`
* [`app/build/zephyr/zephyr.map`](app/build/zephyr/zephyr.map), the memory
  map, with the size and location of functions and symbols

## 7. RTTTL

[RTTTL](https://en.wikipedia.org/wiki/Ring_Tone_Text_Transfer_Language)
(Ring Tone Text Transfer Language) is a text format created by Nokia to
describe monophonic ringtones. Each string follows the pattern:

```
<name>:<control>:<notes>
```

- **name**, identifier (up to 10 characters)
- **control**, defaults: `d=` duration, `o=` octave, `b=` BPM (e.g. `d=4,o=5,b=100`)
- **notes**, comma-separated sequence in the format `[duration]note[octave][.]`,
  where `.` marks a dotted note (1.5×) and `p` is a rest

Example (Mario theme, abbreviated):

```
Mario:d=4,o=5,b=100:16e6,16e6,32p,8e6,16c6,8e6,8g6
```

This project plays RTTTL on the passive buzzer (GP6, PWM) through the
[`lib/ringtone/`](lib/ringtone/) library, in three ways:

**1. Shell (UART/USB CDC or Telnet on port 23):**

```
uart:~$ ringtone test                       # plays the built-in Mario theme
uart:~$ ringtone alarm 1                    # predefined alarms
uart:~$ ringtone play "Beep:d=4,o=5,b=100:c,e,g"
uart:~$ ringtone stop
uart:~$ ringtone status
```

**2. REST API** (`/api/buzzer`, see [doc/pages/api-buzzer.md](doc/pages/api-buzzer.md)):

```bash
$ curl -X POST http://embarcados.local/api/buzzer \
    -H "Content-Type: application/json" \
    -d '{"action":"play","rtttl":"Beep:d=4,o=5,b=100:c,e,g"}'
$ curl -X POST http://embarcados.local/api/buzzer -d '{"action":"stop"}'
```

**3. C API** (including `<gzm/ringtone.h>`):

```c
ringtone_play_custom("Beep:d=4,o=5,b=100:c,e,g");
ringtone_play_notification(RINGTONE_ALARM1);
ringtone_stop();
```

Ready-made RTTTL string collections:
[picaxe.com/rtttl-ringtones-for-tune](http://www.picaxe.com/rtttl-ringtones-for-tune/),
[mines.lumpylumpy.com/Electronics/Computers/Software/Cpp/MFC/RingTones.php](https://mines.lumpylumpy.com/Electronics/Computers/Software/Cpp/MFC/RingTones.php).

## 8. DebugProbe Firmware

The [Raspberry Pi Debug Probe](https://www.raspberrypi.com/products/debug-probe/)
is a USB→SWD/UART adapter based on the RP2040 that implements the
**CMSIS-DAP** protocol, allowing flash + debug through OpenOCD without
relying on BOOTSEL mode. You can also turn a regular Pico into a probe by
flashing the
[`debugprobe_on_pico.uf2`](https://github.com/raspberrypi/debugprobe/releases/tag/debugprobe-v2.3.0)
firmware (use `debugprobe.uf2` for the official board).

**Probe flashing procedure:**

1. Hold BOOTSEL on the Pico/Probe and plug in the USB, then mount the `RPI-RP2` volume
2. Copy the matching `.uf2`, and the device automatically reboots as
   CMSIS-DAP (`lsusb` should show `2e8a:000c Raspberry Pi Debug Probe`)

## 9. Shell over Telnet

```bash
telnet embarcados.local
# or
ncat embarcados.local 23
```
