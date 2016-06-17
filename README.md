# iMe
Firmware for the Micro 3D printer

© 2015-2016 Exploit Kings. All rights reserved.

### Description
iMe is firmware for the Micro 3D printer that attempts to fix the printer's biggest problem: limited software compatibility. iMe uses RepRap G-code protocol, so it allows you to use the Micro 3D printer with any 3D printing software that supports that protocol. It also features builtin backlash and bed compensation which makes creating good looking prints with other software possible since the G-code never has to be pre-processed beforehand.

The latest version of iMe is V00.00.00.09 released on June 1st, 2016, and an entire changelog for it can be found [here](https://raw.githubusercontent.com/donovan6000/iMe/master/Changelog).

### Features
* Uses RepRap's G-code protocol
* Open source (iMe's source code can be found [here](https://github.com/donovan6000/iMe))
* Homing uses the accelerometer to minimize grinding
* Builtin backlash and bed compensation
* Monitor's the extruder's current to maintain a steady flow of filament
* Prevents moving the extruder out of bounds in the X and Y directions

### Installation
The easiest way to install iMe is with M3D Manager, which is available for [Windows](https://raw.githubusercontent.com/donovan6000/iMe/master/M3D%20Manager/M3D%20Manager%20Windows.zip), [OS X](https://raw.githubusercontent.com/donovan6000/iMe/master/M3D%20Manager/M3D%20Manager%20OS%20X.zip),  and [Linux](https://raw.githubusercontent.com/donovan6000/iMe/master/M3D%20Manager/M3D%20Manager%20Linux.zip). Just connect the printer to M3D Manager and click the 'Install iMe firmware' button to install iMe.

iMe is also fully compatible with [M3D Fio](https://github.com/donovan6000/M3D-Fio), and it can be installed from there as well.

### Usage
Every time the printer powers on it'll be in bootloader mode, so you'll need to click the 'Switch to firmware mode' button in M3D Manager to put the printer into a mode where other 3D printing software can communicate with it. Make sure to disconnect the printer from M3D Manager or close M3D Manager before using the printer with other software so that the printer's serial port doesn't remain busy.

M3D Manager will let you know which serial port the printer is using when it switches it into firmware mode, so in order to use the printer you just have connect to that serial port at a baudrate of 115200 with the 3D printing software that you want to use.

### Images
M3D Manager can be used on Windows, OS X, and Linux. In addition to installing iMe, it also allows manually sending commands to the printer in both bootloader and firmware mode which can be used to manage all aspects of the printer.
![alt text](http://exploitkings.com/public/ime_manager.png "M3D Manager")
So far iMe has been tested on [Cura](https://ultimaker.com/en/products/cura-software), [OctoPrint](http://octoprint.org/), [Simplify3D](https://www.simplify3d.com/), [Repetier-Host](https://www.repetier.com/), [Printrun](http://www.pronterface.com/), [MatterControl](http://www.mattercontrol.com/), and [CraftWare](https://craftunique.com/craftware).
![alt text](https://raw.githubusercontent.com/donovan6000/iMe/master/images/cura.png "Cura")
![alt text](https://raw.githubusercontent.com/donovan6000/iMe/master/images/octoprint.png "OctoPrint")
![alt text](https://raw.githubusercontent.com/donovan6000/iMe/master/images/simplify3d.png "Simplify3D")
![alt text](https://raw.githubusercontent.com/donovan6000/iMe/master/images/repetier-host.png "Repetier-Host")
![alt text](https://raw.githubusercontent.com/donovan6000/iMe/master/images/printrun.png "Printrun")
![alt text](https://raw.githubusercontent.com/donovan6000/iMe/master/images/mattercontrol.png "MatterControl")
![alt text](https://raw.githubusercontent.com/donovan6000/iMe/master/images/craftware.png "CraftWare")
