/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2009  Hangzhou C-SKY Microsystems co.,ltd.
 * Copyright (C) 2009  Hu Junshan (junshan_hu@c-sky.com)
 *
 */

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

/* cache flush */
//EXPORT_SYMBOL(_flush_cache_mm);
//EXPORT_SYMBOL(_flush_cache_page);
//EXPORT_SYMBOL(_flush_cache_range);
//EXPORT_SYMBOL(flush_dcache_page);
//EXPORT_SYMBOL(_flush_dcache_all);
//EXPORT_SYMBOL(_clear_dcache_all);

EXPORT_SYMBOL(get_wchan);

extern asmlinkage void trap(void);
EXPORT_SYMBOL(trap);

