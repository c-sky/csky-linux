// SPDX-License-Identifier: GPL-2.0
// Copyright (C) 1991-2018 Free Software Foundation, Inc.

#include "memcopy.h"

#undef strcpy

#ifndef STRCPY
# define STRCPY strcpy
#endif

/* Copy SRC to DEST.  */
char *
STRCPY (char *dest, const char *src)
{
  return memcpy (dest, src, strlen (src) + 1);
}
