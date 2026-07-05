# Metricd Architecture

## Overview

```
┌──────────────────────────────────────────────────────────────────────────┐
│                          metricd daemon                                  │
│                                                                          │
│  ┌──────────────┐  ┌─────────────────┐  ┌──────────────┐  ┌───────────┐ │
│  │ CpuCollector  │  │ MemoryCollector │  │ DiskCollector│  │NetCollector│ │
│  │ /proc/stat    │  │ /proc/meminfo   │  │ /proc/mounts │  │/proc/net   │ │
│  │ /proc/cpuinfo │  │ + swap fields   │  │ + statvfs()  │  │ /dev       │ │
│  │ thermal zones  │  │                 │  │ /proc/disk-  │  │            │ │
 │  │                │  │                 │  │ /proc/disk-  │  │            │ │
│  └──────┬────────┘  └──────┬──────────┘  └──────┬───────┘  └─────┬─────┘ │
│         │                  │                     │                │       │
│  ┌──────┴────────┐  ┌──────┴──────────┐  ┌──────┴───────┐  ┌─────┴─────┐ │
│  │ GpuCollector  │  │ BatteryCollector│  │TempCollector │  │JsonSerial-│ │
│  │ nvidia-smi    │  │ power_supply/*  │  │ /sys/class/  │  │izer       │ │
│  │ (5s cache)    │  │ (filter Batte-  │  │ hwmon/therm  │  │(nlohmann) │ │
│  │               │  │  ry)            │  │ al           │  │           │ │
│  └──────┬────────┘  └──────┬──────────┘  └──────┬───────┘  └───────────┘ │
│         │                  │                     │                        │
│         └────────┬─────────┴─────────────────────┴──────────────────┘     │
│                  │                                                       │
│         ┌────────┴────────┐                                              │
│         │  Server (IPC)   │                                              │
│         │  io_uring       │──── broadcasts JSON ────► Clients            │
│         │  UNIX socket    │                                              │
│         │  timerfd        │                                              │
│         └─────────────────┘                                              │
│                                                                          │
│  ┌──────────────┐   ┌────────────────┐                                  │
│  │ Config       │   │ Daemon         │                                  │
│  │ TOML parsing │   │ signal handler │                                  │
│  └──────────────┘   └────────────────┘                                  │
└──────────────────────────────────────────────────────────────────────────┘
```

## Layers

### 1. Collectors (`include/metricd/collectors/`)

Interface `ICollector` with pure virtual `collect() -> json` and `name() -> string`.

Seven implementations, each reading directly from `/proc` or system interfaces with zero dynamic allocations per line:

| Collector | Source | Fields |
|---|---|---|
| `CpuCollector` | `/proc/stat`, `/proc/cpuinfo`, thermal zones | usage %, clock GHz, temp °C, threads, per-core % (optionnel) |
| `MemoryCollector` | `/proc/meminfo` | RAM + swap totals/usage/percent |
| `DiskCollector` | `/proc/mounts` + `statvfs()` + `/proc/diskstats` | Per-filesystem space + I/O bytes |
| `NetworkCollector` | `/proc/net/dev` | All interfaces, speed (bps), cumulative + today bytes |
| `GpuCollector` | `nvidia-smi` subprocess (5s cache) | Load, memory, temp, label |
| `BatteryCollector` | `/sys/class/power_supply/*/type` filter Battery | Per-battery capacity, status, voltage, current, energy, health, temp, cycles |
| `SystemCollector` | `/proc/loadavg`, `/proc/uptime` | Load 1/5/15min, procs running/total, uptime |

**Performance**: Collectors sont des instances réutilisées (pas de recréation par tick). Plus aucun `sleep_for()` bloquant — les deltas sont calculés entre les ticks du `timerfd`. Le GPU collector cache les résultats 5s pour éviter de spawner `nvidia-smi` trop souvent.

### 2. IPC Server (`include/metricd/ipc/Server.hpp`)

- Event-driven avec **io_uring** (Linux 5.1+)
- UNIX domain socket (`SOCK_STREAM`)
- Timer via `timerfd` for periodic broadcasts
- Pub/sub: single `broadcast()` writes to all connected clients
- Zero-copy writes via io_uring SQEs

### 3. Core (`include/metricd/core/`)

- **Config**: TOML-like parser, environment variable overrides (`METRICD_SOCKET`, `METRICD_INTERVAL`, `METRICD_CONFIG`)
- **Daemon**: Orchestrator with signal handling (SIGINT/SIGTERM → graceful shutdown), owns Server

### 4. Logger (`Logger/`)

- Thread-safe, rotating log levels, color output, file output

## Data Flow

```
timerfd tick → collectMetrics() → broadcast(JSON) → queueWrite per client
```

Les collecteurs maintiennent leur état interne (snapshots précédents) pour calculer les deltas sans sleep.

## Configuration

```toml
socket_path = "/run/user/1000/metricd.sock"
interval = 1

[collectors]
cpu     = true
memory  = true
disk    = true
network = true
gpu     = true
battery = true
per_core = false    # per-core CPU désactivé par défaut (économie)
```

## Dependencies

- C++20
- nlohmann/json (v3.11.3, fetched by CMake)
- liburing (system package)
- pthread

## Build

```bash
cmake -B build && cmake --build build
ctest --test-dir build
sudo cmake --install build
```
