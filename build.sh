#!/usr/bin/env bash

# Build script for ZeroScale
# Concatenates modular source files into a single deployable script

set -euo pipefail

echo "Building ZeroScale..."

cat src/*.sh > zeroscale.sh
chmod +x zeroscale.sh

echo "Build complete! Output saved to zeroscale.sh"
