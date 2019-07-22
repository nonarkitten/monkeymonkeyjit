# monkeymonkeyjit
This is an m68k on m68k JIT library -- for those not in the know, the m68k is the venerable Motorola 68000 processor used in a wide variety of classic home computers and game consoles. This is implemented as a real AmigaOS .library that can be loaded and shared between applications.

While monkeymonkeyjit is much faster than any other m68k emulation on the Amiga, it is not as fast as native execution available with Macintosh emulators and definately requires a strong 680x0 processor; a 68040 or better is recommended.

Currently, only the "classic" 68000 will be emulated. I have no intention of handling the 68020 or higher processors, 32-bit memory or floating point arithmetic.

# theory of operation

My JIT is very simple; from an entry point we will copy and/or translate each 68000 instruction into a new buffer one at a time until we encounter a unconditional flow-control instruction, wrap the routine with the necessary entry and exit decorations and then call it.  Generally, all instructions are left unmodified, with the exception to the addressing modes on operations that actually load and store data from RAM as well as branching instructions. This makes the translation engine very simple when compared to other JIT engines that must have equivalent opcodes to every possible source operation.

## entry code

The JIT only runs user opcodes; as such, we will only save the address and data registers before jumping. This is simply:
```
move.w #$4000,_intena            ; disable interrupts
movem.l a0-a7/d0-d7,host_state   ; save all current registers
movem.l jit_state,a0-a7/d0-d7    ; restore our JIT state
move.w #$C000,_intena            ; enable interrupts
```        

## addressing modes

Generally, the main problem is with addressing memory. We'll use the 68020's base-displacement modes to add a 32-bit offset for all operations. This only needs to handle 68000 addressing modes.

**Note** that for simplicity, monkeymonkeyjit assumes a flat, 16MB block of memory has been allocated for the JIT.

In any addressing mode where the 68K code is already using a displacement; this displacement needs to be adjusted to long word with the base_addresses added to it. When handling 68000 original code, the address may need to be bound to a 16MB region and shouldn't "leak" into memory before or after.

```
     Original                Translated

000 nnn Dn              ->   000 nnn   Dn              Leave as-is
001 nnn An              ->   001 nnn   An              Leave as-is

010 nnn (An)            ->   110 nnn   (bd,An)         bd = pg
011 nnn (An)+           ->   110 nnn   (bd,An)         bd = pg; lea +width(An),An
100 nnn -(An)           ->   110 nnn   (bd,An)         lea -width(An),An; bd = pg
101 nnn (d16,An)        ->   110 nnn   (bd,An)         bd = pg + d16

110 nnn (d8,An,Xn)      ->   110 nnn   (bd,An,Xn)      bd = pg + d8

111 010 (d16,PC)        ->   111 001   (xxx).L         xxx = pg + d16 + PC
111 011 (d8,PC,Xn)      ->   110 nnn   (bd,Xn)         bd = pg + d8 + PC
111 000 (xxx).W         ->   111 001   (xxx).L         xxx += pg
111 001 (xxx).L         ->   111 001   (xxx).L         xxx += pg

Dn      Any data register
An      Any address register
Xn      Any register, data or address
pg      Page, the 16MB page (ideally aligned to 24-bit) where the emulated code lays
bd      The base-displacement, a 32-bit value provided to the opcode (2 words)
d8      The original 8-bit diplacement, this must be wrapped to 24-bits
d16     The original 16-bit diplacement, this must be wrapped to 24-bits
PC      At JIT we know the PC and can replace this with the actual address
```

## bank-switching and self-modifying code

Most 16-bit era hardware implemented features that could react on both memory reads and writes, such as writing data to a serial port, reading the raster position or performing a bank-switch. In most cases these can be safely ignored until handled by the appropriate hardware emulation, but bank-switching poses a unique problem and this is the same problem of self-modifying code: the next instruction might not be the one we originally expected.

### To-Do: figure out how to handle bank-switching and self-modifying code

## branching

Generally, any control-flow operation should exit the block because we cannot gaurantee address validity of the jump target. This is especially tricky with the 68000 and it's wide variety of addressing modes that can be used with JSR and JMP opcodes. For the purposes of this version of monkeymonkey JIT, only two branching coditions will NOT exit the compiler:

1. A reverse Bcc or DBcc to a point between the current entry point and the branch is presumed to be a loop. This will decrement our global cycle counter by the extra number of cycles to take the branch and re-execute the code between the branch target address and the branch. There is no exit on a cycle count underflow -- this will make our emulator slightly not cycle PRECISE even if it's still cycle exact.

2. A forward branch that does not cross any unconditional opcodes such as JSR, JMP, RTS or RTD but can land on them and is within the range of the JIT's maximum block length size (TBD). This is commonly used as either exception-handling code, jump tables or to jump forward to the loop conditional (zero-time exit).

Under no circumstance can JSR, JMP, RTS or RTD even be included in the JIT code. Any Bcc or DBcc that does not meet the criteria will also terminate the JIT compiler.

## branch cycle counting

When cycle counting, we exclude, by default, any code that MIGHT be branched over. That is, the JIT block will only count the cycles of instructions that are KNOWN to execute.

Any forward branch is immediately follwed by a SUB[Q] opcode to deduct the number of cycles that it would have skipped if the branch had instead been taken. Since this SUB[Q] opcode is not executed if the branch is taken, the JIT block tally must not also include this cycle time.

Any reverse branch is immediately preceeded by a SUB[Q] opcode to deduct the number of cycles executed between the branch target address and the branch itself. Because this code must have been executedd at least once, the SUB[Q] will tally the cycle count and this should also be omitted from the final cycle count.

## exit code

Once complete, obviously the reverse of the above is required. We also need to eat some CPU cycles (for timing) and return the NEXT entry point decided by the flow-control instruction we're handling as the last instruction. This will be dependant upon the opcode we're emulating.
```
move.w #$4000,_intena            ; disable interrupts

; jump exit pre-handler

movem.l a0-a7/d0-d7,jit_state    ; save all current registers
movem.l host_state,a0-a7/d0-d7   ; restore our HOST state
sub[q].l #<cycles_used>,cycles   ; consume some cpu cycles

; jump exit post-handler

move.w #$C000,_intena            ; enable interrupts
rts                              ; and return to the host
```

## jump exit handlers

When we need to terminate our block because of an unconditional jump or unhandled branch, we need to store the destination PC for the JIT engine to re-enter.

**Bcc**
```
pre:
  bcc .taken
.nottaken
  move #jit_pc,jit_pc_temp
  bra .done
.taken
  move #jit_pc+displacement,jit_pc_temp
.done

post:
  move.l jit_pc_temp,d0  
```

**DBcc**
```
pre:
  dbcc Dx,.taken
.nottaken
  move #jit_pc,jit_pc_temp
  bra .done
.taken
  move #jit_pc+displacement,jit_pc_temp
.done

post:
  move.l jit_pc_temp,d0  
```

**JMP**
```
post:
  lea <original-ea>,a0
  move.l a0,d0
```

**JSR**
```
pre:
  move.l #jit_pc,-(sp)
post:
  lea <original-ea>,a0
  move.l a0,d0
```

**RTS**
```
pre:
  move.l (sp)+,jit_pc_temp
post:
  move.l jit_pc_temp,d0
```

**RTD**
```
pre:
  move.l (sp)+,jit_pc_temp
  add.w #displacememt,sp
post:
  move.l jit_pc_temp,d0
```

# Useful Links
[Compiler Explorer (Amiga 68K Edition)](https://franke.ms/cex)

[Compiler Explorer (Atari 68K Edition)](http://brownbot.mooo.com/)

[Compiler Explorer (Original ARM/PPC/x86)](https://godbolt.org/)

[Online Disassembler (Supports all 68K CPUs)](https://onlinedisassembler.com/odaweb/)
