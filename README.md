# iMe
Custom firmware for the Micro 3D printer
<br>
© 2015-2016 Exploit Kings. All rights reserved.
<br>
<br>
This is not a fully functional firmware yet. This firmware currently only sets up the printer so that it responds to the commands M115, M105, and M115 S628.
<br>
<br>
It can be compiled with <a href="http://www.atmel.com/tools/ATMELAVRTOOLCHAINFORLINUX.aspx">Atmel's AVR 8-bit toolchain</a>, and it can be installed with <a href="https://github.com/donovan6000/M3D-Linux">M3D Linux</a>. So after setting the appropriate locations for 'avr-gcc', 'avr-objcopy', 'avr-size', and 'm3d-linux' in the Makefile you can run 'Make' to compile the firmware and 'Make run' to install and run it. Alternatively you can use <a href="https://github.com/donovan6000/M3D-Fio">M3D Fio</a> to install and run the compiled firmware.
