# Chip-8 Interpreter

<p>
    This is my chip-8 interpreter that I wrote in C. Chip-8 is a simple interpreted programming language that was used on diy computers in the late 70s and 80s. It is well documented online and a starting point into the world of emulation development. <br><br>
    During the development I relied on the following resources for guidance and I encourage you to do the same if you want to try to build a chip-8 interpreter.<br>
<p>

*http://devernay.free.fr/hacks/chip8/C8TECH10.HTM*

*https://tobiasvl.github.io/blog/write-a-chip-8-emulator/*


<p>
The keyboard for the chip-8 is a 16 key hexadecimal keyboard with the following layout. <br>

<p>


    key setup

    Keypad       Keyboard
    +-+-+-+-+    +-+-+-+-+
    |1|2|3|C|    |1|2|3|4|
    +-+-+-+-+    +-+-+-+-+
    |4|5|6|D|    |Q|W|E|R|
    +-+-+-+-+ => +-+-+-+-+
    |7|8|9|E|    |A|S|D|F|
    +-+-+-+-+    +-+-+-+-+
    |A|0|B|F|    |Z|X|C|V|
    +-+-+-+-+    +-+-+-+-+

<p>
It is also important to note that different programs for the chip-8 run at different clock cycle speeds. I have allowed the user to mess with the system speed by pressing f1 (slowdown) and f2 (speedup).<br><br>
I have provided some roms in the rom folder but I encourage you to checkout some of the roms at<br>
<p>

*https://johnearnest.github.io/chip8Archive/*

<p>
    Please make sure that the platform listed is chip8 and not any chip8 extensions such as the superchip8.
<p>


<p>
To run the emulator you will need SDL2 intalled on your system and the gcc compiler. I have provided a makefile for compilation. To run the program execute
<p>

---
./chip8 *romfile*