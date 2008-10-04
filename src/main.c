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

#include <gtk/gtk.h>

#include <stdio.h>

#include <string.h>

void
print_usage ()
{
    print (_("Xfce4 Sensors -- \n"
                     "Displays information about your sensors and ACPI.\n"
                     "Synopsis: \n"
                     "  xfce4-sensors options\n"
                     "where options are one or more of the following:\n"
                     "  -h, --help Print this help dialog.\n"
                     "\n"
                     "This program is published under the GPL v2.\n")
                );
}

int
main (int argc, char **argv)
{
    if ( argc > 1 && (strcmp(argv[1], "--help")==0 || strcmp(argv[1], "-h")==0) )
    {
        print_usage ();
        return 0;
    }

}
