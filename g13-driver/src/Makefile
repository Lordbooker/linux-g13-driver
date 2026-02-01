# =============================================================================
# Modernized Makefile for G13 Driver & GUI Installation
# =============================================================================

# --- User Detection (Robust against sudo) ---
# Erkennt den echten User, auch wenn 'sudo make' ausgeführt wird
ifdef SUDO_USER
    REAL_USER := $(SUDO_USER)
    # Ermittelt das Home-Verzeichnis des echten Users aus /etc/passwd
    USER_HOME := $(shell getent passwd $(SUDO_USER) | cut -d: -f6)
else
    REAL_USER := $(shell whoami)
    USER_HOME := $(HOME)
endif

# --- Compiler & Flags ---
CXX      := g++
CXXFLAGS := -Wall -std=c++17 `pkg-config --cflags libusb-1.0 appindicator3-0.1` -pthread -Icpp
LDFLAGS  := `pkg-config --libs libusb-1.0 appindicator3-0.1` -pthread

# --- Directories & Paths ---
SRC_DIR     := cpp
BUILD_DIR   := ../build
TARGET      := ../Linux-G13-Driver

# GUI Paths (Relative to g13-driver/src)
GUI_ROOT    := ../../g13-config-tool
GUI_JAR_SRC := $(GUI_ROOT)/target/Linux-G13-GUI.jar
GUI_JAR_NAME:= Linux-G13-GUI.jar

# Config Paths
BINDINGS_SRC:= ../bindings/.g13

# Install Destinations (Now uses detected USER_HOME)
INSTALL_DIR := $(USER_HOME)/.g13

# --- Source Files ---
SRCS := G13.cpp G13Action.cpp Macro.cpp MacroAction.cpp Main.cpp Output.cpp PassThroughAction.cpp
OBJS := $(addprefix $(BUILD_DIR)/, $(SRCS:.cpp=.o))

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
	@echo " Installation erfolgreich abgeschlossen!"
	@echo " Installiert für User: $(REAL_USER)"
	@echo " Speicherort:          $(INSTALL_DIR)"
	@echo " Starten Sie den Treiber mit: $(INSTALL_DIR)/Linux-G13-Driver"
	@echo "========================================================"

# =============================================================================
# Step 1: Install Dependencies
# =============================================================================
dependencies:
	@echo "--- Checking and Installing Dependencies ---"
ifdef HAS_PACMAN
	@echo "Detected Arch Linux (pacman)"
	# sudo wird nur aufgerufen, wenn wir nicht schon root sind
	sudo pacman -S --noconfirm base-devel libusb gtk3 libappindicator-gtk3 maven jdk17-openjdk
else ifdef HAS_APT
	@echo "Detected Debian/Ubuntu (apt)"
	sudo apt-get update
	sudo apt-get install -y build-essential libusb-1.0-0-dev libgtk-3-dev libappindicator3-dev maven openjdk-17-jdk
else ifdef HAS_DNF
	@echo "Detected Fedora/RHEL (dnf)"
	sudo dnf install -y make automake gcc gcc-c++ kernel-devel libusb1-devel gtk3-devel libappindicator-gtk3-devel maven java-17-openjdk-devel
endif

# =============================================================================
# Step 2: Install UDEV Rules
# =============================================================================
install-udev:
	@echo "--- Installing UDEV Rules ---"
	@UDEV_RULES_DIR=""; \
	if [ -d /etc/udev/rules.d ]; then UDEV_RULES_DIR="/etc/udev/rules.d"; \
	elif [ -d /lib/udev/rules.d ]; then UDEV_RULES_DIR="/lib/udev/rules.d"; \
	else echo "Error: Udev directory not found." >&2; exit 1; fi; \
	\
	UDEV_FILE="udev/99-g13-input.rules"; \
	if getent group plugdev >/dev/null; then \
		UDEV_FILE="udev/99-g13-plugdev.rules"; \
		echo "Using 'plugdev' group rule."; \
	else \
		echo "Using 'input' group rule."; \
	fi; \
	sudo cp $$UDEV_FILE $$UDEV_RULES_DIR/99-g13.rules
	@echo "Reloading UDEV rules..."
	sudo udevadm control --reload-rules && sudo udevadm trigger

# =============================================================================
# Step 3: Build C++ Driver
# =============================================================================
build-driver: $(TARGET)

$(TARGET): $(OBJS)
	@echo "Linking executable: $@"
	$(CXX) $(OBJS) -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	@echo "Compiling $< -> $@"
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR):
	@mkdir -p $@

# =============================================================================
# Step 4: Build Java GUI (Maven)
# =============================================================================
build-gui:
	@echo "--- Building Java GUI with Maven ---"
	# Maven sollte idealerweise nicht als root laufen, um Cache-Probleme zu vermeiden.
	# Wir versuchen es als der echte User auszuführen, falls sudo genutzt wird.
ifdef SUDO_USER
	cd $(GUI_ROOT) && sudo -u $(REAL_USER) mvn clean package
else
	cd $(GUI_ROOT) && mvn clean package
endif

# =============================================================================
# Step 5: Deploy to User Home (~/.g13)
# =============================================================================
deploy:
	@echo "--- Deploying files to $(INSTALL_DIR) ---"
	mkdir -p $(INSTALL_DIR)
	
	# Dateien kopieren
	cp $(TARGET) $(INSTALL_DIR)/
	cp $(GUI_JAR_SRC) $(INSTALL_DIR)/$(GUI_JAR_NAME)
	
	# Configs kopieren (nicht überschreiben, falls vorhanden)
	if [ -z "$$(ls -A $(INSTALL_DIR)/*.properties 2>/dev/null)" ]; then \
		echo "Copying default configs..."; \
		cp $(BINDINGS_SRC)/*.properties $(INSTALL_DIR)/; \
	else \
		echo "Configs exist. Skipping overwrite."; \
	fi
	
	# Executable Bit setzen
	chmod +x $(INSTALL_DIR)/Linux-G13-Driver

	# WICHTIG: Besitzrechte korrigieren, falls wir als root (sudo) laufen
ifdef SUDO_USER
	@echo "Fixing ownership for user $(REAL_USER)..."
	chown -R $(REAL_USER): $(INSTALL_DIR)
endif

# =============================================================================
# Step 6: Cleanup Build Artifacts
# =============================================================================
clean-build:
	@echo "--- Cleaning up build files ---"
	rm -rf $(BUILD_DIR)
	rm -f $(TARGET)
ifdef SUDO_USER
	cd $(GUI_ROOT) && sudo -u $(REAL_USER) mvn clean
else
	cd $(GUI_ROOT) && mvn clean
endif

clean: clean-build
	@echo "Full clean finished."

uninstall:
	@echo "Removing UDEV rules..."
	sudo rm -f /etc/udev/rules.d/99-g13.rules /lib/udev/rules.d/99-g13.rules
	sudo udevadm control --reload-rules
	@echo "To remove the application, simply delete $(INSTALL_DIR)"