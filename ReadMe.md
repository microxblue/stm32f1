# STM32 #

This repo contains bootloader, applications for BluePill STM32F103.

## Shell ##
This is a bootloader for STM32F103. It provides a shell console through USB interface, and also implements module support to allow running test code from USB interface directly.

### Build ###
To build the bootloader image, run following commands:

    cd Shell
    make clean
    make


### Flash ###
Please follow the similar instruction mentioned [here](https://github.com/microxblue/pikvm_hid#program-bluepill) for flashing the Shell/out/Shell.bin image into BluePill.

### Install Drivers ###
On Windows, when connecting the BluePill to host through USB, it will show up as a composite USB device. One interface is for USB console and the other interface is for USB data communication. Drivers need to be installed before using.  Please use [Zadig](https://zadig.akeo.ie/) to install libusb-win32 driver for both of the interfaces.

### Start Shell Console ###
To access Shell console through USB interface, please use Python3. pyusb module needs to be installed for python first. The following python command will start the Shell console.

    sudo python Host/Script/stmtalk.py ""

Shell command can also be executed directly through the interface. For example, run "?" command to list supported shell command:

    sudo python Host/Script/stmtalk.py "?"


