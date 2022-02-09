# STM32 #

This repo contains bootloader, applications for BluePill STM32F103.

## Shell ##
This is a bootloader for STM32F103. It provides a shell console through USB interface, and also implements module support to
allow running test code from USB interface directly.

### Build ###
Clone this git repo with "--recurse-submodules" so that submoulde can be created.
To build the bootloader image, run following commands:

    cd Shell
    make clean
    make


### Flash ###
Please follow the similar instruction mentioned [here](https://github.com/microxblue/pikvm_hid#program-bluepill) for flashing
the Shell/out/Shell.bin image into BluePill.

### Install Drivers ###
On Windows, when connecting the BluePill to host through USB, it will show up as a composite USB device. One interface is for
USB console and the other interface is for USB data communication. Drivers need to be installed before using.  Please use
[Zadig](https://zadig.akeo.ie/) to install libusb-win32 driver for both of the interfaces.

### Start Shell Console ###
To access Shell console through USB interface, please use Python3. pyusb module needs to be installed for python first.
The following python command will start the Shell console.

    sudo python Tools/stmtalk.py ""

Shell command can also be executed directly through the interface. For example, run "?" command to list supported shell command:

    sudo python Tools/stmtalk.py "?"


## Module ##
Modules are loadable code segements. It is compiled seperately and can be loaded into SRAM for execution within the Shell environment.
It is helpful for code development and test since it does not need to burn the flash.  Modules will use the APIs provided by Shell
environment to reduce the module size so as to fit into limited SRAM space. Currently the module size is limited to be under 15KB
due to the STM32 SRAM size constraints.

### Build ###
To build a module, run following command: (Replace CmdXXXX with the actual module name):

    cd Module
    make MOD=CmdXXXX

### Run ###
To run a module in memory, first boot into Shell environment and start a shell console and then run command:

    cd Module
    ..\Tools\stmtalk.py ""
    make run MOD=CmdXXXX

The module outputs should show up in shell console.

### FLASH ###
Once a module is well tested, it can be burnt into flash for execution. The module will be written into flash location 0x8018000.

    cd Module
    ..\Tools\stmtalk.py ""
    make clean
    make user MOD=CmdXXXX

To let the module start automatically after the power up,  short the STM32F103 SWC and SWD pins on the SWD debug connector.

