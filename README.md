# STRNG4DS

Project Summary:
Use a small radioactive sample to generate true random to seed /dev/random on linux dedicated servers.

Compiling Arduino:
Use Arduino IDE with modifications for your board, this prototype used the mega.

Compiling linux source to binary:
gcc strng4ds.c -o strng4ds.out -lm
