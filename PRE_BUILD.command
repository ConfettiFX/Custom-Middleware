#!/bin/bash
# Change active working directory in case we run script for outside of TheForge
cd "$(dirname "$0")"

echo "Syncing The-Forge"
git clone --recursive https://github.com/ConfettiFX/The-Forge.git

exit 0