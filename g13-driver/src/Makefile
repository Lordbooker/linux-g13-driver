# =============================================================================
# Universal Makefile for G13 Driver (Arch, Ubuntu, Fedora)
# =============================================================================

# --- User Detection ---
# Detects the real user even when running with 'sudo make'
ifdef SUDO_USER
    REAL_USER := $(SUDO_USER)
    USER_HOME := $(shell getent passwd $(SUDO_USER) | cut -d: -f6)
else
    REAL_USER := $(shell whoami)
    USER_HOME := $(HOME)
endif

# --- Directories & Paths ---
BUILD_DIR   := ../build
TARGET_NAME := Linux-G13-Driver
TARGET_PATH := ../$(TARGET_NAME)

# GUI Paths
GUI_ROOT    := ../../g13-config-tool
GUI_JAR_SRC := $(GUI_ROOT)/target/Linux-G13-GUI.jar
GUI_JAR_NAME:= Linux-G13-GUI.jar

# Config Paths
BINDINGS_SRC:= ../bindings/.g13
INSTALL_DIR := $(USER_HOME)/.g13

# --- Distro Detection ---
HAS_PACMAN := $(shell command -v pacman 2> /dev/null)
HAS_APT    := $(shell command -v apt-get 2> /dev/null)
HAS_DNF    := $(shell command -v dnf 2> /dev/null)

.PHONY: all dependencies install-udev build-driver build-gui deploy clean uninstall

# =============================================================================
# Main Workflow
# =============================================================================
all: dependencies install-udev build-driver build-gui deploy clean-build
	@echo "========================================================"
	@echo " Installation Complete!"
	@echo " Distro Support: Arch, Ubuntu, Fedora detected."
	@echo "========================================================"
	@echo " Installed for User:   $(REAL_USER)"
	@echo " Install Location:     $(INSTALL_DIR)"
	@echo " Start the driver via: $(INSTALL_DIR)/$(TARGET_NAME)"
	@echo "========================================================"

# =============================================================================
# Step 1: Install Dependencies (Distro-Specific)
# =============================================================================
dependencies:
	@echo "--- Checking and Installing Dependencies ---"
ifdef HAS_PACMAN
	@echo "Detected Arch Linux (pacman)"
	sudo pacman -S --noconfirm base-devel cmake libusb gtk3 libappindicator-gtk3 maven jdk17-openjdk
else ifdef HAS_APT
	@echo "Detected Debian/Ubuntu (apt)"
	sudo apt-get update
	sudo apt-get install -y build-essential cmake libusb-1.0-0-dev libgtk-3-dev libappindicator3-dev maven openjdk-17-jdk
else ifdef HAS_DNF
	@echo "Detected Fedora/RHEL (dnf)"
	sudo dnf install -y make automake cmake gcc gcc-c++ kernel-devel libusb1-devel gtk3-devel libappindicator-gtk3-devel maven java-17-openjdk-devel
endif

# =============================================================================
# Step 2: Install UDEV Rules (Universal)
# =============================================================================
install-udev:
	@echo "--- Installing Universal UDEV Rules ---"
	@# Find correct udev directory
	@UDEV_RULES_DIR=""; \
	if [ -d /etc/udev/rules.d ]; then UDEV_RULES_DIR="/etc/udev/rules.d"; \
	elif [ -d /lib/udev/rules.d ]; then UDEV_RULES_DIR="/lib/udev/rules.d"; \
	else echo "Error: Udev directory not found." >&2; exit 1; fi; \
	\
	# Copy rule file
	sudo cp udev/99-g13.rules $$UDEV_RULES_DIR/99-g13.rules; \
	\
	# Reload rules
	echo "Reloading UDEV rules..."; \
	sudo udevadm control --reload-rules && sudo udevadm trigger

# =============================================================================
# Step 3: Build C++ Driver (Via CMake)
# =============================================================================
build-driver:
	@echo "--- Building C++ Driver with CMake ---"
	mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR) && cmake ../src && make
	cp $(BUILD_DIR)/$(TARGET_NAME) $(TARGET_PATH)

# =============================================================================
# Step 4: Build Java GUI (Maven)
# =============================================================================
build-gui:
	@echo "--- Building Java GUI with Maven ---"
ifdef SUDO_USER
	cd $(GUI_ROOT) && sudo -u $(REAL_USER) mvn clean package
else
	cd $(GUI_ROOT) && mvn clean package
endif

# =============================================================================
# Step 5: Deploy
# =============================================================================
deploy:
	@echo "--- Deploying files to $(INSTALL_DIR) ---"
	mkdir -p $(INSTALL_DIR)
	cp $(TARGET_PATH) $(INSTALL_DIR)/
	cp $(GUI_JAR_SRC) $(INSTALL_DIR)/$(GUI_JAR_NAME)
	# Only copy configs if none exist to avoid overwriting user settings
	if [ -z "$$(ls -A $(INSTALL_DIR)/*.properties 2>/dev/null)" ]; then \
		echo "Copying default configs..."; \
		cp $(BINDINGS_SRC)/*.properties $(INSTALL_DIR)/; \
	fi
	chmod +x $(INSTALL_DIR)/$(TARGET_NAME)
ifdef SUDO_USER
	chown -R $(REAL_USER): $(INSTALL_DIR)
endif

# =============================================================================
# Step 6: Cleanup
# =============================================================================
clean-build:
	rm -rf $(BUILD_DIR)

clean: clean-build
	rm -f $(TARGET_PATH)
ifdef SUDO_USER
	cd $(GUI_ROOT) && sudo -u $(REAL_USER) mvn clean
else
	cd $(GUI_ROOT) && mvn clean
endif
	@echo "Full clean finished."

uninstall:
	@echo "Removing UDEV rules..."
	sudo rm -f /etc/udev/rules.d/99-g13.rules /lib/udev/rules.d/99-g13.rules
	sudo udevadm control --reload-rules
	@echo "To remove the application, delete $(INSTALL_DIR)"