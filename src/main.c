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

#include <libxfce4util/libxfce4util.h>

#include <stdio.h>

#include <string.h>

/* #ifdef HAVE_CONFIG_H */
 #include <config.h>
/* #endif */

/* Package includes */
#include <sensors-interface.h>
#include <sensors-interface-common.h>

/* Local includes */
#include "actions.h"
#include "callbacks.h"
#include "interface.h"

void
print_license ()
{
    printf (_("Xfce4 Sensors %s\n"
                      "This program is published under the GPL v2.\n"
                      "The license text can be found inside the program's source archive "
                      "or under /usr/share/apps/LICENSES/GPL_V2 or at "
                      "http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt\n" ), PACKAGE_VERSION
                );
}


void
print_usage ()
{
    printf (_("Xfce4 Sensors %s\n"
                      "Displays information about your sensors and ACPI.\n"
                      "Synopsis: \n"
                      "  xfce4-sensors options\n"
                      "where options are one or more of the following:\n"
                      "  -h, --help    Print this help dialog.\n"
                      "  -l, --license Print license information.\n"
                      "  -V, --version Print version information.\n"
                      "\n"
                      "This program is published under the GPL v2.\n"), PACKAGE_VERSION
                );
}


void
print_version ()
{
    printf (_("Xfce4 Sensors %s\n"), PACKAGE_VERSION);
}

int
main (int argc, char **argv)
{
    GtkWidget *window;
    t_sensors_dialog *sd;
    t_sensors *sensors;

    xfce_textdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");

    if ( argc > 1 && (strcmp(argv[1], "--help")==0 || strcmp(argv[1], "-h")==0) )
    {
        print_usage ();
        return 0;
    }
    else if ( argc > 1 && (strcmp(argv[1], "--license")==0 || strcmp(argv[1], "-l")==0) )
    {
        print_license ();
        return 0;
    }
    else if ( argc > 1 && (strcmp(argv[1], "--version")==0 || strcmp(argv[1], "-V")==0) )
    {
        print_version ();
        return 0;
    }

    /* start the Gtk engine */
    gtk_init (&argc, &argv);

    /* initialize sensor stuff */
    sensors = sensors_new (NULL, NULL);
    sd = g_new0 (t_sensors_dialog, 1);
    sd->sensors = sensors;
    sd->plugin_dialog = FALSE;

    /* build main application */
    window = create_main_window (sd);

    /* show window and run forever */
    gtk_widget_show_all(window);
    /* gtk_window_set_default_size(GTK_WINDOW(window), 480, 640); */
    gtk_widget_set_size_request(window, 480,640);
    gtk_main();


    /* do the cleaning? */
    /*
    gtk_widget_destroy(window);
    g_free (window);
    */

    return 0;
}
