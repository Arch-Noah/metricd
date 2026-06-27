# Metricd Protocol

## Transport

- **Type**: Unix Domain Socket (`SOCK_STREAM`)
- **Default path**: `/run/user/<uid>/metricd.sock`
- **Overridable via**: `METRICD_SOCKET` env var or config file

## Data Format

The server broadcasts newline-delimited JSON (NDJSON). Each message is a complete JSON object terminated by `\n`.

### Message Structure

```json
{
  "cpu": {
    "cpu_usage_percent": 23.5,
    "clock_ghz": 3.4,
    "temp_c": 45.0,
    "threads": 16,
    "per_core": [12.3, 45.6, ...]
  },
  "memory": {
    "total_mb": 15226,
    "used_mb": 9684,
    "available_mb": 5542,
    "used_percent": 63.6,
    "swap_total_mb": 2048,
    "swap_used_mb": 128,
    "swap_used_percent": 6.25
  },
  "gpu": {
    "load_percent": 45.0,
    "memory_total_gb": 8.0,
    "memory_used_gb": 2.5,
    "temp_c": 65.0,
    "label": "NVIDIA GeForce RTX ..."
  },
  "network": {
    "interfaces": [
      {
        "name": "wlp2s0",
        "download_speed_bps": 1234567,
        "upload_speed_bps": 345678,
        "rx_bytes_cumulative": 9876543210,
        "tx_bytes_cumulative": 1234567890,
        "download_today_bytes": 500000000,
        "upload_today_bytes": 100000000
      }
    ]
  },
  "disk": {
    "filesystems": [
      {
        "mount": "/",
        "device": "/dev/nvme0n1p6",
        "total_gb": 459.3,
        "used_gb": 405.7,
        "available_gb": 42.6,
        "used_percent": 88.3,
        "disk_read_bytes": 123456,
        "disk_write_bytes": 789012
      }
    ]
  }
}
```

Notes:
- `cpu.per_core` n'est présent que si `per_core = true` dans la config (désactivé par défaut)
- `gpu` est `{}` si aucun GPU NVIDIA ou si `nvidia-smi` n'est pas disponible
- `network.download_speed_bps` et `upload_speed_bps` = 0 sur la première trame (delta)
- `disk.disk_read_bytes` / `disk_write_bytes` = compteurs cumulatifs depuis boot

### Frequency

Broadcast interval is configurable (default: 1 second) via `METRICD_INTERVAL` env var or `interval` in config file.

## Configuration

```toml
socket_path = "/run/user/1000/metricd.sock"
interval = 1

[collectors]
cpu       = true
memory    = true
disk      = true
network   = true
gpu       = true
per_core  = false
```

## Client Example (Python)

```python
import socket, json

s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
s.connect("/run/user/1000/metricd.sock")

while True:
    data = s.recv(4096)
    if not data:
        break
    for line in data.decode().split("\n"):
        if line:
            metrics = json.loads(line)
            print(f"CPU: {metrics['cpu']['cpu_usage_percent']:.1f}%")
```

## Client Example (JavaScript / Bun)

```javascript
const sock = Bun.connect("/run/user/1000/metricd.sock", data => {
    const metrics = JSON.parse(data.toString());
    console.log(`CPU: ${metrics.cpu.cpu_usage_percent}%`);
});
```

## Notes

- The socket is user-local (`/run/user/<uid>/`), no root required
- Les collecteurs sont persistants et comparent les snapshots entre ticks pour les deltas
- Le GPU collector interroge `nvidia-smi` au maximum toutes les 5s (cache interne)
- Clients only read — the server never expects messages from clients
- A single `\n` delimiter separates messages (NDJSON)
