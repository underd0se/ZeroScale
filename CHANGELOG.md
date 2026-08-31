# Changelog ─ ZeroScale

All notable changes to the ZeroScale project are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

## [0.3.0] - 2026-08-31

### 🔍 Live Peer Search & Multi-Mode Tailnet Sorting

ZeroScale `v0.3.0` brings full real-time interactive peer search/filtering (`/`, `f`) and multi-mode peer sorting (`o`) to the live monitor dashboard while preserving `s` exclusively for starting the Tailscale daemon.

#### 🌟 Added & Enhanced
* **Live Peer Search & Filter (Press `/` or `f`):**
  * Added real-time in-line search bar at the bottom of the dashboard.
  * Filters peer nodes instantly as you type matching across device names, Tailscale IPs, users, operating systems, DERP relays, or active status.
  * Clear search anytime with `Esc` or `Ctrl+U`.
* **Multi-Mode Peer Sorting (Press `o` / Order):**
  * Cycle dynamically through 4 sorting modes without colliding with the top action shortcuts:
    1. **Default:** Tailscale tailnet natural order.
    2. **Online First:** Sorts active direct nodes ➔ active relays ➔ idle nodes ➔ offline nodes, with alphabetical ties.
    3. **Name (A–Z):** Case-insensitive alphabetical sorting.
    4. **Operating System:** Groups peers by OS (Linux, macOS, iOS, Windows, Android, Synology).
  * The local router (Self) is always kept pinned to the top for immediate access.

---

## [0.2.5] - 2026-08-31

### 🛡️ Crash Signal Recovery, Hardware Badges & Input Validation

ZeroScale `v0.2.5` introduces fatal crash signal recovery for clean terminal restoration, dynamic router hardware/firmware badges, strict IPv4 CIDR validation, custom flag shell sanitization, and native desktop simulation mode (`--mock`).

#### 🌟 Added & Enhanced
* **Fatal Crash Signal Recovery (`handle_fatal_signal`):**
  * Registered POSIX signal handlers (`SIGSEGV`, `SIGBUS`, `SIGILL`, `SIGFPE`, `SIGABRT`, `SIGHUP`) to guarantee that `tb_shutdown()` is executed before termination.
  * Terminal cursor visibility, mouse reporting, and raw mode are cleanly and reliably restored even in abnormal crash events.
* **Dynamic Router Model & Firmware Header Badge:**
  * Automatically queries `nvram get model` (e.g. `RT-AX86U`, `GT-AXE16000`) and `nvram get buildno` (e.g. `3004.388.8_2`) to render a hardware badge in the dashboard header.
* **Strict IPv4 CIDR Validator & Custom Flags Sanitizer:**
  * Validates subnet route inputs against IPv4 CIDR format (`X.X.X.X/Y`, `0-255`, `/0-32`) with inline syntax error toasts.
  * Sanitizes custom flags (`customparams`) to prevent dangerous shell escape characters.
* **Native Desktop Simulation Mode (`--mock` / `make sim`):**
  * Allows building and running ZeroScale natively on macOS/Linux desktops with synthetic multi-device tailnet peers, exit nodes, and logs.

---

## [0.2.4] - 2026-08-31

### 🛡️ Interactive Migration Consent, Namespace Isolation & Safe Coexistence

ZeroScale `v0.2.4` introduces interactive migration prompts based on explicit user consent, strict CLI namespace isolation, non-invasive boot hook handling, and clean uninstallation with third-party reactivation guidance.

#### 🌟 Added & Enhanced
* **Interactive Migration Prompt on Existing Setup Detection:**
  * When an existing TAILMON installation is detected during setup, the installer prompts the user interactively (`[y/N]`) before importing configuration.
  * If the user declines, all third-party files, configuration, cron jobs, and boot hooks remain 100% untouched.
* **Strict CLI Namespace Isolation:**
  * Removed the `/opt/bin/tailmon` compatibility symlink. ZeroScale now installs strictly and exclusively to `/opt/bin/zeroscale` to prevent command collisions.
* **Non-Invasive Boot Hook & Daemon Management:**
  * C-TUI configuration and uninstall routines now strictly modify `/zeroscale/` lines in `/jffs/scripts/post-mount`, preserving any existing third-party hooks.
* **Legacy Symlink & Artifact Sanitization:**
  * Automatically detects and removes stale `/opt/bin/tailmon` symlinks leftover from older ZeroScale/Tailmon-Zero versions that pointed to ZeroScale.
  * Restores `/opt/bin/tailmon` to point back to the authentic TAILMON script (`/jffs/scripts/tailmon.sh`) if present on disk.
  * Purges obsolete development artifacts (`/opt/bin/tailmon-zero`, `/jffs/scripts/tailmon-zero`).
* **Clean Uninstallation & Re-activation Hint:**
  * Uninstallation cleanly removes all ZeroScale directories, binaries, and cron jobs, and detects whether TAILMON is present on the filesystem to display a helpful re-activation tip.

---

## [0.2.3] - 2026-08-31

### 🛠️ PuTTY Redraw Fixes, Custom Mode Flags Editor & Smart Menu Navigation

ZeroScale `v0.2.3` introduces interactive Custom operating mode management, in-line modal text cursor navigation, smart two-digit numeric menu input buffering, dynamic router LAN subnet detection, and robust subshell stdout isolation preventing screen artifacts in PuTTY and SSH clients.

#### 🌟 Added & Enhanced
* **Tailscale Custom Operating Mode & Flags Editor:**
  * Added full `Custom` mode support cycling seamlessly through `Userspace` ➔ `Kernel (TUN)` ➔ `Custom`.
  * Preserves user-defined flags from legacy Tailmon (`customparams` / `customflags`).
  * Automatically opens an interactive flag editor modal when entering Custom mode or pressing `(4)` / `Enter` while in Custom mode.
  * Displays configured custom startup arguments directly on the configuration line.
* **Modal Text Input Inline Cursor Navigation:**
  * Full inline cursor navigation with `←` / `→` arrow keys, `Home` / `End`, `Ctrl+A` / `Ctrl+E`.
  * True character insertion and inline deletion with `Backspace` and `Delete` (`Ctrl+D`).
  * Quick line clear with `Ctrl+U` / `Ctrl+K`.
  * Visual block cursor indicating precise caret position within the text box.
* **Subnet Routes CIDR Prompt Clarity & Dynamic LAN Lookup:**
  * Updated subnet prompt to explicitly guide multiple comma-separated CIDR entries (`192.168.1.0/24,10.0.0.0/24`).
  * Added dynamic router LAN subnet lookup via `nvram get lan_ipaddr` to ensure defaults always match the router's active subnet.
* **Smart Two-Digit Numeric Menu Navigation:**
  * Buffered number key input in the configuration view: pressing `1` followed by `0`–`5` selects items `(10)` through `(15)` cleanly without triggering single-digit toggles.
  * Single-digit options `(2)`–`(9)` fire immediately.
* **PuTTY Screen Redraw & Background Process Isolation:**
  * Wrapped all compound background maintenance and daemon restart commands inside isolated subshells (`( ... ) >/dev/null 2>&1 &`), preventing package managers like `opkg` from leaking raw output over the terminal screen.
  * Integrated Termbox buffer invalidation (`tb_invalidate()`) on action executions to force clean hardware repaints.
  * Added global screen repaint hotkey: press `Ctrl+L` or `Ctrl+R` to force an immediate full-screen redraw anywhere in the app.
* **Global `e` / `E` Exit Key:**
  * Added `e` / `E` as a universal exit and cancel key across all menus and modals (matching AMTM standards).

---

## [0.2.2] - 2026-08-30

### 🖥️ Dynamic Terminal Detection & Platform-Agnostic Copying

ZeroScale `v0.2.2` introduces automatic client terminal emulator detection and displays dynamic copy shortcut hints.

#### 🌟 Added & Enhanced
* **Dynamic Terminal Detection (`detect_terminal`):**
  * Auto-detects client terminal emulators (Ghostty, Kitty, Alacritty, WezTerm, iTerm2, Apple Terminal) via `$TERM_PROGRAM` and `$TERM`.
  * Renders accurate native text selection shortcut hints directly in the dashboard footer (`Shift+Drag: Copy` for Ghostty/Kitty/Linux vs. `Fn+Drag: Copy` for Terminal.app/iTerm2).
* **Platform-Agnostic Controls Documentation:**
  * Updated README navigation table to reflect universal text selection modifiers across all terminal emulators.

---

## [0.2.1] - 2026-08-29

### 🔧 Fixes & Accessibility Polish

ZeroScale `v0.2.1` resolves configuration parsing issues, adds full keyboard accessibility to the Peer Node Inspector modal, implements live syslog event recording, and simplifies binary and command naming.

#### 🌟 Added & Fixed
* **Peer Node Inspector Keyboard & Mouse Navigation (`VIEW_PEER_DETAIL`):**
  * Full keyboard accessibility: cycle through `Ping`, `Tailscale Ping`, and `Close` buttons with `←` / `→` or `Tab`, and execute with `Enter` / `Space`.
  * Dedicated direct shortcuts (`p`, `t`, `c`/`Esc`) remain active for power users.
  * Added subtle navigation hint typography.
  * Added left-click mouse support for all modal buttons.
* **Subnet Routes String Parser Fix:**
  * Fixed quote-trimming parser in `load_config` that previously truncated quoted values (`routes="192.168.x.x/24"`), resolving the empty parentheses issue on the configuration screen.
  * Added safe fallback to router LAN subnet.
* **Live Event Logger (`zeroscale.log`):**
  * Integrated native syslog-style event logging (`log_event`) across daemon operations, configuration updates, and session lifecycles.
* **Streamlined Naming:**
  * Standardized single executable command to `zeroscale` (with backward-compatibility symlink to `tailmon`).

---

## [0.2.0] - 2026-08-29

### 🎨 Major UI Overhaul & Complete Keyboard Navigation

ZeroScale `v0.2.0` introduces complete full-keyboard navigation across all views, a major visual UI overhaul with streamlined button hierarchy, and resilient daemon installation/update workflows.

#### 🌟 Added & Enhanced
* **Streamlined UI & Dynamic Button Hierarchy:**
  * Clean text buttons with zero background fill on unfocused state to eliminate visual clutter.
  * Elevated, high-emphasis filled containers upon active keyboard or mouse focus (Bold Green for positive/nav actions, Bold Red for destructive/stop/quit actions, Bold White for secondary/dismiss actions).
  * Applied consistently across Dashboard Header buttons, Confirmation Modals, Input Modals, and Peer Node Inspector.
* **Full Keyboard Navigation Across All Views:**
  * **Dashboard Action Bar:** Navigate with `←` / `→` arrows and execute with `Enter` / `Space`, with visual tracking indicator.
  * **Configuration View:** Seamless `↑` / `↓` cursor navigation across all 14 configuration items with `Enter`/`Space` to toggle or edit.
  * **Confirmation Dialogs:** Keyboard navigable with `←` / `→` / `Tab` and executable with `Enter`, with context-specific action labels (`Disconnect`, `Restart`, `Stop`, `Update`, `Reset State`, `Reinstall`, `Uninstall` vs `Cancel`).
  * **Input Modals:** Interactive navigation between Text Field and Save/Cancel buttons via `Tab`, `↓`, `↑`, and `Enter`.
* **Clean Configuration Menu & Preserved Backgrounds:**
  * Removed verbose brackets and helper callout text for a sleek, minimal interface.
  * Modal dialogs opened from the Configuration menu now preserve the Configuration view directly in the background.
* **Service Safety & Daemon Fixes:**
  * Added confirmation prompt to Tailscale Binary Update.
  * Reinstalling Tailscale from Entware safely stops the running daemon first, avoiding opkg conflicts and terminal freezes.
* **Muted Dialog Hint Typography:**
  * Soft, muted gray formatting for navigation shortcut instructions.

---

## [0.1.0] - 2026-08-29

### 🚀 Initial Public Release

ZeroScale `v0.1.0` is the first native C99 implementation of the ZeroScale Tailscale manager for Asuswrt-Merlin routers.

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
