#!/usr/bin/env python3
"""
metricd client — displays all metrics consolidated per tick.
"""

import socket
import json
import sys
import os

SOCKET_PATH = os.environ.get("METRICD_SOCKET", "/run/user/1000/metricd.sock")


def main():
    sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    try:
        sock.connect(SOCKET_PATH)
    except FileNotFoundError:
        print(f"metricd socket not found: {SOCKET_PATH}", file=sys.stderr)
        return 1

    print(f"Connected to {SOCKET_PATH}", file=sys.stderr)

    buf = {}
    prev_ts = 0

    def flush():
        nonlocal buf
        parts = []

        cpu = buf.get("cpu", {})
        if cpu:
            parts.append(f"CPU: {cpu.get('cpu_usage_percent', 0):5.1f}%")

        mem = buf.get("memory", {})
        if mem:
            parts.append(f"MEM: {mem.get('used_percent', 0):5.1f}%")

        disk = buf.get("disk", {})
        if disk:
            total = sum(fs.get("total_gb", 0) for fs in disk.get("filesystems", []))
            used = sum(fs.get("used_gb", 0) for fs in disk.get("filesystems", []))
            if total:
                parts.append(f"DSK: {used:.0f}/{total:.0f}GB ({used/total*100:.0f}%)")

        net = buf.get("network", {})
        if net:
            down = sum(i.get("download_speed_bps", 0) for i in net.get("interfaces", []))
            up = sum(i.get("upload_speed_bps", 0) for i in net.get("interfaces", []))
            parts.append(f"NET: ↓{down//1000:>4}kb ↑{up//1000:>4}kb")

        gpu = buf.get("gpu", {})
        if gpu:
            parts.append(f"GPU: {gpu.get('load_percent', 0):.0f}% {gpu.get('temp_c', 0):.0f}°C")

        temp = buf.get("temperature", {})
        if temp:
            sensors = temp.get("sensors", [])
            if sensors:
                max_t = max(s.get("temp_c", 0) for s in sensors)
                parts.append(f"TMP: {max_t:.0f}°C")

        bat = buf.get("battery", {})
        if bat:
            batteries = bat.get("batteries", [])
            if batteries:
                b = batteries[0]
                cap = b.get("capacity_percent")
                if cap is not None:
                    parts.append(f"BAT: {cap}%")

        if parts:
            print("  |  ".join(parts), flush=True)

        buf = {}

    try:
        for line in sock.makefile("r"):
            msg = json.loads(line)
            ts = msg.get("timestamp", 0)
            if ts != prev_ts and prev_ts != 0:
                flush()
            prev_ts = ts
            buf[msg.get("type")] = msg
    except KeyboardInterrupt:
        pass
    finally:
        flush()
        sock.close()

    return 0


if __name__ == "__main__":
    sys.exit(main())
