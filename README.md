# STRNG4DS

## Project Summary:
Use a small radioactive sample to generate true random to seed /dev/random on linux dedicated servers.<br/><br/>
![Fiestaware Plate, Arduino and Geiger Counter](prototype.png)<br/><br/>
Using a Fiestaware Plate, an Arduino and a Geiger Counter we can collect truely random entropy from the decay of uranium fission decay products!

# Quick Start Guide

1. To get started plug your Arduino into your machine.
2. Update the interruptPin variable in the `arduino-trng.ino` sketch to the pin number you want to use as the trigger.
3. Upload the sketch to your Arduino.
4. Connect your Geiger counter or trigger source to the specified interrupt pin.
5. In the Linux terminal look for your device at `/dev/ttyACMx` or `/dev/ttyUSBx` (Where x is a dynamically assigned number).
6. Based on this you should set the build option variable serialInterface in `strng4ds.c` | `0` for `/dev/ttyACMx` and `1` for `/dev/ttyUSBx`
7. Compile the Linux program: `gcc strng4ds.c -o strng4ds.out -lm`
8. Run/Test the program: `sudo ./strng4ds.out`

# Arduino

### Compiling Arduino:
Use Arduino IDE with modifications for your board, this prototype used the mega however many other boards should work perfectly fine.
If you are using a different board you may need to change the `interruptPin` used by `attachInterrupt`.  
For further reference please check the [documentation](https://www.arduino.cc/reference/en/language/functions/external-interrupts/attachinterrupt/).



# Linux

### Compiling linux source to binary:
`gcc strng4ds.c -o strng4ds.out -lm`

### Running binary
You will need to use the sudo command or run as root to use the binary as it requires root to modify `/dev/random`. For example: `sudo ./strng4ds.out`

### Modes: (Normal / Diehard)
For normal operation set the dieHardMode build option to 0 (default).  
For testing with diehard or similar test suite set dieHardMode to 1.

# Devices validated
- Arduino: Mega, Uno
- Geiger counters: eBay device (Seen in prototype image), GMC-300E+
- G-M tubes: eBay tube, SBM-20
- Radioactive source: Uranium oxide glaze (dinnerware)

# Notes
The radioactive entropy source used in this project is [uranium oxide glaze](https://en.wikipedia.org/wiki/Fiesta_(dinnerware)#Radioactive_glazes).  
In theory any beta/gamma source should work fine however the glazes used in plates seem to make it safer to work with.    
Normal operation using a safe beta/gamma source should result in roughly 1000+ cpm.
This should add 100+ bits of actual entropy to the pool every second.
To be safe we only add 1/4 of the added entropy to the count in the add32BitsToEntropy function.
This large safety margin is to account for the possibility of biases being introduced in the various setups which are used to generate the entropy. (Eg, bad Arduino clock, poor trigger signal quality, etc)
