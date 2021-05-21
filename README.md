# UFC32
UFC32 is a planned emulated game platform based on two 32-bit RISC processors known as the PEX, 2MB of main memory, and 16MB of cartridge space available, as well as a resolution of 512x224 with 32,768 colors available. Currently, there is a partially implemented PEX emulator written in C, as well as a partially written PEX assembler in Python. There is also a compiler known as "pexc" that is written in D, providing a semi-high-level compiled language to the developer's toolkit. A standard library is to come after that, and then more and more software, as well as more substantial documentation and example code for developers.

The C compiler is found in the "compiler" folder and currently supports the following
- Global and local variables
- Expressions (and pointers)
- Functions
- Conditional statements (if/else) 
- Local and global arrays
- Loops (while and for)
- Inline ASM

The remaining components to still be added include
- Preprocessor
- Comments (both C89/C90 and C99 style)
- Assigning a char value or array to a u32
- C99-style for loops (declare variable while declaring for loop)
- C99-style VLAs (Variable Length Arrays)

There are currently no plans to add support for structs/unions, gotos, or any C++ features/constructs at this time. 



**Usage**

You'll need to compile the C compiler using your D compiler of choice. For example, using DMD, type the commands "dub build" in the compiler directory. Once it's built, use the following syntax to compile your code: compiler _input.pexc_ _output.pexs_

Once you have an assembly language file, you can look at or edit the file in any standard ASCII text editor, potentially performing manual optimizations, but generally you'll want to assemble it. To do this, copy your _output.pexs_ file into the directory with asm.py (or vice-versa), and then, assuming Python is in your path environment variable, run the following command: python asm.py _output.pexs_ _output.bin_

You now have a binary ROM image that you can load into the PEX/UFC32 emulator. Copy the bin file into the same directory as cpu.c, compile it using your C compiler of choice, and then run: cpu _output.bin_

Currently, the virtual machine goes straight into a debugger, which shows a live disassembly and status of the registers, along with the status of the memory location 510 (0x1FE), as that memory address has often been used for testing.
