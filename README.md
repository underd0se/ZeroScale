# ZeroScale
## **High-performance, swapless Tailscale manager & live monitor for Asuswrt-Merlin routers.**

> [!NOTE]
> **Attribution & Origin:** ZeroScale is based on and inspired by [TAILMON](https://github.com/ViktorJp/TAILMON) originally created by **Viktor Jaep** (licensed under GPLv3). ZeroScale is an independently developed, standalone evolution engineered specifically for swapless, zero-overhead Tailscale monitoring and management on Asuswrt-Merlin routers.

---

## 100% Swapless Tailscale Management & Monitoring

Running modern Go-based services like Tailscale on embedded, memory-constrained Asus routers has traditionally required dedicated USB swap partitions to prevent memory exhaustion, out-of-memory (OOM) kernel kills, and fork failures.

**ZeroScale completely eliminates the need for USB swap partitions:**

* 🛡️ **Zero Swap Dependency:** Operates smoothly on bare router RAM without requiring an external USB flash drive or swap file.
* 💾 **Ultra-Low Memory Footprint:** Consumes **less than 800 KB of RAM** (compared to 10–25 MB with heavy multi-process shell loops).
* ⚡ **Zero Fork Overhead:** Native C99 execution avoids spawning hundreds of background `awk`, `sed`, `grep`, and subshell processes every polling cycle.
* 🔒 **Flash Wear Protection:** Event-driven architecture eliminates disk thrashing, significantly extending the lifespan of router NAND flash and attached storage.
* ⏱️ **0.0% Idle CPU Usage:** Sleeps on an event-driven terminal loop, reserving 100% of router CPU power for NAT routing, QoS, and WireGuard cryptography.

---

## 📊 Technical Comparison

| Metric / Capability | Traditional Shell Monitors | **ZeroScale (`C99`)** |
|:---|:---:|:---:|
| **Swap Requirement** | USB swap partition recommended | **100% Swapless (No USB swap needed)** |
| **RAM Footprint** | ~5–15 MB (spawning subshells) | **< 800 KB resident memory** |
| **CPU Usage (Idle/Poll)** | 2–5% spikes on subshell forks | **0.0% CPU (event-driven tick)** |
| **Binary Size** | Multiple scripts / disk files | **~149 KB single static musl binary** |
| **Rendering Engine** | ANSI escapes (flickers on refresh) | **Double-buffered differential matrix (0% flicker)** |
| **Input Navigation** | Number-key menus | **Full Keyboard (Arrows/Tab) + SGR Mouse Navigation** |
| **Dependencies** | Requires `coreutils`, `awk`, `sed`, `screen` | **Zero external dependencies (pure static C99)** |

---

## ✨ Features

### 1. 🖥️ Live Monitor Dashboard (`VIEW_DASHBOARD`)
<img width="1552" height="982" alt="Dashboard" src="https://github.com/user-attachments/assets/20ffb87e-095b-4a0d-8ff2-2d979da514b0" />

* **Real-time Status Cards:** Live indicators for Daemon status, Tailnet connectivity, Operating Mode (`Userspace` / `Kernel`), Watchdog Keepalive state, Exit Node advertisement, and advertised Subnet Routes.
* **Interactive Action Bar:** Dynamic action buttons with instant keyboard shortcuts (`u`, `d`, `r`, `s`, `t`, `l`, `c`, `q`) and arrow navigation (`←`/`→`).
* **Color-Coded Peer Topology:**
  * 🟣 **Self (Router):** Bold Magenta (`router - self`)
  * 🟡 **Exit Nodes:** Vivid Gold / Yellow (`offers exit node`)
  * 🟢 **Active (Direct WireGuard):** Bright Green (`active; direct ...`)
  * 🔵 **Idle (Online):** Soft Cyan (`idle; ...`)
  * ⚫ **Offline Nodes:** Dim Gray (`offline, last seen ...`)
* **Smooth Scrolling:** Scroll through large peer lists with the mouse wheel or arrow keys.

### 2. 🔍 Modal Peer Inspector (`VIEW_PEER_DETAIL`)
<img width="1552" height="982" alt="Peer Detail" src="https://github.com/user-attachments/assets/177f1f40-a568-405e-b4cf-d0d874f86487" />

* Select any peer with mouse click or `Enter` to open a floating, bordered modal inspector. 
* Inspect Node Name, Tailscale IP, OS Platform, Account Owner, Connection Endpoint (Direct vs. DERP Relay), and real-time Tailnet status.
* Interactive actions directly inside modal:
  * Press `p`: Instant ICMP ping test.
  * Press `t`: Real Tailscale WireGuard latency ping.
  * Press `c` / `Esc`: Close inspector.

### 3. ⚙️ Unified Configuration & Service Management (`VIEW_CONFIG`)
<img width="1552" height="982" alt="Config" src="https://github.com/user-attachments/assets/fccb97a3-b09a-4d49-a71c-c2050374fdc8" />

Consolidated, interactive settings management with instant persistence to `/jffs/addons/zeroscale.d/zeroscale.cfg`:
* **Section 1: Daemon & Health Monitor** — Toggle Watchdog Keepalive, Persistent Settings, and Boot Autostart (`/jffs/scripts/post-mount`).
* **Section 2: Tailscale Routing & Mode** — Toggle `Userspace` ⟷ `Kernel` mode, Exit Node advertisement, Subnet Routes advertisement, and interactive Subnet CIDR editor.
* **Section 3: Interface & Logging** — Cycle status check intervals (`10s` ➔ `300s`) and edit Event Log row retention limits.
* **Section 4: Notifications & Automation** — Configure amtm email alerts and scheduled daily autoupdates.
* **Section 5: Binary & Maintenance** — Check & update Tailscale binaries, reset daemon state, and run safe install/reinstall/uninstall workflows.

### 4. 📜 Full-Screen Event Log Viewer (`VIEW_LOGS`)

<img width="1552" height="982" alt="Log Viewer" src="https://github.com/user-attachments/assets/d388df8b-be5f-4fc2-b1f5-c9b711cc86c6" />

* Real-time logging of all configuration changes and daemon events directly to `/jffs/addons/zeroscale.d/zeroscale.log`.
* Syntax highlighting for log levels (`INFO`, `WARN`, `FAIL`, `ONLINE`).
* Opens directly to the most recent log entries filling the entire screen.
* Supports `↑`/`↓`, `PgUp`/`PgDn`, `g` (top), `G` (bottom), and `r` (live reload).

---

## 🛠️ Universal Installation on Asuswrt-Merlin
Run the following one-line installer on your router over SSH. It **automatically detects your router's architecture** (ARMv7 vs ARM64) and **migrates any existing Tailmon settings**:

```sh
curl -fsSL https://raw.githubusercontent.com/underd0se/ZeroScale/main/install.sh | sh
```

> [!TIP]
> **Existing Tailmon Users:** The installer automatically detects `/jffs/addons/tailmon.d/tailmon.cfg`, migrates your routing and exit-node settings to `zeroscale.cfg`, cleans up old boot hooks, and creates a `/opt/bin/tailmon` compatibility alias.

```sh
# Launch ZeroScale
zeroscale
```

---

## 💻 Building from Source (macOS / Linux)

Cross-compiling requires **[Zig](https://ziglang.org/)**:

```bash
# 1. Install Zig via Homebrew (macOS)
brew install zig

# 2. Clone repository
git clone https://github.com/underd0se/ZeroScale.git
cd ZeroScale

# 3. Build release binaries for Asus routers (ARMv7 & ARM64)
./build-all.sh
```

---

## 🎮 Navigation & Keyboard Controls

| Context | Key / Action | Description |
|:---|:---:|:---|
| **Anywhere** | `Mouse Left-Click` | Activate buttons, select peers, toggle settings |
| **Anywhere** | `Shift + Drag` (or `Fn + Drag`) | Native terminal text selection and copying across all terminal emulators |
| **Dashboard** | `↑` / `↓` | Navigate between Header Action Bar and Peer Node Table |
| **Dashboard** | `←` / `→` | Cycle through Header Action Bar buttons when focused |
| **Dashboard** | `Enter` | Trigger focused button or open Peer Inspector for selected node |
| **Dashboard** | `u` / `d` | Quick direct shortcut for Tailscale `up` / `down` |
| **Dashboard** | `r` / `s` / `t` | Quick direct shortcut for daemon restart / start / stop |
| **Dashboard** | `c` / `l` / `q` | Open Configuration / Logs / Quit |
| **Config Menu** | `↑` / `↓` / `←` / `→` | Navigate through all 17 configuration items in 2-column layout |
| **Config Menu** | `Enter` / `Space` | Toggle setting / Open Subnet CIDR editor / Edit Log retention / Edit Auto-Update Time |
| **Config Menu** | `1`–`9`, `t`, `z`, `u`, `x`, `i` | Direct jump and toggle hotkeys (`z`: Update ZScale, `u`: Update TS) |
| **Config Menu** | `Esc` / `e` / `q` | Return to Dashboard |
| **Modals & Dialogs** | `←` / `→` / `Tab` | Switch between Action and Cancel buttons |
| **Modals & Dialogs** | `Enter` / `Space` | Execute focused modal action |
| **Modals & Dialogs** | `Esc` / `e` / `c` / `q` | Cancel modal and return cleanly |
| **Input Modals** | `Tab` / `↓` / `↑` | Move focus between Text Input field and Save/Cancel buttons |
| **Peer Inspector** | `p` / `t` / `c` / `e` | Live ICMP Ping / Tailscale WireGuard Ping / Close modal |
| **Log Viewer** | `↑` / `↓` / `PgUp` / `PgDn` | Scroll log viewer |
| **Log Viewer** | `/` / `n` / `N` | Real-time substring search through event logs |
| **Log Viewer** | `g` / `G` / `r` / `e` | Jump to Top / Bottom / Refresh logs / Exit |

---

## 🙏 Acknowledgments & Credits

* **[Viktor Jaep](https://github.com/ViktorJp)** — Creator of [TAILMON](https://github.com/ViktorJp/TAILMON), whose original Asuswrt-Merlin addon laid the foundation and inspired this project.
* **[Eric Sauvageau (RMerl)](https://www.asuswrt-merlin.net/)** — Developer of Asuswrt-Merlin firmware.
* **[Tailscale](https://tailscale.com/)** — Zero-config WireGuard mesh VPN.

---

## 📄 License

GPL-3.0 License. See [LICENSE](LICENSE) for details.

