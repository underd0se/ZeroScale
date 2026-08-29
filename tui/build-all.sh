#!/usr/bin/env bash
set -euo pipefail

mkdir -p tui/bin/release

echo "=========================================================="
echo "  Building ZeroScale C-TUI v0.1.0 for Asuswrt-Merlin"
echo "=========================================================="

# 1. ARMv7 (32-bit ARM Cortex-A7/A9/A15 - Asus RT-AX86U, RT-AC86U, RT-AC68U)
echo "--> Compiling [ARMv7] (arm-linux-musleabihf)..."
zig cc -target arm-linux-musleabihf -O2 -s -std=c99 -D_GNU_SOURCE -D_DEFAULT_SOURCE -Itui/include \
    tui/src/main.c tui/src/config.c tui/src/ui.c -o tui/bin/release/zeroscale-tui-v0.1.0-armv7-linux-musl

# 2. ARM64 (64-bit ARM Cortex-A53/A72 - Asus RT-AX88U Pro, GT-AXE16000, GT6)
echo "--> Compiling [ARM64] (aarch64-linux-musl)..."
zig cc -target aarch64-linux-musl -O2 -s -std=c99 -D_GNU_SOURCE -D_DEFAULT_SOURCE -Itui/include \
    tui/src/main.c tui/src/config.c tui/src/ui.c -o tui/bin/release/zeroscale-tui-v0.1.0-arm64-linux-musl

# Copy ARMv7 to default local binary
cp tui/bin/release/zeroscale-tui-v0.1.0-armv7-linux-musl tui/bin/zeroscale-tui-armv7

echo ""
echo "=========================================================="
echo "  Asus Router Binaries Built Successfully!"
echo "=========================================================="
ls -lh tui/bin/release/
