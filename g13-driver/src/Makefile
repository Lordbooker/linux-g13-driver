# =============================================================================
# Universal Makefile for G13 Driver
# Delegates dependency handling to scripts/install_deps.sh
# =============================================================================

# --- User Detection ---
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
SCRIPT_DIR  := ./scripts

# GUI Paths
GUI_ROOT    := ../../g13-config-tool
GUI_JAR_SRC := $(GUI_ROOT)/target/Linux-G13-GUI.jar
GUI_JAR_NAME:= Linux-G13-GUI.jar

# Config Paths
BINDINGS_SRC:= ../bindings/.g13
INSTALL_DIR := $(USER_HOME)/.g13

.PHONY: all dependencies install-udev build-driver build-gui deploy clean uninstall

# =============================================================================
# Main Workflow
# =============================================================================
all: dependencies install-udev build-driver build-gui deploy clean-build
	@echo "========================================================"
	@echo " Installation Complete!"
	@echo "========================================================"
	@echo " Installed for User:   $(REAL_USER)"
	@echo " Install Location:     $(INSTALL_DIR)"
	@echo " Start the driver via: $(INSTALL_DIR)/$(TARGET_NAME)"
	@echo "========================================================"

# =============================================================================
# Step 1: Install Dependencies (External Script)
# =============================================================================
dependencies:
	@echo "--- Checking and Installing Dependencies ---"
	@chmod +x $(SCRIPT_DIR)/install_deps.sh
	@$(SCRIPT_DIR)/install_deps.sh

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