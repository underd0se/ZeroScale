# Changelog ─ ZeroScale C-TUI

All notable changes to the ZeroScale C-TUI project are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

## [0.1.0] - 2026-08-29

### 🚀 Initial Public Release

ZeroScale C-TUI `v0.1.0` is the first native C99 implementation of the ZeroScale Tailscale manager for Asuswrt-Merlin routers.

#### 🌟 Added
* **Native C99 Engine:** Single-header `termbox2` backend delivering high-performance, double-buffered differential rendering with 0% screen flicker and 0% CPU consumption during idle polling.
* **Full Mouse Interactivity (SGR 1006):**
  * Instant left-click execution on the Action Bar: `(U)p`, `(D)own`, `(R)estart`, `(S)tart`, `S(t)op`, `(L)ogs`, `(C)onfiguration`, `(Q)uit`.
  * Interactive peer row selection and double-click modal trigger.
  * Mouse wheel smooth scrolling across Peer lists and Event logs.
  * Seamless support for native macOS text selection via `Option (⌥) + Drag`.
* **Color-Coded Peer Topology:**
  * Bold Magenta for self router node (`router - self`).
  * Vivid Gold/Yellow for advertised exit nodes (`offers exit node`).
  * Vivid Green for active Direct WireGuard sessions (`active; direct ...`).
  * Soft Cyan for idle nodes (`idle; ...`).
  * Dim Gray for offline nodes (`offline, last seen ...`).
* **Modal Peer Inspector (`VIEW_PEER_DETAIL`):**
  * High-contrast Unicode box borders (`┌─┐`, `│ │`, `└─┘`).
  * Real-time network telemetry: IP address, OS platform, owner, direct/relay endpoint details, and status.
  * In-modal live ICMP ping (`p`) and WireGuard latency ping (`t`).
* **Unified Configuration & Service Management (`VIEW_CONFIG`):**
  * Consolidated Daemon monitor, Tailscale Routing, Logging, AMTM email alerts, and Cron Autoupdate into a single 5-section menu.
  * Instant configuration persistence to `/jffs/addons/zeroscale.d/zeroscale.cfg`.
  * Built-in Subnet Route CIDR editor modal.
  * Built-in Tailscale service installation and uninstallation workflows.
* **Full-Height Event Log Viewer (`VIEW_LOGS`):**
  * Automatic tail-end positioning (most recent logs fill the entire viewport).
  * Level colorization (`INFO`, `WARN`, `FAIL`, `ONLINE`).
  * Keyboard navigation (`g`/`G`, `PgUp`/`PgDn`, `↑`/`↓`) and live refresh (`r`).
* **Centered Multi-Stage ASCII Splash:**
  * Dynamically calculates terminal geometry and centers the iconic ZeroScale banner on startup and exit.
* **Universal Auto-Detecting Installer (`install.sh`):**
  * Automatically detects router CPU architecture (`ARMv7` vs `ARM64`) and pulls the corresponding release binary.
  * Seamlessly detects and migrates legacy Tailmon configurations (`tailmon.cfg` ➔ `zeroscale.cfg`), cleans legacy post-mount hooks, and sets up `/opt/bin/tailmon` backward-compatibility symlinks.
* **Asuswrt-Merlin Static Musl Binaries:**
  * `armv7-linux-musl` (140 KB) for 32-bit ARM Asus routers (RT-AX86U, RT-AC86U, RT-AC68U).
  * `arm64-linux-musl` (152 KB) for 64-bit ARM Asus routers (RT-AX88U Pro, GT-AXE16000, GT6).

#### 🔧 Binary & Resource Benchmarks
* **Binary Size:** 140 KB (ARMv7) / 152 KB (ARM64)
* **RAM Usage:** < 800 KB resident memory
* **CPU Usage:** 0.0% idle, < 0.1% on tick
* **Dependencies:** 0 shared libraries, 0 external runtime packages
