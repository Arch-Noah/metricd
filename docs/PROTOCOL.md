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
    "cpu_usage_percent": 23.5
  },
  "memory": {
    "total_mb": 15226,
    "used_mb": 9684,
    "available_mb": 5542,
    "used_percent": 63.6
  },
  "disk": {
    "filesystems": [
      {
        "mount": "/",
        "device": "/dev/nvme0n1p6",
        "total_gb": 459.3,
        "used_gb": 405.7,
        "available_gb": 42.6,
        "used_percent": 88.3
      }
    ]
  },
  "network": {
    "interface": "eth0",
    "download_speed_mb_s": 1.2,
    "upload_speed_mb_s": 0.3
  }
}
```

### Frequency

Broadcast interval is configurable (default: 1 second) via `METRICD_INTERVAL` env var or `interval` in config file.

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
- Clients only read — the server never expects messages from clients
- A single `\n` delimiter separates messages (NDJSON)
