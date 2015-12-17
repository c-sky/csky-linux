/*
 *  linux/arch/csky/lib/memset.c
 *
 *  user address space access functions.
 *  the non inlined parts of asm-csky/uaccess.h are here.
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License.  See the file "COPYING" in the main directory of this archive
 *  for more details.
 *
 *  Copyright (C) 2009  Hangzhou C-SKY Microsystems.
 *
 */

#include <asm/uaccess.h>
#include <linux/types.h>


unsigned long __generic_copy_from_user(void *to, const void *from, 
							unsigned long n)
{
	if (access_ok(VERIFY_READ, from, n)) {
		__copy_user_zeroing(to,from,n); 
	}
	else/* security hole - plug it */
		memset(to,0, n);
	return n;

}

unsigned long __generic_copy_to_user(void *to, const void *from,
							unsigned long n)
{
	if (access_ok(VERIFY_WRITE, to, n)) {
		__copy_user(to,from,n);
	}
	return n;

}


/*
 * copy a null terminated string from userspace.	
 */
#define __do_strncpy_from_user(dst,src,count,res)       \
do{                                                     \
        int tmp;                                        \
        long faultres;                                  \
        __asm__ __volatile__(                           \
        "       cmpnei  %3, 0           \n"             \
        "       bf      4f              \n"             \
        "1:     cmpnei  %1, 0          	\n"             \
        "       bf      5f              \n"             \
        "2:     ldb     %4, (%3, 0)     \n"             \
        "       stb     %4, (%2, 0)     \n"             \
        "       cmpnei  %4, 0           \n"             \
        "       bf      3f              \n"             \
        "       addi    %3,  1          \n"             \
        "       addi    %2,  1          \n"             \
        "       subi    %1,  1          \n"             \
        "       br      1b              \n"             \
        "3:     subu	%0, %1          \n"             \
        "       br      5f              \n"             \
        "4:     mov     %0, %5          \n"             \
        "       br      5f              \n"             \
        ".section __ex_table, \"a\"     \n"             \
        ".align   2                     \n"             \
        ".long    2b, 4b                \n"             \
        ".previous                      \n"             \
        "5:                             \n"             \
          :"=r"(res),"=r"(count),"=r"(dst),"=r"(src), "=r"(tmp),"=r"(faultres) \
          : "5"(-EFAULT),"0"(count), "1"(count), "2"(dst),"3"(src)     	       \
          : "memory" );	                                \
} while(0)		

/*
 * __strncpy_from_user: - Copy a NUL terminated string from userspace, with less checking.
 * @dst:   Destination address, in kernel space.  This buffer must be at
 *         least @count bytes long.
 * @src:   Source address, in user space.
 * @count: Maximum number of bytes to copy, including the trailing NUL.
 * 
 * Copies a NUL-terminated string from userspace to kernel space.
 * Caller must check the specified block with access_ok() before calling
 * this function.
 *
 * On success, returns the length of the string (not including the trailing
 * NUL).
 *
 * If access to userspace fails, returns -EFAULT (some data may have been
 * copied).
 *
 * If @count is smaller than the length of the string, copies @count bytes
 * and returns @count.
 */
long
__strncpy_from_user(char *dst, const char *src, long count)
{
	long res;
	__do_strncpy_from_user(dst, src, count, res);
	return res;
}
/*
 * strncpy_from_user: - Copy a NUL terminated string from userspace.
 * @dst:   Destination address, in kernel space.  This buffer must be at
 *         least @count bytes long.
 * @src:   Source address, in user space.
 * @count: Maximum number of bytes to copy, including the trailing NUL.
 * 
 * Copies a NUL-terminated string from userspace to kernel space.
 *
 * On success, returns the length of the string (not including the trailing
 * NUL).
 *
 * If access to userspace fails, returns -EFAULT (some data may have been
 * copied).
 *
 * If @count is smaller than the length of the string, copies @count bytes
 * and returns @count.
 */
long
strncpy_from_user(char *dst, const char *src, long count)
{
	long res = -EFAULT;
	if (access_ok(VERIFY_READ, src, 1))
		__do_strncpy_from_user(dst, src, count, res);
	return res;
}

/*
 * strlen_user: - Get the size of a string in user space.
 * @str: The string to measure.
 * @n:   The maximum valid length
 *
 * Get the size of a NUL-terminated string in user space.
 *
 * Returns the size of the string INCLUDING the terminating NUL.
 * On exception, returns 0.
 * If the string is too long, returns a value greater than @n.
 */
long strnlen_user(const char *s, long n)
{

	unsigned long res,tmp;
	if(s){
		__asm__ __volatile__(
        "       cmpnei  %1, 0           \n"
        "       bf      3f              \n"
        "1:     cmpnei  %0, 0           \n"              
        "       bf      3f              \n"
        "2:     ldb     %3, (%1, 0)     \n"             
        "       cmpnei  %3, 0           \n"             
        "       bf      3f              \n"             
        "       subi    %0,  1          \n"             
        "       addi    %1,  1          \n"             
        "       br      1b              \n"
        "3:     subu    %2, %0          \n"
        "       addi    %2,  1          \n"             
        "       br      5f              \n"             
        "4:     movi    %0, 0           \n"             
        "       br      5f              \n"             
        ".section __ex_table, \"a\"     \n"             
        ".align   2                     \n"
        ".long    2b, 4b                \n"             
        ".previous                      \n"             
        "5:                             \n"             
        :"=r"(n),"=r"(s), "=r"(res), "=r"(tmp)   
        : "0"(n), "1"(s), "2"(n)      
        : "cc" );
		return res;     
	}
	return 0;     
}

#define __do_clear_user(addr,size)                      \
do {                                                    \
        int __d0;                                       \
        int zvalue;                                     \
        __asm__ __volatile__(                           \
        "       cmpnei  %1, 0           \n"             \
        "       bf      3f              \n"             \
        "0:     cmpnei  %0, 0           \n"             \
        "       bf      3f              \n"             \
        "1:     stb     %2,(%1,0)       \n"             \
        "       subi    %0, 1           \n"             \
        "       addi    %1, 1           \n"             \
        "       cmpnei  %0, 0           \n"             \
        "       bt      1b              \n"             \
        "2:     br      3f              \n"             \
        ".section __ex_table,\"a\"      \n"             \
        ".align   2                     \n"             \
        ".long    1b,2b	                \n"             \
        ".previous                      \n"             \
        "3:                             \n"             \
       	  : "=r"(size), "=r" (__d0),"=r"(zvalue)        \
          : "0"(size), "1"(addr), "2"(0));              \
} while (0)

/*
 * clear_user: - Zero a block of memory in user space.
 * @to:   Destination address, in user space.
 * @n:    Number of bytes to zero.
 *
 * Zero a block of memory in user space.
 *
 * Returns number of bytes that could not be cleared.
 * On success, this will be zero.
 */	
unsigned long 
clear_user(void __user *to, unsigned long n)
{
	if (access_ok(VERIFY_WRITE, to, n))
		__do_clear_user(to, n);
	return n;
}
/*
 * __clear_user: - Zero a block of memory in user space, with less checking.
 * @to:   Destination address, in user space.
 * @n:    Number of bytes to zero.
 *
 * Zero a block of memory in user space.  Caller must check
 * the specified block with access_ok() before calling this function.
 *
 * Returns number of bytes that could not be cleared.
 * On success, this will be zero.
 */
unsigned long
__clear_user(void __user *to, unsigned long n)
{
	__do_clear_user(to, n);
	return n;
}
                 
