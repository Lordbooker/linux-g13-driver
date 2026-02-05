# =============================================================================
# Universal Makefile for G13 Driver
# =============================================================================

# --- Standard Build Variables (System Install) ---
PREFIX      ?= /usr
BINDIR      ?= $(PREFIX)/bin
DATADIR     ?= $(PREFIX)/share
JAVADIR     ?= $(DATADIR)/java/linux-g13-driver
UDEVDIR     ?= /usr/lib/udev/rules.d
SYSTEMDDIR  ?= /usr/lib/systemd/user

# --- Directories & Paths ---
BUILD_DIR   := ../build
TARGET_NAME := Linux-G13-Driver
TARGET_PATH := ../$(TARGET_NAME)
SCRIPT_DIR  := ./scripts
SERVICE_SRC := ./systemd/g13.service

# GUI Paths
GUI_ROOT    := ../../g13-config-tool
GUI_JAR_SRC := $(GUI_ROOT)/target/Linux-G13-GUI.jar
GUI_JAR_NAME:= Linux-G13-GUI.jar

# Config Paths (Source)
BINDINGS_SRC:= ../bindings/.g13

.PHONY: all dependencies build-driver build-gui clean install install-user uninstall

# =============================================================================
# Main Workflow
# =============================================================================
all: dependencies build-driver build-gui
	@echo "Build complete."
	@echo " -> Run 'sudo make install' to install system-wide (Package Mode)."
	@echo " -> Run 'make install-user' to install for current user (Dev Mode)."

dependencies:
	@chmod +x $(SCRIPT_DIR)/install_deps.sh
	@$(SCRIPT_DIR)/install_deps.sh

build-driver:
	@echo "--- Building C++ Driver ---"
	mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR) && cmake ../src && make
	cp $(BUILD_DIR)/$(TARGET_NAME) $(TARGET_PATH)

build-gui:
	@echo "--- Building Java GUI ---"
	cd $(GUI_ROOT) && mvn clean package -DskipTests

# =============================================================================
# System-Wide Installation (Root / Package Manager)
# Ships the service file to /usr/lib/systemd/user but does NOT enable it.
# =============================================================================
install:
	@echo "--- Installing System-Wide to $(DESTDIR)$(PREFIX) ---"
	
	# Binaries & Jar
	install -d $(DESTDIR)$(BINDIR)
	install -m 755 $(TARGET_PATH) $(DESTDIR)$(BINDIR)/linux-g13-driver
	install -d $(DESTDIR)$(JAVADIR)
	install -m 644 $(GUI_JAR_SRC) $(DESTDIR)$(JAVADIR)/$(GUI_JAR_NAME)
	
	# Wrapper
	echo '#!/bin/sh' > g13-gui
	echo 'java -jar $(JAVADIR)/$(GUI_JAR_NAME) "$$@"' >> g13-gui
	install -m 755 g13-gui $(DESTDIR)$(BINDIR)/g13-gui
	rm g13-gui
	
	# UDEV
	install -d $(DESTDIR)$(UDEVDIR)
	install -m 644 udev/99-g13.rules $(DESTDIR)$(UDEVDIR)/99-g13.rules
	
	# Systemd (Copy static file)
	install -d $(DESTDIR)$(SYSTEMDDIR)
	install -m 644 $(SERVICE_SRC) $(DESTDIR)$(SYSTEMDDIR)/g13.service
	
	@echo "System install done."
	@echo "To enable the service, the user must run: systemctl --user enable --now g13"

# =============================================================================
# User-Local Installation
# Modifies the service file to point to HOME and starts it.
# =============================================================================
USER_INSTALL_DIR := $(HOME)/.local/bin
USER_CONFIG_DIR  := $(HOME)/.config/g13
USER_SYSTEMD_DIR := $(HOME)/.config/systemd/user

install-user:
	@echo "--- Installing for Current User ($(USER)) ---"
	mkdir -p $(USER_INSTALL_DIR)
	mkdir -p $(USER_CONFIG_DIR)
	mkdir -p $(USER_SYSTEMD_DIR)
	
	# Copy Files
	cp $(TARGET_PATH) $(USER_INSTALL_DIR)/linux-g13-driver
	cp $(GUI_JAR_SRC) $(USER_INSTALL_DIR)/$(GUI_JAR_NAME)
	
	# Wrapper
	echo '#!/bin/sh' > $(USER_INSTALL_DIR)/g13-gui
	echo 'java -jar $(USER_INSTALL_DIR)/$(GUI_JAR_NAME) "$$@"' >> $(USER_INSTALL_DIR)/g13-gui
	chmod +x $(USER_INSTALL_DIR)/g13-gui
	
	# Default Configs
	if [ -z "$$(ls -A $(USER_CONFIG_DIR)/*.properties 2>/dev/null)" ]; then \
		echo "Copying default configs..."; \
		cp $(BINDINGS_SRC)/*.properties $(USER_CONFIG_DIR)/; \
	fi
	
	# Systemd: Transform static file to use User Path
	@echo "Configuring Systemd Service..."
	sed 's|/usr/bin/linux-g13-driver|$(USER_INSTALL_DIR)/linux-g13-driver|g' $(SERVICE_SRC) > $(USER_SYSTEMD_DIR)/g13.service
	
	# Enable and Start
	@echo "Enabling and Starting Service..."
	systemctl --user daemon-reload
	systemctl --user enable g13.service
	-systemctl --user restart g13.service
	
	# UDEV (Needs Sudo)
	@echo "Installing UDEV rules (sudo required)..."
	sudo cp udev/99-g13.rules /etc/udev/rules.d/
	sudo udevadm control --reload-rules && sudo udevadm trigger
	
	@echo "====================================================="
	@echo " Installation Complete!"
	@echo " GUI Tool: $(USER_INSTALL_DIR)/g13-gui"
	@echo " Logs: journalctl --user -u g13 -f"
	@echo "====================================================="

clean-build:
	rm -rf $(BUILD_DIR)

clean: clean-build
	rm -f $(TARGET_PATH)
	cd $(GUI_ROOT) && mvn clean