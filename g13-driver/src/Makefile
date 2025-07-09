# Makefile for the G13 C++ Driver Component
# Corrected version assuming this Makefile resides in the 'src' directory.

# --- Variables ---
CXX = g++
# The final executable will be placed in the parent directory (the g13-driver root).
TARGET = ../Linux-G13-Driver

# --- Directories ---
# The source directory is 'cpp', relative to this Makefile in 'src'.
SRC_DIR = cpp
# The build directory for object files will be created in the parent directory.
BUILD_DIR = ../build

# --- Flags ---
# CXXFLAGS are for the compiler.
# -Icpp tells the compiler to look for header files in the 'cpp' directory.
CXXFLAGS = -Wall -std=c++17 `pkg-config --cflags libusb-1.0 appindicator3-0.1` -pthread -I$(SRC_DIR)
# LDFLAGS are for the linker.
LDFLAGS = `pkg-config --libs libusb-1.0 appindicator3-0.1` -pthread

# --- Source and Object Files ---
# List of source files.
SRCS = \
	G13.cpp \
	G13Action.cpp \
	Macro.cpp \
	MacroAction.cpp \
	Main.cpp \
	Output.cpp \
	PassThroughAction.cpp

# Generate the full path for each object file in the build directory.
OBJS = $(addprefix $(BUILD_DIR)/, $(SRCS:.cpp=.o))

# --- Build Targets ---
.PHONY: all clean install uninstall

# Default target executed when 'make' is run.
all: $(TARGET)

# Rule to link the final executable.
# It depends on all object files being created first.
$(TARGET): $(OBJS)
	@echo "Linking executable: $@"
	$(CXX) $(OBJS) -o $@ $(LDFLAGS)

# This is the single pattern rule to compile any .cpp file.
# It creates the object file in the BUILD_DIR from a source file in the SRC_DIR.
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	@echo "Compiling $< -> $@"
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Rule to create the build directory.
$(BUILD_DIR):
	@mkdir -p $@

# --- Installation and Cleanup Targets ---

# Target to install the udev rule for device permissions.
# The path to the udev rules is now relative to the parent directory.
install:
	@UDEV_RULES_DIR=""; \
	if [ -d /etc/udev/rules.d ]; then UDEV_RULES_DIR="/etc/udev/rules.d"; \
	elif [ -d /lib/udev/rules.d ]; then UDEV_RULES_DIR="/lib/udev/rules.d"; \
	else echo "Error: Udev rules directory not found." >&2; exit 1; fi; \
	echo "Using udev directory: $$UDEV_RULES_DIR"; \
	\
	UDEV_SOURCE_FILE=""; \
	if getent group input >/dev/null; then \
		echo "Found 'input' group. Using rule for 'input' group access."; \
		UDEV_SOURCE_FILE="udev/99-g13-input.rules"; \
	elif getent group plugdev >/dev/null; then \
		echo "Found 'plugdev' group. Using rule for 'plugdev' group access."; \
		UDEV_SOURCE_FILE="udev/99-g13-plugdev.rules"; \
	else \
		echo "Warning: Neither 'input' nor 'plugdev' group found. Defaulting to 'input' rule."; \
		UDEV_SOURCE_FILE="udev/99-g13-input.rules"; \
	fi; \
	\
	if [ -f $$UDEV_SOURCE_FILE ]; then \
		echo "Installing $$UDEV_SOURCE_FILE to $$UDEV_RULES_DIR/99-g13.rules"; \
		sudo cp $$UDEV_SOURCE_FILE $$UDEV_RULES_DIR/99-g13.rules; \
		echo "Reloading udev rules..."; \
		sudo udevadm control --reload-rules && sudo udevadm trigger; \
		echo "UDEV rule installed. Please re-plug your G13 device."; \
	else \
		echo "Error: Source udev rule file $$UDEV_SOURCE_FILE not found." >&2; exit 1; \
	fi

# Target to remove the udev rule.
uninstall:
	@echo "Removing udev rule..."
	@UDEV_FILE_1="/etc/udev/rules.d/99-g13.rules"; \
	UDEV_FILE_2="/lib/udev/rules.d/99-g13.rules"; \
	REMOVED=false; \
	if [ -f $$UDEV_FILE_1 ]; then \
		echo "Removing $$UDEV_FILE_1"; \
		sudo rm -f $$UDEV_FILE_1; \
		REMOVED=true; \
	fi; \
	if [ -f $$UDEV_FILE_2 ]; then \
		echo "Removing $$UDEV_FILE_2"; \
		sudo rm -f $$UDEV_FILE_2; \
		REMOVED=true; \
	fi; \
	if [ "$$REMOVED" = true ]; then \
		echo "Reloading udev rules..."; \
		sudo udevadm control --reload-rules; \
	else \
		echo "No udev rule found to remove."; \
	fi; \
	echo "Uninstallation complete."

# Target to clean up all build artifacts.
clean:
	@echo "Cleaning up build directory and executable..."
	@rm -rf $(BUILD_DIR)
	@rm -f $(TARGET)
	@echo "Cleanup complete."
