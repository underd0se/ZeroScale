# ZeroScale C-TUI

> **Ultra-lightweight, high-performance native C99 Terminal User Interface for Tailscale on Asuswrt-Merlin Routers.**

ZeroScale C-TUI is a standalone, pure C99 implementation of the ZeroScale Tailscale management suite. Designed specifically for memory-constrained Asuswrt-Merlin routers, it delivers an instant, flicker-free terminal experience with full mouse interactivity, color-coded peer topology, interactive diagnostics, and complete daemon configuration.

---

## 🚀 Key Advantages Over Shell Scripts

| Metric / Capability | Pure Shell (`ash`/`bash`) | **ZeroScale C-TUI (`C99`)** |
|:---|:---:|:---:|
| **Binary Size** | Multiple scripts / disk files | **~140 KB single static binary** |
| **RAM Footprint** | ~5–12 MB (spawning subshells) | **< 800 KB resident memory** |
| **CPU Usage (Idle/Poll)** | 2–5% spikes on subshell forks | **0.0% CPU (event-driven tick)** |
| **Rendering Engine** | ANSI escapes (flickers on refresh) | **Double-buffered differential matrix (0% flicker)** |
| **Mouse Navigation** | Conflicts with terminal selection | **Native SGR 1006 mouse tracking + Option-Drag copy** |
| **Dependencies** | Requires `coreutils`, `awk`, `sed` | **Zero external libraries (pure static musl)** |

---

## ✨ Features

### 1. 🖥️ Live Monitor Dashboard (`VIEW_DASHBOARD`)
* **Real-time Status Cards:** Live indicators for Daemon status, Tailnet connectivity, Operating Mode (`Userspace` / `Kernel`), Watchdog Keepalive state, Exit Node advertisement, and advertised Subnet Routes.
* **Interactive Action Bar:** Direct mouse-clickable buttons and keyboard shortcuts for `(U)p`, `(D)own`, `(R)estart`, `(S)tart`, `S(t)op`, `(L)ogs`, `(C)onfiguration`, and `(Q)uit`.
* **Color-Coded Peer Topology:**
  * 🟣 **Self (Router):** Bold Magenta (`router - self`)
  * 🟡 **Exit Nodes:** Vivid Gold / Yellow (`offers exit node`)
  * 🟢 **Active (Direct WireGuard):** Bright Green (`active; direct ...`)
  * 🔵 **Idle (Online):** Soft Cyan (`idle; ...`)
  * ⚫ **Offline Nodes:** Dim Gray (`offline, last seen ...`)
* **Smooth Scrolling:** Scroll through large peer lists with the mouse wheel or arrow keys.

### 2. 🔍 Modal Peer Inspector (`VIEW_PEER_DETAIL`)
* Select any peer with mouse click or `Enter` to open a floating, bordered modal inspector.
* Inspect Node Name, Tailscale IP, OS Platform, Account Owner, Connection Endpoint (Direct vs. DERP Relay), and real-time Tailnet status.
* Interactive actions directly inside modal:
  * Press `p`: Instant ICMP ping test.
  * Press `t`: Real Tailscale wireguard latency ping.
  * Press `c` / `Esc`: Close inspector.

### 3. ⚙️ Unified Configuration & Service Management (`VIEW_CONFIG`)
Consolidated, interactive settings management with instant persistence to `/jffs/addons/zeroscale.d/zeroscale.cfg`:
* **Section 1: Daemon & Health Monitor** — Toggle Watchdog Keepalive, Persistent Settings, and Boot Autostart (`/jffs/scripts/post-mount`).
* **Section 2: Tailscale Routing & Mode** — Toggle `Userspace` ⟷ `Kernel` mode, Exit Node advertisement, Subnet Routes advertisement, and interactive Subnet CIDR editor.
* **Section 3: Interface & Logging** — Cycle status check intervals (`10s` ➔ `300s`) and edit Event Log row retention limits.
* **Section 4: Notifications & Automation** — Configure AMTM email alerts and scheduled daily autoupdates.
* **Section 5: Binary & Maintenance** — Check & update Tailscale binaries, reset daemon state, and run full install/uninstall workflows.

### 4. 📜 Full-Screen Event Log Viewer (`VIEW_LOGS`)
* Displays `/jffs/addons/zeroscale.d/zeroscale.log` with syntax highlighting (`INFO`, `WARN`, `FAIL`, `ONLINE`).
* Opens directly to the most recent log entries filling the entire screen.
* Supports `↑`/`↓`, `PgUp`/`PgDn`, `g` (top), `G` (bottom), and `r` (live reload).

### 5. 🌟 Centered Multi-Stage ASCII Splash
* Dynamically calculates terminal geometry and centers the iconic ZeroScale banner both horizontally and vertically on startup and exit.

---

## 📦 Asus Router Binary Releases

ZeroScale C-TUI binaries are compiled statically using `musl-libc`, guaranteeing standalone execution on Asuswrt-Merlin without external dependencies:

| Release Artifact | Target Architecture | Compatible Asus Router Models |
|:---|:---|:---|
| [`zeroscale-tui-v0.1.0-armv7-linux-musl`](https://raw.githubusercontent.com/underd0se/ZeroScale/c-tui/tui/bin/release/zeroscale-tui-v0.1.0-armv7-linux-musl) | `armv7l` (32-bit ARM Cortex-A7/A9/A15) | RT-AX86U, RT-AC86U, RT-AC68U, RT-AX58U, RT-AX56U |
| [`zeroscale-tui-v0.1.0-arm64-linux-musl`](https://raw.githubusercontent.com/underd0se/ZeroScale/c-tui/tui/bin/release/zeroscale-tui-v0.1.0-arm64-linux-musl) | `aarch64` (64-bit ARM Cortex-A53/A72) | RT-AX88U Pro, GT-AXE16000, GT6, RT-BE96U, GT-BE98 |

---

## 🛠️ Quick Installation on Asuswrt-Merlin

```sh
# For 32-bit ARMv7 routers (RT-AX86U, RT-AC86U, etc.):
curl -fsSL https://raw.githubusercontent.com/underd0se/ZeroScale/c-tui/tui/bin/release/zeroscale-tui-v0.1.0-armv7-linux-musl \
  -o /jffs/scripts/zeroscale-tui

# For 64-bit ARM64 routers (RT-AX88U Pro, GT-AXE16000, etc.):
# curl -fsSL https://raw.githubusercontent.com/underd0se/ZeroScale/c-tui/tui/bin/release/zeroscale-tui-v0.1.0-arm64-linux-musl \
#   -o /jffs/scripts/zeroscale-tui

# Make executable and symlink to PATH
chmod 755 /jffs/scripts/zeroscale-tui
ln -sf /jffs/scripts/zeroscale-tui /opt/bin/zeroscale-tui

# Launch ZeroScale C-TUI
zeroscale-tui
```

---

## 💻 Building from Source (macOS / Linux)

Cross-compiling requires **[Zig](https://ziglang.org/)**:

```bash
# 1. Install Zig via Homebrew (macOS)
brew install zig

# 2. Clone repository
git clone -b c-tui https://github.com/underd0se/ZeroScale.git
cd ZeroScale

# 3. Build release binaries for Asus routers (ARMv7 & ARM64)
./tui/build-all.sh
```

---

## 🎮 Navigation & Keyboard Controls

| Context | Key / Action | Description |
|:---|:---:|:---|
| **Anywhere** | `Mouse Left-Click` | Activate buttons, select peers, toggle settings |
| **Anywhere** | `Option (⌥) + Drag` | Native terminal text selection and copying (macOS) |
| **Dashboard** | `↑` / `↓` / `Mouse Wheel` | Scroll through peer network table |
| **Dashboard** | `Enter` / Double Click | Open Peer Inspector modal |
| **Dashboard** | `u` / `d` | Tailscale `up` (connect) / `down` (disconnect) |
| **Dashboard** | `r` / `s` / `t` | Tailscale daemon restart / start / stop |
| **Dashboard** | `c` | Open Configuration & Service Menu |
| **Dashboard** | `l` | Open Event Log Viewer |
| **Dashboard** | `q` / `Esc` | Clean exit with shutdown splash |
| **Peer Inspector** | `p` | ICMP Ping peer node |
| **Peer Inspector** | `t` | WireGuard latency ping |
| **Peer Inspector** | `c` / `Esc` | Close modal |
| **Log Viewer** | `↑` / `↓` / `PgUp` / `PgDn` | Scroll logs |
| **Log Viewer** | `g` / `G` | Jump to Top / Bottom of logs |
| **Log Viewer** | `r` | Reload log file |
| **Log Viewer** | `q` / `Esc` | Return to Live Monitor |

---

## 📄 License

GPL-3.0 License. Designed with ❤️ for the Asuswrt-Merlin and Tailscale community.
