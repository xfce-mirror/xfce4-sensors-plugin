/* interface.cc
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <libxfce4ui/libxfce4ui.h>

/* Package includes */
#include <tacho.h>
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


/* -------------------------------------------------------------------------- */
static void
add_tachos_box (GtkWidget *child_widget, const Ptr<t_sensors_dialog> &dialog)
{
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing (GTK_GRID(grid), BORDER);
    gtk_grid_set_column_spacing (GTK_GRID(grid), BORDER);
    gtk_widget_show (grid);
    dialog->sensors->widget_sensors = grid;
    gtk_container_add (GTK_CONTAINER(child_widget), grid);
}


/* -------------------------------------------------------------------------- */
static void
add_notebook (GtkWidget *box, const Ptr<t_sensors_dialog> &dialog)
{
    GtkWidget *notebook = gtk_notebook_new();
    gtk_widget_show (notebook);

    GtkWidget *child_vbox = gtk_vbox_new (FALSE, BORDER);
    gtk_container_set_border_width (GTK_CONTAINER (child_vbox), BORDER);
    gtk_widget_show (child_vbox);

    add_type_box (child_vbox, dialog);
    add_sensor_settings_box (child_vbox, dialog);
    add_temperature_unit_box (child_vbox, dialog);
    add_update_time_box (child_vbox, dialog);

    GtkWidget *tab_label = gtk_label_new_with_mnemonic(_("_Overview"));
    gtk_widget_show (tab_label);
    gtk_notebook_append_page (GTK_NOTEBOOK(notebook), child_vbox, tab_label);

    child_vbox = gtk_vbox_new (FALSE, BORDER);
    gtk_container_set_border_width (GTK_CONTAINER (child_vbox), BORDER);
    gtk_widget_show (child_vbox);

    GtkWidget *scrolled_window  = gtk_scrolled_window_new(NULL,  NULL);
    gtk_box_pack_start(GTK_BOX(child_vbox), scrolled_window, TRUE, TRUE, 0);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_show_all (scrolled_window);

    add_tachos_box (scrolled_window, dialog);

    GtkWidget *font_button = gtk_font_button_new_with_font(default_font);
    xfce4::connect_font_set (GTK_FONT_BUTTON (font_button), [dialog](GtkFontButton *button) {
        on_font_set (button, dialog);
    });
    gtk_widget_show (font_button);
    gtk_box_pack_end (GTK_BOX(child_vbox), font_button, FALSE, FALSE, 0);

    tab_label = gtk_label_new_with_mnemonic(_("_Tachometers"));
    gtk_widget_show (tab_label);
    gtk_notebook_append_page (GTK_NOTEBOOK(notebook), child_vbox, tab_label);

    gtk_box_pack_start(GTK_BOX(box), notebook, TRUE, TRUE, 0);
}


/* -------------------------------------------------------------------------- */
GtkWidget *
create_main_window (const Ptr<t_sensors_dialog> &dialog)
{
    /* start and populate */
    GtkWidget *xfce_dialog = xfce_titled_dialog_new_with_buttons(
                _("Sensors Viewer"), // title
                NULL, // parent window
                (GtkDialogFlags) 0,
                "gtk-close", // button text
                GTK_RESPONSE_OK, // response id for previous button
                NULL
            );
    gtk_window_set_icon_name(GTK_WINDOW(xfce_dialog),"xfce-sensors");

    gtk_container_set_border_width (GTK_CONTAINER (xfce_dialog), BORDER>>1);

    GtkWidget *content_area_vbox = gtk_dialog_get_content_area(GTK_DIALOG (xfce_dialog));
    gtk_box_set_spacing(GTK_BOX(content_area_vbox), BORDER>>1);

    gtk_widget_show (content_area_vbox);

    dialog->dialog = xfce_dialog;
    g_object_ref (dialog->dialog);

    dialog->myComboBox = gtk_combo_box_text_new();
    gtk_widget_show (dialog->myComboBox);

    init_widgets (dialog);

    gtk_combo_box_set_active (GTK_COMBO_BOX(dialog->myComboBox), 0);

    add_notebook (content_area_vbox, dialog);

    gtk_window_set_default_size (GTK_WINDOW(xfce_dialog), dialog->sensors->preferred_width, dialog->sensors->preferred_height);
    xfce4::connect_check_resize (GTK_CONTAINER(xfce_dialog), [dialog](GtkContainer*) {
        refresh_tacho_view (dialog);
    });

    // also captures the dialog-destroy event and the closekeybinding-pressed event
    xfce4::connect_response (GTK_DIALOG (xfce_dialog), [](GtkDialog*, gint response) {
        gtk_main_quit ();
    });

    gtk_widget_show (xfce_dialog);

    return xfce_dialog;
}
