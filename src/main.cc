/* main.cc
 * Part of xfce4-sensors-plugin
 *
 * Copyright (c) 2008-2017 Fabian Nowak <timystery@arcor.de>
 * Copyright (c) 2021-2022 Jan Ziak <0xe2.0x9a.0x9b@xfce.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_XFCE_REVISION_H
#include "xfce-revision.h"
#endif

#include <gtk/gtk.h>
#include <libxfce4util/libxfce4util.h>
#include <stdio.h>
#include <string.h>

/* Package includes */
#include <sensors-interface.h>
#include <sensors-interface-common.h>

/* Local includes */
#include "actions.h"
#include "callbacks.h"
#include "interface.h"


/**
 * Prints license information
 */
static void
print_license ()
{
    printf (_("Xfce4 Sensors %s\n"
                      "This program is published under the GPL v2.\n"
                      "The license text can be found inside the program's source archive "
                      "or under /usr/share/apps/LICENSES/GPL_V2 or at "
                      "http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt\n" ), VERSION_FULL
                );
}


/**
 * Prints help/usage information
 */
static void
print_usage ()
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
                      "This program is published under the GPL v2.\n"), VERSION_FULL
                );
}


/**
 * Prints version information as requested by "xfce4-sensors -V"
 */
static void
print_version ()
{
    printf (_("Xfce4 Sensors %s\n"), VERSION_FULL);
}


/**
 * Initializes the required sensor structures.
 * @return pointer to newly allocated sensors dialog information
 */
static Ptr0<t_sensors_dialog>
initialize_sensors_structures ()
{
    auto sensors = sensors_new (NULL, NULL);
    if (sensors)
    {
        auto dialog = xfce4::make<t_sensors_dialog>(sensors.toPtr());
        dialog->plugin_dialog = false;
        return dialog;
    }
    else
    {
        return nullptr;
    }
}


/**
 * Main routine.
 * @param argc: number of arguments
 * @param argv: array of strings passed as arguments
 */
int
main (int argc, char **argv)
{
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
    Ptr0<t_sensors_dialog> dialog0 = initialize_sensors_structures ();

    if (dialog0)
    {
        auto dialog = dialog0.toPtr();

        /* build main application */
        GtkWidget *window = create_main_window (dialog);

        /* automatic refresh callback */
        dialog->sensors->timeout_id = xfce4::timeout_add (
            dialog->sensors->sensors_refresh_time * 1000,
            [dialog]() {
                refresh_view (dialog);
                return xfce4::TIMEOUT_AGAIN;
            }
        );

        /* show window and run forever */
        gtk_window_resize(GTK_WINDOW(window), 800, 500);
        gtk_widget_show_all(window);
        gtk_main();

        g_source_remove (dialog->sensors->timeout_id);
        free_widgets (dialog);
        gtk_widget_destroy (window);

        return 0;
    }
    else
    {
        return 1;
    }
}
