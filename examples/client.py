#!/usr/bin/env python3
"""
Minimal metricd client.
Reads newline-delimited JSON from the Unix socket and prints metrics.
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

    try:
        for line in sock.makefile("r"):
            metrics = json.loads(line)
            cpu = metrics.get("cpu", {}).get("cpu_usage_percent", 0)
            mem = metrics.get("memory", {}).get("used_percent", 0)
            print(f"CPU: {cpu:6.1f}%  MEM: {mem:6.1f}%")
    except KeyboardInterrupt:
        pass
    finally:
        sock.close()

    return 0


if __name__ == "__main__":
    sys.exit(main())
