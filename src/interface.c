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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

/* Xfce includes */
#include <libxfce4ui/libxfce4ui.h>

/* Package includes */
#include <cpu.h>
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
void add_tachos_box (GtkWidget *child, t_sensors_dialog *);
void add_notebook (GtkWidget *box, t_sensors_dialog *sd);

/* actual implementations */

void
add_tachos_box (GtkWidget *child, t_sensors_dialog *sd)
{
    GtkWidget *table;

    //table = gtk_table_new (5, 4, TRUE);
    table = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(table), BORDER);
    gtk_grid_set_column_spacing(GTK_GRID(table), BORDER);
    gtk_widget_show (table);
    sd->sensors->widget_sensors = table;
    gtk_container_add (GTK_CONTAINER(child), table);
}


void
add_notebook (GtkWidget *box, t_sensors_dialog *sd)
{
    GtkWidget *nb, *child, *tab_label, *scr_win, *fontbutton;
    //int res;

    nb = gtk_notebook_new();
    gtk_widget_show (nb);

    child = gtk_vbox_new (FALSE, BORDER);
    gtk_container_set_border_width (GTK_CONTAINER (child), BORDER);
    gtk_widget_show (child);

    add_type_box(child, sd);
    add_sensor_settings_box(child, sd);
    add_temperature_unit_box(child, sd);
    add_update_time_box (child, sd);

    tab_label = gtk_label_new_with_mnemonic(_("_Overview"));
    gtk_widget_show (tab_label);
    gtk_notebook_append_page       (GTK_NOTEBOOK(nb), child, tab_label);

    child = gtk_vbox_new (FALSE, BORDER);
    gtk_container_set_border_width (GTK_CONTAINER (child), BORDER);
    gtk_widget_show (child);

    scr_win  = gtk_scrolled_window_new( NULL,  NULL);
    gtk_box_pack_start(GTK_BOX(child), scr_win, TRUE, TRUE, 0);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scr_win), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_show_all (scr_win);

    add_tachos_box (scr_win, sd);

    fontbutton = gtk_font_button_new();
    g_signal_connect (G_OBJECT(fontbutton), "font-set", G_CALLBACK(on_font_set), sd);
    gtk_widget_show (fontbutton);
    gtk_box_pack_end (GTK_BOX(child), fontbutton, FALSE, FALSE, 0);

    tab_label = gtk_label_new_with_mnemonic(_("_Tachometers"));
    gtk_widget_show (tab_label);
    gtk_notebook_append_page       (GTK_NOTEBOOK(nb), child, tab_label);

    gtk_box_pack_start(GTK_BOX(box), nb, TRUE, TRUE, 0);
}


GtkWidget *
create_main_window (t_sensors_dialog *sd)
{
    GtkWidget *dlg, *vbox;
    //GtkWidget *tacho;
    //GtkWidget *drawing_area;

    /* start and populate */
    dlg = xfce_titled_dialog_new_with_buttons(
                _("Sensors Viewer"), // title
                NULL, // parent window
                0, // flags
                "gtk-close", // button text
                GTK_RESPONSE_OK, // response id for previous button
                NULL
            );
    gtk_widget_show (dlg);

    gtk_window_set_icon_name(GTK_WINDOW(dlg),"xfce-sensors");

    gtk_container_set_border_width (GTK_CONTAINER (dlg), BORDER>>1);

    vbox = gtk_dialog_get_content_area(GTK_DIALOG (dlg));
    gtk_box_set_spacing(GTK_BOX(vbox), BORDER>>1);

    gtk_widget_show (vbox);

    sd->dialog = dlg;

    sd->myComboBox = gtk_combo_box_text_new();
    gtk_widget_show (sd->myComboBox);

    init_widgets (sd);

    gtk_combo_box_set_active (GTK_COMBO_BOX(sd->myComboBox), 0);


    add_notebook (vbox, sd);

    //tacho = gtk_sensorstacho_new();
    //gtk_box_pack_end(GTK_BOX(vbox), tacho, TRUE, TRUE, 4);
    //gtk_widget_set_realized(tacho, TRUE);
    //gtk_widget_show(tacho);

  //drawing_area = gtk_drawing_area_new ();
  //gtk_widget_set_size_request (drawing_area, 100, 100);
  //g_signal_connect (G_OBJECT (drawing_area), "draw",
                    //G_CALLBACK (draw_callback), NULL);
    //gtk_box_pack_end(GTK_BOX(vbox), drawing_area, TRUE, TRUE, 4);

    gtk_window_set_default_size (GTK_WINDOW(dlg), sd->sensors->preferred_width, sd->sensors->preferred_height);


    g_signal_connect (G_OBJECT(dlg), "response", G_CALLBACK(on_main_window_response), sd); // also captures the dialog-destroy event and the closekeybinding-pressed event
    //g_signal_connect (G_OBJECT(dlg), "destroy", G_CALLBACK(on_main_window_response), sd);

    return dlg;
}

gboolean draw_callback(GtkWidget    *widget,
                       cairo_t *cr,
                       gpointer      user_data)
{
  guint width, height;
  GdkRGBA color;
  GtkStyleContext *context;

  context = gtk_widget_get_style_context (widget);

  width = gtk_widget_get_allocated_width (widget);
  height = gtk_widget_get_allocated_height (widget);

  //gtk_render_background (context, cr, 0, 0, width, height);


  gtk_style_context_get_color (context,
                               gtk_style_context_get_state (context),
                               &color);
  color.red = 1.0;
  gdk_cairo_set_source_rgba (cr, &color);

  cairo_arc (cr,
             width / 2.0, height / 2.0,
             MIN (width, height) / 2.0,
             0, 2 * G_PI);

  cairo_fill (cr);

   return FALSE;
}
