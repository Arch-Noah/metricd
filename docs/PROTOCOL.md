# Metricd Protocol

## Transport

- **Type**: Unix Domain Socket (`SOCK_STREAM`)
- **Default path**: `/run/user/<uid>/metricd.sock`
- **Permissions**: `0600` (owner-only), created with `umask(0077)`
- **Overridable via**: `METRICD_SOCKET` env var or config file

## Data Format

The server broadcasts newline-delimited JSON (NDJSON). Each line is a complete JSON object terminated by `\n`.

Each message represents one collector and always includes these fields:

| Field | Type | Description |
|-------|------|-------------|
| `type` | string | Collector name (e.g. `"cpu"`, `"temperature"`) |
| `timestamp` | integer | Unix epoch seconds (UTC) |
| `proto_version` | integer | Protocol version — currently `1` |

### Message Types

#### cpu

```json
{
  "type": "cpu",
  "timestamp": 1735991234,
  "proto_version": 1,
  "cpu_usage_percent": 23.5,
  "clock_ghz": 3.4,
  "threads": 16,
  "per_core": [12.3, 45.6, 10.2, 34.1]
}
```

- `cpu_usage_percent` — delta-based from `/proc/stat` (0–100)
- `clock_ghz` — current CPU frequency from `/proc/cpuinfo`
- `threads` — logical core count
- `per_core` — array of per-core usage %; only present if `per_core = true` in config (disabled by default)

Counter reset guard: if a per-core or aggregate delta is negative or exceeds `1e15`, the collector resets and reports 0% for that tick (handles CPU hotplug, suspend/resume, 32-bit counter overflow).

#### memory

```json
{
  "type": "memory",
  "timestamp": 1735991234,
  "proto_version": 1,
  "total_mb": 15226,
  "used_mb": 9684,
  "available_mb": 5542,
  "used_percent": 63.6,
  "swap_total_mb": 2048,
  "swap_used_mb": 128,
  "swap_used_percent": 6.25
}
```

#### disk

```json
{
  "type": "disk",
  "timestamp": 1735991234,
  "proto_version": 1,
  "filesystems": [
    {
      "mount": "/",
      "device": "/dev/nvme0n1p6",
      "total_gb": 459.3,
      "used_gb": 405.7,
      "available_gb": 42.6,
      "used_percent": 88.3,
      "disk_read_bytes": 123456789,
      "disk_write_bytes": 987654321
    }
  ]
}
```

- `disk_read_bytes` / `disk_write_bytes` — cumulative counters from `/proc/diskstats` (since boot)

#### network

```json
{
  "type": "network",
  "timestamp": 1735991234,
  "proto_version": 1,
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
}
```

- `*_speed_bps` — delta-based bytes/second between consecutive ticks (0 on first frame)
- `*_cumulative` — total bytes since boot
- `*_today_bytes` — bytes since daemon start
- Counter reset guard: if a delta is negative or exceeds `1e15`, speed is reported as 0 for that tick (handles interface reset, overflow).

#### gpu

```json
{
  "type": "gpu",
  "timestamp": 1735991234,
  "proto_version": 1,
  "load_percent": 45.0,
  "memory_total_gb": 8.0,
  "memory_used_gb": 2.5,
  "temp_c": 65.0,
  "label": "NVIDIA GeForce RTX 3070"
}
```

- Present only if NVIDIA GPU detected and `nvidia-smi` is available
- Calls `nvidia-smi` at most every 5 seconds (internal cache)

#### temperature

```json
{
  "type": "temperature",
  "timestamp": 1735991234,
  "proto_version": 1,
  "sensors": [
    { "label": "CPU Package", "temp_c": 45.0 },
    { "label": "NVMe", "temp_c": 35.2 },
    { "label": "acpitz", "temp_c": 30.0 }
  ]
}
```

- Reads from `/sys/class/hwmon/hwmon*/temp*_input` with labels from `temp*_label`
- Falls back to `/sys/class/thermal/thermal_zone*/temp` if hwmon is unavailable
- Each sensor has a `label` (e.g. `"CPU Package"`, `"NVMe"`, `"acpitz"`) and `temp_c` in Celsius

#### battery

```json
{
  "type": "battery",
  "timestamp": 1735991234,
  "proto_version": 1,
  "batteries": [
    {
      "name": "BAT0",
      "capacity_percent": 78,
      "status": "Discharging",
      "voltage_now": 11.52,
      "current_now": -1.234,
      "power_now": 14.21,
      "energy_now": 48.5,
      "energy_full": 62.0,
      "energy_full_design": 65.0,
      "charge_now": 0.981,
      "charge_full": 1.592,
      "charge_full_design": 3.950,
      "health_percent": 95.4,
      "temp_c": 30.5,
      "cycle_count": 342,
      "model": "Primary",
      "manufacturer": "Hewlett-Packard",
      "technology": "Li-ion"
    }
  ]
}
```

- Enumerates `/sys/class/power_supply/*/type` and filters for `"Battery"` (supports multiple batteries and varying paths like `BAT0`, `BAT1`, `CMB0`, etc.)
- All fields are optional — only available sysfs files are included
- `capacity_percent` — 0–100 battery charge level
- `status` — `"Charging"`, `"Discharging"`, `"Full"`, `"Not charging"`, `"Unknown"`
- `voltage_now` — current voltage in volts (converted from µV)
- `current_now` — current in amps (converted from µA; negative = discharging)
- `power_now` — power in watts (converted from µW)
- `energy_*` — energy in watt-hours (converted from µWh)
- `charge_*` — charge in ampere-hours (converted from µAh)
- `temp_c` — battery temperature in Celsius (converted from tenths of °C)
- `health_percent` — `energy_full / energy_full_design * 100` (falls back to `charge_full / charge_full_design`)
- `cycle_count` — number of charge/discharge cycles
- `model`, `manufacturer`, `technology` — battery identity info

### Collectors disabled

When a collector is disabled (`false` in config), its message type is omitted from the broadcast entirely. There is no null/empty placeholder — clients should treat absence as "collector disabled".

### Frequency

Broadcast interval is configurable (default: 1 second) via `METRICD_INTERVAL` env var or `interval` in config file. All enabled collectors are sampled and broadcast together on each tick.

## Behaviour

### Non-blocking broadcast

If a client is slow and its socket send buffer fills up, the message is dropped for that client. `metricd` never blocks its collection loop due to a slow consumer — the slow client is disconnected.

### Multi-client

Multiple simultaneous connections are supported. Each connected client receives every broadcast tick.

### Protocol versioning

The `proto_version` field (currently `1`) allows clients to detect incompatible changes. The major version will be incremented on breaking changes.

## Configuration

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
per_core    = false
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
            msg = json.loads(line)
            print(f"[{msg['type']}] {msg}")
```

## Notes

- The socket is user-local (`/run/user/<uid>/`), permissions `0600`, no root required
- Les collecteurs sont persistants et comparent les snapshots entre ticks pour les deltas
- Le GPU collector interroge `nvidia-smi` au maximum toutes les 5s (cache interne)
- Clients only read — the server never expects messages from clients
- A single `\n` delimiter separates messages (NDJSON)
