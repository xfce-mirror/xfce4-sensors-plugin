/* $Id$ */

/*  Copyright 2008-2010 Fabian Nowak (timystery@arcor.de)
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

 #include <config.h>

/* Package includes */
#include <sensors-interface.h>
#include <sensors-interface-common.h>

/* Local includes */
#include "actions.h"
#include "callbacks.h"
#include "interface.h"

/* forward declarations for -Wall */
void print_license (void);
void print_usage (void);
void print_version (void);
t_sensors_dialog * initialize_sensors_structures (void);

void
print_license (void)
{
    printf (_("Xfce4 Sensors %s\n"
                      "This program is published under the GPL v2.\n"
                      "The license text can be found inside the program's source archive "
                      "or under /usr/share/apps/LICENSES/GPL_V2 or at "
                      "http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt\n" ), PACKAGE_VERSION
                );
}


void
print_usage (void)
{
    printf (_("Xfce4 Sensors %s\n"
                      "Displays information about your hardware sensors, ACPI "
                                            "status, harddisk temperatures and Nvidia GPU's temperature.\n"
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
print_version (void)
{
    printf (_("Xfce4 Sensors %s\n"), PACKAGE_VERSION);
}


t_sensors_dialog *
initialize_sensors_structures (void)
{
    t_sensors *ptr_sensors_structure;
    t_sensors_dialog *ptr_sensors_dialog_structure;
    int idx_chip, idx_feature;
    
    ptr_sensors_structure = sensors_new (NULL, NULL);
    ptr_sensors_dialog_structure = g_new0 (t_sensors_dialog, 1);
    ptr_sensors_dialog_structure->sensors = ptr_sensors_structure;
    ptr_sensors_dialog_structure->plugin_dialog = FALSE;
    
    for (idx_chip=0; idx_chip<MAX_NUM_CHIPS; idx_chip++)
    {
        for (idx_feature=0; idx_feature<MAX_NUM_FEATURES; idx_feature++)
        {
            ptr_sensors_structure->tachos[idx_chip][idx_feature] = NULL;
        }
    }
      
    return ptr_sensors_dialog_structure;
}


static void
on_window_destroy_event (GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
  DBG("on_window_destroy_event\n");
}


int
main (int argc, char **argv)
{
    GtkWidget *window;
    t_sensors_dialog *sd;

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
    sd = initialize_sensors_structures ();

    /* build main application */
    window = create_main_window (sd);
        
    /* automatic refresh callback */
    sd->sensors->timeout_id  = g_timeout_add (
        sd->sensors->sensors_refresh_time * 1000,
        (GtkFunction) refresh_view, (gpointer) sd
    );

    /* show window and run forever */
    gtk_widget_show_all(window); /* to make sure everything is shown */
    gtk_window_resize(GTK_WINDOW(window), 400, 500);
    gtk_widget_realize(sd->sensors->widget_sensors); /* without this call, the table will only be realized when the tab is shown and hence, toggled tachos are not drawn as they lack a drawable window. */
    
    g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(on_window_destroy_event), NULL);
    
    gtk_main();

    /* do the cleaning? */
    /*
    gtk_widget_destroy(window);
    g_free (window);
    g_free (sd->sensors);
    g_free (sd);
  */
    

    return 0;
}
