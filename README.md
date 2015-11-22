# iMe
Custom firmware for the Micro 3D printer
<br>
© 2015 Exploit Kings. All rights reserved.
<br>
<br>
The firmware currently just sets up the printer so that it echos back anything sent to it. I wanted to put this out there in case anyone else wanted to develop for the Micro 3d printer. <a href="http://www.exploitkings.com/public/2015080602.zip">Here's</a> the latest official firmware, V2015080602, so that you can put your printer back into a working state.
<br>
<br>
It can be compiled with <a href="http://www.atmel.com/tools/ATMELAVRTOOLCHAINFORLINUX.aspx">Atmel's AVR 8-bit toolchain</a>, and it can be installed with <a href="https://github.com/donovan6000/M3D-Linux">M3D Linux</a>. So after setting the appropriate locations for 'avr-gcc', 'avr-objcopy', and 'm3d-linux' in the Makefile you can run 'Make' to compile the firmware and 'Make run' to install and run it. Alternatively you can use <a href="https://github.com/donovan6000/M3D-Fio">M3D Fio</a> to install and run the compiled firmware.
