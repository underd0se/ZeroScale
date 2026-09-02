#!/bin/sh
# =========================================================================================================================
# ZeroScale Universal Installer for Asuswrt-Merlin
# Automatically detects router architecture (ARMv7 vs ARM64) and migrates legacy Tailmon setups.
# =========================================================================================================================

set -e

export PATH="/opt/bin:/opt/sbin:/usr/bin:/usr/sbin:/bin:/sbin:${PATH:-}"

REPO_RAW_URL="https://raw.githubusercontent.com/underd0se/ZeroScale/main"
VERSION="v0.3.5"
INSTALL_DIR="/jffs/scripts"
TARGET_BIN="${INSTALL_DIR}/zeroscale"
CONFIG_DIR="/jffs/addons/zeroscale.d"
LEGACY_DIR="/jffs/addons/tailmon.d"
POST_MOUNT="/jffs/scripts/post-mount"

# ANSI Colors
C_RESET="\033[0m"
C_BOLD="\033[1m"
C_GREEN="\033[1;32m"
C_CYAN="\033[1;36m"
C_YELLOW="\033[1;33m"
C_RED="\033[1;31m"

for arg in "$@"; do
    case "$arg" in
        --silent|-s)
            exec >/dev/null 2>&1
            ;;
    esac
done

printf "\n%b" "${C_GREEN}"
cat <<'EOF'
    ______  ______ ____   ____ 
   /___  / / ____// __ \ / __ \
     / /  / __/  / /_/ // / / /
   / /__ / /___ / _, _// /_/ / 
 /_____//_____//_/ |_| \____/  
   _____ ______ ___    __    ______
  / ___// ____//   |  / /   / ____/
  \__ \/ /    / /| | / /   / __/   
 ___/ / /___ / ___ |/ /___/ /___   
/____/\____//_/  |_/_____/_____/ Universal Installer
EOF
printf "%b\n\n" "${C_RESET}"

# -------------------------------------------------------------------------------------------------------------------------
# Step 1: Pre-Flight Environment Checks

printf "%b[*] Checking router environment...%b\n" "${C_CYAN}" "${C_RESET}"

if [ ! -d "/jffs/scripts" ]; then
    printf "%b[!] Error: /jffs/scripts not found. Please enable JFFS custom scripts in Asuswrt-Merlin Administration settings.%b\n" "${C_RED}" "${C_RESET}"
    exit 1
fi

if [ ! -d "/opt/bin" ]; then
    printf "%b[!] Warning: /opt/bin not found. Entware may not be initialized.%b\n" "${C_YELLOW}" "${C_RESET}"
fi

# -------------------------------------------------------------------------------------------------------------------------
# Step 2: Detect Hardware Architecture

ARCH="$(uname -m)"
printf "%b[*] Detecting router architecture: %s%b\n" "${C_CYAN}" "${ARCH}" "${C_RESET}"

case "${ARCH}" in
    armv7l|armv7|arm)
        RELEASE_FILE="zeroscale-${VERSION}-armv7-linux-musl"
        ARCH_NAME="ARMv7 (32-bit)"
        ;;
    aarch64|arm64)
        RELEASE_FILE="zeroscale-${VERSION}-arm64-linux-musl"
        ARCH_NAME="ARM64 (64-bit)"
        ;;
    *)
        printf "%b[!] Error: Unsupported CPU architecture '%s'. ZeroScale supports ARMv7 and ARM64 Asus routers.%b\n" "${C_RED}" "${ARCH}" "${C_RESET}"
        exit 1
        ;;
esac

printf "%b[+] Target architecture verified: %s%b\n" "${C_GREEN}" "${ARCH_NAME}" "${C_RESET}"

# -------------------------------------------------------------------------------------------------------------------------
# Step 3: Check for Existing Tailmon Setup

mkdir -p "${CONFIG_DIR}"

if [ -d "${LEGACY_DIR}" ] || [ -f "/jffs/scripts/tailmon" ] || [ -f "/jffs/scripts/tailmon.sh" ]; then
    printf "\n%b[!] Existing TAILMON installation detected.%b\n" "${C_YELLOW}" "${C_RESET}"
    printf "Note: Running two Tailscale management services simultaneously on low-RAM routers\n"
    printf "can cause daemon conflicts and kernel memory limits.\n\n"
    
    # Read user choice from terminal (support curl | sh execution)
    MIGRATE_CHOICE=""
    if [ -t 0 ]; then
        printf "%bWould you like to import your settings and disable TAILMON's boot hook? [y/N]: %b" "${C_BOLD}" "${C_RESET}"
        read -r MIGRATE_CHOICE
    elif [ -c /dev/tty ]; then
        printf "%bWould you like to import your settings and disable TAILMON's boot hook? [y/N]: %b" "${C_BOLD}" "${C_RESET}" < /dev/tty
        read -r MIGRATE_CHOICE < /dev/tty
    fi

    case "${MIGRATE_CHOICE}" in
        [yY]|[yY][eE][sS])
            printf "%b[*] Importing TAILMON settings...%b\n" "${C_CYAN}" "${C_RESET}"
            screen -S tailmon -X quit 2>/dev/null || true
            killall -9 tailmon tailmon.sh 2>/dev/null || true

            if [ -f "${LEGACY_DIR}/tailmon.cfg" ] && [ ! -f "${CONFIG_DIR}/zeroscale.cfg" ]; then
                cp "${LEGACY_DIR}/tailmon.cfg" "${CONFIG_DIR}/zeroscale.cfg"
                printf "%b[+] Imported configuration from %s/tailmon.cfg -> %s/zeroscale.cfg%b\n" "${C_GREEN}" "${LEGACY_DIR}" "${CONFIG_DIR}" "${C_RESET}"
            fi

            if [ -f "${POST_MOUNT}" ]; then
                sed -i -e '/tailmon\.sh/d' -e '/tailmon -screen/d' "${POST_MOUNT}" 2>/dev/null || true
            fi
            cru d tailmon_autoupdate 2>/dev/null || true
            printf "%b[+] TAILMON migration completed.%b\n\n" "${C_GREEN}" "${C_RESET}"
            ;;
        *)
            printf "%b[*] Keeping TAILMON installation untouched.%b\n" "${C_CYAN}" "${C_RESET}"
            printf "%b[*] Please ensure only one Tailscale monitor is active to avoid daemon conflicts.%b\n\n" "${C_YELLOW}" "${C_RESET}"
            ;;
    esac
fi

# -------------------------------------------------------------------------------------------------------------------------
# Step 4: Download Binary Release

DOWNLOAD_URL="${REPO_RAW_URL}/bin/release/${RELEASE_FILE}"
TMP_BIN="/tmp/zeroscale.tmp"

printf "%b[*] Downloading ZeroScale (%s)...%b\n" "${C_CYAN}" "${RELEASE_FILE}" "${C_RESET}"

if which curl >/dev/null 2>&1; then
    curl -fsSL "${DOWNLOAD_URL}" -o "${TMP_BIN}"
elif [ -x /usr/sbin/curl ]; then
    /usr/sbin/curl -fsSL "${DOWNLOAD_URL}" -o "${TMP_BIN}"
elif which wget >/dev/null 2>&1; then
    wget -q "${DOWNLOAD_URL}" -O "${TMP_BIN}"
elif [ -x /usr/sbin/wget ]; then
    /usr/sbin/wget -q "${DOWNLOAD_URL}" -O "${TMP_BIN}"
else
    printf "%b[!] Error: Neither curl nor wget found on system.%b\n" "${C_RED}" "${C_RESET}"
    exit 1
fi

if [ ! -s "${TMP_BIN}" ]; then
    printf "%b[!] Error: Download failed or binary is empty.%b\n" "${C_RED}" "${C_RESET}"
    rm -f "${TMP_BIN}"
    exit 1
fi

# -------------------------------------------------------------------------------------------------------------------------
# Step 5: Install Binary & Create PATH Symlinks

printf "%b[*] Installing binary to %s...%b\n" "${C_CYAN}" "${TARGET_BIN}" "${C_RESET}"

chmod 755 "${TMP_BIN}"
mv -f "${TMP_BIN}" "${TARGET_BIN}"

# Create convenience symlinks in /opt/bin
if [ -d "/opt/bin" ]; then
    ln -sf "${TARGET_BIN}" "/opt/bin/zeroscale"

    # Sanitize stale /opt/bin/tailmon if it was pointing to ZeroScale from previous versions
    if [ -L "/opt/bin/tailmon" ]; then
        LINK_TARGET="$(readlink /opt/bin/tailmon 2>/dev/null || true)"
        case "${LINK_TARGET}" in
            *zeroscale*|*tailmon-zero*)
                rm -f "/opt/bin/tailmon"
                # If legitimate TAILMON script exists on disk, restore its pointer
                if [ -f "/jffs/scripts/tailmon.sh" ]; then
                    ln -sf "/jffs/scripts/tailmon.sh" "/opt/bin/tailmon"
                elif [ -f "/jffs/scripts/tailmon" ]; then
                    ln -sf "/jffs/scripts/tailmon" "/opt/bin/tailmon"
                fi
                ;;
        esac
    fi

    # Clean legacy development symlinks and binaries
    rm -f "/opt/bin/tailmon-zero" "/jffs/scripts/tailmon-zero" 2>/dev/null || true
fi

# -------------------------------------------------------------------------------------------------------------------------
# Step 6: Installation Complete

printf "\n%b================================================================%b\n" "${C_GREEN}" "${C_RESET}"
printf "%b  ZeroScale %s installed successfully!%b\n" "${C_GREEN}" "${VERSION}" "${C_RESET}"
printf "%b================================================================%b\n\n" "${C_GREEN}" "${C_RESET}"
printf "To launch the interface, run:\n\n"
printf "  %bzeroscale%b\n\n" "${C_BOLD}" "${C_RESET}"
