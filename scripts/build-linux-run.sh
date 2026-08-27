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

do_install=1
output_path=""
while [[ $# -gt 0 ]]; do
  case "$1" in
    --no-install|--build-only) do_install=0; shift ;;
    --output)
      [[ $# -ge 2 ]] || { echo '--output requires a file path.' >&2; exit 2; }
      output_path="$2"; shift 2 ;;
    --help|-h)
      echo "Usage: $0 [--build-only|--no-install] [--output FILE]"
      echo "Build and test JAGLINK locally, create a +native1 Debian package and optionally install it through APT."
      exit 0
      ;;
    *) echo "Unknown option: $1" >&2; exit 2 ;;
  esac
done
if [[ -n "$output_path" && $do_install -eq 1 ]]; then
  echo '--output is valid only with --build-only/--no-install.' >&2
  exit 2
fi

need=()
for cmd in cmake cpack cc pkg-config tar dpkg dpkg-deb dpkg-query; do
  command -v "$cmd" >/dev/null 2>&1 || need+=("$cmd")
done
if ! pkg-config --exists 'gtk4 >= 4.6' 2>/dev/null; then need+=("libgtk-4-dev"); fi
if ! pkg-config --exists bluez 2>/dev/null; then need+=("libbluetooth-dev"); fi

run_as_root()
{
  if [[ $EUID -eq 0 ]]; then
    "$@"
  elif command -v sudo >/dev/null 2>&1; then
    sudo "$@"
  else
    echo 'Administrator access is required and sudo is unavailable.' >&2
    return 1
  fi
}

if [[ ${#need[@]} -ne 0 ]]; then
  if command -v apt-get >/dev/null 2>&1; then
    echo "Installing native build dependencies: build-essential cmake dpkg-dev pkg-config libgtk-4-dev libbluetooth-dev"
    run_as_root apt-get update
    run_as_root apt-get install -y build-essential cmake dpkg-dev pkg-config libgtk-4-dev libbluetooth-dev
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

version="$(tr -d '[:space:]' < "$work/VERSION")"
native_version="${version}+native1"
build="$work/build"
package_dir="$work/package"

cmake -S "$work" -B "$build" \
  -DCMAKE_BUILD_TYPE=Release \
  -DJAGLINK_BUILD_LINUX_APP=ON \
  -DJAGLINK_BUILD_PROFILE=native \
  -DJAGLINK_PACKAGE_VERSION="$native_version" \
  -DBUILD_TESTING=ON \
  -DCMAKE_INSTALL_PREFIX=/usr
cmake --build "$build" --parallel
ctest --test-dir "$build" --output-on-failure --parallel

mkdir -p "$package_dir"
cpack --config "$build/CPackConfig.cmake" -G DEB -B "$package_dir"
package_path="$(find "$package_dir" -maxdepth 1 -type f -name '*.deb' -print -quit)"
[[ -n "$package_path" && -s "$package_path" ]] || { echo 'Native Debian package was not produced.' >&2; exit 1; }

[[ "$(dpkg-deb -f "$package_path" Package)" == jaglink ]] || { echo 'Native package has the wrong package name.' >&2; exit 1; }
[[ "$(dpkg-deb -f "$package_path" Version)" == "$native_version" ]] || { echo 'Native package has the wrong version.' >&2; exit 1; }
[[ "$(dpkg-deb -f "$package_path" Architecture)" == "$(dpkg --print-architecture)" ]] || { echo 'Native package has the wrong architecture.' >&2; exit 1; }

if [[ $do_install -eq 0 ]]; then
  if [[ -z "$output_path" ]]; then
    output_path="$PWD/jaglink_${native_version}_$(dpkg --print-architecture).deb"
  fi
  mkdir -p "$(dirname "$output_path")"
  install -m 0644 "$package_path" "$output_path"
  echo "JAGLINK native Debian package created: $output_path"
  exit 0
fi

(
  cd "$(dirname "$package_path")"
  run_as_root apt-get install -y "./$(basename "$package_path")"
)

# Remove files left by pre-0.2.38 unmanaged /usr/local native installers.
run_as_root rm -f \
  /usr/local/bin/jaglink \
  /usr/local/share/icons/hicolor/128x128/apps/jaglink.png \
  /usr/local/share/pixmaps/jaglink.png \
  /usr/local/share/applications/com.github.The-First-Infiltrator.Jaglink.desktop
run_as_root rm -rf /usr/local/share/doc/jaglink

installed_version="$(dpkg-query -W -f='${Version}' jaglink 2>/dev/null || true)"
[[ "$installed_version" == "$native_version" ]] || {
  printf 'Native package installation verification failed: expected %s, found %s\n' "$native_version" "$installed_version" >&2
  exit 1
}

echo "JAGLINK $version was compiled locally, tested, packaged and installed as jaglink $native_version."
echo "APT now owns the native installation and will offer only a genuinely newer release."
exit 0
__JAGLINK_ARCHIVE_BELOW__
STUB
cat "$work/source.tar.gz" >> "$out_dir/$name"
chmod +x "$out_dir/$name"
rm -rf "$out_dir/_CPack_Packages"
printf '%s\n' "$out_dir/$name"
