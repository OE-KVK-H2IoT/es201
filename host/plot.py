#!/usr/bin/env python3
"""Plot numeric series from a serial log — the host half of "log data, then look".

The es201 demos narrate as `key=value` (e.g. `x=2011 y=1950`, `joystick x=2028
y=1983`). This script pulls every such number out of a log and plots each key as
a line over sample index. No fancy parsing, no per-demo config.

Capture with the pure-bash logger, then plot:
    ./serial-log.sh | tee adc.log        # capture (Ctrl-C to stop)
    python3 host/plot.py adc.log         # plot every key it finds (value vs sample)
    python3 host/plot.py adc.log x y     # only these keys
    python3 host/plot.py adc.log x --hist  # distribution (ADC noise: Gaussian or not?)
    ./serial-log.sh | python3 host/plot.py     # or pipe straight in (then Ctrl-C)

Needs matplotlib:  pip install matplotlib
"""
import sys, os, re

PAIR = re.compile(r'([A-Za-z_]\w*)\s*=\s*(-?\d+(?:\.\d+)?)')

def main():
    args = sys.argv[1:]
    hist = "--hist" in args                 # --hist: distribution instead of time series
    args = [a for a in args if a != "--hist"]
    infile = None
    if args and os.path.exists(args[0]):
        infile = args[0]
        args = args[1:]
    wanted = set(args)                      # keys to keep; empty = all

    series = {}                             # key -> [values in order seen]
    source = open(infile) if infile else sys.stdin
    try:
        for line in source:
            for key, value in PAIR.findall(line):
                if wanted and key not in wanted:
                    continue
                series.setdefault(key, []).append(float(value))
    except KeyboardInterrupt:
        pass                                # piping live: Ctrl-C ends capture

    if not series:
        sys.exit("no 'key=value' numbers found — is the log in key=value form?")

    import matplotlib
    import matplotlib.pyplot as plt
    import statistics
    if hist:                                # distribution view — e.g. ADC noise at rest
        for key, values in series.items():
            sd = statistics.pstdev(values) if len(values) > 1 else 0.0
            plt.hist(values, bins=40, alpha=0.6, label=f"{key}  (n={len(values)}, σ={sd:.1f})")
        plt.xlabel("value"); plt.ylabel("count")
        plt.title(f"es201 histogram: {infile or 'stdin'}")
    else:                                   # time series — value vs sample index
        for key, values in series.items():
            plt.plot(values, label=f"{key}  (n={len(values)})")
        plt.xlabel("sample index"); plt.ylabel("value")
        plt.title(f"es201 serial log: {infile or 'stdin'}")
    plt.legend()
    plt.grid(True, alpha=0.3)

    out = (os.path.splitext(infile)[0] if infile else "serial") + ".png"
    plt.savefig(out, dpi=110, bbox_inches="tight")
    print(f"wrote {out}  ({sum(len(v) for v in series.values())} points across {len(series)} series)")
    if matplotlib.get_backend().lower() != "agg":
        plt.show()                          # pop up a window only if we have a display

if __name__ == "__main__":
    main()
