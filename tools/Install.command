#!/bin/bash
set -euo pipefail
PACKAGE_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
TARGET_DIR="${MDA_VST3_INSTALL_DIR:-$HOME/Library/Audio/Plug-Ins/VST3}"
BACKUP_DIR="${MDA_VST3_BACKUP_DIR:-$HOME/Library/Audio/Plug-Ins/MDA Revival Backups/$(date +%Y%m%d-%H%M%S)}"
mkdir -p "$TARGET_DIR"
for plugin in "$PACKAGE_DIR"/VST3/*.vst3; do
    test -d "$plugin"
    /usr/bin/codesign --verify --strict "$plugin"
    name="$(basename -- "$plugin")"
    staging="$(mktemp -d "$TARGET_DIR/.mda-revival.XXXXXX")"
    /usr/bin/ditto "$plugin" "$staging/$name"
    /usr/bin/codesign --verify --strict "$staging/$name"
    if test -e "$TARGET_DIR/$name"; then
        mkdir -p "$BACKUP_DIR"
        mv "$TARGET_DIR/$name" "$BACKUP_DIR/$name"
    fi
    mv "$staging/$name" "$TARGET_DIR/$name"
    rmdir "$staging"
    echo "Installed $name"
done
echo "Installation complete. Restart Cubase to scan the plugins."
