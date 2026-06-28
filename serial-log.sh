#!/usr/bin/env bash
# Log serial output — read-only, no interactive terminal. Just prints what the
# board sends; pipe it to a file if you want to keep it.
#
#   ./serial-log.sh                     # default /dev/ttyACM1 @ 115200 -> stdout
#   ./serial-log.sh /dev/ttyACM0 9600   # pick port + baud
#   ./serial-log.sh | tee adc.log       # also save to a file
#
# (For the lab board the Debug Probe's UART bridge is usually /dev/ttyACM1.)
set -euo pipefail

PORT="${1:-/dev/ttyACM1}"
BAUD="${2:-115200}"

[ -e "$PORT" ] || { echo "no serial port at '$PORT'" >&2; exit 1; }

stty -F "$PORT" "$BAUD" raw -echo            # configure the line, don't echo
echo "logging $PORT @ $BAUD  (Ctrl-C to stop)" >&2
exec cat "$PORT"                              # read-only: just stream it out
