# iMe
Custom firmware for the Micro 3D printer

© 2015-2016 Exploit Kings. All rights reserved.

This is not a fully functional firmware yet since it doesn't fully implement the calibration commands G30 and G32. It is currently only intended to be used by those who want to test it or develop for it.

iMe uses the RepRap G-code protocol, so it allows you to use the Micro 3D printer with any 3D printing software that supports that protocol, such as Cura, Simplify3D, AstroPrint, etc. It features builtin backlash and bed compensation which makes creating good looking prints with other software possible since the G-code never has to be pre-processed beforehand.

The easiest way to install and use iMe is with M3D Manager, which is available for [Windows](https://raw.githubusercontent.com/donovan6000/iMe/master/M3D%20Manager/M3D%20Manager%20Windows.zip), [OS X](https://raw.githubusercontent.com/donovan6000/iMe/master/M3D%20Manager/M3D%20Manager%20OS%20X.zip), and [Linux](https://raw.githubusercontent.com/donovan6000/iMe/master/M3D%20Manager/M3D%20Manager%20Linux.zip). Simply click the 'Install iMe firmware' button to install iMe. When the printer powers on it'll be in bootloder mode, so you'll need to click the 'Switch to firmware mode' button to put the printer into a mode where other 3D printing software can communicate with it.
