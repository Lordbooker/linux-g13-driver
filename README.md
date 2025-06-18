# Readme File

I forked the repo because parts of it are already 10 years old, and I still tinker with the hardware.
So far, I have only modified the C++ part (i.e., the driver itself) and the java build process for maven (now JAVA 17). 
I'm not familiar with Java, and the config program isn't strictly necessary, as I can overwrite the default settings at any time using other key mapping tools.

Nevertheless, it still works with the included mapping tool, and you can use it to save 4 different mappingsets.


## Requirements

### libusb-1.0 (recommended for the driver)

For Ubuntu and other Ubuntu based systems, it should be installed already but if you don't have it, you can get it by typing:

    sudo apt-get install libusb-1.0-0

For Arch Linux and other Arch based systems you can install it by typing:

    sudo pacman -S libusb

For Fedora and other Fedora based Systems you can install it by typing:

    sudo dnf install libusb


### Java version 17 or higher (needed only for the GUI)

For Ubuntu and other Ubuntu based systems, it can be installed by typing:

    sudo apt-get install default-jre

For Arch and other Arch based systems, it can be installed by typing:

    sudo pacman -S jre-openjdk

For Fedora and other Fedora based Systems, it can be installed by typing:

    sudo dnf install java-latest-openjdk.x86_64


## Build

Open a console (command prompt)  
Go to the directory where you unzipped your download  

type `make all` to build the driver
then type `make install` to write the UDEV-Rule

for cleaning up 
use `make clean`
and `make uninstall`

this will delete the build-files, the driver an the UDEV-Rule.


## How to use the driver and the GUI App

### Use the driver buildin Mappingset for mapping with other external tools like "Input Remapper"

The Driver needs a Ruleset to work. 
I've implemented a Standard Key-Set for the G13. So its now "plug'n'play".


### Manually made your own Mappingset

If you don't want to use the GUI App, just copy the Folder ".g13" from /bindings to your 'home' Directory. 
So you can make entries manually in the files.

In the 'docs' folder is an example List for Eventcodes.
Usage Example:

    If you want to map the G20 key on your G13 to the T key on the keyboard:
    Find T in the table. The Event Code is 20.
    Open your bindings-0.properties file.
    Add or change the following line: G20=p,k.20.


### Use the config tool
  
In a command prompt go to the directory where you unzipped your download and type:

    java -jar Linux-G13-GUI.jar

This will bring up the UI and create the initial files needed for your driver.  
All config files are saved in `$(HOME)/.g13`

![alt text](docs/ConfigTool.jpg)

The top 4 buttons under the LCD screen select the bindings.
The joystick currently only supports key mappings


### Run the driver

In a command prompt go to the directory where you unzipped your download and type:

If you have set the UDEV Rules, you now can start the driver with 

    ./G13-Linux-Driver 

else without UDEV-Rule Installation

    sudo -E ./G13-Linux-Driver

The `-E` is to run it using your environment variables so it doesn't look for the `.g13` directory in `/root`  
If you want to run the command and then detach it so you can close the terminal:

    sudo -E ./G13-Linux-Driver &

If you are configuring the application while the driver is running, the driver will not pick up changes unless you select a different bindings set or you can restart the driver.


## Notes

The code for the driver is completly refactored in modern C++ standards.

The driver has now a fixed mapping in the driver itself, so you don't need the old GUI. You can now map with every other tool, like "Input Remapper".
only the quick change with the the four little Buttons under the display doesn't work anymore. This works only with the GUI tool.


I've tried this on 64-bit Arch Linux and it works so far.  