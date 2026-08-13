# TAILMON ZER0 v0.2.1
Asus-Merlin Tailscale Installer, Configurator and Monitor (Swapless Edition)

---

**Original Project:** This is a fork of the amazing [TAILMON](https://github.com/ViktorJp/TAILMON) project originally created by ViktorJp. All credit for the core implementation and terminal UI goes to the original author. 

### Why TAILMON ZER0?
TAILMON ZER0 was created to explicitly support Asuswrt-Merlin routers running **without swap space** (e.g. strict 512MB RAM limits) which natively causes Go-based `tailscaled` binaries to crash with `Segmentation fault` on startup. 

**Key Changes from Upstream:**
- **Swapless Compatibility:** Allows Tailscale to run on routers without a swap file by dynamically managing system memory requirements. It respects your setup—whether you use a swap file or not, the script adapts accordingly.
- **Lower Memory Footprint:** Aggressively limits background memory usage to help prevent Tailscale from exhausting your router's RAM.
- **Reduced Flash Wear:** Moves temporary downloads and tracking files to the RAM disk to minimize writes to the router's internal storage (`/jffs/`).
- **Stable Execution Flow:** Replaced fragmented process restarts with clean function calls in the setup menus. This eliminates screen flashing and prevents the wizard from accidentally skipping steps.
- **Codebase Modernization:** Split the original 4,500-line monolithic script into organized, modular files for easier maintenance, and configured diagnostics to correctly support Asuswrt's `ash` shell.
- **Immediate Execution:** Adds a global shortcut during installation so you can type `tailmon-zer0` to launch the menu right away without needing to log out.
- **Cleaner Uninstallation:** The uninstaller has been updated to restore all original system settings and remove leftover files more reliably.

---

### Installation
To install TAILMON ZER0 via SSH on your Asuswrt-Merlin router, simply run:
```sh
curl --silent --retry 3 "https://raw.githubusercontent.com/underd0se/TAILMON-Zero/main/tailmon-zero.sh" -o "/jffs/scripts/tailmon-zero.sh" && chmod 755 "/jffs/scripts/tailmon-zero.sh" && sh /jffs/scripts/tailmon-zero.sh
```

> **Note on Migration:** TAILMON and TAILMON ZER0 cannot run alongside each other. If you already have the original TAILMON installed, the setup process will automatically detect it and prompt you to cleanly remove it before proceeding. All necessary memory management adaptations will then be applied for you.

### Uninstallation
If you ever choose to completely uninstall TAILMON ZER0, all modified router memory management settings (such as overcommit bypass rules) will be cleanly reverted back to their original system defaults.

*(For support and discussion regarding the original upstream project, visit the [SNBForums Thread](https://www.snbforums.com/threads/tailmon-v1-3-4-2026-jul-12-wireguard-based-tailscale-installer-configurator-and-monitor-available-in-amtm.97556/)).*
