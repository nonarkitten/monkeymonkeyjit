#include "mmj.h"

typedef struct MMJ_CODE {
  MMJ_CODE* next;
  uint32_t entry;
  uint16_t size;
  uint16_t code;
} MMJ_CODE;

typedef struct MMJ_MEM_ACCESS {
  void* (*get_address)(MMJ_CPU*,uint32_t);
  uint8_t (*read_byte)(MMJ_CPU*,uint32_t);
  uint16_t (*read_word)(MMJ_CPU*,uint32_t);
  uint32_t (*read_long)(MMJ_CPU*,uint32_t);
  void (*write_byte)(MMJ_CPU*,uint32_t,uint8);
  void (*write_word)(MMJ_CPU*,uint32_t,uint16);
  void (*write_long)(MMJ_CPU*,uint32_t,uint32);
} MMJ_MEM_ACCESS;

typedef struct MMJ_CPU {
  uint32_t dreg[8];
  uint32_t areg[8];
  uint32_t pc;
  MMJ_MEM_ACCESS mem[256];
} MMJ_CPU;

/* The JIT performs a 'soft' context switch on each JIT block; this means saving the
** current registers, restoring the emulator ones, running our converted code and then
** performing the oposite at the tail end before returning with the next entry address
** in D0. This is enough overhead that we need to avoid 'pogo'ing in and out of short
** JIT blocks as much as possible!
*/
static const uint16_t MMJ_INTRO[] = {
  
};
static const uint16_t MMJ_INTRO_SIZE = sizeof(MMJ_INTRO);
static const uint16_t MMJ_OUTRO[] = {

};
static const uint16_t MMJ_OUTRO_SIZE = sizeof(MMJ_OUTRO);

/* The speed of our JIT will be highly dependent on how fast we can find our entry vector.
** As I see it, there are three options; dynamic array with binary search (very fast find
** but very slow insertion), a binary search tree (fairly fast find and insertion but can
** lead to memory fragmentation) or a hash table (generally considered the fastest but not
** east to interate through).
**
** I'm going with a simple hash map. In our case, we'll use the highest 8-bit prime for the
** number of tables with the table index taken as the modulo of that prime number. Each
** hash table entry contains a simple linked list, that we'll then search through; the LRU
** will always be at the head, so we're naturally optimizing towards the most recent code.
*/
#define MMJ_HASH_TABLE_SIZE 251

static MMJ_CODE* code[MMJ_HASH_TABLE_SIZE] = { 0 };


uint32_t mmj_compile(MMJ_CPU* cpu, uint32_t entry, uint16_t* emit) {
  uint8_t page = entry >> 16, opcode, length = 0;
  if(cpu && cpu->mem[page].get_address(cpu, entry)) {
    uint16_t* fetch = cpu->mem[page].get_address(cpu, entry)
    opcode = *fetch++;
    
    // A lot of copdes will just get copied one-to-one but we need to know a few things
    
    // First, we need to know the total size of the instruciton, including all the data
    // as we don't want to be converting any immediate data as if they were opcodes
    
    // Second, we need to know the addressing mode(s) to see if anything needs to get
    // fixed up. Most instructions fall into two types, the common ALU operation has
    // six bits that define the addressing mode and optional register. All move
    // instructions also have a second addressing mode and register for the source
    // data. For everything else, we'll handle it on a case-by-case basis. Since the
    // instructions are FAIRLY well grouped by the high-nibble, we'll span on that.
    
    switch(opcode >> 12) {
    case 0: // bit operations
    case 1: // move byte
    case 2: // move word
    case 3: // move long
      
    case 4:
    
    
    if(emit) emit[length] = opcode;
    length++;
  }
  return length;
}

MMJ_CODE* mmj_find(MMJ_CPU* cpu, uint32_t entry) {
  // All 68K addresses are even, so we'll skip the low bit
  uint8_t index = (entry >> 1) % MMJ_HASH_TABLE_SIZE;
  // Start with the last recently used
  MMJ_CODE* mmj = code[index];
  // And look for a matching entry or the end of list
  while(mmj && (mmj->entry != entry)) mmj = mmj->next;
  // Didn't find it, so we'll make it up
  if(!mmj) {
    // We don't know how much room we'll need so do a quick scan
    uint32_t size = mmj_compile(entry, NULL);
    // Then allocate the room
    mmj = malloc(sizeof(MMJ_CODE) + MMJ_INTRO_SIZE + size * 2 + MMJ_OUTRO_SIZE);
    // Insert our singly-linked list item
    mmj->next = code[index]; code[index] = mmj;
    // Record our entry point and size
    mmj->entry = entry; mmj->size = size;
    // Copy in the intro and outro
    memcpy(&mmj->code, MMJ_INTRO, MMJ_INTRO_SIZE);
    memcpy(MMJ_INTRO_SIZE + size * 2, MMJ_OUTRO, MMJ_OUTRO_SIZE);
    // And perform the ACTUAL compile
    mmj_compile(entry, MMJ_INTRO_SIZE + &mmj->code);
  }
  return mmj;
}


