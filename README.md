# iMe
Custom firmware for the Micro 3D printer

© 2015-2016 Exploit Kings. All rights reserved.

This is not a fully functional firmware yet since it doesn't have full control over the heater yet. It does provide pretty much everything else though.

It can be compiled with Atmel's AVR 8-bit toolchain for [Windows](http://www.atmel.com/tools/ATMELAVRTOOLCHAINFORWINDOWS.aspx) or [Linux](http://www.atmel.com/tools/atmelavrtoolchainforlinux.aspx), and it can be installed from Linux with [M3D Linux](https://github.com/donovan6000/M3D-Linux). To simplify the devloping process, just set the appropriate locations for `avr-gcc`, `avr-objcopy`, `avr-size`, and `m3d-linux` in the `Makefile` which will allow using the commands `make` to compile the firmware and `make run` to install and run it.

Also, it can be compiled using [Atmel Studio](http://www.atmel.com/tools/ATMELSTUDIO.aspx) by opening and building the project file `iMe.atsln`. However the resulting ELF will need to be stripped before it can be flashed into the printer, which you can do by running the following command. Currently the only way to flash the firmware from Windows is with [M3D Fio](https://github.com/donovan6000/M3D-Fio).
```shell
avr-objcopy -O binary iMe.elf "iMe 1900000001.hex"
```
