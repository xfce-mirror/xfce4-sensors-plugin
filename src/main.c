/* File: main.c
 *
 *  Copyright 2008-2017 Fabian Nowak (timystery@arcor.de)
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


#define gtk_hbox_new(homogeneous, spacing) \
        gtk_box_new(GTK_ORIENTATION_HORIZONTAL, spacing)

#define gtk_vbox_new(homogeneous, spacing) \
        gtk_box_new(GTK_ORIENTATION_VERTICAL, spacing)


/* forward declarations for -Wall */
void print_license (void);
void print_usage (void);
void print_version (void);
t_sensors_dialog * initialize_sensors_structures (void);

/* -------------------------------------------------------------------------- */
/**
 * Prints license information
 */
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


/* -------------------------------------------------------------------------- */
/**
 * Prints help/usage information
 */
void
print_usage (void)
{
    printf (_("Xfce4 Sensors %s\n"
                      "Displays information about your hardware sensors, ACPI "
                                            "status, harddisk temperatures and Nvidia GPU's temperature.\n"
                      "Synopsis: \n"
                      "  xfce4-sensors [option]\n"
                      "where [option] is one of the following:\n"
                      "  -h, --help    Print this help dialog.\n"
                      "  -l, --license Print license information.\n"
                      "  -V, --version Print version information.\n"
                      "\n"
                      "This program is published under the GPL v2.\n"), PACKAGE_VERSION
                );
}


/* -------------------------------------------------------------------------- */
/**
 * Prints version information as requested by "xfce4-sensors -V"
 */
void
print_version (void)
{
    printf (_("Xfce4 Sensors %s\n"), PACKAGE_VERSION);
}


/* -------------------------------------------------------------------------- */
/**
 * Initializes the required sensor structures.
 * @return pointer to newly allocated sensors dialog information
 */
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


/* -------------------------------------------------------------------------- */
/**
 * Main routine.
 * @param argc: number of arguments
 * @param argv: array of strings passed as arguments
 */
int
main (int argc, char **argv)
{
    GtkWidget *window;
    t_sensors_dialog *ptr_sensors_dialog;

    xfce_textdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");

    if (argc > 1)
    {
        if (strcmp(argv[1], "--help")==0 || strcmp(argv[1], "-h")==0)
        {
            print_usage ();
            return 0;
        }
        else if (strcmp(argv[1], "--license")==0 || strcmp(argv[1], "-l")==0)
        {
            print_license ();
            return 0;
        }
        else if (strcmp(argv[1], "--version")==0 || strcmp(argv[1], "-V")==0)
        {
            print_version ();
            return 0;
        }
    }

    /* start the Gtk engine */
    gtk_init (&argc, &argv);

    /* declare callback functions for libxfce4sensors */
    adjustment_value_changed = &adjustment_value_changed_;
    sensor_entry_changed = &sensor_entry_changed_;
    list_cell_text_edited= &list_cell_text_edited_;
    list_cell_toggle = &list_cell_toggle_;
    list_cell_color_edited = &list_cell_color_edited_;
    minimum_changed = &minimum_changed_;
    maximum_changed = &maximum_changed_;
    temperature_unit_change = &temperature_unit_change_;

    /* initialize sensor stuff */
    ptr_sensors_dialog = initialize_sensors_structures ();

    /* build main application */
    window = create_main_window (ptr_sensors_dialog);

    /* automatic refresh callback */
    ptr_sensors_dialog->sensors->timeout_id  = g_timeout_add (
        ptr_sensors_dialog->sensors->sensors_refresh_time * 1000,
        refresh_view, ptr_sensors_dialog
    );

    /* show window and run forever */
    gtk_widget_show_all(window); /* to make sure everything is shown */
    gtk_window_resize(GTK_WINDOW(window), 400, 500);

    gtk_main();

    /* do the cleaning? */
    free_widgets(ptr_sensors_dialog); /* counterpart to init_widgets() inside create_main_window() */
    gtk_widget_destroy(window);

    g_free (ptr_sensors_dialog->sensors);
    g_free (ptr_sensors_dialog);

    return 0;
}
