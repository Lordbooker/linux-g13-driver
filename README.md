# G13 Linux Driver & GUI (Modernized Fork)

This is a modernized fork of the G13 driver for Linux.
The original project is over 10 years old. This fork has been refactored to use modern C++ standards for the driver and modern Java standards (Java 17 with Maven) for the configuration GUI.

## Features

* **Modern C++ Driver:** The core driver has been updated for better performance and compatibility.
* **Java GUI:** The configuration utility is built with Java 17 and Maven, ensuring it runs on modern systems.
* **Flexible Configuration:** Offers multiple ways to configure your G13: via the user-friendly GUI, manual file editing, or using the driver's fixed mapping with external tools.

## Requirements

### Base Requirements

You need to install the following packages via your package manager:

* `make`
* `cmake`
* `gtk3` / `gtk3-devel`
* `libusb-1.0-0` (on some distros named `libusb-1.0-0-dev` or `libusb1-devel`)
* `libappindicator-gtk3` (or similar)
* `Java 17` or higher
* `python-psutil` (for the monitor script)

### Automated Dependency Installation

Alternatively, all needed dependencies can be installed via the `install_deps.sh` script located in the scripts folder.

```bash
cd src/scripts
chmod +x install_deps.sh
./install_deps.sh
```

## Build & Installation

1.  Open a terminal and navigate to the project directory.
2.  Build the driver:

    ```bash
    make all
    ```

The installation process will clean up automatically after finishing.

## Choose your Installation Method

### Option A: System-Wide Installation (Standard)
This is the recommended method for standard usage. It installs binaries to /usr/bin and resources to /usr/share/.

```bash
sudo make install
```
Note: As per standard Linux security practices, the installation does not auto-start user services. You must enable the driver for your user manually once:

```bash
systemctl --user enable --now g13
```

#### Option B: User-Local Installation (Developer Mode)
This method installs everything to your home directory (~/.local/bin). It is intended for development, testing, or users without root access. Automatically creates and starts the Systemd service.

```bash
make install-user
```
Driver: Installed to ~/.local/bin/linux-g13-driver

Service: Automatically enabled and started immediately.

Note on Permissions: Both methods install a UDEV rule (/etc/udev/rules.d/99-g13.rules) to allow access to the G13 without sudo. You might need to unplug and replug your device once after installation if it's not detected immediately.

## How to use the Driver and GUI

### Run the driver

### Controlling the Driver
The driver runs in the background via Systemd.

Check Status:

```bash
systemctl --user status g13
```

View Logs:

```bash
journalctl --user -u g13 -f
```

Restart Driver:

```bash
systemctl --user restart g13
```


### Use the Config Tool

After starting the driver, you will see a new icon in your system tray/taskbar. This allows you to open the config menu or quit the driver.

Alternatively, run it from the terminal:

```bash
g13-gui
```

This will bring up the UI.

Profiles: The top 4 buttons under the LCD (M1, M2, M3, MR) switch between binding profiles.

Save: Changes are saved automatically to `~/.config/g13/bindings-*.properties`.

Live Reload: The driver automatically detects file changes and reloads the config immediately.

![Config Tool Screenshot](docs/ConfigTool.png)

The top 4 buttons under the LCD screen select the bindings (M1-M3, MR).

> **Important:** If you configure the application while the driver is running, the driver will not pick up changes unless you select a different binding set or restart the driver.

### Use the built-in Mapping Set (for external tools)

The driver now includes a fixed default mapping. This means the GUI is not strictly necessary if you prefer other tools. You can map the keys using software like **Input Remapper**.

*(Note: The quick profile change via the four small buttons under the display only works when using the G13 GUI tool.)*

### Manually create your own Mapping Set

If you don't want to use the GUI App, you can edit the files manually in `~/.config/g13/`.

* **Usage Example:** To map the **G20** key to the letter **T**, find the event code for T (which is 20). Then, in your `bindings-0.properties` file, add or edit the line:
    ```ini
    G20=p,k.20
    ```

### Using the Display (scripting)

You can write text to the display using a simple pipe command:

The driver creates a Named Pipe (FIFO) to receive text for the LCD.

Location: `/run/user/$UID/g13-lcd` (Check `/tmp/g13-lcd` as fallback if `/run` is unavailable).

```bash
# Find your pipe path (usually based on your user ID, e.g., 1000)
PIPE="/run/user/$(id -u)/g13-lcd"

# Send simple text
echo "Hello World!" > $PIPE

# Send multi-line text (CPU/RAM stats)
echo -e "CPU: 50%\nRAM: 4GB" > $PIPE
```

Currently, only one font size is implemented. There is an example script for system monitoring in the `scripts` folder. Feel free to try it out, modify it, or share your own scripts!


### Uninstallation

To remove the driver and all installed files:

```bash
make uninstall
```

(Note: This removes the binaries, UDEV rules, and service files, but keeps your configuration in ~/.config/g13 to prevent data loss.)


## Notes

* Tested on 64-bit Arch Linux.