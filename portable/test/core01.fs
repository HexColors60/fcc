\ Basic assumptions: numeric representations

T{ -> }T ( Start with a clean slate )
( Test if any bits are set; Answer in base 1 )
T{ : BITSSET? IF 0 0 ELSE 0 THEN ; -> }T
T{  0 BITSSET? -> 0 }T ( Zero is all bits clear )
T{  1 BITSSET? -> 0 0 }T ( Other numbers have at least one bit )
T{ -1 BITSSET? -> 0 0 }T ( Other numbers have at least one bit )

\ Helpers
 0 CONSTANT 0S
-1 CONSTANT 1S

HEX

\ Booleans
\ AND
T{ 0 0 and -> 0 }T
T{ 0 1 and -> 0 }T
T{ 1 0 and -> 0 }T
T{ 1 1 and -> 1 }T

T{ 0 invert 1 and -> 1 }T
T{ 1 invert 1 and -> 0 }T

T{ 0S 0S AND -> 0S }T
T{ 0S 1S AND -> 0S }T
T{ 1S 0S AND -> 0S }T
T{ 1S 1S AND -> 1S }T

\ INVERT
T{ 0S invert -> 1S }T
T{ 1S invert -> 0S }T

\ OR
T{ 0S 0S or -> 0S }T
T{ 0S 1S or -> 1S }T
T{ 1S 0S or -> 1S }T
T{ 1S 1S or -> 1S }T

\ XOR
T{ 0S 0S xor -> 0S }T
T{ 0S 1S xor -> 1S }T
T{ 1S 0S xor -> 1S }T
T{ 1S 1S xor -> 0S }T


\ Shifts
1S 1 rshift invert CONSTANT MSB
T{ msb bitsset? -> 0 0 }T

\ 2*
T{   0S 2*       ->   0S }T
T{    1 2*       ->    2 }T
T{ 4000 2*       -> 8000 }T
T{   1S 2* 1 xor ->   1S }T
T{  MSB 2*       ->   0S }T

\ 2/
T{          0S 2/ ->   0S }T
T{           1 2/ ->    0 }T
T{        4000 2/ -> 2000 }T
T{          1S 2/ ->   1S }T \ MSB propagated
T{    1S 1 XOR 2/ ->   1S }T
T{ MSB 2/ MSB AND ->  MSB }T

\ RSHIFT
T{    1 0 rshift -> 1 }T
T{    1 1 rshift -> 0 }T
T{    2 1 rshift -> 1 }T
T{    4 2 rshift -> 1 }T
T{ 8000 F rshift -> 1 }T \ biggest 16-bit shift TODO maybe convert to 32-bit?
T{  MSB 1 rshift MSB and -> 0 }T \ rshift zero-fills MSBs.
T{  MSB 1 rshift     2*  -> MSB }T

\ LSHIFT
T{    1 0 lshift ->    1 }T
T{    1 1 lshift ->    2 }T
T{    1 2 lshift ->    4 }T
T{    1 F lshift -> 8000 }T \ biggest 16-bit shift TODO maybe convert to 32-bit?
T{   1S 1 lshift 1 xor -> 1S }T
T{  MSB 1 lshift ->    0 }T


\ Numeric representation
\ NB: Commenting out the tests for the trailing-dot double number notation.
\ That seems to be common but nonstandard. There's no mention in 3.4.1.3.
\ Instead, this assumes 32-bit minimums.
DECIMAL
T{ #1289       -> 1289       }T
T{ #-1289      -> -1289      }T
T{ $12eF       -> 4847       }T
T{ $12aBcDeF   -> 313249263  }T
T{ $-12eF      -> -4847      }T
T{ $-12aBcDeF  -> -313249263 }T
T{ %10010110   -> 150        }T
T{ %-10010110  -> -150       }T
T{ 'z'         -> 122        }T
HEX


\ Comparisons
0 invert CONSTANT MAX-UINT
0 invert 1 rshift CONSTANT MAX-INT
0 invert 1 rshift invert CONSTANT MIN-INT
0 invert 1 rshift CONSTANT MID-UINT
0 invert 1 rshift invert CONSTANT MID-UINT+1

0S CONSTANT <FALSE>
1S CONSTANT <TRUE>

\ 0=
T{  0       0= -> <TRUE>  }T
T{  1       0= -> <FALSE> }T
T{  2       0= -> <FALSE> }T
T{ -1       0= -> <FALSE> }T
T{ MAX-UINT 0= -> <FALSE> }T
T{ MIN-INT  0= -> <FALSE> }T
T{ MAX-INT  0= -> <FALSE> }T

\ =
T{  0  0 = -> <TRUE>  }T
T{  1  1 = -> <TRUE>  }T
T{ -1 -1 = -> <TRUE>  }T
T{  1  0 = -> <FALSE> }T
T{ -1  0 = -> <FALSE> }T
T{  0  1 = -> <FALSE> }T
T{  0 -1 = -> <FALSE> }T

\ 0<
T{       0 0< -> <FALSE> }T
T{      -1 0< -> <TRUE>  }T
T{ MIN-INT 0< -> <TRUE>  }T
T{       1 0< -> <FALSE> }T
T{ MAX-INT 0< -> <FALSE> }T

\ <
T{       0       1 < -> <TRUE>  }T
T{       1       2 < -> <TRUE>  }T
T{      -1       0 < -> <TRUE>  }T
T{      -1       1 < -> <TRUE>  }T
T{ MIN-INT       0 < -> <TRUE>  }T
T{ MIN-INT MAX-INT < -> <TRUE>  }T
T{       0 MAX-INT < -> <TRUE>  }T
T{       0       0 < -> <FALSE> }T
T{       1       1 < -> <FALSE> }T
T{       1       0 < -> <FALSE> }T
T{       2       1 < -> <FALSE> }T
T{       0      -1 < -> <FALSE> }T
T{       1      -1 < -> <FALSE> }T
T{       0 MIN-INT < -> <FALSE> }T
T{ MAX-INT MIN-INT < -> <FALSE> }T
T{ MAX-INT       0 < -> <FALSE> }T

\ >
T{       0       1 > -> <FALSE> }T
T{       1       2 > -> <FALSE> }T
T{      -1       0 > -> <FALSE> }T
T{      -1       1 > -> <FALSE> }T
T{ MIN-INT       0 > -> <FALSE> }T
T{ MIN-INT MAX-INT > -> <FALSE> }T
T{       0 MAX-INT > -> <FALSE> }T
T{       0       0 > -> <FALSE> }T
T{       1       1 > -> <FALSE> }T
T{       1       0 > -> <TRUE>  }T
T{       2       1 > -> <TRUE>  }T
T{       0      -1 > -> <TRUE>  }T
T{       1      -1 > -> <TRUE>  }T
T{       0 MIN-INT > -> <TRUE>  }T
T{ MAX-INT MIN-INT > -> <TRUE>  }T
T{ MAX-INT       0 > -> <TRUE>  }T

\ U<
T{        0        1 U< -> <TRUE>  }T
T{        1        2 U< -> <TRUE>  }T
T{        0 MID-UINT U< -> <TRUE>  }T
T{        0 MAX-UINT U< -> <TRUE>  }T
T{ MID-UINT MAX-UINT U< -> <TRUE>  }T
T{        0        0 U< -> <FALSE> }T
T{        1        1 U< -> <FALSE> }T
T{        1        0 U< -> <FALSE> }T
T{        2        1 U< -> <FALSE> }T
T{ MID-UINT        0 U< -> <FALSE> }T
T{ MAX-UINT        0 U< -> <FALSE> }T
T{ MAX-UINT MID-UINT U< -> <FALSE> }T

\ MIN
T{       0       1 MIN ->       0 }T
T{       1       2 MIN ->       1 }T
T{      -1       0 MIN ->      -1 }T
T{      -1       1 MIN ->      -1 }T
T{ MIN-INT       0 MIN -> MIN-INT }T
T{ MIN-INT MAX-INT MIN -> MIN-INT }T
T{       0 MAX-INT MIN ->       0 }T
T{       0       0 MIN ->       0 }T
T{       1       1 MIN ->       1 }T
T{       1       0 MIN ->       0 }T
T{       2       1 MIN ->       1 }T
T{       0      -1 MIN ->      -1 }T
T{       1      -1 MIN ->      -1 }T
T{       0 MIN-INT MIN -> MIN-INT }T
T{ MAX-INT MIN-INT MIN -> MIN-INT }T
T{ MAX-INT       0 MIN ->       0 }T

\ MAX
T{       0       1 MAX ->       1 }T
T{       1       2 MAX ->       2 }T
T{      -1       0 MAX ->       0 }T
T{      -1       1 MAX ->       1 }T
T{ MIN-INT       0 MAX ->       0 }T
T{ MIN-INT MAX-INT MAX -> MAX-INT }T
T{       0 MAX-INT MAX -> MAX-INT }T
T{       0       0 MAX ->       0 }T
T{       1       1 MAX ->       1 }T
T{       1       0 MAX ->       1 }T
T{       2       1 MAX ->       2 }T
T{       0      -1 MAX ->       0 }T
T{       1      -1 MAX ->       1 }T
T{       0 MIN-INT MAX ->       0 }T
T{ MAX-INT MIN-INT MAX -> MAX-INT }T
T{ MAX-INT       0 MAX -> MAX-INT }T


\ Stack Operations

\ DROP
T{ 1 2 DROP -> 1 }T
T{ 0   DROP ->   }T

\ DUP
T{ 1 DUP -> 1 1 }T

\ OVER
T{ 1 2 OVER -> 1 2 1 }T

\ ROT
T{ 1 2 3 ROT -> 2 3 1 }T

\ SWAP
T{ 1 2 SWAP -> 2 1 }T

\ 2DROP
T{ 1 2 2DROP -> }T

\ 2DUP
T{ 1 2 2DUP -> 1 2 1 2 }T

\ 2OVER
T{ 1 2 3 4 2OVER -> 1 2 3 4 1 2 }T

\ 2SWAP
T{ 1 2 3 4 2SWAP -> 3 4 1 2 }T

\ ?DUP
T{ -1 ?DUP -> -1 -1 }T
T{  0 ?DUP ->  0    }T
T{  1 ?DUP ->  1  1 }T

\ DEPTH
T{ 0 1 DEPTH -> 0 1 2 }T
T{   0 DEPTH -> 0 1   }T
T{     DEPTH -> 0     }T


\ Return stack

T{ : GR1 >R R> ; -> }T
T{ : GR2 >R R@ R> DROP ; -> }T
T{ 123 GR1 -> 123 }T
T{ 123 GR2 -> 123 }T
T{  1S GR1 ->  1S }T      ( Return stack holds cells )


\ Addition and subtraction

\ +
T{        0  5 + ->          5 }T
T{        5  0 + ->          5 }T
T{        0 -5 + ->         -5 }T
T{       -5  0 + ->         -5 }T
T{        1  2 + ->          3 }T
T{        1 -2 + ->         -1 }T
T{       -1  2 + ->          1 }T
T{       -1 -2 + ->         -3 }T
T{       -1  1 + ->          0 }T
T{ MID-UINT  1 + -> MID-UINT+1 }T

\ -
T{          0  5 - ->       -5 }T
T{          5  0 - ->        5 }T
T{          0 -5 - ->        5 }T
T{         -5  0 - ->       -5 }T
T{          1  2 - ->       -1 }T
T{          1 -2 - ->        3 }T
T{         -1  2 - ->       -3 }T
T{         -1 -2 - ->        1 }T
T{          0  1 - ->       -1 }T
T{ MID-UINT+1  1 - -> MID-UINT }T

\ 1+
T{        0 1+ ->          1 }T
T{       -1 1+ ->          0 }T
T{        1 1+ ->          2 }T
T{ MID-UINT 1+ -> MID-UINT+1 }T

\ 1-
T{          2 1- ->        1 }T
T{          1 1- ->        0 }T
T{          0 1- ->       -1 }T
T{ MID-UINT+1 1- -> MID-UINT }T

\ ABS
T{       0 ABS ->          0 }T
T{       1 ABS ->          1 }T
T{      -1 ABS ->          1 }T
T{ MIN-INT ABS -> MID-UINT+1 }T

\ NEGATE
T{  0 NEGATE ->  0 }T
T{  1 NEGATE -> -1 }T
T{ -1 NEGATE ->  1 }T
T{  2 NEGATE -> -2 }T
T{ -2 NEGATE ->  2 }T

\ S>D
T{       0 S>D ->       0  0 }T
T{       1 S>D ->       1  0 }T
T{       2 S>D ->       2  0 }T
T{      -1 S>D ->      -1 -1 }T
T{      -2 S>D ->      -2 -1 }T
T{ MIN-INT S>D -> MIN-INT -1 }T
T{ MAX-INT S>D -> MAX-INT  0 }T


\ Multiplication and division

\ *
T{  0  0 * ->  0 }T          \ TEST IDENTITIES
T{  0  1 * ->  0 }T
T{  1  0 * ->  0 }T
T{  1  2 * ->  2 }T
T{  2  1 * ->  2 }T
T{  3  3 * ->  9 }T
T{ -3  3 * -> -9 }T
T{  3 -3 * -> -9 }T
T{ -3 -3 * ->  9 }T
T{ MID-UINT+1 1 RSHIFT 2 *               -> MID-UINT+1 }T
T{ MID-UINT+1 2 RSHIFT 4 *               -> MID-UINT+1 }T
T{ MID-UINT+1 1 RSHIFT MID-UINT+1 OR 2 * -> MID-UINT+1 }T

\ UM*
T{ 0 0 UM* -> 0 0 }T
T{ 0 1 UM* -> 0 0 }T
T{ 1 0 UM* -> 0 0 }T
T{ 1 2 UM* -> 2 0 }T
T{ 2 1 UM* -> 2 0 }T
T{ 3 3 UM* -> 9 0 }T
T{ MID-UINT+1 1 RSHIFT 2 UM* ->  MID-UINT+1 0 }T
T{ MID-UINT+1          2 UM* ->           0 1 }T
T{ MID-UINT+1          4 UM* ->           0 2 }T
T{         1S          2 UM* -> 1S 1 LSHIFT 1 }T
T{   MAX-UINT   MAX-UINT UM* ->    1 1 INVERT }T

\ M*
T{       0       0 M* ->       0 S>D }T
T{       0       1 M* ->       0 S>D }T
T{       1       0 M* ->       0 S>D }T
T{       1       2 M* ->       2 S>D }T
T{       2       1 M* ->       2 S>D }T
T{       3       3 M* ->       9 S>D }T
T{      -3       3 M* ->      -9 S>D }T
T{       3      -3 M* ->      -9 S>D }T
T{      -3      -3 M* ->       9 S>D }T
T{       0 MIN-INT M* ->       0 S>D }T
T{       1 MIN-INT M* -> MIN-INT S>D }T
T{       2 MIN-INT M* ->       0 1S  }T
T{       0 MAX-INT M* ->       0 S>D }T
T{       1 MAX-INT M* -> MAX-INT S>D }T
T{       2 MAX-INT M* -> MAX-INT     1 LSHIFT 0 }T
T{ MIN-INT MIN-INT M* ->       0 MSB 1 RSHIFT   }T
T{ MAX-INT MIN-INT M* ->     MSB MSB 2/         }T
T{ MAX-INT MAX-INT M* ->       1 MSB 2/ INVERT  }T

\ UM/MOD
T{        0            0        1 UM/MOD -> 0        0 }T
T{        1            0        1 UM/MOD -> 0        1 }T
T{        1            0        2 UM/MOD -> 1        0 }T
T{        3            0        2 UM/MOD -> 1        1 }T
T{ MAX-UINT        2 UM*        2 UM/MOD -> 0 MAX-UINT }T
T{ MAX-UINT        2 UM* MAX-UINT UM/MOD -> 0        2 }T
T{ MAX-UINT MAX-UINT UM* MAX-UINT UM/MOD -> 0 MAX-UINT }T

\ FM/MOD
T{       0 S>D              1 FM/MOD ->  0       0 }T
T{       1 S>D              1 FM/MOD ->  0       1 }T
T{       2 S>D              1 FM/MOD ->  0       2 }T
T{      -1 S>D              1 FM/MOD ->  0      -1 }T
T{      -2 S>D              1 FM/MOD ->  0      -2 }T
T{       0 S>D             -1 FM/MOD ->  0       0 }T
T{       1 S>D             -1 FM/MOD ->  0      -1 }T
T{       2 S>D             -1 FM/MOD ->  0      -2 }T
T{      -1 S>D             -1 FM/MOD ->  0       1 }T
T{      -2 S>D             -1 FM/MOD ->  0       2 }T
T{       2 S>D              2 FM/MOD ->  0       1 }T
T{      -1 S>D             -1 FM/MOD ->  0       1 }T
T{      -2 S>D             -2 FM/MOD ->  0       1 }T
T{       7 S>D              3 FM/MOD ->  1       2 }T
T{       7 S>D             -3 FM/MOD -> -2      -3 }T
T{      -7 S>D              3 FM/MOD ->  2      -3 }T
T{      -7 S>D             -3 FM/MOD -> -1       2 }T
T{ MAX-INT S>D              1 FM/MOD ->  0 MAX-INT }T
T{ MIN-INT S>D              1 FM/MOD ->  0 MIN-INT }T
T{ MAX-INT S>D        MAX-INT FM/MOD ->  0       1 }T
T{ MIN-INT S>D        MIN-INT FM/MOD ->  0       1 }T
T{    1S 1                  4 FM/MOD ->  3 MAX-INT }T
T{       1 MIN-INT M*       1 FM/MOD ->  0 MIN-INT }T
T{       1 MIN-INT M* MIN-INT FM/MOD ->  0       1 }T
T{       2 MIN-INT M*       2 FM/MOD ->  0 MIN-INT }T
T{       2 MIN-INT M* MIN-INT FM/MOD ->  0       2 }T
T{       1 MAX-INT M*       1 FM/MOD ->  0 MAX-INT }T
T{       1 MAX-INT M* MAX-INT FM/MOD ->  0       1 }T
T{       2 MAX-INT M*       2 FM/MOD ->  0 MAX-INT }T
T{       2 MAX-INT M* MAX-INT FM/MOD ->  0       2 }T
T{ MIN-INT MIN-INT M* MIN-INT FM/MOD ->  0 MIN-INT }T
T{ MIN-INT MAX-INT M* MIN-INT FM/MOD ->  0 MAX-INT }T
T{ MIN-INT MAX-INT M* MAX-INT FM/MOD ->  0 MIN-INT }T
T{ MAX-INT MAX-INT M* MAX-INT FM/MOD ->  0 MAX-INT }T

\ SM/REM
T{       0 S>D              1 SM/REM ->  0       0 }T
T{       1 S>D              1 SM/REM ->  0       1 }T
T{       2 S>D              1 SM/REM ->  0       2 }T
T{      -1 S>D              1 SM/REM ->  0      -1 }T
T{      -2 S>D              1 SM/REM ->  0      -2 }T
T{       0 S>D             -1 SM/REM ->  0       0 }T
T{       1 S>D             -1 SM/REM ->  0      -1 }T
T{       2 S>D             -1 SM/REM ->  0      -2 }T
T{      -1 S>D             -1 SM/REM ->  0       1 }T
T{      -2 S>D             -1 SM/REM ->  0       2 }T
T{       2 S>D              2 SM/REM ->  0       1 }T
T{      -1 S>D             -1 SM/REM ->  0       1 }T
T{      -2 S>D             -2 SM/REM ->  0       1 }T
T{       7 S>D              3 SM/REM ->  1       2 }T
T{       7 S>D             -3 SM/REM ->  1      -2 }T
T{      -7 S>D              3 SM/REM -> -1      -2 }T
T{      -7 S>D             -3 SM/REM -> -1       2 }T
T{ MAX-INT S>D              1 SM/REM ->  0 MAX-INT }T
T{ MIN-INT S>D              1 SM/REM ->  0 MIN-INT }T
T{ MAX-INT S>D        MAX-INT SM/REM ->  0       1 }T
T{ MIN-INT S>D        MIN-INT SM/REM ->  0       1 }T
T{      1S 1                4 SM/REM ->  3 MAX-INT }T
T{       2 MIN-INT M*       2 SM/REM ->  0 MIN-INT }T
T{       2 MIN-INT M* MIN-INT SM/REM ->  0       2 }T
T{       2 MAX-INT M*       2 SM/REM ->  0 MAX-INT }T
T{       2 MAX-INT M* MAX-INT SM/REM ->  0       2 }T
T{ MIN-INT MIN-INT M* MIN-INT SM/REM ->  0 MIN-INT }T
T{ MIN-INT MAX-INT M* MIN-INT SM/REM ->  0 MAX-INT }T
T{ MIN-INT MAX-INT M* MAX-INT SM/REM ->  0 MIN-INT }T
T{ MAX-INT MAX-INT M* MAX-INT SM/REM ->  0 MAX-INT }T


\ NB: Below here, we're using the SM/REM, IFSYM versions of everything.
\ /MOD
: T/MOD >R S>D R> SM/REM ;
T{       0       1 /MOD ->       0       1 T/MOD }T
T{       1       1 /MOD ->       1       1 T/MOD }T
T{       2       1 /MOD ->       2       1 T/MOD }T
T{      -1       1 /MOD ->      -1       1 T/MOD }T
T{      -2       1 /MOD ->      -2       1 T/MOD }T
T{       0      -1 /MOD ->       0      -1 T/MOD }T
T{       1      -1 /MOD ->       1      -1 T/MOD }T
T{       2      -1 /MOD ->       2      -1 T/MOD }T
T{      -1      -1 /MOD ->      -1      -1 T/MOD }T
T{      -2      -1 /MOD ->      -2      -1 T/MOD }T
T{       2       2 /MOD ->       2       2 T/MOD }T
T{      -1      -1 /MOD ->      -1      -1 T/MOD }T
T{      -2      -2 /MOD ->      -2      -2 T/MOD }T
T{       7       3 /MOD ->       7       3 T/MOD }T
T{       7      -3 /MOD ->       7      -3 T/MOD }T
T{      -7       3 /MOD ->      -7       3 T/MOD }T
T{      -7      -3 /MOD ->      -7      -3 T/MOD }T
T{ MAX-INT       1 /MOD -> MAX-INT       1 T/MOD }T
T{ MIN-INT       1 /MOD -> MIN-INT       1 T/MOD }T
T{ MAX-INT MAX-INT /MOD -> MAX-INT MAX-INT T/MOD }T
T{ MIN-INT MIN-INT /MOD -> MIN-INT MIN-INT T/MOD }T

\ /
: T/ T/MOD SWAP DROP ;
T{       0       1 / ->       0       1 T/ }T
T{       1       1 / ->       1       1 T/ }T
T{       2       1 / ->       2       1 T/ }T
T{      -1       1 / ->      -1       1 T/ }T
T{      -2       1 / ->      -2       1 T/ }T
T{       0      -1 / ->       0      -1 T/ }T
T{       1      -1 / ->       1      -1 T/ }T
T{       2      -1 / ->       2      -1 T/ }T
T{      -1      -1 / ->      -1      -1 T/ }T
T{      -2      -1 / ->      -2      -1 T/ }T
T{       2       2 / ->       2       2 T/ }T
T{      -1      -1 / ->      -1      -1 T/ }T
T{      -2      -2 / ->      -2      -2 T/ }T
T{       7       3 / ->       7       3 T/ }T
T{       7      -3 / ->       7      -3 T/ }T
T{      -7       3 / ->      -7       3 T/ }T
T{      -7      -3 / ->      -7      -3 T/ }T
T{ MAX-INT       1 / -> MAX-INT       1 T/ }T
T{ MIN-INT       1 / -> MIN-INT       1 T/ }T
T{ MAX-INT MAX-INT / -> MAX-INT MAX-INT T/ }T
T{ MIN-INT MIN-INT / -> MIN-INT MIN-INT T/ }T

\ MOD
: TMOD T/MOD DROP ;
T{       0       1 MOD ->       0       1 TMOD }T
T{       1       1 MOD ->       1       1 TMOD }T
T{       2       1 MOD ->       2       1 TMOD }T
T{      -1       1 MOD ->      -1       1 TMOD }T
T{      -2       1 MOD ->      -2       1 TMOD }T
T{       0      -1 MOD ->       0      -1 TMOD }T
T{       1      -1 MOD ->       1      -1 TMOD }T
T{       2      -1 MOD ->       2      -1 TMOD }T
T{      -1      -1 MOD ->      -1      -1 TMOD }T
T{      -2      -1 MOD ->      -2      -1 TMOD }T
T{       2       2 MOD ->       2       2 TMOD }T
T{      -1      -1 MOD ->      -1      -1 TMOD }T
T{      -2      -2 MOD ->      -2      -2 TMOD }T
T{       7       3 MOD ->       7       3 TMOD }T
T{       7      -3 MOD ->       7      -3 TMOD }T
T{      -7       3 MOD ->      -7       3 TMOD }T
T{      -7      -3 MOD ->      -7      -3 TMOD }T
T{ MAX-INT       1 MOD -> MAX-INT       1 TMOD }T
T{ MIN-INT       1 MOD -> MIN-INT       1 TMOD }T
T{ MAX-INT MAX-INT MOD -> MAX-INT MAX-INT TMOD }T
T{ MIN-INT MIN-INT MOD -> MIN-INT MIN-INT TMOD }T

\ */MOD
: T*/MOD >R M* R> SM/REM ;
T{       0 2       1 */MOD ->       0 2       1 T*/MOD }T
T{       1 2       1 */MOD ->       1 2       1 T*/MOD }T
T{       2 2       1 */MOD ->       2 2       1 T*/MOD }T
T{      -1 2       1 */MOD ->      -1 2       1 T*/MOD }T
T{      -2 2       1 */MOD ->      -2 2       1 T*/MOD }T
T{       0 2      -1 */MOD ->       0 2      -1 T*/MOD }T
T{       1 2      -1 */MOD ->       1 2      -1 T*/MOD }T
T{       2 2      -1 */MOD ->       2 2      -1 T*/MOD }T
T{      -1 2      -1 */MOD ->      -1 2      -1 T*/MOD }T
T{      -2 2      -1 */MOD ->      -2 2      -1 T*/MOD }T
T{       2 2       2 */MOD ->       2 2       2 T*/MOD }T
T{      -1 2      -1 */MOD ->      -1 2      -1 T*/MOD }T
T{      -2 2      -2 */MOD ->      -2 2      -2 T*/MOD }T
T{       7 2       3 */MOD ->       7 2       3 T*/MOD }T
T{       7 2      -3 */MOD ->       7 2      -3 T*/MOD }T
T{      -7 2       3 */MOD ->      -7 2       3 T*/MOD }T
T{      -7 2      -3 */MOD ->      -7 2      -3 T*/MOD }T
T{ MAX-INT 2 MAX-INT */MOD -> MAX-INT 2 MAX-INT T*/MOD }T
T{ MIN-INT 2 MIN-INT */MOD -> MIN-INT 2 MIN-INT T*/MOD }T


\ */
: T*/ T*/MOD SWAP DROP ;
T{       0 2       1 */ ->       0 2       1 T*/ }T
T{       1 2       1 */ ->       1 2       1 T*/ }T
T{       2 2       1 */ ->       2 2       1 T*/ }T
T{      -1 2       1 */ ->      -1 2       1 T*/ }T
T{      -2 2       1 */ ->      -2 2       1 T*/ }T
T{       0 2      -1 */ ->       0 2      -1 T*/ }T
T{       1 2      -1 */ ->       1 2      -1 T*/ }T
T{       2 2      -1 */ ->       2 2      -1 T*/ }T
T{      -1 2      -1 */ ->      -1 2      -1 T*/ }T
T{      -2 2      -1 */ ->      -2 2      -1 T*/ }T
T{       2 2       2 */ ->       2 2       2 T*/ }T
T{      -1 2      -1 */ ->      -1 2      -1 T*/ }T
T{      -2 2      -2 */ ->      -2 2      -2 T*/ }T
T{       7 2       3 */ ->       7 2       3 T*/ }T
T{       7 2      -3 */ ->       7 2      -3 T*/ }T
T{      -7 2       3 */ ->      -7 2       3 T*/ }T
T{      -7 2      -3 */ ->      -7 2      -3 T*/ }T
T{ MAX-INT 2 MAX-INT */ -> MAX-INT 2 MAX-INT T*/ }T
T{ MIN-INT 2 MIN-INT */ -> MIN-INT 2 MIN-INT T*/ }T


\ Memory
\ , CONSTANT HERE @ ! CELL+ CELLS 2@ 2!
HERE 1 ,
HERE 2 ,
CONSTANT 2ND
CONSTANT 1ST
T{       1ST 2ND U< -> <TRUE> }T \ HERE MUST GROW WITH ALLOT
T{       1ST CELL+  -> 2ND }T \ ... BY ONE CELL
T{   1ST 1 CELLS +  -> 2ND }T
T{     1ST @ 2ND @  -> 1 2 }T
T{         5 1ST !  ->     }T
T{     1ST @ 2ND @  -> 5 2 }T
T{         6 2ND !  ->     }T
T{     1ST @ 2ND @  -> 5 6 }T
T{           1ST 2@ -> 6 5 }T
T{       2 1 1ST 2! ->     }T
T{           1ST 2@ -> 2 1 }T
T{ 1S 1ST !  1ST @  -> 1S  }T    \ CAN STORE CELL-WIDE VALUE

\ +!
T{  0 1ST !        ->   }T
T{  1 1ST +!       ->   }T
T{    1ST @        -> 1 }T
T{ -1 1ST +! 1ST @ -> 0 }T

: BITS ( X -- U )
   0 SWAP BEGIN DUP WHILE
     DUP MSB AND IF >R 1+ R> THEN 2*
   REPEAT DROP ;
( CELLS >= 1 AU, INTEGRAL MULTIPLE OF CHAR SIZE, >= 16 BITS )
T{ 1 CELLS 1 <         -> <FALSE> }T
T{ 1 CELLS 1 CHARS MOD ->    0    }T
T{ 1S BITS 10 <        -> <FALSE> }T

\ C, C@ C!
HERE 1 C,
HERE 2 C,
CONSTANT 2NDC
CONSTANT 1STC
T{    1STC 2NDC U< -> <TRUE> }T \ HERE MUST GROW WITH ALLOT
T{      1STC CHAR+ ->  2NDC  }T \ ... BY ONE CHAR
T{  1STC 1 CHARS + ->  2NDC  }T
T{ 1STC C@ 2NDC C@ ->   1 2  }T
T{       3 1STC C! ->        }T
T{ 1STC C@ 2NDC C@ ->   3 2  }T
T{       4 2NDC C! ->        }T
T{ 1STC C@ 2NDC C@ ->   3 4  }T

\ CHARS
( CHARACTERS >= 1 AU, <= SIZE OF CELL, >= 8 BITS )
T{ 1 CHARS 1 <       -> <FALSE> }T
T{ 1 CHARS 1 CELLS > -> <FALSE> }T

\ ALIGN ALIGNED
ALIGN 1 ALLOT HERE ALIGN HERE 3 CELLS ALLOT
CONSTANT A-ADDR CONSTANT UA-ADDR
T{ UA-ADDR ALIGNED -> A-ADDR }T
T{       1 A-ADDR C!         A-ADDR       C@ ->       1 }T
T{    1234 A-ADDR !          A-ADDR       @  ->    1234 }T
T{ 123 456 A-ADDR 2!         A-ADDR       2@ -> 123 456 }T
T{       2 A-ADDR CHAR+ C!   A-ADDR CHAR+ C@ ->       2 }T
T{       3 A-ADDR CELL+ C!   A-ADDR CELL+ C@ ->       3 }T
T{    1234 A-ADDR CELL+ !    A-ADDR CELL+ @  ->    1234 }T
T{ 123 456 A-ADDR CELL+ 2!   A-ADDR CELL+ 2@ -> 123 456 }T

\ ALLOT
HERE 1 ALLOT
HERE
CONSTANT 2NDA
CONSTANT 1STA
T{ 1STA 2NDA U< -> <TRUE> }T    \ HERE MUST GROW WITH ALLOT
T{      1STA 1+ ->   2NDA }T    \ ... BY ONE ADDRESS UNIT
( MISSING TEST: NEGATIVE ALLOT )



\ Characters
\ BL
T{ BL -> 20 }T

\ CHAR
T{ CHAR X     -> 58 }T
T{ CHAR HELLO -> 48 }T

\ [CHAR]
T{ : GC1 [CHAR] X     ; -> }T
T{ : GC2 [CHAR] HELLO ; -> }T
T{ GC1 -> 58 }T
T{ GC2 -> 48 }T

\ [ ]
T{ : GC3 [ GC1 ] LITERAL ; -> }T
T{ GC3 -> 58 }T

\ S"
T{ : GC4 S" XY" ; ->   }T
T{ GC4 SWAP DROP  -> 2 }T
T{ GC4 DROP DUP C@ SWAP CHAR+ C@ -> 58 59 }T
: GC5 S" A String"2DROP ; \ There is no space between the " and 2DROP
T{ GC5 -> }T


\ Dictionary
\ '
T{ : GT1 123 ;   ->     }T
T{ ' GT1 EXECUTE -> 123 }T

\ [']
T{ : GT2 ['] GT1 ; IMMEDIATE -> }T
T{ GT2 EXECUTE -> 123 }T

\ FIND
HERE 3 C, CHAR G C, CHAR T C, CHAR 1 C, CONSTANT GT1STRING
HERE 3 C, CHAR G C, CHAR T C, CHAR 2 C, CONSTANT GT2STRING
T{ GT1STRING FIND -> ' GT1 -1 }T
T{ GT2STRING FIND -> ' GT2 1  }T
( HOW TO SEARCH FOR NON-EXISTENT WORD? )

\ LITERAL
T{ : GT3 GT2 LITERAL ; -> }T
T{ GT3 -> ' GT1 }T

\ COUNT
T{ GT1STRING COUNT -> GT1STRING CHAR+ 3 }T

\ POSTPONE
T{ : GT4 POSTPONE GT1 ; IMMEDIATE -> }T
T{ : GT5 GT4 ; -> }T
T{ GT5 -> 123 }T
T{ : GT6 345 ; IMMEDIATE -> }T
T{ : GT7 POSTPONE GT6 ; -> }T
T{ GT7 -> 345 }T

\ STATE
T{ : GT8 STATE @ ; IMMEDIATE -> }T
T{ GT8 -> 0 }T
T{ : GT9 GT8 LITERAL ; -> }T
T{ GT9 0= -> <FALSE> }T



\ Control flow

\ IF ELSE THEN
T{ : GI1 IF 123 THEN ; -> }T
T{ : GI2 IF 123 ELSE 234 THEN ; -> }T
T{  0 GI1 ->     }T
T{  1 GI1 -> 123 }T
T{ -1 GI1 -> 123 }T
T{  0 GI2 -> 234 }T
T{  1 GI2 -> 123 }T
T{ -1 GI1 -> 123 }T
\ Multiple ELSEs in an IF statement
: melse IF 1 ELSE 2 ELSE 3 ELSE 4 ELSE 5 THEN ;
T{ <FALSE> melse -> 2 4 }T
T{ <TRUE>  melse -> 1 3 5 }T

\ BEGIN WHILE REPEAT
T{ : GI3 BEGIN DUP 5 < WHILE DUP 1+ REPEAT ; -> }T
T{ 0 GI3 -> 0 1 2 3 4 5 }T
T{ 4 GI3 -> 4 5 }T
T{ 5 GI3 -> 5 }T
T{ 6 GI3 -> 6 }T
T{ : GI5 BEGIN DUP 2 > WHILE
      DUP 5 < WHILE DUP 1+ REPEAT
      123 ELSE 345 THEN ; -> }T
bye
T{ 1 GI5 -> 1 345 }T
bye
T{ 2 GI5 -> 2 345 }T
T{ 3 GI5 -> 3 4 5 123 }T
T{ 4 GI5 -> 4 5 123 }T
T{ 5 GI5 -> 5 123 }T



