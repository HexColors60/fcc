#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include <readline/readline.h>
#include <readline/history.h>

// TODO: Make these portable to other platforms.
// Sizes in address units.
#define CHAR_SIZE 1
#define CELL_SIZE sizeof(intptr_t)

#define COMPILING (1)
#define INTERPRETING (0)

typedef unsigned char character;
typedef intptr_t cell;
typedef uintptr_t ucell;
typedef unsigned char bool;
#define true (1)
#define false (0)

struct fstate_;

typedef struct string_ {
  unsigned int length;
  char* value;
} string;

typedef void nativeFn(struct fstate_*);

typedef struct word_ {
  struct word_* link;
  unsigned char nameLen;
  bool hidden;
  bool immediate;
  bool native;
  char* name;
  union {
    nativeFn* raw;
    struct word_** words;
  } code;
} word;

typedef struct fstate_ {
  // All stacks, and codespace, are empty-ascending.
  unsigned char space[1024*1024];
  unsigned char* here;

  word* dictionary;
  word** returnStack[1024];
  unsigned int rsp;
  cell stack[4096];
  unsigned int sp;

  word** nextWord;
  cell state; // This is a cell for use by the STATE word.
  cell base;  // Likewise.

  char internalBuffer[1024];
  char* inputStart;
  cell inputIndex;
  unsigned int parseLength;
  unsigned char inputSource; // -1 = EVALUATE, 0 = keyboard, > 0 = fd
} fstate;

word* latestWord = NULL;


cell pop(fstate* f) {
  return f->stack[--f->sp];
}

void push(fstate* f, cell value) {
  f->stack[f->sp++];
}

word** popR(fstate* f) {
  return f->returnStack[--f->rsp];
}

void pushR(fstate* f, word** value) {
  f->returnStack[f->rsp++];
}


void write_byte(fstate* f, unsigned char c) {
  *(f->here++) = c;
}
void write_cell(fstate* f, cell c) {
  cell* p = (cell*) (f->here);
  *p = c;
  f->here += sizeof(cell);
}


// Main interpreter words.
void run(fstate* f);


// Parsing words
void refill_buffer(fstate* f) {
  // TODO: EVALUATE, file input
  // Read a line from the keyboard into the buffer.
  char * raw = readline("> ");
  unsigned int len = strlen(raw);
  strcpy(f->internalBuffer, raw);
  f->parseLength = len;
  f->inputStart = f->internalBuffer;
  f->inputIndex = 0;
  free(raw);
}

// Parses up to the given delimiter. Does NOT skip leading spaces.
string* parse(fstate* f, unsigned char delim) {
  string* s = (string*) malloc(sizeof(string));
  s->value = (char*) f->inputStart + f->inputIndex;
  s->length = 0;

  while (f->inputIndex < f->parseLength && f->inputStart[f->inputIndex] != delim) {
    s->length++;
    f->inputIndex++;
  }

  return s;
}


void skip_leading(fstate* f, unsigned char delim) {
  while (f->inputIndex < f->parseLength && f->inputStart[f->inputIndex] == delim) {
    f->inputIndex++;
  }
}

string* parse_word(fstate *f) {
  skip_leading(f, ' ');
  return parse(f, ' ');
}

word* find_word(fstate* f, string* s) {
  word* w = f->dictionary;
  while (w != NULL) {
    if (!w->hidden && w->nameLen == s->length && strcmp(w->name, s->value) == 0) {
      break;
    }
    w = w->link;
  }
  return w;
}

word* read_word(fstate* f) {
  string* s = parse_word(f);
  if (s->length == 0) return NULL;
  word* w = find_word(f, s);
  free(s);
  return w;
}


#define NATIVE(name, str) word word_ ## name;\
void code_ ## name (fstate* f)

#define NATIVE_SPEC(n, str) do { word *w = & word_ ## n ;\
w->link = latestWord;\
latestWord = w;\
w->name = str ;\
w->nameLen = strlen( str );\
w->native = true;\
w->code.raw = & code_ ## n;\
} while(0)


NATIVE(store, "!") {
  cell* addr = (cell*) pop(f);
  *addr = pop(f);
}

// Unimplemented: # #> #S

NATIVE(tick, "'") {
  push(f, (cell) read_word(f));
}

NATIVE(minus, "-") {
  cell b = pop(f);
  cell a = pop(f);
  push(f, a - b);
}

// Unimplemented: ( */ */MOD

NATIVE(times, "*") {
  push(f, pop(f) * pop(f));
}

NATIVE(plus, "+") {
  push(f, pop(f) * pop(f));
}

// Unimplemented: +! +LOOP
NATIVE(comma, ",") {
  write_cell(f, pop(f));
}

NATIVE(dot, ".") {
  // TODO: Handle BASE here.
  printf("%" PRIdPTR " ", pop(f));
}

// Unimplemented: ." /MOD
NATIVE(divide, "/") {
  cell b = pop(f);
  cell a = pop(f);
  push(f, a / b);
}

// Unimplemented: 0< 0= 1+ 1- 2! 2* 2/ 2@ 2DROP 2DUP 2OVER 2SWAP

word* latest_definition;

NATIVE(colon, ":") {
  // Parse a name. Define (with malloc?) a new word record, and add it to the
  // dictionary chain. Enter compiling mode.
  string* s = parse_word(f);
  word *w = (word*) malloc(sizeof(word));
  latest_definition = w;

  w->link = f->dictionary;
  // Deliberately not adding this word to the dictionary, yet.
  w->native = false;
  w->immediate = false;
  w->name = s->value;
  w->nameLen = s->length;
  w->code.words = (word**) (f->here);

  f->state = COMPILING;
}

// Executed at the end of a colon-definition.
// Returns to interpretation mode, and handles the return stack.
NATIVE(exitcolon, "(EXITCOLON)") {
  f->state = INTERPRETING;
  f->nextWord = popR(f);
}

// Immediate word.
NATIVE(semicolon, ";") {
  // Compile one last word.
  write_cell(f, (cell) &word_exitcolon);
  // That's the end of this word's code. Now add it back to the dictionary.
  // If the link pointer is null, this was a :NONAME, so we skip it.
  if (latest_definition->link != NULL) {
    f->dictionary = latest_definition;
  }
  latest_definition = NULL;
}



NATIVE(lessthan, "<") {
  cell b = pop(f);
  cell a = pop(f);
  push(f, a < b ? -1 : 0);
}

// Unimplemented: <# >

NATIVE(equals, "=") {
  push(f, pop(f) == pop(f) ? -1 : 0);
}

// CREATE writes LIT, &data, EXIT EXIT, (data points here)
// And DOES> can overwrite the EXITs with a jump to its code.
NATIVE(to_body, ">BODY") {
  word* xt = (word*) pop(f);
  word** ws = xt->code.words;
  push(f, (cell) ws[1]);
}

NATIVE(inptr, ">IN") {
  push(f, (cell) &(f->inputStart[f->inputIndex]));
}

void to_number_(cell &len, char* &addr, cell &hi, cell &lo) {
  while (len > 0) {
    char c = *addr;
    // Convert blindly as if in base 36, then double-check we're below our base.
    int digit = 1000;
    if ('0' <= c && c <= '9') digit = (int) (c - '0');
    if ('a' <= c && c <= 'z') digit = (int) (c - 'a' + 10);
    if ('A' <= c && c <= 'Z') digit = (int) (c - 'A' + 10);

    if (digit < f->base) {
      // Consume it.
      lo *= f->base;
      lo += digit;
      len--;
      addr++;
    } else {
      break;
    }
  }
}

// Ignoring the upper cell.
NATIVE(to_number, ">NUMBER") {
  cell len = pop(f);
  char* addr = (char*) pop(f);

  cell hi = pop(f);
  cell lo = pop(f);

  to_number_(len, addr, hi, lo);

  push(f, lo);
  push(f, hi);
  push(f, (cell) addr);
  push(f, len);
}


NATIVE(to_r, ">R") {
  pushR(f, (word**) pop(f));
}

NATIVE(from_r, "R>") {
  push(f, (cell) popR(f));
}

NATIVE(qdup, "?DUP") {
  cell value = pop(f);
  if (value != 0) push(f, value);
  push(f, value);
}

NATIVE(fetch, "@") {
  cell* addr = (cell*) pop(f);
  push(f, *addr);
}

NATIVE(abort, "ABORT") {
  f->sp = 0;
  f->rsp = 0;
  // TODO Call QUIT from here.
}


NATIVE(abs, "ABS") {
  push(f, imaxabs(pop(f)));
}

// Unimplemented: ACCEPT
NATIVE(align, "ALIGN") {
  unsigned int mask = sizeof(cell) - 1;
  f->here = (~mask) & (f->here + mask);
}

NATIVE(aligned, "ALIGNED") {
  cell c = pop(f);
  cell mask = sizeof(cell) - 1;
  c = (~mask) & (c + mask);
  push(f, c);
}

// Unimplemented: ALLOT
NATIVE(and, "AND") {
  push(f, pop(f) & pop(f));
}

NATIVE(base, "BASE") {
  push(f, & (f->base));
}

NATIVE(branch, "(BRANCH)") {
  // Read the offset from the nextWord pointer, and jump to it.
  // The offset is in CELLS, NOT BYTES.
  f->nextWord += *(f->nextWord);
}

NATIVE(zbranch, "(0BRANCH)") {
  // When the popped value is 0, branch. Otherwise, skip the offset.
  cell c = pop(f);
  if (c == 0) code_branch(f);
  else f->nextWord++;
}

// Unimplemented: BEGIN, BL

NATIVE(cstore, "C!") {
  char* addr = (char*) pop(f);
  *addr = (char) pop(f);
}

NATIVE(ccomma, "C,") {
  write_byte(f, (unsigned char) pop(f));
}

NATIVE(cfetch, "C@") {
  char* addr = (char*) pop(f);
  push(f, (cell) *addr);
}

// Unimplemented: CELL+ CHAR CHAR+ CHARS CONSTANT COUNT
NATIVE(cells, "CELLS") {
  push(f, sizeof(cell) * pop(f));
}

NATIVE(cr, "CR") {
  printf("\n");
}

// Unimplemented: CREATE DECIMAL DO DOES>
NATIVE(depth, "DEPTH") {
  push(f, (cell) f->sp);
}

NATIVE(drop, "DROP") {
  (void) pop(f);
}
NATIVE(dup, "DUP") {
  cell c = pop(f);
  push(f, c);
  push(f, c);
}


// Unimplemented: ELSE ENVIRONMENT?
NATIVE(emit, "EMIT") {
  printf("%c", (unsigned char) pop(f));
}

// TODO: EVALUATE

void execute_(fstate* f, word* w) {
  if (w->native) {
    w->code.raw(f);
  } else {
    pushR(f, f->nextWord);
    f->nextWord = w->code.words;
  }
}

NATIVE(execute, "EXECUTE") {
  execute_(f, (word*) pop(f));
}

void exit_(fstate* f) {
  f->nextWord = popR(f);
}

NATIVE(exit, "EXIT") {
  exit_(f);
}

// Unimplemented: FILL
NATIVE(find, "FIND") {
  char* addr = (char*) pop(f);
  string s;
  s.length = (unsigned int) *addr;
  s.value = addr + 1;
  word* w = find_word(f, &s);

  if (w == NULL) {
    push(f, addr);
    push(f, 0);
  } else {
    push(f, (cell) w);
    push(f, w->immediate ? 1 : -1);
  }
}

// Unimplemented: FM/MOD HOLD I IF

// TODO: Double-check semantics of HERE. It's singly-indirect with this code.
NATIVE(here, "HERE") {
  push(f, (cell) f->here);
}

NATIVE(immediate, "IMMEDIATE") {
  f->dictionary->immediate = true;
}

NATIVE(invert, "INVERT") {
  push(f, ~pop(f));
}

// Unimplemented: J KEY LEAVE

NATIVE(lit, "(LIT)") {
  push(f, (cell) *(f->nextWord++));
}

// IMMEDIATE word - compiles (LIT) from above and then the number.
NATIVE(literal, "LITERAL") {
  write_cell(f, (cell) &word_lit);
  write_cell(f, pop(f));
}

// Unimplemented: LOOP M* MAX MIN

NATIVE(lshift, "LSHIFT") {
  cell by = pop(f);
  cell n = pop(f);
  push(f, n << by);
}

NATIVE(mod, "MOD") {
  cell divisor = pop(f);
  cell dividend = pop(f);
  push(f, dividend % divisor);
}

// Unimplemented: MOVE NEGATE

NATIVE(or, "OR") {
  push(f, pop(f) | pop(f));
}

NATIVE(over, "OVER") {
  push(f, f->stack[f->sp - 2]);
}

// IMMEDIATE
NATIVE(postpone, "POSTPONE") {
  word* w = read_word(f);
  write_cell(f, (cell) w);
}


NATIVE(quit, "QUIT") {
  // Main interpreting loop: empty the stacks, then read a line, interpret that
  // line, etc. etc.
  // Calls into run, which will return back here if it has a null next-word.
  f->rsp = 0;
  f->sp = 0;
  f->inputSource = 0; // keyboard
  f->state = INTERPRETING;

  refill_buffer(f);

  while (true) {
    string *s = NULL;
    while (s == NULL) {
      s = parse_word(f);
      if (s->length == 0) {
        free(s);
        s = NULL;
        refill_buffer(f);
      }
    }

    word* words[2];
    words[0] = find_word(f, s);

    // If we're interpreting or the word is immediate, execute it.
    // If we're compiling, compile it.
    // If it's a number, we either compile LIT n or push the number.
    if (words[0] != NULL) { // Found a word, so execute it.
      if (f->state == INTERPRETING || words[0]->immediate) {
        words[1] = &word_exit;
        pushR(f, NULL);
        f->nextWord = words;
        run(f);
      } else {
        write_cell(f, (cell) words[0]);
      }
    } else {
      // Attempt to parse as a number.
      cell len = s->length;
      char* addr = s->value;
      cell lo = 0;
      cell hi = 0;
      to_number_(len, addr, hi, lo);
      // If len == 0 now, parsing successful, otherwise error.
      if (len != 0) {
        char* errnum = (char*) malloc(s->length + 1);
        strncpy(errnum, s->value, s->length);
        errnum[s->length] = '\0';
        fprintf(stderr, "ERROR: Failed to parse number: %s", errnum);
      } else {
        if (f->state == INTERPRETING) {
          push(f, lo);
        } else {
          write_cell(f, &word_lit);
          write_cell(f, lo);
        }
      }
    }
  }
}


NATIVE(rfetch, "R@") {
  push(f, (cell) f->returnStack[f->rsp - 1]);
}

NATIVE(recurse, "RECURSE") {
  // Should work even for :NONAME
  write_cell(f, (cell) latest_definition);
}

// Unimplemented: REPEAT
NATIVE(rot, "ROT") {
  cell c = pop(f);
  cell b = pop(f);
  cell a = pop(f);
  push(f, b);
  push(f, c);
  push(f, a);
}

// TODO: Test this. I don't trust it.
NATIVE(rshift, "RSHIFT") {
  cell amount = pop(f);
  ucell value = (ucell) pop(f);
  value = value >> amount;
  push(f, (cell) value);
}

// Internal word, equivalent of LIT for strings.
// Pushes the string, and skips over the string values.
NATIVE(litstring, "(LITSTRING)") {
  unsigned char* str = (unsigned char*) f->nextWord;
  unsigned char len = *str;
  str++;
  push(f, (cell) str);
  push(f, (cell) len);

  // Bump and realign.
  f->nextWord = (f->nextWord + len + sizeof(cell) - 1) & ~(sizeof(cell) - 1);
}

// IMMEDIATE
NATIVE(squote, "S\"") {
  write_cell(f, (cell) &word_litstring);
  string* s = parse(f, '"');
  write_byte(f, (char) s->length);
  for (int i = 0; i < s->length; i++) {
    write_byte(f, s->value[i]);
  }

  f->here = (f->here + s->length + sizeof(cell) - 1) & !(sizeof(cell) - 1);
}

// Unimplemented: S>D SIGN SM/REM

NATIVE(source, "SOURCE") {
  push(f, (cell) f->inputStart);
  push(f, (cell) f->parseLength);
}

// Unimplemented: SPACE SPACES

NATIVE(state, "STATE") {
  push(f, (cell) &(f->state));
}

NATIVE(swap, "SWAP") {
  cell b = pop(f);
  cell a = pop(f);
  push(f, b);
  push(f, a);
}

// Unimplemented: THEN TYPE

NATIVE(udot, "U.") {
  printf("%" PRIuPTR " ", (ucell) pop(f));
}

NATIVE(ulessthan, "U<") {
  ucell b = (ucell) pop(f);
  ucell a = (ucell) pop(f);
  push(f, a < b ? -1 : 0);
}

// Unimplemented: UM* UM/MOD UNLOOP UNTIL VARIABLE WHILE
NATIVE(word, "WORD") {
  char delim = (char) pop(f);
  string* s = parse(f, delim);
  char* str = (char*) f->here;
  *str = (char) s->length;
  str++;
  strncpy(str, s->value, s->length);
  free(s);
  // Transient region, so don't move HERE.
  push(f, (cell) str);
}

NATIVE(xor, "XOR") {
  push(f, pop(f) ^ pop(f));
}

// IMMEDIATE
NATIVE(lbrac, "[") {
  f->state = INTERPRETING;
}

NATIVE(rbrac, "]") {
  f->state = COMPILING;
}

// Unimplemented: ['] [CHAR]


void run(fstate* f) {
  // Basic sequence: get the next word, check if it's native.
  // If native, execute it and move on.
  // If not native, push RSP and jump into it.
  while (f->nextWord != NULL) {
    word* next = *(f->nextWord);
    f->nextWord++;
    execute_(f, next);
  }
}


int main(char** argv, int argc) {
  NATIVE_SPEC(store, "!");
  NATIVE_SPEC(tick, "'");
  NATIVE_SPEC(minus, "-");
  NATIVE_SPEC(times, "*");
  NATIVE_SPEC(plus, "+");
  NATIVE_SPEC(comma, ",");
  NATIVE_SPEC(dot, ".");
  NATIVE_SPEC(divide, "/");
  NATIVE_SPEC(colon, ":");
  NATIVE_SPEC(exitcolon, "(exitcolon)");
  NATIVE_SPEC(semicolon, ";"); word_semicolon.immediate = true;
  NATIVE_SPEC(lessthan, "<");
  NATIVE_SPEC(equals, "=");
  NATIVE_SPEC(to_body, ">BODY");
  NATIVE_SPEC(inptr, ">IN");
  NATIVE_SPEC(to_number, ">NUMBER");
  NATIVE_SPEC(to_r, ">R");
  NATIVE_SPEC(from_r, "R>");
  NATIVE_SPEC(qdup, "?DUP");
  NATIVE_SPEC(fetch, "@");
  NATIVE_SPEC(abort, "ABORT");
  NATIVE_SPEC(abs, "ABS");
  NATIVE_SPEC(align, "ALIGN");
  NATIVE_SPEC(aligned, "ALIGNED");
  NATIVE_SPEC(and, "AND");
  NATIVE_SPEC(base, "BASE");
  NATIVE_SPEC(branch, "(BRANCH)");
  NATIVE_SPEC(zbranch, "(0BRANCH)");
  NATIVE_SPEC(cstore, "C!");
  NATIVE_SPEC(ccomma, "C,");
  NATIVE_SPEC(cfetch, "C@");
  NATIVE_SPEC(cells, "CELLS");
  NATIVE_SPEC(cr, "CR");
  NATIVE_SPEC(depth, "DEPTH");
  NATIVE_SPEC(drop, "DROP");
  NATIVE_SPEC(dup, "DUP");
  NATIVE_SPEC(emit, "EMIT");
  NATIVE_SPEC(execute, "EXECUTE");
  NATIVE_SPEC(exit, "EXIT");
  NATIVE_SPEC(find, "FIND");
  NATIVE_SPEC(here, "HERE");
  NATIVE_SPEC(immediate, "IMMEDIATE");
  NATIVE_SPEC(invert, "INVERT");
  NATIVE_SPEC(lit, "(LIT)");
  NATIVE_SPEC(literal, "LITERAL"); word_literal.immediate = true;
  NATIVE_SPEC(lshift, "LSHIFT");
  NATIVE_SPEC(mod, "MOD");
  NATIVE_SPEC(or, "OR");
  NATIVE_SPEC(over, "OVER");
  NATIVE_SPEC(postpone, "POSTPONE"); word_postpone.immediate = true;
  NATIVE_SPEC(quit, "QUIT");
  NATIVE_SPEC(rfetch, "R@");
  NATIVE_SPEC(recurse, "RECURSE"); word_recurse.immediate = true;
  NATIVE_SPEC(rot, "ROT");
  NATIVE_SPEC(rshift, "RSHIFT");
  NATIVE_SPEC(litstring, "(LITSTRING)");
  NATIVE_SPEC(squote, "S\""); word_squote.immediate = true;
  NATIVE_SPEC(source, "SOURCE");
  NATIVE_SPEC(state, "STATE");
  NATIVE_SPEC(swap, "SWAP");
  NATIVE_SPEC(udot, "U.");
  NATIVE_SPEC(ulessthan, "U<");
  NATIVE_SPEC(word, "WORD");
  NATIVE_SPEC(xor, "XOR");
  NATIVE_SPEC(lbrac, "["); word_lbrac.immediate = true;
  NATIVE_SPEC(rbrac, "]");

  fstate f;
  f.here = f.space;
  f.dictionary = latest_definition;
  f.rsp = 0;
  f.sp = 0;
  f.nextWord = NULL;
  f.state = INTERPRETING;
  f.base = 10;
  f.inputSource = 0;

  code_quit(f);
}
