#!/bin/bash

# Script: install_deps.sh
# Description: Detects the package manager and installs build dependencies.
# Supports: Arch, Debian/Ubuntu, Fedora, OpenSUSE.

set -e

# --- Package Lists ---

# ARCH LINUX
PKG_ARCH="base-devel cmake libusb gtk3 libappindicator-gtk3 qt6-base qt6-tools"

# DEBIAN / UBUNTU
# Note: qt6-base-dev might vary slightly by version
PKG_DEBIAN="build-essential cmake libusb-1.0-0-dev libgtk-3-dev libappindicator3-dev qt6-base-dev qt6-tools-dev qt6-tools-dev-tools"

# FEDORA
PKG_FEDORA="make automake cmake gcc gcc-c++ kernel-devel libusb1-devel gtk3-devel libappindicator-gtk3-devel qt6-qtbase-devel qt6-qttools-devel"

# OPENSUSE
PKG_SUSE="make cmake gcc-c++ libusb-1_0-devel gtk3-devel libappindicator3-devel qt6-base-devel qt6-tools-devel"


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
    echo "Detected System: OpenSUSE (zypper)"
    echo "Installing: $PKG_SUSE"
    sudo zypper install -y $PKG_SUSE

else
    echo "================================================================="
    echo " WARNING: Could not detect a supported package manager."
    echo "================================================================="
    echo " You need to install: CMake, g++, libusb-1.0, GTK3, Qt6 (Base & Tools)"
    echo "================================================================="
fi

echo "--- Dependency Check Complete ---"