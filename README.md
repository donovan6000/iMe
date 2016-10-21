# iMe
Firmware that allows using the Micro 3D printer with third-party 3D printing software.

© 2015-2016 Exploit Kings. All rights reserved.

### Description
iMe is firmware for the Micro 3D printer that attempts to fix the printer's biggest problem: limited software compatibility. iMe uses RepRap G-code protocol, so it allows you to use the Micro 3D printer with any 3D printing software that supports that protocol. It also features builtin backlash and bed compensation which makes creating good looking prints with other software possible since the G-code never has to be pre-processed beforehand.

The latest version of iMe is V00.00.01.23 released on October 20th, 2016, and an entire changelog for it can be found [here](https://raw.githubusercontent.com/donovan6000/iMe/master/Changelog).

### Features
* Uses RepRap's G-code protocol
* Open source (iMe's source code can be found [here](https://github.com/donovan6000/iMe))
* Homing uses the accelerometer to minimize grinding
* Builtin backlash and bed compensation
* Prevents moving the extruder out of bounds in the X and Y directions
* Faster printing
* Allows configuring the steps/mm for each of the motors

### Installation
The easiest way to install iMe is with M33 Manager, which is available for [Windows](https://raw.githubusercontent.com/donovan6000/iMe/master/M33%20Manager/M33%20Manager%20Windows.zip), [OS X](https://raw.githubusercontent.com/donovan6000/iMe/master/M33%20Manager/M33%20Manager%20OS%20X.zip), and [Linux](https://raw.githubusercontent.com/donovan6000/iMe/master/M33%20Manager/M33%20Manager%20Linux.zip). Just connect the printer to M33 Manager and click the 'Install iMe firmware' button to install iMe.

iMe is also fully compatible with [M33 Fio](https://github.com/donovan6000/M33-Fio), and it can be installed from there as well.

### Usage
Every time the printer powers on it'll be in bootloader mode, so you'll need to click the 'Switch to firmware mode' button in M33 Manager to put the printer into a mode where other 3D printing software can communicate with it. Make sure to disconnect the printer from M33 Manager or close M33 Manager before using the printer with other software so that the printer's serial port doesn't remain busy.

M33 Manager will let you know which serial port the printer is using when it switches it into firmware mode, so in order to use the printer you just have connect to that serial port at a baud rate of 115200 with the 3D printing software that you want to use.

### Images
M33 Manager can be used on Windows, OS X, and Linux. In addition to installing iMe, it also allows manually sending commands to the printer in both bootloader and firmware mode which can be used to manage all aspects of the printer.
![alt text](https://raw.githubusercontent.com/donovan6000/iMe/master/images/m33%20manager.png "M33 Manager")
So far iMe has been tested on [Cura](https://ultimaker.com/en/products/cura-software), [OctoPrint](http://octoprint.org/), [Simplify3D](https://www.simplify3d.com/), [Repetier-Host](https://www.repetier.com/), [Printrun](http://www.pronterface.com/), [MatterControl](http://www.mattercontrol.com/), and [CraftWare](https://craftunique.com/craftware).
![alt text](https://raw.githubusercontent.com/donovan6000/iMe/master/images/cura.png "Cura")
![alt text](https://raw.githubusercontent.com/donovan6000/iMe/master/images/octoprint.png "OctoPrint")
![alt text](https://raw.githubusercontent.com/donovan6000/iMe/master/images/simplify3d.png "Simplify3D")
![alt text](https://raw.githubusercontent.com/donovan6000/iMe/master/images/repetier-host.png "Repetier-Host")
![alt text](https://raw.githubusercontent.com/donovan6000/iMe/master/images/printrun.png "Printrun")
![alt text](https://raw.githubusercontent.com/donovan6000/iMe/master/images/mattercontrol.png "MatterControl")
![alt text](https://raw.githubusercontent.com/donovan6000/iMe/master/images/craftware.png "CraftWare")
