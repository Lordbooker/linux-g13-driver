# =============================================================================
# Universal Makefile for G13 Driver
# =============================================================================

# --- Standard Build Variables ---
PREFIX      ?= /usr
BINDIR      ?= $(PREFIX)/bin
DATADIR     ?= $(PREFIX)/share
UDEVDIR     ?= /usr/lib/udev/rules.d
SYSTEMDDIR  ?= /usr/lib/systemd/user

# --- Directories & Paths ---
BUILD_DIR   := build
TARGET_DRIVER := linux-g13-driver
TARGET_GUI    := g13-gui

# Scripts & Configs
SCRIPT_DIR  := scripts
SERVICE_SRC := src/systemd/g13.service
UDEV_SRC    := src/udev/99-g13.rules

.PHONY: all dependencies build-driver build-gui clean install install-user uninstall

# =============================================================================
# Main Workflow
# =============================================================================
all: dependencies build-driver
	@echo "Build complete."
	@echo " -> Run 'sudo make install' to install system-wide."

dependencies:
	@chmod +x $(SCRIPT_DIR)/install_deps.sh
	@$(SCRIPT_DIR)/install_deps.sh

# Uses CMake to build both targets (Driver + GUI)
build-driver:
	@echo "--- Building Driver & GUI (via CMake) ---"
	mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR) && cmake .. && make

# Alias for compatibility
build-gui: build-driver

# =============================================================================
# Installation
# =============================================================================
install:
	@echo "--- Installing System-Wide to $(DESTDIR)$(PREFIX) ---"
	
	# Binaries
	install -d $(DESTDIR)$(BINDIR)
	install -m 755 $(BUILD_DIR)/$(TARGET_DRIVER) $(DESTDIR)$(BINDIR)/linux-g13-driver
	install -m 755 $(BUILD_DIR)/$(TARGET_GUI) $(DESTDIR)$(BINDIR)/g13-gui
	
	# UDEV
	install -d $(DESTDIR)$(UDEVDIR)
	install -m 644 $(UDEV_SRC) $(DESTDIR)$(UDEVDIR)/99-g13.rules
	
	# Systemd
	install -d $(DESTDIR)$(SYSTEMDDIR)
	install -m 644 $(SERVICE_SRC) $(DESTDIR)$(SYSTEMDDIR)/g13.service
	
	@echo "System install done."
	@echo "To enable the service, run: systemctl --user enable --now g13"

install-user:
	@echo "--- User Local Install not fully supported in this version ---"
	@echo "Please use 'sudo make install' or build an Arch Package."

clean:
	rm -rf $(BUILD_DIR)