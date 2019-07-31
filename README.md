# monkeymonkeyjit
This is an m68k on m68k JIT library -- for those not in the know, the m68k is the venerable Motorola 68000 processor used in a wide variety of classic home computers and game consoles. This is implemented as a real AmigaOS .library that can be loaded and shared between applications.

While monkeymonkeyjit is much faster than any other m68k emulation on the Amiga, it is not as fast as native execution available with Macintosh emulators and definately requires a strong 680x0 processor; a 68040 or better is recommended.

Currently, only the "classic" 68000 will be emulated. I have no intention of handling the 68020 or higher processors, 32-bit memory or floating point arithmetic.

See [Wiki](https://github.com/nonarkitten/monkeymonkeyjit/wiki) for more details.

# Useful Links
[Compiler Explorer (Amiga 68K Edition)](https://franke.ms/cex)

[Compiler Explorer (Atari 68K Edition)](http://brownbot.mooo.com/)

[Compiler Explorer (Original ARM/PPC/x86)](https://godbolt.org/)

[Online Disassembler (Supports all 68K CPUs)](https://onlinedisassembler.com/odaweb/)
