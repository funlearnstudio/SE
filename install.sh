#!/usr/bin/env sh
set -eu

REPO="funlearnstudio/SE"
DEFAULT_VERSION="0.7.3"

latest_release_version() {
  data=""

  if command -v curl >/dev/null 2>&1; then
    data="$(curl -fsSL --connect-timeout 10 "https://api.github.com/repos/$REPO/releases/latest" 2>/dev/null || true)"
  elif command -v wget >/dev/null 2>&1; then
    data="$(wget -qO- "https://api.github.com/repos/$REPO/releases/latest" 2>/dev/null || true)"
  fi

  version="$(printf '%s' "$data" | tr ',' '\n' | sed -n 's/.*"tag_name":[[:space:]]*"v\([^"]*\)".*/\1/p' | head -n 1)"
  if [ -n "$version" ]; then
    printf '%s' "$version"
  else
    printf '%s' "$DEFAULT_VERSION"
  fi
}

if [ -n "${SE_VERSION:-}" ]; then
  VERSION="${SE_VERSION#v}"
else
  VERSION="$(latest_release_version)"
fi

case "$VERSION" in
  *[!0-9A-Za-z.+-]*|.*|*..*|*.)
    echo "SE installer: invalid version '$VERSION'. Use a release such as 0.7.3 or v0.7.3." >&2
    exit 1
    ;;
esac
if ! printf '%s\n' "$VERSION" | awk -F '[.+-]' 'NF >= 3 && $1 ~ /^[0-9]+$/ && $2 ~ /^[0-9]+$/ && $3 ~ /^[0-9]+$/ { valid = 1 } END { exit !valid }'; then
  echo "SE installer: invalid version '$VERSION'. Use a release such as 0.7.3 or v0.7.3." >&2
  exit 1
fi

TAG="v$VERSION"
BASE_URL="https://github.com/$REPO/releases/download/$TAG"
INSTALL_ROOT="${SE_INSTALL_ROOT:-$HOME/.local/share/se}"
VERSION_DIR="$INSTALL_ROOT/$VERSION"
BIN_DIR="${SE_BIN_DIR:-$HOME/.local/bin}"
INSTALL_STDLIB_DEPS="${SE_INSTALL_STDLIB_DEPS:-0}"
SKIP_STDLIB_CHECK="${SE_SKIP_STDLIB_CHECK:-0}"

os="$(uname -s)"
arch="$(uname -m)"

case "$os" in
  Darwin) platform="macos" ;;
  Linux) platform="linux" ;;
  *) echo "SE installer: unsupported operating system: $os" >&2; exit 1 ;;
esac

case "$arch" in
  x86_64|amd64) machine="x64" ;;
  arm64|aarch64) machine="arm64" ;;
  *) echo "SE installer: unsupported architecture: $arch" >&2; exit 1 ;;
esac

if [ "$platform" = "linux" ] && [ "$machine" = "arm64" ]; then
  echo "SE $VERSION currently has no prebuilt Linux arm64 package." >&2
  exit 1
fi

asset="se-$VERSION-$platform-$machine"
archive="$asset.tar.gz"
url="$BASE_URL/$archive"

tmp="$(mktemp -d 2>/dev/null || mktemp -d -t se-install)"
cleanup() { rm -rf "$tmp"; }
trap cleanup EXIT INT TERM

have() {
  command -v "$1" >/dev/null 2>&1
}

fetch() {
  if have curl; then
    curl -fL --retry 3 --connect-timeout 15 "$1" -o "$2"
  elif have wget; then
    wget -O "$2" "$1"
  else
    echo "SE installer needs curl or wget only to download the prebuilt package." >&2
    exit 1
  fi
}

run_as_root() {
  if [ "$(id -u)" -eq 0 ]; then
    "$@"
  elif have sudo; then
    sudo "$@"
  else
    echo "SE installer: installing optional standard-library tools needs root or sudo." >&2
    return 1
  fi
}

install_stdlib_dependencies() {
  [ "$INSTALL_STDLIB_DEPS" = "1" ] || return 0

  echo
  echo "Installing optional SE standard-library tools..."

  if [ "$platform" = "macos" ]; then
    if ! have brew; then
      echo "SE installer: Homebrew was not found; optional tools were not installed automatically." >&2
      echo "Install Homebrew, then install missing tools such as sqlite, zip, or unzip." >&2
      return 0
    fi

    packages=""
    if ! have sqlite3; then packages="$packages sqlite"; fi
    if ! have zip; then packages="$packages zip"; fi
    if ! have unzip; then packages="$packages unzip"; fi

    if [ -n "$packages" ]; then
      # Intentional word splitting: each item is a Homebrew formula name.
      brew install $packages
    fi
    return 0
  fi

  packages=""
  if have apt-get; then
    if ! have sqlite3; then packages="$packages sqlite3"; fi
    if ! have zip; then packages="$packages zip"; fi
    if ! have unzip; then packages="$packages unzip"; fi
    if ! have sha256sum && ! have shasum; then packages="$packages coreutils"; fi
    if [ -n "$packages" ]; then
      run_as_root apt-get update
      # Intentional word splitting: each item is a package name.
      run_as_root apt-get install -y $packages
    fi
  elif have dnf; then
    if ! have sqlite3; then packages="$packages sqlite"; fi
    if ! have zip; then packages="$packages zip"; fi
    if ! have unzip; then packages="$packages unzip"; fi
    if ! have sha256sum && ! have shasum; then packages="$packages coreutils"; fi
    if [ -n "$packages" ]; then
      run_as_root dnf install -y $packages
    fi
  elif have yum; then
    if ! have sqlite3; then packages="$packages sqlite"; fi
    if ! have zip; then packages="$packages zip"; fi
    if ! have unzip; then packages="$packages unzip"; fi
    if ! have sha256sum && ! have shasum; then packages="$packages coreutils"; fi
    if [ -n "$packages" ]; then
      run_as_root yum install -y $packages
    fi
  elif have pacman; then
    if ! have sqlite3; then packages="$packages sqlite"; fi
    if ! have zip; then packages="$packages zip"; fi
    if ! have unzip; then packages="$packages unzip"; fi
    if ! have sha256sum && ! have shasum; then packages="$packages coreutils"; fi
    if [ -n "$packages" ]; then
      run_as_root pacman -S --needed --noconfirm $packages
    fi
  elif have apk; then
    if ! have sqlite3; then packages="$packages sqlite"; fi
    if ! have zip; then packages="$packages zip"; fi
    if ! have unzip; then packages="$packages unzip"; fi
    if ! have sha256sum && ! have shasum; then packages="$packages coreutils"; fi
    if [ -n "$packages" ]; then
      run_as_root apk add $packages
    fi
  elif have zypper; then
    if ! have sqlite3; then packages="$packages sqlite3"; fi
    if ! have zip; then packages="$packages zip"; fi
    if ! have unzip; then packages="$packages unzip"; fi
    if ! have sha256sum && ! have shasum; then packages="$packages coreutils"; fi
    if [ -n "$packages" ]; then
      run_as_root zypper --non-interactive install $packages
    fi
  else
    echo "SE installer: no supported package manager was found for optional standard-library tools." >&2
  fi
}

stdlib_install_hint() {
  if [ "$platform" = "macos" ]; then
    echo "  Hint: brew install sqlite zip unzip"
    return
  fi

  if have apt-get; then
    echo "  Hint: sudo apt-get install sqlite3 zip unzip coreutils"
  elif have dnf; then
    echo "  Hint: sudo dnf install sqlite zip unzip coreutils"
  elif have yum; then
    echo "  Hint: sudo yum install sqlite zip unzip coreutils"
  elif have pacman; then
    echo "  Hint: sudo pacman -S sqlite zip unzip coreutils"
  elif have apk; then
    echo "  Hint: sudo apk add sqlite zip unzip coreutils"
  elif have zypper; then
    echo "  Hint: sudo zypper install sqlite3 zip unzip coreutils"
  else
    echo "  Install sqlite3, zip, unzip, and a SHA-256 tool with your system package manager."
  fi
}

report_stdlib_dependencies() {
  [ "$SKIP_STDLIB_CHECK" = "1" ] && return 0

  echo
  echo "SE standard-library tool check:"

  missing=0

  if have sqlite3; then
    echo "  [ok] sqlite3     sqlite / sqlite3"
  else
    echo "  [--] sqlite3     needed by sqlite / sqlite3"
    missing=1
  fi

  if have zip; then
    echo "  [ok] zip         zip.create"
  else
    echo "  [--] zip         needed by zip.create"
    missing=1
  fi

  if have unzip; then
    echo "  [ok] unzip       zip.extract / zip.list"
  else
    echo "  [--] unzip       needed by zip.extract / zip.list"
    missing=1
  fi

  if have sha256sum; then
    echo "  [ok] sha256sum   hash / hashlib"
  elif have shasum; then
    echo "  [ok] shasum      hash / hashlib"
  else
    echo "  [--] SHA-256     needed by hash / hashlib"
    missing=1
  fi

  if [ "$missing" -ne 0 ]; then
    echo
    echo "SE itself is installed and the rest of the standard library still works."
    echo "Only the modules listed above need the missing host tools."
    stdlib_install_hint
    echo "  Or rerun with SE_INSTALL_STDLIB_DEPS=1 to let this installer try automatically."
  else
    echo "  All optional tools used by the new standard-library modules are available."
  fi
}

report_python_dependencies() {
  echo
  echo "SE Python-backed module check:"

  python_cmd=""
  for candidate in python3.14 python3.13 python3.12 python3.11 python3 python; do
    if have "$candidate"; then
      python_cmd="$candidate"
      break
    fi
  done

  if [ -z "$python_cmd" ]; then
    echo "  [--] Python 3.11+    needed by toml, yaml, xml, markdown, crypto, auth, email, and network integrations"
    echo "  Install Python 3.11 or newer to use these modules."
    return 0
  fi

  python_version="$("$python_cmd" -c 'import sys; print("%d.%d" % sys.version_info[:2])' 2>/dev/null || true)"
  if ! printf '%s\n' "$python_version" | awk -F. '$1 > 3 || ($1 == 3 && $2 >= 11) { ok = 1 } END { exit !ok }'; then
    echo "  [--] $python_cmd $python_version    Python 3.11+ is needed by toml, yaml, xml, markdown, crypto, auth, email, and network integrations"
    return 0
  fi
  echo "  [ok] $python_cmd $python_version"

  missing_python_packages=""
  for package in 'yaml:PyYAML' 'markdown_it:markdown-it-py' 'paramiko:paramiko' 'websockets:websockets'; do
    import_name="${package%%:*}"
    package_name="${package#*:}"
    if "$python_cmd" -c "import $import_name" >/dev/null 2>&1; then
      echo "  [ok] $package_name"
    else
      echo "  [--] $package_name    needed by the matching SE module"
      missing_python_packages="$missing_python_packages $package_name"
    fi
  done

  if [ -n "$missing_python_packages" ]; then
    echo "  Optional packages can be installed with: $python_cmd -m pip install$missing_python_packages"
  fi
}

echo "Installing SE $VERSION for $platform-$machine..."
fetch "$url" "$tmp/$archive"

checksum_file="$tmp/SHA256SUMS.txt"
fetch "$BASE_URL/SHA256SUMS.txt" "$checksum_file"

if ! have sha256sum && ! have shasum; then
  echo "SE installer: a SHA-256 tool (sha256sum or shasum) is required to verify the release package." >&2
  exit 1
fi

expected="$(awk -v file="$archive" '$2 == file || $2 == "*" file { print $1; exit }' "$checksum_file")"
case "$expected" in
  *[!0-9A-Fa-f]*|'')
    echo "SE installer: SHA256SUMS.txt does not contain a valid checksum for $archive." >&2
    exit 1
    ;;
esac
if [ "${#expected}" -ne 64 ]; then
  echo "SE installer: SHA256SUMS.txt does not contain a valid checksum for $archive." >&2
  exit 1
fi

if have sha256sum; then
  actual="$(sha256sum "$tmp/$archive" | awk '{ print $1 }')"
else
  actual="$(shasum -a 256 "$tmp/$archive" | awk '{ print $1 }')"
fi
if [ "$(printf '%s' "$actual" | tr 'A-F' 'a-f')" != "$(printf '%s' "$expected" | tr 'A-F' 'a-f')" ]; then
  echo "SE installer: release checksum verification failed for $archive." >&2
  exit 1
fi
echo "Verified SHA-256 checksum for $archive."

if ! tar -tzf "$tmp/$archive" > "$tmp/archive-files.txt"; then
  echo "SE installer: the downloaded release archive is invalid." >&2
  exit 1
fi
while IFS= read -r member || [ -n "$member" ]; do
  case "$member" in
    "$asset"|"$asset/"|"$asset"/*) ;;
    *) echo "SE installer: archive contains an unexpected path: $member" >&2; exit 1 ;;
  esac
  case "/$member/" in
    */../*|//*) echo "SE installer: archive contains an unsafe path: $member" >&2; exit 1 ;;
  esac
done < "$tmp/archive-files.txt"

tar -xzf "$tmp/$archive" -C "$tmp"

mkdir -p "$INSTALL_ROOT" "$BIN_DIR"
rm -rf "$VERSION_DIR"
mv "$tmp/$asset" "$VERSION_DIR"
ln -sfn "$VERSION_DIR/bin/se" "$BIN_DIR/se"

install_stdlib_dependencies

case ":$PATH:" in
  *":$BIN_DIR:"*) ;;
  *)
    echo
    echo "SE was installed, but $BIN_DIR is not currently in PATH."
    echo "Add this line to ~/.zshrc or ~/.bashrc:"
    echo "  export PATH=\"$BIN_DIR:\$PATH\""
    ;;
esac

echo
echo "Installed: $BIN_DIR/se"
"$BIN_DIR/se" --version
report_stdlib_dependencies
report_python_dependencies

echo
printf '%s\n' 'Try: se run hello.se'
printf '%s\n' 'No CMake, Git, or C++ compiler is required for se run/check/test.'
printf '%s\n' 'Native `se build` still requires a system C++20 compiler.'
