#!/usr/bin/env bash
# Minimal BIDIRECTIONAL serial terminal in pure bash (stty + cat) — no minicom/tio.
# Shows the board's output AND sends what you type (line-buffered: type, then Enter).
# Ctrl-C to quit.
#
#   ./serial-term.sh [port] [baud]
#
# Use serial-log.sh instead if you only need to capture output (it pipes to tee).
# Only one program can hold a port at once — close other monitors first.
set -euo pipefail

PORT="${1:-/dev/ttyACM1}"
BAUD="${2:-115200}"
[ -e "$PORT" ] || { echo "no serial port at '$PORT'" >&2; exit 1; }

stty -F "$PORT" "$BAUD" raw -echo        # configure the serial line

reader_pid=""
cleanup() { [ -n "$reader_pid" ] && kill "$reader_pid" 2>/dev/null || true; echo; }
trap cleanup EXIT INT TERM

echo "connected to $PORT @ $BAUD  (type + Enter to send, Ctrl-C to quit)" >&2
cat "$PORT" &        # device  -> your screen
reader_pid=$!
cat > "$PORT"        # your keyboard -> device
