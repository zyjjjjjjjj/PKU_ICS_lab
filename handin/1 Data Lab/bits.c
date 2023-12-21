/* 
 * CS:APP Data Lab 
 * 
 * <Please put your name and userid here>
 * 
 * bits.c - Source file with your solutions to the Lab.
 *          This is the file you will hand in to your instructor.
 *
 * WARNING: Do not include the <stdio.h> header; it confuses the dlc
 * compiler. You can still use printf for debugging without including
 * <stdio.h>, although you might get a compiler warning. In general,
 * it's not good practice to ignore compiler warnings, but in this
 * case it's OK.  
 */

#if 0
/*
 * Instructions to Students:
 *
 * STEP 1: Read the following instructions carefully.
 */

You will provide your solution to the Data Lab by
editing the collection of functions in this source file.

INTEGER CODING RULES:
 
  Replace the "return" statement in each function with one
  or more lines of C code that implements the function. Your code 
  must conform to the following style:
 
  int Funct(arg1, arg2, ...) {
      /* brief description of how your implementation works */
      int var1 = Expr1;
      ...
      int varM = ExprM;

      varJ = ExprJ;
      ...
      varN = ExprN;
      return ExprR;
  }

  Each "Expr" is an expression using ONLY the following:
  1. Integer constants 0 through 255 (0xFF), inclusive. You are
      not allowed to use big constants such as 0xffffffff.
  2. Function arguments and local variables (no global variables).
  3. Unary integer operations ! ~
  4. Binary integer operations & ^ | + << >>
    
  Some of the problems restrict the set of allowed operators even further.
  Each "Expr" may consist of multiple operators. You are not restricted to
  one operator per line.

  You are expressly forbidden to:
  1. Use any control constructs such as if, do, while, for, switch, etc.
  2. Define or use any macros.
  3. Define any additional functions in this file.
  4. Call any functions.
  5. Use any other operations, such as &&, ||, -, or ?:
  6. Use any form of casting.
  7. Use any data type other than int.  This implies that you
     cannot use arrays, structs, or unions.

 
  You may assume that your machine:
  1. Uses 2s complement, 32-bit representations of integers.
  2. Performs right shifts arithmetically.
  3. Has unpredictable behavior when shifting an integer by more
     than the word size.

EXAMPLES OF ACCEPTABLE CODING STYLE:
  /*
   * pow2plus1 - returns 2^x + 1, where 0 <= x <= 31
   */
  int pow2plus1(int x) {
     /* exploit ability of shifts to compute powers of 2 */
     return (1 << x) + 1;
  }

  /*
   * pow2plus4 - returns 2^x + 4, where 0 <= x <= 31
   */
  int pow2plus4(int x) {
     /* exploit ability of shifts to compute powers of 2 */
     int result = (1 << x);
     result += 4;
     return result;
  }

FLOATING POINT CODING RULES

For the problems that require you to implent floating-point operations,
the coding rules are less strict.  You are allowed to use looping and
conditional control.  You are allowed to use both ints and unsigneds.
You can use arbitrary integer and unsigned constants.

You are expressly forbidden to:
  1. Define or use any macros.
  2. Define any additional functions in this file.
  3. Call any functions.
  4. Use any form of casting.
  5. Use any data type other than int or unsigned.  This means that you
     cannot use arrays, structs, or unions.
  6. Use any floating point data types, operations, or constants.


NOTES:
  1. Use the dlc (data lab checker) compiler (described in the handout) to 
     check the legality of your solutions.
  2. Each function has a maximum number of operators (! ~ & ^ | + << >>)
     that you are allowed to use for your implementation of the function. 
     The max operator count is checked by dlc. Note that '=' is not 
     counted; you may use as many of these as you want without penalty.
  3. Use the btest test harness to check your functions for correctness.
  4. Use the BDD checker to formally verify your functions
  5. The maximum number of ops for each function is given in the
     header comment for each function. If there are any inconsistencies 
     between the maximum ops in the writeup and in this file, consider
     this file the authoritative source.

/*
 * STEP 2: Modify the following functions according the coding rules.
 * 
 *   IMPORTANT. TO AVOID GRADING SURPRISES:
 *   1. Use the dlc compiler to check that your solutions conform
 *      to the coding rules.
 *   2. Use the BDD checker to formally verify that your solutions produce 
 *      the correct answers.
 */


#endif
/* Copyright (C) 1991-2022 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <https://www.gnu.org/licenses/>.  */
/* This header is separate from features.h so that the compiler can
   include it implicitly at the start of every compilation.  It must
   not itself include <features.h> or any other header that includes
   <features.h> because the implicit include comes before any feature
   test macros that may be defined in a source file before it first
   explicitly includes a system header.  GCC knows the name of this
   header in order to preinclude it.  */
/* glibc's intent is to support the IEC 559 math functionality, real
   and complex.  If the GCC (4.9 and later) predefined macros
   specifying compiler intent are available, use them to determine
   whether the overall intent is to support these features; otherwise,
   presume an older compiler has intent to support these features and
   define these macros by default.  */
/* wchar_t uses Unicode 10.0.0.  Version 10.0 of the Unicode Standard is
   synchronized with ISO/IEC 10646:2017, fifth edition, plus
   the following additions from Amendment 1 to the fifth edition:
   - 56 emoji characters
   - 285 hentaigana
   - 3 additional Zanabazar Square characters */
/* 
 * bitXnor - ~(x^y) using only ~ and | 
 *   Example: bitXnor(6, -5) = 2
 *   Legal ops: ~ |
 *   Max ops: 7
 *   Rating: 1
 */
int bitXnor(int x, int y) {
  return ~(~x|~y)|~(x|y);    
}
/* 
 * bitConditional - x ? y : z for each bit respectively
 *   Example: bitConditional(0b00110011, 0b01010101, 0b00001111) = 0b00011101
 *   Legal ops: & | ^ ~
 *   Max ops: 4
 *   Rating: 1
 */
int bitConditional(int x, int y, int z) {
  return (x&y)|(~x&z);
}
/* 
 * byteSwap - swaps the nth byte and the mth byte
 *  Examples: byteSwap(0x12345678, 1, 3) = 0x56341278
 *            byteSwap(0xDEADBEEF, 0, 2) = 0xDEEFBEAD
 *  You may assume that 0 <= n <= 3, 0 <= m <= 3
 *  Legal ops: ! ~ & ^ | + << >>
 *  Max ops: 16
 *  Rating: 2
 */
int byteSwap(int x, int n, int m) {
  int nbit = n<<3;
  int mbit = m<<3;
  int nByte = ((x>>nbit)&0xff)<<mbit;
  int mByte = ((x>>mbit)&0xff)<<nbit;
  x = (~(0xff<<nbit))&x;
  x = (~(0xff<<mbit))&x;
  return x|nByte|mByte;
}
/* 
 * logicalShift - shift x to the right by n, using a logical shift
 *   Can assume that 0 <= n <= 31
 *   Examples: logicalShift(0x87654321,4) = 0x08765432
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 16
 *   Rating: 3
 */
int logicalShift(int x, int n) {
  int tmp;
  tmp = ~(((0x1<<31)>>n)<<1);
  x = x>>n;
  x = x&tmp;
  return x;
}
/* 
 * cleanConsecutive1 - change any consecutive 1 to zeros in the binary form of x.
 *   Consecutive 1 means a set of 1 that contains more than one 1.
 *   Examples cleanConsecutive1(0x10) = 0x10
 *            cleanConsecutive1(0xF0) = 0x0
 *            cleanConsecutive1(0xFFFF0001) = 0x1
 *            cleanConsecutive1(0x4F4F4F4F) = 0x40404040
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 16
 *   Rating: 4
 */
int cleanConsecutive1(int x){
  int tmp2 = (~(x>>1))|(0x1<<31);//右移一位取负，最高位置1，
  int tmp = ~(x<<1);//左移一位取负
  x = x&tmp;
  x = x&tmp2;
  return x;
}
/*
 * leftBitCount - returns count of number of consective 1's in
 *     left-hand (most significant) end of word.
 *   Examples: leftBitCount(-1) = 32, leftBitCount(0xFFF0F0F0) = 12
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 50
 *   Rating: 4
 */
int leftBitCount(int x) {
  int count = 0;
  int bit1 = 1&(!(~x));
  count += (!!(~(x>>16)))<<4;
  count += (!!(~(x>>(count+8))))<<3;
  count += (!!(~(x>>(count+4))))<<2;
  count += (!!(~(x>>(count+2))))<<1;
  count += (!!(~(x>>(count+1))));
  return 32 + ~count + bit1;
}
/* 
 * counter1To5 - return 1 + x if x < 5, return 1 otherwise, we ensure that 1<=x<=5
 *   Examples counter1To5(2) = 3,  counter1To5(5) = 1
 *   Legal ops: ~ & ! | + << >>
 *   Max ops: 15
 *   Rating: 2
 */
int counter1To5(int x) {
  x = (~(((!(x+(~0x5)+1))<<31)>>31))&x;
  x = x + 0x1;
  return x;
}
/* 
 * sameSign - return 1 if x and y have same sign, and 0 otherwise
 *   Examples sameSign(0x12345678, 0) = 1, sameSign(0xFFFFFFFF,0x1) = 0
 *   Legal ops: ! ~ & ! ^ | + << >>
 *   Max ops: 5
 *   Rating: 2
 */
int sameSign(int x, int y) {
  int r = !((x>>31)^(y>>31));
  return r;
}
/*
 * satMul3 - multiplies by 3, saturating to Tmin or Tmax if overflow
 *  Examples: satMul3(0x10000000) = 0x30000000
 *            satMul3(0x30000000) = 0x7FFFFFFF (Saturate to TMax)
 *            satMul3(0x70000000) = 0x7FFFFFFF (Saturate to TMax)
 *            satMul3(0xD0000000) = 0x80000000 (Saturate to TMin)
 *            satMul3(0xA0000000) = 0x80000000 (Saturate to TMin)
 *  Legal ops: ! ~ & ^ | + << >>
 *  Max ops: 25
 *  Rating: 3
 */
int satMul3(int x) {
  int x2 = x + x;
  int x3 = x2 + x;
  int sign1 = (x>>31);// - = ffffffff,+ = 00000000
  int sign2 = (x2>>31);
  int sign3 = (x3>>31);
  int of = ((sign1^sign3)|(sign1^sign2));//of = 0xffffffff,nof = 0x00000000
  int posOF = of&(~sign1)&(~(0x1<<31));//positive overflow
  int negOF = of&sign1&(0x1<<31);// negative overflow
  int r = (~of)&x3;
  return r + posOF + negOF;
}
/* 
 * isGreater - if x > y  then return 1, else return 0 
 *   Example: isGreater(4,5) = 0, isGreater(5,4) = 1
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 24
 *   Rating: 3
 */
int isGreater(int x, int y) {
  int sf = ((y + ~x + 1)>>31)&0x1;
  int signX = (x>>31)&0x1;
  int signY = (y>>31)&0x1;
  int of = (signY&(~signX)&(~sf))|((~signY)&signX&sf);
  return of^sf;
}
/* 
 * subOK - Determine if can compute x-y without overflow
 *   Example: subOK(0x80000000,0x80000000) = 1,
 *            subOK(0x80000000,0x70000000) = 0, 
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 20
 *   Rating: 3
 */
int subOK(int x, int y) {
  int signX = (x>>31)&0x1;
  int signY = (y>>31)&0x1;
  int subxy = x + ~y +1;
  int signSubxy = (subxy>>31)&0x1;
  return !((signX^signY)&(signX^signSubxy));
}
/*
 * trueFiveEighths - multiplies by 5/8 rounding toward 0,
 *  avoiding errors due to overflow
 *  Examples: trueFiveEighths(11) = 6
 *            trueFiveEighths(-9) = -5
 *            trueFiveEighths(0x30000000) = 0x1E000000 (no overflow)
 *  Legal ops: ! ~ & ^ | + << >>
 *  Max ops: 25
 *  Rating: 4
 */
int trueFiveEighths(int x)
{
  // 1/2 and 1/8
  int tmp1 = x>>3;
  int tmp2 = x>>1;
  // -1bit
  int f1 = 0x1&(x>>2);
  int f2 = 0x1&x;

  int sign = 0x1&(x>>31);
  int round = (0x0)|(x)|(x>>1)|(x>>2);

  x = tmp1 + tmp2 + (f1&f2) + (sign&round);// 5/8 = 1/2 + 1/8
  return x;
}
/* 
 * float_half - Return bit-level equivalent of expression 0.5*f for
 *   floating point argument f.
 *   Both the argument and result are passed as unsigned int's, but
 *   they are to be interpreted as the bit-level representation of
 *   single-precision floating point values.
 *   When argument is NaN, return argument
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
unsigned float_half(unsigned uf) {
  int s = 0x80000000&uf;
  int e = 0x7f800000&uf;
  int f = 0x007fffff&uf;
  int ifrounding = f&(((0x1<<1)&f)>>1);
  int result = 0x0;
  if(e == 0x0)
  {
    f = (f>>1) + ifrounding;
  }
  else if(e == 0x7f800000)
  {
  }
  else
  {
    e = ((e>>23) - 1)<<23;
    if(e == 0x0)
    {
      f = (f>>1);
      f = f|(0x00400000);
      f = f + ifrounding;
    }
  }
  result = s|e|f;
  return result;
}
/* 
 * float_i2f - Return bit-level equivalent of expression (float) x
 *   Result is returned as unsigned int, but
 *   it is to be interpreted as the bit-level representation of a
 *   single-precision floating point values.
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
unsigned float_i2f(int x) {
  int w = 0;
  int s = 0x80000000&x;
  int e, m;
  int rounding1 = 0x0,rounding2 = 0x0;
  int flag = 0;
  int rshift;
  if(s)
  {
    x = ~x + 1;
  }
  while((w < 32)&&(((0x80000000>>w)&x) == 0))
  {
    w++;
  }
  if(w == 32)
    w = 158;
  e = (158 - w);
  m = (x&(0x7fffffff>>w));
  rshift = 8 - w;
  if(rshift>=0)
  {
    rounding1 = (0xff>>(w + 1))&x;
    m = m>>rshift;
  }
  else{
    m = m<<(-rshift);
  }
  if(rshift>0)
  {rounding2 = x&(0x1<<(rshift - 1));}
  if(rounding2)
  {
    if(rounding1)
    {
      flag = 1;
    }
    else
    {
      if(m&0x1)
      {
        flag = 1;
      }
    }
  }
  e = e<<23;
  return s+e+m+flag;
}
/* 
 * float64_f2i - Return bit-level equivalent of expression (int) f
 *   for 64 bit floating point argument f.
 *   Argument is passed as two unsigned int, but
 *   it is to be interpreted as the bit-level representation of a
 *   double-precision floating point value.
 *   Notice: uf1 contains the lower part of the f64 f
 *   Anything out of range (including NaN and infinity) should return
 *   0x80000000u.
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 20
 *   Rating: 4
 */
int float64_f2i(unsigned uf1, unsigned uf2) {
  int sign = uf2 >> 31;
  int exp = (uf2 >> 20) & 0x7FF;
  unsigned frac = (((uf2|0x100000) << 11)>>1) | (uf1 >> 22);
  //printf("s %x\n",sign);
  //printf("e %x\n",exp);
  //printf("f %x\n",frac);
  int e = exp - 1023;
  if((exp == 0x7ff)||(e>=31))
  {
    return 0x80000000u;
  }
  else if(e < 0)
  {
    return 0;
  }
  else {
        frac = frac>>(30 - e);
        //frac |= 0x1<<e;
        return sign ? -frac : frac;
    }
}
/* 
 * float_negpwr2 - Return bit-level equivalent of the expression 2.0^-x
 *   (2.0 raised to the power -x) for any 32-bit integer x.
 *
 *   The unsigned value that is returned should have the identical bit
 *   representation as the single-precision floating-point number 2.0^-x.
 *   If the result is too small to be represented as a denorm, return
 *   0. If too large, return +INF.
 * 
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. Also if, while 
 *   Max ops: 20 
 *   Rating: 4
 */
unsigned float_negpwr2(int x) {
  int e =  -x;
  //printf("e %x\n",e);
  int exp;
  int rshift;
  if((e > 128)||(e == 0x80000000))
  {
    return 0x7f800000;
  }
  else if(e <= (-126))
  {
    exp = 0x00800000;
  }
  else
  {
    exp = (e + 127)<<23;
  }
  rshift = -126 - e;
  //printf("exp %x\n",exp);
  //printf("rshift %x\n",rshift);
  if(rshift > 0)
  {
    if(rshift < 24)
    {exp >>= rshift;}
    else{
      exp = 0;
    }
  }
  return exp;
}
