/*
 * misc.c
 * 
 * This is a collection of several routines from gzip-1.0.3 
 * adapted for Linux.
 *
 *
 * Modified for CSKY Linux by Mao Han
 *
 * Nicolas Pitre <nico@visuaide.com>  1999/04/14 :
 *  For this code to run directly from Flash, all constant variables must
 *  be marked with 'const' and all other variables initialized at run-time 
 *  only.  This way all non constant variables will end up in the bss segment,
 *  which should point to addresses in RAM and cleared to 0 on start.
 *  This allows for a much quicker boot time.
 */

unsigned int __machine_arch_type;

#include <linux/compiler.h>	/* for inline */
#include <linux/types.h>	/* for size_t */
#include <linux/stddef.h>	/* for NULL */
#include <linux/linkage.h>
#include <asm/string.h>

#ifdef STANDALONE_DEBUG
#define putstr printf
#else

static void putstr(const char *ptr);
#include <mach/uncompress.h>


#define flush()	do { } while (0)


static void putstr(const char *ptr)
{
	char c;
	while ((c = *ptr++) != '\0') {
		if (c == '\n')
			putcu('\r');
		putcu(c);
	}
	flush();
}

#endif

#define __ptr_t void *

#define memzero(s,n) __memzero(s,n)

/*
 * Optimised C version of memzero for the CSKY.
 */
void __memzero (__ptr_t s, size_t n)
{
	union { void *vp; unsigned long *ulp; unsigned char *ucp; } u;
	int i;

	u.vp = s;

	for (i = n >> 5; i > 0; i--) {
		*u.ulp++ = 0;
		*u.ulp++ = 0;
		*u.ulp++ = 0;
		*u.ulp++ = 0;
		*u.ulp++ = 0;
		*u.ulp++ = 0;
		*u.ulp++ = 0;
		*u.ulp++ = 0;
	}

	if (n & 1 << 4) {
		*u.ulp++ = 0;
		*u.ulp++ = 0;
		*u.ulp++ = 0;
		*u.ulp++ = 0;
	}

	if (n & 1 << 3) {
		*u.ulp++ = 0;
		*u.ulp++ = 0;
	}

	if (n & 1 << 2)
		*u.ulp++ = 0;

	if (n & 1 << 1) {
		*u.ucp++ = 0;
		*u.ucp++ = 0;
	}

	if (n & 1)
		*u.ucp++ = 0;
}

inline __ptr_t memcpy(__ptr_t __dest, __const __ptr_t __src,
			    size_t __n)
{
	int i = 0;
	unsigned char *d = (unsigned char *)__dest, *s = (unsigned char *)__src;

	for (i = __n >> 3; i > 0; i--) {
		*d++ = *s++;
		*d++ = *s++;
		*d++ = *s++;
		*d++ = *s++;
		*d++ = *s++;
		*d++ = *s++;
		*d++ = *s++;
		*d++ = *s++;
	}

	if (__n & 1 << 2) {
		*d++ = *s++;
		*d++ = *s++;
		*d++ = *s++;
		*d++ = *s++;
	}

	if (__n & 1 << 1) {
		*d++ = *s++;
		*d++ = *s++;
	}

	if (__n & 1)
		*d++ = *s++;

	return __dest;
}

/*
 * gzip delarations
 */
#define OF(args)  args
#define STATIC static

typedef unsigned char  uch;
typedef unsigned short ush;
typedef unsigned long  ulg;

#define WSIZE 0x8000		/* Window size must be at least 32k, */
				/* and a power of two */

static uch *inbuf;		/* input buffer */
static uch window[WSIZE];	/* Sliding window buffer */

static unsigned insize;		/* valid bytes in inbuf */
static unsigned inptr;		/* index of next byte to be processed in inbuf */
static unsigned outcnt;		/* bytes in output buffer */

/* gzip flag byte */
#define ASCII_FLAG   0x01 /* bit 0 set: file probably ascii text */
#define CONTINUATION 0x02 /* bit 1 set: continuation of multi-part gzip file */
#define EXTRA_FIELD  0x04 /* bit 2 set: extra field present */
#define ORIG_NAME    0x08 /* bit 3 set: original file name present */
#define COMMENT      0x10 /* bit 4 set: file comment present */
#define ENCRYPTED    0x20 /* bit 5 set: file is encrypted */
#define RESERVED     0xC0 /* bit 6,7:   reserved */

#define get_byte()  (inptr < insize ? inbuf[inptr++] : fill_inbuf())

/* Diagnostic functions */

#  define Assert(cond,msg)
#  define Trace(x)
#  define Tracev(x)
#  define Tracevv(x)
#  define Tracec(c,x)
#  define Tracecv(c,x)

static int  fill_inbuf(void);
static void flush_window(void);
static void error(char *m);

extern char input_data[];
extern char input_data_end[];

uch *output_data;
ulg output_ptr;
ulg bytes_out;

static void error(char *m);

static void putstr(const char *);

extern int end;
unsigned long free_mem_ptr;
unsigned long free_mem_end_ptr;

#define ARCH_HAS_DECOMP_WDOG

#include "../../../../lib/inflate.c"

/* ===========================================================================
 * Fill the input buffer. This is called only when the buffer is empty
 * and at least one byte is really needed.
 */
int fill_inbuf(void)
{
	if (insize != 0)
		error("ran out of input data");

	inbuf = input_data;
	insize = &input_data_end[0] - &input_data[0];

	inptr = 1;
	return inbuf[0];
}

/* ===========================================================================
 * Write the output window window[0..outcnt-1] and update crc and bytes_out.
 * (Used for the decompressed data only.)
 */
void flush_window(void)
{
	ulg c = crc;
	unsigned n;
	uch *in, *out, ch;

	in = window;
	out = &output_data[output_ptr];
	for (n = 0; n < outcnt; n++) {
		ch = *out++ = *in++;
		c = crc_32_tab[((int)c ^ ch) & 0xff] ^ (c >> 8);
	}
	crc = c;
	bytes_out += (ulg)outcnt;
	output_ptr += (ulg)outcnt;
	outcnt = 0;
	putstr(".");
}

#ifndef arch_error
#define arch_error(x)
#endif

static void error(char *x)
{
	arch_error(x);
	
	putstr("\n\n");
	putstr(x);
	putstr("\n\n -- System halted");
	
	while(1);	/* Halt */
}

extern int do_decompress(u8 *input, int len, u8 *output, void (*error)(char *x));


void
decompress_kernel(unsigned long output_start, unsigned long free_mem_ptr_p,
		unsigned long free_mem_ptr_end_p,
		int arch_id)
{
	int ret;

	output_data		= (unsigned char *)output_start;
	free_mem_ptr		= free_mem_ptr_p;
	free_mem_end_ptr	= free_mem_ptr_end_p;
	__machine_arch_type	= arch_id;

	uart_init();
	arch_decomp_setup();

	putstr("Uncompressing Linux...");
	ret = do_decompress(input_data, input_data_end - input_data,
			    output_data, error);
	if (ret)
		error("decompressor returned an error");
	else
		putstr(" done, booting the kernel.\n");
}
