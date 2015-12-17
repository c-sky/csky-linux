
/*
===============================================================================

This C header file is part of the SoftFloat IEC/IEEE Floating-point
Arithmetic Package, Release 2.

Written by John R. Hauser.  This work was made possible in part by the
International Computer Science Institute, located at Suite 600, 1947 Center
Street, Berkeley, California 94704.  Funding was partially provided by the
National Science Foundation under grant MIP-9311980.  The original version
of this code was written as part of a project to build a fixed-point vector
processor in collaboration with the University of California at Berkeley,
overseen by Profs. Nelson Morgan and John Wawrzynek.  More information
is available through the Web page
http://www.jhauser.us/arithmetic/SoftFloat-2b/SoftFloat-source.txt

THIS SOFTWARE IS DISTRIBUTED AS IS, FOR FREE.  Although reasonable effort
has been made to avoid it, THIS SOFTWARE MAY CONTAIN FAULTS THAT WILL AT
TIMES RESULT IN INCORRECT BEHAVIOR.  USE OF THIS SOFTWARE IS RESTRICTED TO
PERSONS AND ORGANIZATIONS WHO CAN AND WILL TAKE FULL RESPONSIBILITY FOR ANY
AND ALL LOSSES, COSTS, OR OTHER PROBLEMS ARISING FROM ITS USE.

Derivative works are acceptable, even for commercial purposes, so long as
(1) they include prominent notice that the work is derivative, and (2) they
include prominent notice akin to these three paragraphs for those parts of
this code that are retained.

===============================================================================
*/

#ifndef __SOFTFLOAT_H__
#define __SOFTFLOAT_H__

#define BITS64
typedef char flag;
typedef unsigned char uint8;
typedef signed char int8;
typedef int uint16;
typedef int int16;
typedef unsigned int uint32;
typedef signed int int32;
#ifdef BITS64
typedef unsigned long long int bits64;
typedef signed long long int sbits64;
#endif

/*
-------------------------------------------------------------------------------
Each of the following `typedef's defines a type that holds integers
of _exactly_ the number of bits specified.  For instance, for most
implementation of C, `bits16' and `sbits16' should be `typedef'ed to
`unsigned short int' and `signed short int' (or `short int'), respectively.
-------------------------------------------------------------------------------
*/
typedef unsigned char bits8;
typedef signed char sbits8;
typedef unsigned short int bits16;
typedef signed short int sbits16;
typedef unsigned int bits32;
typedef signed int sbits32;
#ifdef BITS64
typedef unsigned long long int uint64;
typedef signed long long int int64;
#endif

#ifdef BITS64
/*
-------------------------------------------------------------------------------
The `LIT64' macro takes as its argument a textual integer literal and if
necessary ``marks'' the literal as having a 64-bit integer type.  For
example, the Gnu C Compiler (`gcc') requires that 64-bit literals be
appended with the letters `LL' standing for `long long', which is `gcc's
name for the 64-bit integer type.  Some compilers may allow `LIT64' to be
defined as the identity macro:  `#define LIT64( a ) a'.
-------------------------------------------------------------------------------
*/
#define LIT64( a ) a##LL
#endif

/*
-------------------------------------------------------------------------------
Software IEC/IEEE floating-point types.
-------------------------------------------------------------------------------
*/
typedef unsigned int float32;
typedef unsigned long long float64;
#if 0
typedef struct {
#ifdef __EB__
    u16 __padding;
    u16 high;
#else
    u16 high;
    u16 __padding;
#endif
    u64 low;
}  __attribute__ ((packed,aligned(4))) floatx80;
#endif


struct roundingData {
	unsigned int mode;
	unsigned int exception;
};


/*
-------------------------------------------------------------------------------
Software IEC/IEEE floating-point underflow tininess-detection mode.
-------------------------------------------------------------------------------
*/
extern signed char float_detect_tininess;
enum {
    float_tininess_after_rounding  = 0,
    float_tininess_before_rounding = 1
};

/*
-------------------------------------------------------------------------------
Software IEC/IEEE floating-point rounding mode.
-------------------------------------------------------------------------------
*/
//extern int8 float_rounding_mode;
enum {
    float_round_nearest_even = 0,
    float_round_to_zero      = 1,
    float_round_down         = 2,
    float_round_up           = 3
};

/*
-------------------------------------------------------------------------------
Software IEC/IEEE floating-point exception flags.
-------------------------------------------------------------------------------
enum {
    float_flag_inexact   =  1,
    float_flag_underflow =  2,
    float_flag_overflow  =  4,
    float_flag_divbyzero =  8,
    float_flag_invalid   = 16
};

ScottB: November 4, 1998
Changed the enumeration to match the bit order in the FPA11.
*/

enum {
    float_flag_invalid   =  1,
    float_flag_divbyzero =  2,
    float_flag_overflow  =  4,
    float_flag_underflow =  8,
    float_flag_inexact   = 16
};

/*
-------------------------------------------------------------------------------
Routine to raise any or all of the software IEC/IEEE floating-point
exception flags.
-------------------------------------------------------------------------------
*/
void float_raise( signed char );

/*
-------------------------------------------------------------------------------
Software IEC/IEEE integer-to-floating-point conversion routines.
-------------------------------------------------------------------------------
*/
float32 int32_to_float32(signed int a, struct roundingData *roundData);
float32 uint32_to_float32(unsigned int a, struct roundingData *roundData);
float64 int32_to_float64( signed int );
#if 0
floatx80 int32_to_floatx80( signed int );
#endif

/*
-------------------------------------------------------------------------------
Software IEC/IEEE single-precision conversion routines.
-------------------------------------------------------------------------------
*/
signed int float32_to_int32(float32 a, struct roundingData *roundData);
uint32 float32_to_uint32(float32 a, struct roundingData *roundData);
signed int float32_to_int32_round_to_zero( float32 );
float64 float32_to_float64( float32 );
#if 0
floatx80 float32_to_floatx80( float32 );
#endif

/*
-------------------------------------------------------------------------------
Software IEC/IEEE single-precision operations.
-------------------------------------------------------------------------------
*/
float32 float32_round_to_int( struct roundingData*, float32 );
float32 float32_add(float32 a, float32 b, struct roundingData *roundData);
float32 float32_sub(float32 a, float32 b, struct roundingData *roundData);
float32 float32_mul(float32 a, float32 b, struct roundingData *roundData);
float32 float32_div(float32 a, float32 b, struct roundingData *roundData);
float32 float32_rem(float32 a, float32 b, struct roundingData *roundData);
float32 float32_sqrt(float32 a, struct roundingData *roundData);
char float32_eq( float32, float32 );
char float32_le( float32, float32 );
char float32_lt( float32, float32 );
char float32_eq_signaling( float32, float32 );
char float32_le_quiet( float32, float32 );
char float32_lt_quiet( float32, float32 );
char float32_is_signaling_nan( float32 );

/*
-------------------------------------------------------------------------------
Software IEC/IEEE double-precision conversion routines.
-------------------------------------------------------------------------------
*/
signed int float64_to_int32(float64 a, struct roundingData *roundData);
signed int float64_to_int32_round_to_zero( float64 );
float32 float64_to_float32(float64 a, struct roundingData *roundData);
#if 0
floatx80 float64_to_floatx80( float64 );
#endif

/*
-------------------------------------------------------------------------------
Software IEC/IEEE double-precision operations.
-------------------------------------------------------------------------------
*/
float64 float64_round_to_int( struct roundingData *, float64 );
float64 float64_add(float64 a, float64 b, struct roundingData *roundData);
float64 float64_sub(float64 a, float64 b, struct roundingData *roundData);
float64 float64_mul(float64 a, float64 b, struct roundingData *roundData);
float64 float64_div(float64 a, float64 b, struct roundingData *roundData);
float64 float64_rem(float64 a, float64 b, struct roundingData *roundData);
float64 float64_sqrt(float64 a, struct roundingData *roundData);
char float64_eq( float64, float64 );
char float64_le( float64, float64 );
char float64_lt( float64, float64 );
char float64_eq_signaling( float64, float64 );
char float64_le_quiet( float64, float64 );
char float64_lt_quiet( float64, float64 );
char float64_is_signaling_nan( float64 );

#if 0

/*
-------------------------------------------------------------------------------
Software IEC/IEEE extended double-precision conversion routines.
-------------------------------------------------------------------------------
*/
signed int floatx80_to_int32( struct roundingData *, floatx80 );
signed int floatx80_to_int32_round_to_zero( floatx80 );
float32 floatx80_to_float32( struct roundingData *, floatx80 );
float64 floatx80_to_float64( struct roundingData *, floatx80 );

/*
-------------------------------------------------------------------------------
Software IEC/IEEE extended double-precision operations.
-------------------------------------------------------------------------------
*/
floatx80 floatx80_round_to_int( struct roundingData *, floatx80 );
floatx80 floatx80_add( struct roundingData *, floatx80, floatx80 );
floatx80 floatx80_sub( struct roundingData *, floatx80, floatx80 );
floatx80 floatx80_mul( struct roundingData *, floatx80, floatx80 );
floatx80 floatx80_div( struct roundingData *, floatx80, floatx80 );
floatx80 floatx80_rem( struct roundingData *, floatx80, floatx80 );
floatx80 floatx80_sqrt( struct roundingData *, floatx80 );
char floatx80_eq( floatx80, floatx80 );
char floatx80_le( floatx80, floatx80 );
char floatx80_lt( floatx80, floatx80 );
char floatx80_eq_signaling( floatx80, floatx80 );
char floatx80_le_quiet( floatx80, floatx80 );
char floatx80_lt_quiet( floatx80, floatx80 );
char floatx80_is_signaling_nan( floatx80 );

extern flag floatx80_is_nan(floatx80);

#endif

static inline flag extractFloat32Sign(float32 a)
{
	return a >> 31;
}

static inline flag float32_eq_nocheck(float32 a, float32 b)
{
	return (a == b) || ((bits32) ((a | b) << 1) == 0);
}

static inline flag float32_lt_nocheck(float32 a, float32 b)
{
	flag aSign, bSign;

	aSign = extractFloat32Sign(a);
	bSign = extractFloat32Sign(b);
	if (aSign != bSign)
		return aSign && ((bits32) ((a | b) << 1) != 0);
	return (a != b) && (aSign ^ (a < b));
}

static inline flag extractFloat64Sign(float64 a)
{
	return a >> 63;
}

static inline flag float64_eq_nocheck(float64 a, float64 b)
{
	return (a == b) || ((bits64) ((a | b) << 1) == 0);
}

static inline flag float64_lt_nocheck(float64 a, float64 b)
{
	flag aSign, bSign;

	aSign = extractFloat64Sign(a);
	bSign = extractFloat64Sign(b);
	if (aSign != bSign)
		return aSign && ((bits64) ((a | b) << 1) != 0);
	return (a != b) && (aSign ^ (a < b));
}

extern flag float32_is_nan( float32 a );
extern flag float64_is_nan( float64 a );

extern int32 float64_to_uint32(float64 a, struct roundingData *roundData);
extern int32 float64_to_uint32_round_to_zero( float64 a );

#endif
