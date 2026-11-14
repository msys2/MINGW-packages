#!/usr/bin/env bash
set -euo pipefail

NEW_VER="${1:-}"

if [[ -z "$NEW_VER" ]]; then
  echo "Usage: $0 <pkgver>"
  echo "Example: $0 2.6.10"
  exit 1
fi

PKGBUILD_FILE="PKGBUILD"

if [[ ! -f "$PKGBUILD_FILE" ]]; then
  echo "Error: PKGBUILD file not found in current directory." >&2
  exit 1
fi

echo "==> Updating PKGBUILD version to ${NEW_VER}..."
sed -i -E "s/^(pkgver=).*/\1${NEW_VER}/" "$PKGBUILD_FILE"
sed -i -E "s/^(pkgrel=).*/\11/" "$PKGBUILD_FILE"

echo "==> Masking secondary sha256sums+= to _sha256sums+= to prevent updpkgsums from overwriting..."
sed -i 's/\bsha256sums+=/_sha256sums+=/g' "$PKGBUILD_FILE"

if command -v updpkgsums >/dev/null 2>&1; then
  echo "==> Running updpkgsums for base sources..."
  updpkgsums || echo "Warning: updpkgsums exited with a non-zero code."
else
  echo "Warning: updpkgsums not found in PATH, skipping automatic base sum updates."
fi

echo "==> Restoring secondary sha256sums+= from _sha256sums+=..."
sed -i 's/\b_sha256sums+=/sha256sums+=/g' "$PKGBUILD_FILE"

ARM64_URL="https://downloads.sourceforge.net/project/sbcl/sbcl/${NEW_VER}/sbcl-${NEW_VER}-arm64-windows-binary.msi"
X86_64_URL="https://downloads.sourceforge.net/project/sbcl/sbcl/${NEW_VER}/sbcl-${NEW_VER}-x86-64-windows-binary.msi"

echo "==> Fetching SHA256 for CLANGARM64 bootstrap archive..."
ARM64_SHA=$(curl -fsSL "$ARM64_URL" | sha256sum | awk '{print $1}')
echo "    ARM64 SHA256: ${ARM64_SHA}"

echo "==> Fetching SHA256 for x86_64 bootstrap archive..."
X86_64_SHA=$(curl -fsSL "$X86_64_URL" | sha256sum | awk '{print $1}')
echo "    x86_64 SHA256: ${X86_64_SHA}"

echo "==> Updating stage-0 bootstrap checksums in PKGBUILD..."
python3 -c "
import sys, re

ver, arm_sha, x86_sha = sys.argv[1], sys.argv[2], sys.argv[3]

with open('PKGBUILD', 'r') as f:
    content = f.read()

# Replace ARM64 stage-0 sha256sum
content = re.sub(
    r'(arm64-windows-binary\.msi[^\n]+\n\s*sha256sums\+=\(\x27)[a-f0-9]{64}(\x27\))',
    rf'\g<1>{arm_sha}\2',
    content
)

# Replace x86_64 stage-0 sha256sum
content = re.sub(
    r'(x86-64-windows-binary\.msi[^\n]+\n\s*sha256sums\+=\(\x27)[a-f0-9]{64}(\x27\))',
    rf'\g<1>{x86_sha}\2',
    content
)

with open('PKGBUILD', 'w') as f:
    f.write(content)
" "$NEW_VER" "$ARM64_SHA" "$X86_64_SHA"

echo "==> PKGBUILD successfully updated to version ${NEW_VER}!"
