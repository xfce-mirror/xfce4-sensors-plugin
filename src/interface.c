/* file: interface.c
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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

/* Xfce includes */
#include <libxfce4ui/libxfce4ui.h>

/* Package includes */
#include <tacho.h>
#include <sensors-interface.h>
#include <sensors-interface-common.h>

/* Local includes */
#include "callbacks.h"
#include "interface.h"


#define gtk_hbox_new(homogeneous, spacing) \
        gtk_box_new(GTK_ORIENTATION_HORIZONTAL, spacing)

#define gtk_vbox_new(homogeneous, spacing) \
        gtk_box_new(GTK_ORIENTATION_VERTICAL, spacing)


/* forward declarations */
void add_tachos_box (GtkWidget *ptr_childwidget, t_sensors_dialog *ptr_sensorsdialog);
void add_notebook (GtkWidget *ptr_boxwidget, t_sensors_dialog *ptr_sensorsdialog);

/* actual implementations */

/* -------------------------------------------------------------------------- */
void
add_tachos_box (GtkWidget *ptr_childwidget, t_sensors_dialog *ptr_sensorsdialog)
{
    GtkWidget *ptr_gridwidget;

    ptr_gridwidget = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(ptr_gridwidget), BORDER);
    gtk_grid_set_column_spacing(GTK_GRID(ptr_gridwidget), BORDER);
    gtk_widget_show (ptr_gridwidget);
    ptr_sensorsdialog->sensors->widget_sensors = ptr_gridwidget;
    gtk_container_add (GTK_CONTAINER(ptr_childwidget), ptr_gridwidget);
}


/* -------------------------------------------------------------------------- */
void
add_notebook (GtkWidget *ptr_boxwidget, t_sensors_dialog *ptr_sensorsdialog)
{
    GtkWidget *ptr_notebook, *ptr_childvbox, *ptr_tablabel, *ptr_scrolledwindow, *ptr_fontbutton;

    ptr_notebook = gtk_notebook_new();
    gtk_widget_show (ptr_notebook);

    ptr_childvbox = gtk_vbox_new (FALSE, BORDER);
    gtk_container_set_border_width (GTK_CONTAINER (ptr_childvbox), BORDER);
    gtk_widget_show (ptr_childvbox);

    add_type_box(ptr_childvbox, ptr_sensorsdialog);
    add_sensor_settings_box(ptr_childvbox, ptr_sensorsdialog);
    add_temperature_unit_box(ptr_childvbox, ptr_sensorsdialog);
    add_update_time_box (ptr_childvbox, ptr_sensorsdialog);

    ptr_tablabel = gtk_label_new_with_mnemonic(_("_Overview"));
    gtk_widget_show (ptr_tablabel);
    gtk_notebook_append_page (GTK_NOTEBOOK(ptr_notebook), ptr_childvbox, ptr_tablabel);

    ptr_childvbox = gtk_vbox_new (FALSE, BORDER);
    gtk_container_set_border_width (GTK_CONTAINER (ptr_childvbox), BORDER);
    gtk_widget_show (ptr_childvbox);

    ptr_scrolledwindow  = gtk_scrolled_window_new( NULL,  NULL);
    gtk_box_pack_start(GTK_BOX(ptr_childvbox), ptr_scrolledwindow, TRUE, TRUE, 0);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(ptr_scrolledwindow), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_show_all (ptr_scrolledwindow);

    add_tachos_box (ptr_scrolledwindow, ptr_sensorsdialog);

    ptr_fontbutton = gtk_font_button_new();
    g_signal_connect (G_OBJECT(ptr_fontbutton), "font-set", G_CALLBACK(on_font_set), ptr_sensorsdialog);
    gtk_widget_show (ptr_fontbutton);
    gtk_box_pack_end (GTK_BOX(ptr_childvbox), ptr_fontbutton, FALSE, FALSE, 0);

    ptr_tablabel = gtk_label_new_with_mnemonic(_("_Tachometers"));
    gtk_widget_show (ptr_tablabel);
    gtk_notebook_append_page (GTK_NOTEBOOK(ptr_notebook), ptr_childvbox, ptr_tablabel);

    gtk_box_pack_start(GTK_BOX(ptr_boxwidget), ptr_notebook, TRUE, TRUE, 0);
}


/* -------------------------------------------------------------------------- */
GtkWidget *
create_main_window (t_sensors_dialog *ptr_sensorsdialog)
{
    GtkWidget *ptr_xfcedialog, *ptr_contentareavbox;

    /* start and populate */
    ptr_xfcedialog = xfce_titled_dialog_new_with_buttons(
                _("Sensors Viewer"), // title
                NULL, // parent window
                0, // flags
                "gtk-close", // button text
                GTK_RESPONSE_OK, // response id for previous button
                NULL
            );
    gtk_widget_show (ptr_xfcedialog);

    gtk_window_set_icon_name(GTK_WINDOW(ptr_xfcedialog),"xfce-sensors");

    gtk_container_set_border_width (GTK_CONTAINER (ptr_xfcedialog), BORDER>>1);

    ptr_contentareavbox = gtk_dialog_get_content_area(GTK_DIALOG (ptr_xfcedialog));
    gtk_box_set_spacing(GTK_BOX(ptr_contentareavbox), BORDER>>1);

    gtk_widget_show (ptr_contentareavbox);

    ptr_sensorsdialog->dialog = ptr_xfcedialog;

    ptr_sensorsdialog->myComboBox = gtk_combo_box_text_new();
    gtk_widget_show (ptr_sensorsdialog->myComboBox);

    init_widgets (ptr_sensorsdialog);

    gtk_combo_box_set_active (GTK_COMBO_BOX(ptr_sensorsdialog->myComboBox), 0);

    add_notebook (ptr_contentareavbox, ptr_sensorsdialog);

    gtk_window_set_default_size (GTK_WINDOW(ptr_xfcedialog), ptr_sensorsdialog->sensors->preferred_width, ptr_sensorsdialog->sensors->preferred_height);

    g_signal_connect (G_OBJECT(ptr_xfcedialog), "response", G_CALLBACK(on_main_window_response), ptr_sensorsdialog); // also captures the dialog-destroy event and the closekeybinding-pressed event

    return ptr_xfcedialog;
}
