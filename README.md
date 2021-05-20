# UFC32
UFC32 is a planned emulated game platform based on two 32-bit RISC processors known as the PEX, 2MB of main memory, and 16MB of cartridge space available, as well as a resolution of 512x224 with 32,768 colors available. Currently, there is a partially implemented PEX emulator written in C, as well as a partially written PEX assembler in Python. To come include these completed versions of these projects, as well as a compiler known as "pexc" that will be written in D and provided a semi-high-level compiled language to the developer's toolkit. A standard library is to come after that, and then more and more software.

The C compiler is found in the "compiler" folder and currently supports the following
- Global variables and arrays
- Local variables
- Expressions (and pointers)
- Functions

The remaining components to still be added include
- Loops (for/while) and conditional statements (if/else)
- Local arrays
- Preprocessor
