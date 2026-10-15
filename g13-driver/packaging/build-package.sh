#!/bin/sh
#
# Baut ein .deb (Debian/Ubuntu) bzw. .rpm (Fedora) fuer das laufende System.
# Gedacht fuer einen frischen Container der Zieldistribution (als root):
#
#   docker run --rm -v "$PWD:/src" -w /src debian:13 g13-driver/packaging/build-package.sh dist
#
# Argument: Ausgabeverzeichnis (Standard: dist/ im Repository)
# Optional: G13_PACKAGE_VERSION, z. B. 2.1.0 (Standard: aus dem Git-Tag)
#
set -eu

# shellcheck source=/dev/null
. /etc/os-release

REPO_DIR=$(cd "$(dirname "$0")/../.." && pwd)
OUT_DIR=${1:-"$REPO_DIR/dist"}
BUILD_DIR=$(mktemp -d)

case "$ID" in
    debian|ubuntu)
        export DEBIAN_FRONTEND=noninteractive
        apt-get update -q
        apt-get install -y -q --no-install-recommends \
            build-essential cmake pkg-config git ca-certificates file dpkg-dev \
            libusb-1.0-0-dev qt6-base-dev qt6-tools-dev qt6-tools-dev-tools qt6-l10n-tools libgl-dev
        # libgl-dev: Qt 6.2 (Ubuntu 22.04) findet Qt6::Gui sonst nicht (WrapOpenGL)
        GENERATOR=DEB
        # z. B. 1~debian13 oder 1~ubuntu24.04 – eindeutige Dateinamen je System
        RELEASE="1~${ID}${VERSION_ID}"
        ;;
    fedora)
        dnf install -y -q \
            gcc-c++ make cmake pkgconf-pkg-config git rpm-build \
            libusb1-devel qt6-qtbase-devel qt6-qttools-devel
        GENERATOR=RPM
        RELEASE="1"          # %{?dist} (z. B. .fc44) haengt CPack an
        ;;
    *)
        echo "Nicht unterstuetzte Distribution: $ID" >&2
        exit 1
        ;;
esac

# Repository gehoert im Container einem anderen Benutzer -> fuer git describe freigeben
git config --global --add safe.directory '*'

set -- -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr -DBUILD_TESTS=OFF \
       -DG13_PACKAGE_RELEASE="$RELEASE"
if [ -n "${G13_PACKAGE_VERSION:-}" ]; then
    set -- "$@" -DG13_PACKAGE_VERSION="$G13_PACKAGE_VERSION"
fi

cmake -S "$REPO_DIR/g13-driver" -B "$BUILD_DIR" "$@"
cmake --build "$BUILD_DIR" -j "$(nproc)"
(cd "$BUILD_DIR" && cpack -G "$GENERATOR")

mkdir -p "$OUT_DIR"
find "$BUILD_DIR" -maxdepth 1 \( -name '*.deb' -o -name '*.rpm' \) -exec cp {} "$OUT_DIR"/ \;
echo "Pakete in $OUT_DIR:"
ls -1 "$OUT_DIR"
