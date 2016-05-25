#include <linux/module.h>
#include <asm/uaccess.h>
#include <asm/cacheflush.h>

asmlinkage long long __ashldi3 (long long, int);
asmlinkage long long __ashrdi3 (long long, int);
asmlinkage long long __lshrdi3 (long long, int);
asmlinkage long long __muldi3 (long long, long long);

/* The following are special because they're not called
 *  explicitly (the C compiler generates them).  Fortunately,
 *  their interface isn't gonna change any time soon now, so
 *  it's OK to leave it out of version control. 
 */
EXPORT_SYMBOL(__ashldi3);
EXPORT_SYMBOL(__ashrdi3);
EXPORT_SYMBOL(__lshrdi3);
EXPORT_SYMBOL(__muldi3);

/*
 * String functions
 */
EXPORT_SYMBOL(memset);
EXPORT_SYMBOL(memcpy);

/* user mem (segment) */
EXPORT_SYMBOL(__generic_copy_from_user);
EXPORT_SYMBOL(__generic_copy_to_user);
EXPORT_SYMBOL(strnlen_user);
EXPORT_SYMBOL(__strncpy_from_user);
EXPORT_SYMBOL(strncpy_from_user);
EXPORT_SYMBOL(clear_user);
EXPORT_SYMBOL(__clear_user);

EXPORT_SYMBOL(get_wchan);

extern asmlinkage void trap(void);
EXPORT_SYMBOL(trap);

