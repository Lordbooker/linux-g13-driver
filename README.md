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
You can use `make clean` to remove build files manually and `make uninstall` to remove the UDEV rules if needed.

## How to use the Driver and GUI

### Run the driver

**The easy way:**
The installation routine creates a new folder `.g13` in your user directory (home).

```bash
cd ~/.g13
./Linux-G13-Driver
```

**Alternative method:**
Open a command prompt and go to the directory where you built the driver.

```bash
./Linux-G13-Driver
```

> **Note:** If you start the driver via `sudo`, it might not find the config files in `~/.g13`. After properly setting the UDEV rules (which `make all` does), you should **not** need `sudo`.

### Use the Config Tool

After starting the driver, you will see a new icon in your system tray/taskbar. This allows you to open the config menu or quit the driver.

Alternatively, run it from the terminal:

```bash
cd ~/.g13
java -jar Linux-G13-GUI.jar
```

This will bring up the UI and create the initial files needed for your driver if they don't exist. All config files are saved in `~/.g13`.

![Config Tool Screenshot](docs/ConfigTool.png)

The top 4 buttons under the LCD screen select the bindings (M1-M3, MR).

> **Important:** If you configure the application while the driver is running, the driver will not pick up changes unless you select a different binding set or restart the driver.

### Use the built-in Mapping Set (for external tools)

The driver now includes a fixed default mapping. This means the GUI is not strictly necessary if you prefer other tools. You can map the keys using software like **Input Remapper**.

*(Note: The quick profile change via the four small buttons under the display only works when using the G13 GUI tool.)*

### Manually create your own Mapping Set

If you don't want to use the GUI App, you can copy the `.g13` folder from `g13-driver/bindings/` to your home directory (`~/`) and edit the files manually.

* **Usage Example:** To map the **G20** key to the letter **T**, find the event code for T (which is 20). Then, in your `bindings-0.properties` file, add or edit the line:
    ```ini
    G20=p,k.20
    ```

### Using the Display

You can write text to the display using a simple pipe command:

```bash
echo "Hello World!" > /tmp/g13-lcd
```

Currently, only one font size is implemented. There is an example script for system monitoring in the `scripts` folder. Feel free to try it out, modify it, or share your own scripts!

## Notes

* Tested on 64-bit Arch Linux.