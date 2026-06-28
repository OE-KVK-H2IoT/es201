# host/ — log and look at the data on your PC

The "measure it, then *see* it" half of the labs. The board narrates over serial
as `key=value`; capture that on the host and plot it — no per-demo wiring.

```bash
# from src/es201/
./serial-log.sh | tee adc.log      # 1. capture (pure bash; Ctrl-C to stop)
python3 host/plot.py adc.log       # 2. plot every key=value series it finds
python3 host/plot.py adc.log x y   #    ...or just these keys
./serial-log.sh | python3 host/plot.py   # or pipe straight in (Ctrl-C ends it)
```

`plot.py` writes a `.png` next to the log and pops a window if you have a display.
It needs matplotlib: `pip install matplotlib`.

This is the same loop the BSc Embedded Systems course uses — log on the target,
analyse on the host — and it's the natural front for the L2 noise/rate work and
the capstone scope: pull the numbers off the wire and let a plot tell you what's
going on.
