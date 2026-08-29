#!/usr/bin/env bash
set -euo pipefail

TARGET_ARCH="${1:-armv7}"
ROUTER_USER="${ROUTER_USER:-admin}"
ROUTER_HOST="${ROUTER_HOST:-}"
ROUTER_PORT="${ROUTER_PORT:-22}"
SSH_KEY="${SSH_KEY:-$HOME/.ssh/id_rsa}"

mkdir -p tui/bin

echo "=== Cross-Compiling ZeroScale C-TUI on macOS for [${TARGET_ARCH}] ==="

if [ "${TARGET_ARCH}" = "armv7" ]; then
    zig cc -target arm-linux-musleabihf -O2 -s -std=c99 -D_GNU_SOURCE -D_DEFAULT_SOURCE -Itui/include \
        tui/src/main.c tui/src/config.c tui/src/ui.c -o tui/bin/zeroscale-tui-armv7
    OUTPUT_BIN="tui/bin/zeroscale-tui-armv7"
elif [ "${TARGET_ARCH}" = "arm64" ] || [ "${TARGET_ARCH}" = "aarch64" ]; then
    zig cc -target aarch64-linux-musl -O2 -s -std=c99 -D_GNU_SOURCE -D_DEFAULT_SOURCE -Itui/include \
        tui/src/main.c tui/src/config.c tui/src/ui.c -o tui/bin/zeroscale-tui-arm64
    OUTPUT_BIN="tui/bin/zeroscale-tui-arm64"
elif [ "${TARGET_ARCH}" = "mac" ] || [ "${TARGET_ARCH}" = "native" ]; then
    clang -O2 -std=c99 -D_GNU_SOURCE -D_DEFAULT_SOURCE -Itui/include \
        tui/src/main.c tui/src/config.c tui/src/ui.c -o tui/bin/zeroscale-tui-mac
    OUTPUT_BIN="tui/bin/zeroscale-tui-mac"
    echo "Native Mac build complete: ${OUTPUT_BIN}"
    exit 0
else
    echo "Unknown target architecture: ${TARGET_ARCH}"
    exit 1
fi

ls -lh "${OUTPUT_BIN}"

if [ -n "${ROUTER_HOST}" ]; then
    echo "=== Deploying static binary to Router (${ROUTER_HOST}) ==="
    cat "${OUTPUT_BIN}" | ssh -i "${SSH_KEY}" -p "${ROUTER_PORT}" "${ROUTER_USER}@${ROUTER_HOST}" \
        "cat > /tmp/zeroscale-tui.tmp && chmod 755 /tmp/zeroscale-tui.tmp && mv -f /tmp/zeroscale-tui.tmp /jffs/scripts/zeroscale-tui && ln -sf /jffs/scripts/zeroscale-tui /opt/bin/zeroscale-tui"
    echo "=== Successfully Deployed to /jffs/scripts/zeroscale-tui ==="
else
    echo "Build complete: ${OUTPUT_BIN}"
    echo "(Set ROUTER_HOST=x.x.x.x to deploy automatically over SSH)"
fi
