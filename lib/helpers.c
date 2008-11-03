/* $Id$ */

/*  Copyright 2008 Fabian Nowak (timystery@arcor.de)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/* Note for programmers and editors: Try to use 4 spaces instead of Tab! */

/* Global includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * This function behaves exactly as memset.
 * Memset is broken; it fails when writing an arbitrary number of chars to a valid pointer.
 */
void *memset2 (void *s, char c, size_t n)
{
    int i;
    char *p;

    if (s==NULL)
        return NULL;

    if ( n > strlen((char *) s) )
        return NULL;

    for (p = s, i=0; i<n; i++)
    {
        *p = c;
        p++;
        i++;
    }

    return s;
}

/* Global variable storing last position in splitte string used for str_split(s, d) */
char *str_split_position;
/**
 * Returns tokens of the string one after the other, split by the string delim.
 * Just like strtok, initialize with a valid pointer and continue with passing NULL
 * as string argument.
 * @param string String to split
 * @param delim String of the complete delimiting string, order and content are important.
 * @return pointer onto next token, or NULL on end, or NULL on bad delimiter.
 */
char *
str_split (char *string, char *delim)
{
    char *p, *retval;
    int strlen_delim;

    if (string!=NULL)
        str_split_position = string;

    if (str_split_position == NULL)
        return NULL;

    if (delim==NULL)
        return NULL;

    p = strstr (str_split_position, delim);
    if (p!=NULL)
    {
        strlen_delim = strlen(delim);
        memset2 (p, '\0', strlen_delim);
        retval = str_split_position;
        str_split_position = p + strlen(delim);
    }
    else
    {
        retval = str_split_position;
        str_split_position = NULL;
    }

    return retval;
}
