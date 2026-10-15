#!/bin/bash

# Script: install_deps.sh
# Description: Detects the package manager and installs the build dependencies
#              for the G13 driver (C++/libusb) and the Qt6 config tool.
# Supports:    Arch, Debian/Ubuntu, Fedora, openSUSE.
#
# Required to build:
#   - C++ compiler with C++23 support, make, CMake >= 3.20, pkg-config
#   - libusb-1.0 (development headers)
#   - Qt6 Base (Widgets, Core, Gui incl. moc/uic) - only for the config tool
#
# GTK3, AppIndicator and Java are no longer needed.

set -e

# --- Package Lists ---

# ARCH LINUX (CachyOS, Manjaro, EndeavourOS)
# 'base-devel' includes gcc, make and pkgconf.
PKG_ARCH="base-devel cmake libusb qt6-base"

# DEBIAN / UBUNTU
PKG_DEBIAN="build-essential cmake pkg-config libusb-1.0-0-dev qt6-base-dev"

# FEDORA
PKG_FEDORA="gcc-c++ make cmake pkgconf-pkg-config libusb1-devel qt6-qtbase-devel"

# OPENSUSE
PKG_SUSE="gcc-c++ make cmake pkg-config libusb-1_0-devel qt6-base-devel"


echo "--- Detecting Package Manager ---"

if command -v pacman &> /dev/null; then
    echo "Detected System: Arch Linux based (pacman)"
    echo "Installing: $PKG_ARCH"
    sudo pacman -S --needed --noconfirm $PKG_ARCH

elif command -v apt-get &> /dev/null; then
    echo "Detected System: Debian/Ubuntu based (apt)"
    echo "Installing: $PKG_DEBIAN"
    sudo apt-get update
    sudo apt-get install -y $PKG_DEBIAN

elif command -v dnf &> /dev/null; then
    echo "Detected System: Fedora based (dnf)"
    echo "Installing: $PKG_FEDORA"
    sudo dnf install -y $PKG_FEDORA

elif command -v zypper &> /dev/null; then
    echo "Detected System: openSUSE (zypper)"
    echo "Installing: $PKG_SUSE"
    sudo zypper install -y $PKG_SUSE

else
    echo "================================================================="
    echo " WARNING: Could not detect a supported package manager."
    echo " (pacman, apt-get, dnf, zypper not found)"
    echo "================================================================="
    echo " Please install the following manually:"
    echo "  1. C++ compiler with C++23 support, make, pkg-config"
    echo "  2. CMake (>= 3.20)"
    echo "  3. libusb-1.0 (development headers)"
    echo "  4. Qt6 Base (Widgets, Core, Gui) - for the config tool"
    echo " If they are already installed, the build will continue."
    echo "================================================================="
fi

echo "--- Dependency Check Complete ---"
