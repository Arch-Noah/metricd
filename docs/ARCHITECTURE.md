# Metricd Architecture

## Overview

```
┌────────────────────────────────────────────────────────────────────┐
│                          metricd daemon                            │
│                                                                    │
│  ┌──────────────┐    ┌─────────────────┐    ┌──────────────────┐  │
│  │ CpuCollector  │    │ MemoryCollector │    │ DiskCollector    │  │
│  │ /proc/stat    │    │ /proc/meminfo   │    │ /proc/mounts     │  │
│  │               │    │                 │    │ + statvfs()      │  │
│  └──────┬───────┘    └───────┬─────────┘    └──────┬───────────┘  │
│         │                    │                     │              │
│  ┌──────┴───────┐    ┌──────┴─────────┐    ┌──────┴───────────┐  │
│  │NetCollector  │    │  Collectors    │    │  JsonSerializer  │  │
│  │/proc/net/dev │    │  (ICollector)  │    │  (nlohmann/json) │  │
│  └──────┬───────┘    └──────┬─────────┘    └──────────────────┘  │
│         │                    │                                     │
│         └────────┬───────────┘                                     │
│                  │                                                 │
│         ┌────────┴────────┐                                        │
│         │  Server (IPC)   │                                        │
│         │  io_uring       │──── broadcasts JSON ────► Clients      │
│         │  UNIX socket    │                                        │
│         │  timerfd        │                                        │
│         └─────────────────┘                                        │
│                                                                    │
│  ┌──────────────┐   ┌────────────────┐                            │
│  │ Config       │   │ Daemon         │                            │
│  │ TOML parsing │   │ signal handler │                            │
│  └──────────────┘   └────────────────┘                            │
└────────────────────────────────────────────────────────────────────┘
```

## Layers

### 1. Collectors (`include/metricd/collectors/`)

Interface `ICollector` with pure virtual `collect() -> json` and `name() -> string`.

Four implementations, each reading directly from `/proc` with zero dynamic allocations per line:

| Collector | Source | Strategy |
|---|---|---|
| `CpuCollector` | `/proc/stat` | Double snapshot (100ms), parse 1st line |
| `MemoryCollector` | `/proc/meminfo` | Single pass, extract 2 fields |
| `DiskCollector` | `/proc/mounts` + `statvfs()` | Per-filesystem space |
| `NetworkCollector` | `/proc/net/dev` | Double snapshot, skip headers |

### 2. IPC Server (`include/metricd/ipc/Server.hpp`)

- Event-driven with **io_uring** (Linux 5.1+)
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

## Dependencies

- C++20
- nlohmann/json (v3.11.3, fetched by CMake)
- liburing (system package)
- pthread

## Build

```bash
cmake -B build && cmake --build build
ctest --test-dir build
```
