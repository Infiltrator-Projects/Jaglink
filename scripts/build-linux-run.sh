#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
out_dir="${1:-$repo_root/dist}"
version="$(tr -d '[:space:]' < "$repo_root/VERSION")"
name="JAGLINK-${version}-linux-native.run"
mkdir -p "$out_dir"
work="$(mktemp -d)"
trap 'rm -rf "$work"' EXIT

tar \
  --exclude='./.git' \
  --exclude='./build*' \
  --exclude='./dist' \
  --exclude='./release-assets' \
  --exclude='*.run' \
  -C "$repo_root" -czf "$work/source.tar.gz" .

cat > "$out_dir/$name" <<'STUB'
#!/usr/bin/env bash
# Self-extracting JAGLINK native Linux build installer.
set -euo pipefail

prefix="/usr/local"
do_install=1
while [[ $# -gt 0 ]]; do
  case "$1" in
    --prefix) prefix="$2"; shift 2 ;;
    --no-install) do_install=0; shift ;;
    *) echo "Unknown option: $1" >&2; exit 2 ;;
  esac
done

need=()
for cmd in cmake cc pkg-config tar; do
  command -v "$cmd" >/dev/null 2>&1 || need+=("$cmd")
done
if ! pkg-config --exists 'gtk4 >= 4.6' 2>/dev/null; then need+=("libgtk-4-dev"); fi
if [[ ${#need[@]} -ne 0 ]]; then
  if command -v apt-get >/dev/null 2>&1; then
    echo "Installing native build dependencies: build-essential cmake pkg-config libgtk-4-dev"
    if [[ $EUID -eq 0 ]]; then
      apt-get update
      apt-get install -y build-essential cmake pkg-config libgtk-4-dev
    else
      sudo apt-get update
      sudo apt-get install -y build-essential cmake pkg-config libgtk-4-dev
    fi
  else
    echo "Missing build dependencies: ${need[*]}" >&2
    exit 3
  fi
fi

self="$0"
marker='__JAGLINK_ARCHIVE_BELOW__'
line="$(awk -v marker="$marker" '$0 == marker { print NR + 1; exit }' "$self")"
[[ -n "$line" ]]
work="$(mktemp -d)"
trap 'rm -rf "$work"' EXIT
tail -n +"$line" "$self" | tar -xzf - -C "$work"
cmake -S "$work" -B "$work/build" -DCMAKE_BUILD_TYPE=Release -DJAGLINK_BUILD_LINUX_APP=ON -DBUILD_TESTING=OFF -DCMAKE_INSTALL_PREFIX="$prefix"
cmake --build "$work/build" --parallel
if [[ $do_install -eq 1 ]]; then
  if [[ -w "$prefix" || $EUID -eq 0 ]]; then
    cmake --install "$work/build"
  else
    sudo cmake --install "$work/build"
  fi
fi
echo "JAGLINK native Linux build completed."
exit 0
__JAGLINK_ARCHIVE_BELOW__
STUB
cat "$work/source.tar.gz" >> "$out_dir/$name"
chmod +x "$out_dir/$name"
rm -rf "$out_dir/_CPack_Packages"
printf '%s\n' "$out_dir/$name"
