#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
out_dir="${1:-$repo_root/dist}"
out_name="${2:-}"
version="$(tr -d '[:space:]' < "$repo_root/VERSION")"
[[ "$version" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]

if [[ "$(uname -s)" != "Darwin" ]]; then
  echo "build-ios-ipa.sh requires macOS with Xcode." >&2
  exit 2
fi
command -v xcodebuild >/dev/null
command -v zip >/dev/null
command -v shasum >/dev/null

mkdir -p "$out_dir"
derived="$(mktemp -d "${TMPDIR:-/tmp}/jaglink-ipa.XXXXXX")"
package="$(mktemp -d "${TMPDIR:-/tmp}/jaglink-payload.XXXXXX")"
trap 'rm -rf "$derived" "$package"' EXIT

xcodebuild \
  -project "$repo_root/app/ios/JAGLINK.xcodeproj" \
  -scheme JAGLINK \
  -configuration Release \
  -sdk iphoneos \
  -destination 'generic/platform=iOS' \
  -derivedDataPath "$derived" \
  MARKETING_VERSION="$version" \
  CODE_SIGNING_ALLOWED=NO \
  CODE_SIGNING_REQUIRED=NO \
  CODE_SIGN_IDENTITY='' \
  build

app="$derived/Build/Products/Release-iphoneos/JAGLINK.app"
test -d "$app"
test -x "$app/JAGLINK"
/usr/bin/lipo -info "$app/JAGLINK" | grep -q arm64

if [[ -z "$out_name" ]]; then
  out_name="JAGLINK-${version}-unsigned.ipa"
fi
ipa="$out_dir/$out_name"
mkdir -p "$package/Payload"
/usr/bin/ditto "$app" "$package/Payload/JAGLINK.app"
(
  cd "$package"
  /usr/bin/zip -qry "$ipa" Payload
)
/usr/bin/unzip -tq "$ipa"
(
  cd "$out_dir"
  shasum -a 256 "$out_name" > "$out_name.sha256"
)
printf '%s\n' "$ipa"
