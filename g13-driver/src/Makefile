EXE_NAME = Linux-G13-Driver
SRC_DIR  = cpp
BUILD_DIR = build
OBJS     = $(BUILD_DIR)/G13.o $(BUILD_DIR)/G13Action.o $(BUILD_DIR)/Macro.o $(BUILD_DIR)/MacroAction.o $(BUILD_DIR)/Main.o $(BUILD_DIR)/Output.o $(BUILD_DIR)/PassThroughAction.o
CC       = g++
CXXFLAGS = -pthread
CPPFLAGS =
LDFLAGS  = -L/lib64
LDLIBS   = -lusb-1.0

# Udev Regel-Konfiguration
UDEV_DEST_FILE = /etc/udev/rules.d/99-g13.rules

.PHONY: all clean install uninstall

all: $(EXE_NAME)

$(EXE_NAME): $(OBJS)
	$(CC) $(OBJS) -o $(EXE_NAME) $(LDFLAGS) $(CXXFLAGS) $(LDLIBS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	$(CC) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@

$(BUILD_DIR):
	@mkdir -p $@

clean:
	rm -rf $(BUILD_DIR)
	rm -f $(EXE_NAME)

# NEUE, intelligente Installationsroutine
install:
	@echo "Installing udev rule..."
	@if getent group input >/dev/null; then \
		echo "Found 'input' group. Installing rule for 'input'."; \
		sudo cp 99-g13-input.rules $(UDEV_DEST_FILE); \
	elif getent group plugdev >/dev/null; then \
		echo "Found 'plugdev' group. Installing rule for 'plugdev'."; \
		sudo cp 99-g13-plugdev.rules $(UDEV_DEST_FILE); \
	else \
		echo "Warning: Neither 'input' nor 'plugdev' group found. Installing 'input' rule as default."; \
		echo "You may need to create the 'input' group and add your user to it manually."; \
		sudo cp 99-g13-input.rules $(UDEV_DEST_FILE); \
	fi
	@echo "Reloading udev rules..."
	sudo udevadm control --reload-rules
	sudo udevadm trigger
	@echo "Installation complete. Please re-plug your G13 device."
	@echo "Access should be granted automatically (via uaccess)."
	@echo "If you have issues, ensure you are in the group mentioned above and log out/in."

# Angepasste Deinstallationsroutine
uninstall:
	@echo "Removing udev rule..."
	sudo rm -f $(UDEV_DEST_FILE)
	@echo "Reloading udev rules..."
	sudo udevadm control --reload-rules
	sudo udevadm trigger
	@echo "Uninstallation complete."