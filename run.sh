#!/usr/bin/env bash
# es201 lab runner — build, flash (SWD via the Debug Probe), and monitor demos.
#
#   ./run.sh list                list buildable lab targets
#   ./run.sh build [target]      configure + build all, or just one target
#   ./run.sh flash <target>      build that target and flash it over SWD
#   ./run.sh monitor [device]    open the USB-serial console (default /dev/ttyACM0)
#   ./run.sh <target>            shorthand: flash <target>, then monitor
#
# Env overrides: PICO_BOARD (pico2|pico2_w), SERIAL (/dev/ttyACMx), ADAPTER_SPEED.
# Needs a working toolchain + RP2350-capable OpenOCD — see the course docs:
#   Architecture of Embedded Systems -> Environment Setup (Linux).
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD="$HERE/build"
SERIAL_DEFAULT="${SERIAL:-/dev/ttyACM0}"
ADAPTER_SPEED="${ADAPTER_SPEED:-5000}"
OCD_IFACE="interface/cmsis-dap.cfg"
OCD_TARGET="target/rp2350.cfg"

# If OPENOCD_SCRIPTS isn't exported, fall back to the Pi-fork install under ~/.pico-sdk.
ocd_scripts_arg() {
    [ -n "${OPENOCD_SCRIPTS:-}" ] && return
    local d
    d=$(ls -d "$HOME"/.pico-sdk/openocd/*/scripts 2>/dev/null | head -1 || true)
    [ -n "$d" ] && printf -- '-s\n%s\n' "$d"
}

configure() {
    if [ ! -f "$BUILD/CMakeCache.txt" ]; then
        echo ">> configuring (PICO_BOARD=${PICO_BOARD:-pico2})"
        cmake -S "$HERE" -B "$BUILD" ${PICO_BOARD:+-DPICO_BOARD=$PICO_BOARD}
    fi
}

list_targets() {
    grep -rhoE '(es201_app|add_executable)\(\s*[a-z0-9_]+' "$HERE"/l*/CMakeLists.txt 2>/dev/null \
        | sed -E 's/.*\(\s*//' | sort -u
}

build_target() {
    configure
    if [ -n "${1:-}" ]; then
        echo ">> building $1"; cmake --build "$BUILD" --target "$1" -j"$(nproc)"
    else
        echo ">> building all labs"; cmake --build "$BUILD" -j"$(nproc)"
    fi
}

flash_target() {
    local target="$1" elf
    build_target "$target"
    elf="$(find "$BUILD" -name "${target}.elf" -print -quit)"
    [ -n "$elf" ] || { echo "!! no ELF for '$target' — run './run.sh list'"; exit 1; }
    echo ">> flashing $elf over SWD (power the Pico's own USB too!)"
    local s=(); mapfile -t s < <(ocd_scripts_arg)
    openocd "${s[@]}" -f "$OCD_IFACE" -f "$OCD_TARGET" \
        -c "adapter speed $ADAPTER_SPEED" \
        -c "program \"$elf\" verify reset exit"
}

monitor() {
    local dev="${1:-$SERIAL_DEFAULT}"
    echo ">> serial monitor on $dev"
    if   command -v tio     >/dev/null; then exec tio "$dev"
    elif command -v picocom >/dev/null; then exec picocom -b 115200 "$dev"
    elif command -v minicom >/dev/null; then exec minicom -D "$dev"
    else echo "(install tio/picocom/minicom for a nicer console)"; exec cat "$dev"; fi
}

cmd="${1:-}"; shift || true
case "$cmd" in
    list)         list_targets ;;
    build)        build_target "${1:-}" ;;
    flash)        [ -n "${1:-}" ] || { echo "usage: ./run.sh flash <target>"; exit 2; }; flash_target "$1" ;;
    monitor)      monitor "${1:-}" ;;
    ""|-h|--help) sed -n '2,9p' "$0" | sed 's/^# \{0,1\}//' ;;
    *)            flash_target "$cmd"; monitor ;;   # e.g. ./run.sh l3_pwm_tone
esac
