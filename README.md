# Readme File

I forked the repo because parts of it are already 10 years old, and I still tinker with the hardware.
So far, I have only modified the C++ part (i.e., the driver itself). I'm not familiar with Java, and the config program isn't strictly necessary, as I can overwrite the default settings at any time using other key mapping tools.

Nevertheless, it still works with the included mapping tool, and you can use it to save 4 sets.

## Notes

I've tried this on 64-bit Arch Linux and it works so far.  

## Requirements

### libusb-1.0

For Ubuntu, it should be installed already but if you don't have it, you can get it by typing:

    sudo apt-get install libusb-1.0-0

For Arch Linux you can install it by typing:

    sudo pacman -S libusb

### Java version 1.6 or higher

For Ubuntu, it can be installed by typing:

    sudo apt-get install default-jre

For Arch it can be installed by typing:

    sudo pacman -S jre8-openjdk

## Build

Open a console (command prompt)  
Go to the directory where you unzipped your download  
type `make`

## Running Application

Run the config tool first!  
In a command prompt go to the directory where you unzipped your download and type:

    java -jar Linux-G13-GUI.jar

This will bring up the UI and create the initial files needed for your driver.  
All config files are saved in `$(HOME)/.g13`

Run the driver  
In a command prompt go to the directory where you unzipped your download and type:

    sudo -E ./G13-Linux-Driver

The `-E` is to run it using your environment variables so it doesn't look for the `.g13` directory in `/root`  
If you want to run the command and then detach it so you can close the terminal:

    sudo -E ./G13-Linux-Driver &

If you are configuring the application while the driver is running, the driver will not pick up changes unless you select a different bindings set or you can restart the driver.

The top 4 buttons under the LCD screen select the bindings.

The joystick currently only supports key mappings.
