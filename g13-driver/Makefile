# =============================================================================
# Universal Makefile for G13 Driver
# =============================================================================

# --- Standard Build Variables ---
PREFIX      ?= /usr
BINDIR      ?= $(PREFIX)/bin
DATADIR     ?= $(PREFIX)/share
UDEVDIR     ?= /usr/lib/udev/rules.d
SYSTEMDDIR  ?= /usr/lib/systemd/user
SYSCONFDIR  ?= /etc
APPDIR      := $(DATADIR)/applications
ICONDIR     := $(DATADIR)/icons/hicolor
AUTOSTARTDIR:= $(SYSCONFDIR)/xdg/autostart
ICON_SIZES  := 22 32 48 64 128 256

# --- Directories & Paths ---
BUILD_DIR   := build
TARGET_DRIVER := linux-g13-driver
TARGET_GUI    := g13-gui

# Scripts & Configs
SCRIPT_DIR  := scripts
SERVICE_SRC := src/systemd/g13.service
UDEV_SRC    := src/udev/99-g13.rules
DESKTOP_SRC := src/desktop/g13-gui.desktop
TRAY_SRC    := src/desktop/g13-gui-tray.desktop
ICON_SRC    := src/resources/icons

.PHONY: all dependencies build-driver build-gui clean install install-user uninstall remove-legacy

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
	
	# Menueeintrag, Tray-Autostart, Icons
	install -Dm 644 $(DESKTOP_SRC) $(DESTDIR)$(APPDIR)/g13-gui.desktop
	install -Dm 644 $(TRAY_SRC) $(DESTDIR)$(AUTOSTARTDIR)/g13-gui-tray.desktop
	install -Dm 644 $(ICON_SRC)/g13-gui.svg $(DESTDIR)$(ICONDIR)/scalable/apps/g13-gui.svg
	for s in $(ICON_SIZES); do \
		install -Dm 644 $(ICON_SRC)/g13-gui-$$s.png $(DESTDIR)$(ICONDIR)/$${s}x$${s}/apps/g13-gui.png; \
	done
	
	@echo "System install done."
	@echo "To enable the service, run: systemctl --user enable --now g13"
	
	# Warn about files of the old version (GTK daemon / Java tool). Never fails.
	# Skipped for package builds (DESTDIR set), they must not inspect the build host.
	-@if [ -z "$(DESTDIR)" ]; then bash $(SCRIPT_DIR)/remove_legacy.sh --check; fi

install-user:
	@echo "--- User Local Install not fully supported in this version ---"
	@echo "Please use 'sudo make install' or build an Arch Package."

# Removes files of the old version (GTK tray daemon, Java config tool) from
# old 'make install' / 'make install-user' runs. Run WITHOUT sudo; the script
# asks for sudo itself. Files owned by a package manager are not touched.
remove-legacy:
	@bash $(SCRIPT_DIR)/remove_legacy.sh

# Removes exactly the files installed by 'install'.
# The user configuration in ~/.config/g13 is kept.
uninstall:
	@echo "--- Removing installed files from $(DESTDIR)$(PREFIX) ---"
	rm -f $(DESTDIR)$(BINDIR)/linux-g13-driver
	rm -f $(DESTDIR)$(BINDIR)/g13-gui
	rm -f $(DESTDIR)$(UDEVDIR)/99-g13.rules
	rm -f $(DESTDIR)$(SYSTEMDDIR)/g13.service
	rm -f $(DESTDIR)$(APPDIR)/g13-gui.desktop
	rm -f $(DESTDIR)$(AUTOSTARTDIR)/g13-gui-tray.desktop
	rm -f $(DESTDIR)$(ICONDIR)/scalable/apps/g13-gui.svg
	for s in $(ICON_SIZES); do rm -f $(DESTDIR)$(ICONDIR)/$${s}x$${s}/apps/g13-gui.png; done
	@echo "Uninstall done. Your configuration in ~/.config/g13 was kept."
	@echo "If the service was enabled, run: systemctl --user disable --now g13"

clean:
	rm -rf $(BUILD_DIR)