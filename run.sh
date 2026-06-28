#!/usr/bin/env bash
# es201 lab runner — build, flash (SWD via the Debug Probe), and monitor demos.
#
#   ./run.sh list                list buildable lab targets
#   ./run.sh build [target]      configure + build all, or just one target
#   ./run.sh flash <target>      build that target and flash it over SWD
#   ./run.sh monitor [device]    open the USB-serial console (default /dev/ttyACM0)
#   ./run.sh <target>            shorthand: flash <target>, then monitor
#   ./run.sh flash l4_optimize START_MODE=1   override a compile-time #define
#
# Trailing KEY=VAL args become -DKEY=VAL build defines (they persist in the CMake
# cache until you set them again). Env overrides: PICO_BOARD (pico2|pico2_w),
# SERIAL (/dev/ttyACMx), ADAPTER_SPEED.
# Needs a working toolchain + RP2350-capable OpenOCD — see the course docs:
#   Architecture of Embedded Systems -> Environment Setup (Linux).
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD="$HERE/build"
ADAPTER_SPEED="${ADAPTER_SPEED:-5000}"

# Find a ttyACM by USB PID: 0009 = Pico's own USB-CDC, 000c = Debug Probe UART bridge.
find_acm_by_pid() {
    local pid="$1" d p
    for d in /dev/ttyACM*; do
        [ -e "$d" ] || continue
        p="$(udevadm info -q property -n "$d" 2>/dev/null)"
        echo "$p" | grep -q 'ID_USB_VENDOR_ID=2e8a' && echo "$p" | grep -q "ID_USB_MODEL_ID=$pid" && { echo "$d"; return 0; }
    done
    return 1
}
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
    # Programs print to BOTH the Pico USB-CDC and UART0 (GP0/1 -> Debug Probe bridge).
    # Prefer the probe's UART bridge: it's the reliable path (USB-CDC can be flaky on
    # some RP2350 setups). Override with: SERIAL=/dev/ttyACMx ./run.sh monitor
    local dev="${1:-${SERIAL:-}}"
    [ -z "$dev" ] && dev="$(find_acm_by_pid 000c || true)"   # Debug Probe UART bridge
    [ -z "$dev" ] && dev="$(find_acm_by_pid 0009 || true)"   # Pico USB-CDC
    [ -z "$dev" ] && { echo "!! no Pico/probe serial port found (probe plugged in? UART wired to GP0/GP1?)"; exit 1; }
    echo ">> serial monitor on $dev"
    if   command -v tio     >/dev/null; then exec tio "$dev"
    elif command -v picocom >/dev/null; then exec picocom -b 115200 "$dev"
    elif command -v minicom >/dev/null; then exec minicom -D "$dev"
    else echo "(install tio/picocom/minicom for a nicer console)"; exec cat "$dev"; fi
}

cmd="${1:-}"; shift || true

# Split remaining args into KEY=VAL build overrides (forwarded as -D...) and
# positional args. Lets you override compile-time defines without editing source:
#   ./run.sh flash l4_optimize START_MODE=1 ENABLE_DBUF=0
defs=(); pos=()
for a in "$@"; do
    case "$a" in
        *=*) defs+=("-D$a") ;;
        *)   pos+=("$a") ;;
    esac
done
if [ "${#defs[@]}" -gt 0 ]; then
    echo ">> build overrides: ${defs[*]}"
    cmake -S "$HERE" -B "$BUILD" ${PICO_BOARD:+-DPICO_BOARD=$PICO_BOARD} "${defs[@]}"
fi

case "$cmd" in
    list)         list_targets ;;
    build)        build_target "${pos[0]:-}" ;;
    flash)        [ -n "${pos[0]:-}" ] || { echo "usage: ./run.sh flash <target> [KEY=VAL ...]"; exit 2; }; flash_target "${pos[0]}" ;;
    monitor)      monitor "${pos[0]:-}" ;;
    ""|-h|--help) sed -n '2,10p' "$0" | sed 's/^# \{0,1\}//' ;;
    *)            flash_target "$cmd"; monitor ;;   # e.g. ./run.sh l4_optimize START_MODE=1
esac
