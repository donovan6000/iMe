# iMe
Firmware for the Micro 3D printer

© 2015-2016 Exploit Kings. All rights reserved.

iMe is firmware for the Micro 3D printer that attempts to fix the printer's biggest problem: limited software compatibility. iMe uses RepRap G-code protocol, so it allows you to use the Micro 3D printer with any 3D printing software that supports that protocol. It also features builtin backlash and bed compensation which makes creating good looking prints with other software possible since the G-code never has to be pre-processed beforehand.

The easiest way to install and use iMe is with M3D Manager, which is available for [Windows](https://raw.githubusercontent.com/donovan6000/iMe/master/M3D%20Manager/M3D%20Manager%20Windows.zip), [OS X](https://raw.githubusercontent.com/donovan6000/iMe/master/M3D%20Manager/M3D%20Manager%20OS%20X.zip), and [Linux](https://raw.githubusercontent.com/donovan6000/iMe/master/M3D%20Manager/M3D%20Manager%20Linux.zip). Simply click the 'Install iMe firmware' button to install iMe. Every time the printer powers on it'll be in bootloder mode, so you'll need to click the 'Switch to firmware mode' button to put the printer into a mode where 3D printing software can communicate with it. Make sure to disconnect the printer from M3D Manager or close M3D Manager when using the printer with other software so that the printer's serial port doesn't remain busy.
![alt text](https://raw.githubusercontent.com/donovan6000/iMe/master/images/m3d%20manager.png "M3D Manager")
So far iMe has been tested on [Cura](https://ultimaker.com/en/products/cura-software), [OctoPrint](http://octoprint.org/), [Simplify3D](https://www.simplify3d.com/), [Repetier-Host](https://www.repetier.com/), and [Printrun](http://www.pronterface.com/).
![alt text](https://raw.githubusercontent.com/donovan6000/iMe/master/images/cura.png "Cura")
![alt text](https://raw.githubusercontent.com/donovan6000/iMe/master/images/octoprint.png "OctoPrint")
![alt text](https://raw.githubusercontent.com/donovan6000/iMe/master/images/simplify3d.png "Simplify3D")
![alt text](https://raw.githubusercontent.com/donovan6000/iMe/master/images/repetier-host.png "Repetier-Host")
![alt text](https://raw.githubusercontent.com/donovan6000/iMe/master/images/printrun.png "Printrun")
