# ZeroScale v1.0.1
Swapless Tailscale Installer, Configurator and Monitor for Asuswrt-Merlin

---

**Attribution:** ZeroScale is based on and forked from [TAILMON](https://github.com/ViktorJp/TAILMON) by Viktor Jaep, licensed under GPLv3. All credit for the original core implementation and terminal UI concept goes to the original author.

### Why ZeroScale?
ZeroScale was created to explicitly support Asuswrt-Merlin routers running **without swap space** (e.g., strict 512MB RAM limits) which natively causes Go-based `tailscaled` binaries to crash with `Segmentation fault` on startup. 

**Key Enhancements & Changes:**
- **Swapless Compatibility:** Allows Tailscale to run on routers without a swap file by dynamically managing system memory constraints (`GOMEMLIMIT`, `GOGC`, `GOMAXPROCS`). It respects your setup—whether you use a swap file or not, the script adapts accordingly.
- **Lower Memory Footprint:** Aggressively limits background memory usage to prevent Tailscale from exhausting your router's RAM.
- **Reduced Flash Wear:** Moves temporary downloads and tracking files to the RAM disk to minimize writes to the router's internal storage (`/jffs/`).
- **Stable Execution Flow:** Replaced fragmented process restarts with clean function calls in the setup menus, eliminating screen flashing and preventing wizard step skipping.
- **Codebase Modernization:** Modularized source architecture (`src/`) for maintainability, with diagnostics configured for Asuswrt's `ash` shell.
- **Go 1.24+ SIGBUS Protection:** Injects `GODEBUG=tlsmlkem=0` to protect 32-bit ARM routers from low-level memory alignment hardware faults.
- **Immediate Execution:** Adds a global shortcut during installation so you can type `zeroscale` to launch the menu right away without needing to log out.
- **Cleaner Uninstallation:** The uninstaller cleanly restores all original system settings (such as `overcommit_memory`) and removes leftover files reliably.

---

### Changelog
**v1.0.1 (Rebranding & Stability Release)**
- **Rebranded to ZeroScale:** Independent project identity respecting upstream naming guidelines while maintaining proper GPLv3 attribution.
- **SIGBUS Crash Fix:** Injected `GODEBUG=tlsmlkem=0` to prevent fatal memory alignment crashes on 32-bit ARM routers running Go 1.24+.
- **Watchdog Race Fix:** Changed keepalive logic to monitor process PID rather than socket status, eliminating false-positive reboot loops during initialization.
- **Architectural Rewrite:** Transitioned the monolithic script into a modular `src/` component system, and fixed 0-second UI screen flashing.

---

### Installation
To install ZeroScale via SSH on your Asuswrt-Merlin router, simply run:
```sh
curl --silent --retry 3 "https://raw.githubusercontent.com/underd0se/ZeroScale/main/zeroscale.sh" -o "/jffs/scripts/zeroscale.sh" && chmod 755 "/jffs/scripts/zeroscale.sh" && sh /jffs/scripts/zeroscale.sh
```

> **Note on Migration:** ZeroScale cannot run alongside legacy TAILMON installations. If you already have TAILMON installed, the setup process will automatically detect it and prompt you to cleanly remove it before proceeding. All necessary memory management adaptations will then be applied for you.

### Uninstallation
If you ever choose to completely uninstall ZeroScale, all modified router memory management settings (such as overcommit bypass rules) will be cleanly reverted back to their original system defaults.

*(For support and discussion regarding the original upstream project, visit the [SNBForums Thread](https://www.snbforums.com/threads/tailmon-v1-3-4-2026-jul-12-wireguard-based-tailscale-installer-configurator-and-monitor-available-in-amtm.97556/)).*
