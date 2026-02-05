#!/bin/bash

# Script: install_deps.sh
# Description: Detects the package manager and installs build dependencies.
# Supports: Arch (inc. CachyOS, Manjaro), Debian/Ubuntu, Fedora, OpenSUSE.

set -e # Exit immediately if a command exits with a non-zero status.

# --- Package Lists ---

# ARCH LINUX (CachyOS, Manjaro, EndeavourOS)
# 'base-devel' includes: make, gcc, automake, etc.
PKG_ARCH="base-devel cmake libusb gtk3 libappindicator-gtk3 maven jdk17-openjdk"

# DEBIAN / UBUNTU (Pop!_OS, Mint, Kali)
# 'build-essential' includes: make, gcc, g++, etc.
PKG_DEBIAN="build-essential cmake libusb-1.0-0-dev libgtk-3-dev libappindicator3-dev maven openjdk-17-jdk"

# FEDORA (RHEL, CentOS, Nobara)
# Explicitly listing 'make' here.
PKG_FEDORA="make automake cmake gcc gcc-c++ kernel-devel libusb1-devel gtk3-devel libappindicator-gtk3-devel maven java-17-openjdk-devel"

# OPENSUSE (Tumbleweed, Leap)
# Explicitly listing 'make' here.
PKG_SUSE="make cmake gcc-c++ libusb-1_0-devel gtk3-devel libappindicator3-devel maven java-17-openjdk-devel"


echo "--- Detecting Package Manager ---"

if command -v pacman &> /dev/null; then
    echo "Detected System: Arch Linux based (pacman)"
    echo "Installing: $PKG_ARCH"
    # --needed: Skips packages that are already up-to-date
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
    # FALLBACK: If no known package manager is found
    echo "================================================================="
    echo " WARNING: Could not detect a supported package manager."
    echo " (apt, dnf, pacman, zypper not found or not standard)"
    echo "================================================================="
    echo " You need to install the following dependencies manually:"
    echo "  1. C++ Compiler (g++ / clang) & Make"
    echo "  2. CMake (>= 3.10)"
    echo "  3. libusb-1.0 (Development Headers)"
    echo "  4. GTK 3 (Development Headers)"
    echo "  5. AppIndicator3 (Development Headers)"
    echo "  6. Java JDK 17 & Maven"
    echo "================================================================="
    read -p "Press ENTER to continue anyway (if you have installed them manually)..."
fi

echo "--- Dependency Check Complete ---"