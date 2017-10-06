# DCC_AVR
The repository for AVR sourcecode for the DCC project. (Expresnet Interface)

The DCC project is an attempt to implement a DCC (Digital Command Control) Command Station based on the BeagleBone Black platform.
The project consists of four parts:
- DCC_BBB : the Linux code for the BeagleBone.
- DCC_PRU : the PRU code for generating the actuel DCC signal (including RailCom cutout).
- DCC_AVR (this repo) : the AVR code for additional interfaces (for example Expressnet).
- DCC_Hardware : the schematics and PCB-layout files for the hardware (KiCad)
