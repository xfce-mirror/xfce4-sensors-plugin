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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

/* Xfce includes */
#include <libxfcegui4/libxfcegui4.h>

/* Package includes */
#include <sensors-interface.h>
#include <sensors-interface-common.h>

/* Local includes */
#include "callbacks.h"
#include "interface.h"


GtkWidget *
create_main_window (t_sensors_dialog *sd)
{

    GtkWidget *dlg, *vbox;


    /* start and populate */
    /* dlg = (GtkWidget *) gtk_dialog_new_with_buttons (_("Sensors Viewer"),
                NULL, // anciently: GTK_WINDOW(gtk_get_toplevel(plugin));
                GTK_DIALOG_NO_SEPARATOR, // anciently: | GTK_DIALOG_DESTROY_WITH_PARENT
                GTK_STOCK_CLOSE, GTK_RESPONSE_OK, NULL);

    gtk_container_set_border_width (GTK_CONTAINER (dlg), 2);

    header = (GtkWidget *) xfce_create_framebox (_("View sensor values"), dlg); */
/*    gtk_widget_set_size_request (GTK_BIN (header)->child, -1, 32);
    gtk_container_set_border_width (GTK_CONTAINER (header), BORDER-2); */
    /* gtk_widget_show (header);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), header,
                        FALSE, TRUE, 0); */

    dlg = xfce_titled_dialog_new_with_buttons(
                _("Sensors Viewer"),
                NULL, //GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (plugin))),
                GTK_DIALOG_NO_SEPARATOR,
                GTK_STOCK_CLOSE,
                GTK_RESPONSE_OK,
                NULL
            );

    gtk_window_set_icon_name(GTK_WINDOW(dlg),"xfce-sensors");

    gtk_container_set_border_width (GTK_CONTAINER (dlg), 2);

    vbox = GTK_DIALOG (dlg)->vbox;

    sd->dialog = dlg;

    sd->myComboBox = gtk_combo_box_new_text();

    init_widgets (sd);

    gtk_combo_box_set_active (GTK_COMBO_BOX(sd->myComboBox), 0);

    /* notebook = gtk_notebook_new ();

    gtk_box_pack_start (GTK_BOX(vbox), notebook, TRUE, TRUE, 0);
    gtk_widget_show(notebook);

    add_sensors_frame (notebook, sd); */
    add_type_box(vbox, sd);
    add_sensor_settings_box(vbox, sd);
    add_temperature_unit_box(vbox, sd);
    gtk_widget_show_all (vbox);

    g_signal_connect (dlg, "response",
            G_CALLBACK(on_main_window_response), sd);

    return dlg;
}
