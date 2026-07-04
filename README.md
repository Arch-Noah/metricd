# metricd

Ultra-lightweight system metrics daemon written in C++20.

Reads `/proc` directly, broadcasts JSON over a UNIX domain socket.

## Quick Start

```bash
cmake -B build && cmake --build build
./build/src/metricd
```

## Configuration

Config file: `~/.config/metricd/metricd.toml` (or `METRICD_CONFIG` env var)

```toml
socket_path = "/run/user/1000/metricd.sock"
interval = 1

[collectors]
cpu         = true
memory      = true
disk        = true
network     = true
gpu         = true
temperature = true
```

Environment variables override config file: `METRICD_SOCKET`, `METRICD_INTERVAL`.

## Client

Connect to the socket and read newline-delimited JSON:

```python
import socket, json
s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
s.connect("/run/user/1000/metricd.sock")
for line in s.makefile("r"):
    print(json.loads(line))
```

## Tests

```bash
ctest --test-dir build
```

## Project

See `docs/ARCHITECTURE.md` and `docs/PROTOCOL.md`.
