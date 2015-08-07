#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include <readline/readline.h>
//#include <readline/history.h>

#include <md.h>

// Sizes in address units.
#define COMPILING (1)
#define INTERPRETING (0)

typedef intptr_t cell;
typedef uintptr_t ucell;
typedef unsigned char bool;
#define true (-1)
#define false (0)
#define CHAR_SIZE 1
#define CELL_SIZE sizeof(cell)

typedef void code(void);

typedef struct header_ {
  struct header_ *link;
  cell metadata; // See below.
  char *name;
  code *code_field;
} header;


// Globals that drive the Forth engine: SP, RSP, IP, CFA.
// These should generally be pinned to registers.
// Stacks are full-descending, and can therefore be used like arrays.

#define DATA_STACK_SIZE 16384
cell _stack_data[DATA_STACK_SIZE];
cell *spTop = &(_stack_data[DATA_STACK_SIZE]);
cell *sp;

#define RETURN_STACK_SIZE 1024
cell _stack_return[RETURN_STACK_SIZE];
cell *rspTop = &(_stack_return[RETURN_STACK_SIZE]);
cell *rsp;

code ***ip;
code **cfa;

code *quitTop = NULL;
code **quitTopPtr = &quitTop;

union {
  cell* cells;
  char* chars;
} dsp;


// A few more core globals.
cell state;
cell base;
header *dictionary;
header *latest;

typedef struct {
  cell parseLength;
  cell inputPtr; // Indexes into parseBuffer.
  cell type;     // 0 = KEYBOARD, -1 = EVALUATE, 0> fileid
  char parseBuffer[256];
} source;

source inputSources[16];
cell inputIndex;
#define SRC (inputSources[inputIndex])


// And some miscellaneous helpers. These exist because I can't use locals in
// these C implementations without leaking stack space.
cell c1;
char ch1;
char* str1;
char** strptr1;
size_t tempSize;
header* tempHeader;
char tempBuf[256];

// NB: If NEXT changes, EXECUTE might need to change too (it uses NEXT1)
#define NEXT1 do { goto **cfa; } while(0)
#define NEXT do { cfa = *ip++; goto **cfa; } while(0)

// Implementations of the VM primitives.
// These functions MUST NOT USE locals, since that will use the stack.
// They should probably be one-liners, or nearly so, in C.
// They should generally finish with the NEXT or NEXT1 macros.

#define LEN_MASK        (0xff)
#define LEN_HIDDEN_MASK (0x1ff)
#define HIDDEN          (0x100)
#define IMMEDIATE       (0x200)

#define WORD(id, name, metadata, link) __attribute__((__noreturn__, __used__)) void code_ ## id (void);\
header header_ ## id = { link, metadata, name, &code_ ## id };\
__attribute__((__noreturn__, __used__)) void code_ ## id (void)


void print(char *str, cell len) {
  str1 = (char*) malloc(len + 1);
  strncpy(str1, str, len);
  str1[len] = '\0';
  printf("%s", str1);
  free(str1);
}


// Math operations
WORD(plus, "+", 1, NULL) {
  sp[1] = sp[0] + sp[1];
  sp++;
  NEXT;
}

WORD(minus, "-", 1, &header_plus) {
  sp[1] = sp[1] - sp[0];
  sp++;
  NEXT;
}

WORD(times, "*", 1, &header_minus) {
  sp[1] = sp[1] * sp[0];
  sp++;
  NEXT;
}

WORD(div, "/", 1, &header_times) {
  sp[1] = sp[1] / sp[0];
  sp++;
  NEXT;
}

WORD(mod, "MOD", 3, &header_div) {
  sp[1] = sp[1] % sp[0];
  sp++;
  NEXT;
}


// Bitwise ops
WORD(and, "AND", 3, &header_mod) {
  sp[1] = sp[1] & sp[0];
  sp++;
  NEXT;
}
WORD(or, "OR", 2, &header_and) {
  sp[1] = sp[1] | sp[0];
  sp++;
  NEXT;
}
WORD(xor, "XOR", 3, &header_or) {
  sp[1] = sp[1] ^ sp[0];
  sp++;
  NEXT;
}

// Shifts
WORD(lshift, "LSHIFT", 6, &header_xor) {
  sp[1] = ((ucell) sp[1]) << sp[0];
  sp++;
  NEXT;
}

WORD(rshift, "RSHIFT", 7, &header_lshift) {
  sp[1] = ((ucell) sp[1]) >> sp[0];
  sp++;
  NEXT;
}

WORD(base, "BASE", 4, &header_rshift) {
  *(--sp) = (cell) base;
  NEXT;
}

// Comparison
WORD(less_than, "<", 1, &header_base) {
  sp[1] = (sp[1] < sp[0]) ? -1 : 0;
  sp++;
  NEXT;
}

WORD(less_than_unsigned, "U<", 2, &header_less_than) {
  sp[1] = ((ucell) sp[1]) < ((ucell) sp[0]) ? -1 : 0;
  sp++;
  NEXT;
}

WORD(equal, "=", 1, &header_less_than_unsigned) {
  sp[1] = sp[0] == sp[1] ? -1 : 0;
  sp++;
  NEXT;
}

// Stack manipulation
WORD(dup, "DUP", 3, &header_equal) {
  sp--;
  sp[0] = sp[1];
  NEXT;
}

WORD(swap, "SWAP", 4, &header_dup) {
  c1 = sp[0];
  sp[0] = sp[1];
  sp[1] = c1;
  NEXT;
}

WORD(drop, "DROP", 4, &header_swap) {
  sp++;
  NEXT;
}

WORD(to_r, ">R", 2, &header_drop) {
  *(--rsp) = *(sp++);
  NEXT;
}

WORD(from_r, "R>", 2, &header_to_r) {
  *(--sp) = *(rsp++);
  NEXT;
}

// Memory access
WORD(fetch, "@", 1, &header_from_r) {
  sp[0] = *((cell*) sp[0]);
  NEXT;
}
WORD(store, "!", 1, &header_fetch) {
  *((cell*) sp[0]) = sp[1];
  sp += 2;
  NEXT;
}
WORD(cfetch, "C@", 2, &header_store) {
  sp[0] = (cell) *((char*) sp[0]);
  NEXT;
}
WORD(cstore, "C!", 2, &header_cfetch) {
  *((char*) sp[0]) = (char) sp[1];
  sp += 2;
  NEXT;
}

// Allocates new regions. Might use malloc, or just an advancing pointer.
// The library calls this to acquire somewhere to put HERE.
// ( size-in-address-units -- a-addr )
WORD(raw_alloc, "(ALLOCATE)", 10, &header_cstore) {
  sp[0] = (cell) malloc(sp[0]);
  NEXT;
}

WORD(here_ptr, "(>HERE)", 7, &header_raw_alloc) {
  *(--sp) = (cell) (&dsp);
  NEXT;
}

WORD(state, "STATE", 5, &header_here_ptr) {
  *(--sp) = (cell) &state;
  NEXT;
}

// TODO: DEBUG Remove me
WORD(dot, ".", 1, &header_state) {
  printf("%" PRIdPTR " ", *(sp++));
  NEXT;
}
WORD(udot, "U.", 2, &header_dot) {
  printf("%" PRIuPTR " ", (ucell) *(sp++));
  NEXT;
}
// TODO: DEBUG Remove me
WORD(debug, "debug", 5, &header_udot) {
  NEXT;
}

// Branches
// Jumps unconditionally by the delta (in bytes) of the next CFA.
WORD(branch, "(BRANCH)", 8, &header_debug) {
  str1 = (char*) ip;
  str1 += (cell) *ip;
  ip = (code***) str1;
  NEXT;
}

// Consumes the top argument on the stack. If it's 0, jumps over the branch
// address. Otherwise, identical to branch above.
WORD(zbranch, "(0BRANCH)", 9, &header_branch) {
  str1 = (char*) ip;
  str1 += *(sp++) == 0 ? (cell) *ip : (cell) sizeof(cell);
  ip = (code***) str1;
  NEXT;
}

//WORD(literal, "LITERAL", 7, &header_zbranch) {
//  *(dsp.cells++) = &(header_dolit.code_field);
//  *(dsp.cells++) = *(sp++);
//  NEXT;
//}

WORD(execute, "EXECUTE", 7, &header_zbranch) {
  cfa = (code**) *(sp++);
  NEXT1;
}


// Input
cell refill_(void) {
  if (SRC.type == -1) { // EVALUATE
    // EVALUATE strings cannot be refilled. Pop the source.
    inputIndex--;
    return 0;
  } else if ( SRC.type == 0) { // KEYBOARD
    str1 = readline("> ");
    SRC.parseLength = strlen(str1);
    strncpy(SRC.parseBuffer, str1, SRC.parseLength);
    SRC.inputPtr = 0;
    free(str1);
    return -1;
  } else {
    str1 = NULL;
    tempSize = 0;
    c1 = getline(&str1, &tempSize, (FILE*) SRC.type);

    if (c1 == -1) {
      // Dump the source and recurse.
      inputIndex--;
      return 0;
    } else {
      // Knock off the trailing newline, if present.
      if (str1[c1 - 1] == '\n') c1--;
      strncpy(SRC.parseBuffer, str1, c1);
      free(str1);
      SRC.parseLength = c1;
      SRC.inputPtr = 0;
      return -1;
    }
  }
}

WORD(refill, "REFILL", 6, &header_execute) {
  *(--sp) = refill_();
  NEXT;
}

WORD(latest, "(LATEST)", 8, &header_refill) {
  *(--sp) = (cell) latest;
  NEXT;
}

WORD(in_ptr, ">IN", 3, &header_latest) {
  *(--sp) = (cell) (&SRC.inputPtr);
  NEXT;
}

WORD(emit, "EMIT", 4, &header_in_ptr) {
  fputc(*(sp++), stdout);
  NEXT;
}


// Sizes and metadata
WORD(size_cell, "(/CELL)", 7, &header_emit) {
  *(--sp) = (cell) sizeof(cell);
  NEXT;
}

WORD(size_char, "(/CHAR)", 7, &header_size_cell) {
  *(--sp) = (cell) sizeof(char);
  NEXT;
}

// Converts a header* eg. from (latest) into the CFA.
WORD(to_code, ">CODE", 5, &header_size_char) {
  tempHeader = (header*) sp[0];
  sp[0] = (cell) &(tempHeader->code_field);
  NEXT;
}

// Advances a CFA to the data-space pointer.
WORD(to_body, ">BODY", 5, & header_to_code) {
  sp[0] += (cell) sizeof(cell);
  NEXT;
}


// Compiler helpers

// Pushes ip -> rsp, and puts my own data field into ip.
WORD(docol, "(DOCOL)", 7, &header_to_body) {
  *(--rsp) = (cell) ip;
  ip = (code***) &(cfa[1]);
  NEXT;
}

// Pushes its data field onto the stack.
WORD(dolit, "(DOLIT)", 7, &header_docol) {
  *(--sp) = (cell) *(ip++);
  NEXT;
}

WORD(dostring, "(DOSTRING)", 10, &header_dolit) {
  str1 = ((char*) ip);
  c1 = (cell) *str1;
  sp -= 2;
  sp[1] = (cell) (str1 + 1);
  sp[0] = c1;

  str1 += c1 + 1 + (sizeof(cell) - 1);
  str1 = (char*) (((cell) str1) & ~(sizeof(cell) - 1));
  ip = (code***) str1;

  NEXT;
}

// CREATE compiles 0 and then the user's code into the data space.
// It uses (dodoes) as the doer word, not docol! That will push the address of
// the user's data space area, as intended (cfa + 2 cells) and then check that
// 0 at cfa + 1 cell. If it's 0, do nothing. Otherwise, jump to that point.
WORD(dodoes, "(DODOES)", 8, &header_dostring) {
  *(--sp) = (cell) &(cfa[2]);
  c1 = (cell) cfa[1];

  // Similar to docol, push onto the return stack and jump.
  if (c1 != 0) {
    *(--rsp) = (cell) ip;
    ip = (code***) c1;
  }
  NEXT;
}

void parse_(void) {
  if ( SRC.inputPtr >= SRC.parseLength ) {
    sp[0] = 0;
    *(--sp) = 0;
  } else {
    ch1 = (char) sp[0];
    str1 = SRC.parseBuffer + SRC.inputPtr;
    c1 = 0;
    while ( SRC.inputPtr < SRC.parseLength && SRC.parseBuffer[SRC.inputPtr] != ch1 ) {
      SRC.inputPtr++;
      c1++;
    }
    if ( SRC.inputPtr < SRC.parseLength ) SRC.inputPtr++; // Skip over the delimiter.
    sp[0] = (cell) str1;
    *(--sp) = c1;
  }
}

void parse_name_(void) {
  // Skip any leading delimiters.
  while ( SRC.inputPtr < SRC.parseLength && SRC.parseBuffer[SRC.inputPtr] == ' ' ) {
    SRC.inputPtr++;
  }
  c1 = 0;
  str1 = SRC.parseBuffer + SRC.inputPtr;
  while ( SRC.inputPtr < SRC.parseLength && SRC.parseBuffer[SRC.inputPtr] != ' ' ) {
    SRC.inputPtr++;
    c1++;
  }
  if (SRC.inputPtr < SRC.parseLength) SRC.inputPtr++; // Jump over a trailing delimiter.
  *(--sp) = (cell) str1;
  *(--sp) = c1;
}

void to_number_(void) {
  // sp[0] is the length, sp[1] the pointer, sp[2] the high word, sp[3] the low.
  // ( lo hi c-addr u -- lo hi c-addr u )
  str1 = (char*) sp[1];
  while (sp[0] > 0) {
    c1 = (cell) *str1;
    if ('0' <= c1 && c1 <= '9') {
      c1 -= '0';
    } else if ('A' <= c1 && c1 <= 'Z') {
      c1 = c1 - 'A' + 10;
    } else if ('a' <= c1 && c1 <= 'z') {
      c1 = c1 - 'a' + 10;
    } else {
      break;
    }

    if (c1 >= base) break;

    // Otherwise, a valid character, so multiply it in.
    sp[3] *= base;
    sp[3] += c1;
    sp[0]--;
    str1++;
  }
  sp[1] = (cell) str1;
}

// Expects c-addr u on top of the stack.
// Returns 0 0 on not found, xt 1 for immediate, or xt -1 for not immediate.
void find_(void) {
  tempHeader = dictionary;
  while (tempHeader != NULL) {
    if ((tempHeader->metadata & LEN_HIDDEN_MASK) == sp[0]) {
      if (strncasecmp(tempHeader->name, (char*) sp[1], sp[0]) == 0) {
        sp[1] = (cell) (&(tempHeader->code_field));
        sp[0] = (tempHeader->metadata & IMMEDIATE) == 0 ? -1 : 1;
        return;
      }
    }
    tempHeader = tempHeader->link;
  }
  sp[1] = 0;
  sp[0] = 0;
}

WORD(parse, "PARSE", 5, &header_dodoes) {
  parse_();
  NEXT;
}

WORD(parse_name, "PARSE-NAME", 10, &header_parse) {
  parse_name_();
  NEXT;
}

WORD(to_number, ">NUMBER", 7, &header_parse_name) {
  to_number_();
  NEXT;
}

// Parses a name, and constructs a header for it.
// When finished, HERE is the data space properly, ready for compilation.
WORD(create, "CREATE", 6, &header_to_number) {
  parse_name_(); // sp[0] = length, sp[1] = string
  dsp.chars = (char*) ((((cell)dsp.chars) + sizeof(cell) - 1) & ~(sizeof(cell) - 1));
  tempHeader = (header*) dsp.chars;
  dsp.chars += sizeof(header);
  tempHeader->link = dictionary;
  latest = dictionary = tempHeader;

  tempHeader->metadata = sp[0];
  tempHeader->name = (char*) malloc(sp[0] * sizeof(char));
  strncpy(tempHeader->name, (char*) sp[1], sp[0]);
  sp += 2;
  tempHeader->code_field = &code_dodoes;

  // Add the extra cell for dodoes; this is the DOES> address, or 0 for none.
  *(dsp.cells++) = 0;
  NEXT;
}

WORD(find, "(FIND)", 6, &header_create) {
  find_();
  NEXT;
}

WORD(depth, "DEPTH", 5, &header_find) {
  c1 = (cell) (((char*) spTop) - ((char*) sp)) / sizeof(cell);
  *(--sp) = c1;
  NEXT;
}

// This could easily enough be turned into a Forth word.
char *savedString;
cell savedLength;

void quit_(void) {
  // Empty the stacks.
quit_top:
  sp = spTop;
  rsp = rspTop;
  state = INTERPRETING;
  // Refill the input buffer.
  // TODO: When different input sources are supported, reset to keyboard.
  refill_();
  // And start trying to parse things.
  while (true) {
    // ( )
quit_loop:
    while (true) {
      parse_name_(); // ( c-addr u )
      if (sp[0] != 0) {
        break;
      } else {
        if (SRC.type == 0) printf("  ok\n");
        refill_();
        sp += 2;
      }
    }

    // ( c-addr u ) and u is nonzero
    savedString = (char*) sp[1]; // Set aside the string and length.
    savedLength = sp[0];
    //print(savedString, savedLength);
    //printf("\n");
    find_(); // xt immediate (or 0 0)
    if (sp[0] == 0) { // Failed to parse. Try to parse as a number.
      // I can use the existing ( 0 0 ) as the empty number for >number
      // TODO: Handle negative numbers.
      // TODO: Parse $ff numbers and so on. Maybe do that from Forth?
      ch1 = 0; // ch1 = negate here.
      if (*savedString == '-') { // Negative number.
        ch1 = 1;
        savedString++;
        savedLength--;
      }

      sp -= 2;
      sp[0] = savedLength;
      sp[1] = (cell) savedString; // Bring back the string and length.

      to_number_();
      if (sp[0] == 0) { // Successful parse, handle the number.
        if (ch1) sp[3] = -sp[3];

        if (state == COMPILING) {
          *(dsp.cells++) = (cell) &(header_dolit.code_field);
          *(dsp.cells++) = sp[3]; // Compile low word as the literal.
          sp += 4; // And clear the stack.
        } else {
          // Clear my mess from the stack, but leave the new number atop it.
          sp += 3;
        }
      } else { // Failed parse of a number. Unrecognized word.
        strncpy(tempBuf, savedString, savedLength);
        tempBuf[savedLength] = '\0';
        fprintf(stderr, "*** Unrecognized word: %s\n", tempBuf);
        goto quit_top;
      }
    } else {
      // Successful parse. ( xt 1 ) indicates immediate, ( xt -1 ) not.
      if (sp[0] == 1 || state == INTERPRETING) {
        quitTop = &&quit_loop;
        ip = &quitTopPtr;
        cfa = (code**) sp[1];
        sp += 2;
        //NEXT1;
        QUIT_JUMP_IN;
        __builtin_unreachable();
      } else { // Compiling mode
        *(dsp.cells++) = sp[1];
        sp += 2;
      }
    }
  }
  // Should never be reachable.
}

WORD(quit, "QUIT", 4, &header_depth) {
  inputIndex = 0;
  quit_();
  NEXT;
}

WORD(colon, ":", 1, &header_quit) {
  tempHeader = (header*) dsp.chars;
  dsp.chars += sizeof(header);
  tempHeader->link = dictionary;
  dictionary = tempHeader;
  latest = tempHeader;
  parse_name_(); // ( c-addr u )
  if (sp[0] == 0) {
    fprintf(stderr, "*** Colon definition with no name\n");
    code_quit();
    // Never returns
  }

  tempHeader->name = (char*) malloc(sp[0]);
  strncpy(tempHeader->name, (char*) sp[1], sp[0]);
  tempHeader->metadata = sp[0] | HIDDEN;
  tempHeader->code_field = &code_docol;

  state = COMPILING;
  NEXT;
}

WORD(exit, "EXIT", 4, &header_colon) {
  // Pop the return stack and NEXT into it.
  ip = (code***) *(rsp++);
  NEXT;
}

WORD(see, "SEE", 3, &header_exit) {
  // Parses a word and visualizes its contents.
  parse_name_();
  printf("Decompiling ");
  print((char*) sp[1], sp[0]);
  printf("\n");

  find_(); // Now xt and flag on the stack.
  if (sp[0] == 0) {
    printf("NOT FOUND!\n");
  } else {
    cfa = (code**) sp[1];
    if (*cfa != &code_docol) {
      printf("Not compiled using DOCOL; can't SEE native words.\n");
    } else {
      do {
        cfa++;
        str1 = (char*) *cfa;
        tempHeader = (header*) (str1 - sizeof(cell) * 3);
        printf("%" PRIuPTR ": ", (ucell) cfa);
        print(tempHeader->name, tempHeader->metadata & LEN_MASK);
        printf("\n");
      } while (*cfa != (code*) &(header_exit.code_field));
    }
  }

  sp += 2; // Drop the parsed values.
  NEXT;
}

WORD(semicolon, ";", 1 | IMMEDIATE, &header_see) {
  latest->metadata &= (~HIDDEN); // Clear the hidden bit.
  // Compile an EXIT
  *(dsp.cells++) = (cell) &(header_exit.code_field);
  // And stop compiling.
  state = INTERPRETING;
  NEXT;
}

// NB: If anything gets added after SEMICOLON, change the dictionary below.

// TODO: File input
int main(int argc, char **argv) {
  dictionary = &header_semicolon;
  base = 10;
  inputIndex = 0;
  SRC.type = SRC.parseLength = SRC.inputPtr = 0;

  // Open the input files in reverse order and push them as file inputs.
  argc--;
  for (; argc > 0; argc--) {
    inputIndex++;
    SRC.type = (cell) fopen(argv[argc], "r");
    if ((FILE*) SRC.type == NULL) {
      fprintf(stderr, "Could not load input file: %s\n", argv[argc]);
      exit(1);
    }

    SRC.inputPtr = 0;
    SRC.parseLength = 0;
  }

  quit_();
}

