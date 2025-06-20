# G13 Linux Driver & GUI (Modernized Fork)

This is a modernized fork of the G13 driver for Linux.
The original project is over 10 years old. This fork has been refactored to use modern C++ standards for the driver and modern Java standards (Java 17 with Maven) for the configuration GUI.

## Features

* Modern C++ Driver: The core driver has been updated for better performance and compatibility.
* Java GUI: The configuration utility is built with Java 17 and Maven, ensuring it runs on modern systems.
* Flexible Configuration: Offers multiple ways to configure your G13: via the user-friendly GUI, manual file editing, or using the driver's fixed mapping with external tools.

## Requirements

### Base Requirements

* 'make' has to be installed

### Driver Requirements

* **`libusb-1.0`**: This library is essential for the driver to communicate with the G13 device. (also the development version)

    * **Debian / Ubuntu**
        ```bash
        sudo apt-get install libusb-1.0-0 libusb-1.0-0-dev
        ```
    * **Arch Linux and other Arch based distros**
        ```bash
        sudo pacman -S libusb
        ```
    * **Fedora / Nobara / etc.**
        ```bash
        sudo dnf install libusb1-devel
        ```

### GUI Requirements

* **Java 17 (or higher)**: The graphical configuration tool requires a Java Runtime Environment.

    * **Debian / Ubuntu**
        ```bash
        sudo apt-get install default-jre
        ```
    * **Arch Linux and other Arch based distros**
        ```bash
        sudo pacman -S jre-openjdk
        ```
    * **Fedora / Nobara / etc.**
        ```bash
        sudo dnf install java-latest-openjdk.x86_64
        ```

## Build & Installation

* Open a terminal and navigate to the project directory.
* Build the driver:
    ```bash
    make all
    ```
* Install the driver and UDEV rule for secure access:
    ```bash
    make install
    ```

(For cleanup, you can use `make clean` to remove build files and `make uninstall` to remove the driver and the UDEV rule.)


## How to use the driver and the GUI App

### Use the config tool
  
In a command prompt go to the downloaded and unzipped folder and type:

    java -jar Linux-G13-GUI.jar

or with rightclick on the file in your file-explorer.

![alt text](docs/image.png)

This will bring up the UI and create the initial files needed for your driver.  
All config files are saved in `$(HOME)/.g13`

![alt text](docs/ConfigTool.jpg)

The top 4 buttons under the LCD screen select the bindings.
The joystick currently only supports key mappings

If you are configuring the application while the driver is running, the driver will not pick up changes unless you select a different bindings set or you can restart the driver.

### Use the driver buildin Mappingset for mapping with other external tools like "Input Remapper"

The driver has now a fixed mapping included, so the GUI is not strictly necessesary. 
You can now map the keys with every other tool, like "Input Remapper".
( Only the quick change with the four small Buttons under the display doesn't work anymore. This works only with the GUI tool.)


### Manually made your own Mappingset

If you don't want to use the GUI App, you can copy the folder .g13 from the g13-driver/bindings/ directory to your home directory (~/) and edit the files manually.

* Usage Example: To map the G20 key to the letter T, find the event code for T (which is 20). Then, in your bindings-0.properties file, add or edit the line: G20=p,k.20.


### Run the driver

* **In a command prompt go to the directory where you build the driver**

* If you have set the UDEV Rules, you now can start the driver with 
    ```bash
    ./G13-Linux-Driver 
    ```

    or with rightclick on the file in your file-explorer.

    ![alt text](docs/image2.png)

* for easier use, put the driver to your 'Autostart' Option in your distro. the driver has hotplug capabilty and its waiting for a new connection in the backround.   



* else without UDEV-Rule Installation ('Autostart' is not possible without UDEV-Rule set)

 `!!! strictly not recommended and only for test purposes !!!`
 
    sudo HOME=$HOME ./G13-Linux-Driver


## Notes

I've tried this on 64-bit Arch Linux and it works so far.  
