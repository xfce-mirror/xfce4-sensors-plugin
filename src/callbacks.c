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

/* Global includes */
#include <stdlib.h>

/* Xfce includes */
#include <libxfce4ui/libxfce4ui.h>

/* Package includes */
#include <sensors-interface.h>
#include <tacho.h> /* contains "extern gchar * font" */

/* Local includes */
#include "callbacks.h"
#include "actions.h"


void on_font_set (GtkWidget *widget, gpointer data)
{
    if (font!=NULL)
        g_free (font);

    font = g_strdup(gtk_font_button_get_font_name(GTK_FONT_BUTTON(widget)));
    refresh_view(data); /* data is pointer to sensors_dialog */
}


void
on_main_window_response (GtkWidget *dlg, int response, t_sensors_dialog *sd)
{
    TRACE ("enters on_main_window_response");

    DBG("quitting gtk main routine");
        gtk_main_quit();

    // dialog OK button or close keybinding or window close button
    if (response==GTK_RESPONSE_OK) // || response==GTK_RESPONSE_DELETE_EVENT)
    {
        //xfce_titled_dialog_close(GTK_DIALOG(dlg)); // not implemented in libxfcegui4/xfce-titled-dialog.h rev 21491 -- stay compatible

        // g_free (sd->sensors)
        //g_free(sd);

        //gtk_widget_destroy (sd->sensors->widget_sensors);

        //gtk_widget_destroy (sd->dialog);

        //gtk_widget_destroy (sd->dialog);
    }

    TRACE ("leaves on_main_window_response");
}


void
sensor_entry_changed_ (GtkWidget *widget, t_sensors_dialog *sd)
{
    gint gtk_combo_box_active;
    t_chip *chip;

    TRACE ("enters sensor_entry_changed");

    gtk_combo_box_active = gtk_combo_box_get_active(GTK_COMBO_BOX (widget));

    chip = (t_chip *) g_ptr_array_index (sd->sensors->chips,
                                         gtk_combo_box_active);

    gtk_label_set_label (GTK_LABEL(sd->mySensorLabel), chip->description);

    gtk_tree_view_set_model (GTK_TREE_VIEW (sd->myTreeView),
                    GTK_TREE_MODEL ( sd->myListStore[gtk_combo_box_active] ) );

    TRACE ("leaves sensor_entry_changed");
}


void
list_cell_text_edited_ (GtkCellRendererText *cellrenderertext,
                      gchar *path_str, gchar *new_text, t_sensors_dialog *sd)
{
    gint gtk_combo_box_active;
    GtkTreeModel *model;
    GtkTreePath *path;
    GtkTreeIter iter;
    t_chip *chip;
    t_chipfeature *chipfeature;

    TRACE ("enters list_cell_text_edited");

    /* if (sd->sensors->display_values_graphically == TRUE) {
        DBG("removing graphical panel");
        sensors_remove_graphical_panel(sd->sensors);
        DBG("done removing grap. panel");
    } */
    /* better send a D-BUS message instead */

    gtk_combo_box_active =
        gtk_combo_box_get_active(GTK_COMBO_BOX (sd->myComboBox));

    model =
        (GtkTreeModel *) sd->myListStore [gtk_combo_box_active];
    path = gtk_tree_path_new_from_string (path_str);

    /* get model iterator */
    gtk_tree_model_get_iter (model, &iter, path);

    /* set new value */
    gtk_tree_store_set (GTK_TREE_STORE (model), &iter, 0, new_text, -1);
    chip = (t_chip *) g_ptr_array_index(sd->sensors->chips, gtk_combo_box_active);

    /* if (chip->type!=ACPI) { */ /* No Bug. The cryptic filesystem names are
                                  needed for the update in ACPI and hddtemp. */
        chipfeature = (t_chipfeature *) g_ptr_array_index (chip->chip_features,
                                                            atoi(path_str));
        g_free(chipfeature->name);
        chipfeature->name = g_strdup (new_text);
    /* } */

    /* clean up */
    gtk_tree_path_free (path);

    /* update panel */
    /* sensors_show_panel ((gpointer) sd->sensors); */
    /* better send a D-BUS message instead or alter the configuration file */

    if (sd->sensors->tachos [gtk_combo_box_active][atoi(path_str)]!=NULL)
      gtk_sensorstacho_set_text (GTK_SENSORSTACHO(sd->sensors->tachos [gtk_combo_box_active][atoi(path_str)]), new_text);

    refresh_view ((gpointer) sd);

    TRACE ("leaves list_cell_text_edited");
}


void
list_cell_toggle_ (GtkCellRendererToggle *cell, gchar *path_str,
                  t_sensors_dialog *sd)
{
    t_chip *chip;
    t_chipfeature *chipfeature;
    gint gtk_combo_box_active;
    GtkTreeModel *model;
    GtkTreePath *path;
    GtkTreeIter iter;
    gboolean toggle_item; //, res;
    //GtkWidget *tacho;

    TRACE ("enters list_cell_toggle");

    //if (sd->sensors->display_values_type != DISPLAY_TEXT) {
        //sensors_remove_graphical_panel(sd->sensors);
    //}
    gtk_combo_box_active =
        gtk_combo_box_get_active(GTK_COMBO_BOX (sd->myComboBox));

    model = (GtkTreeModel *) sd->myListStore[gtk_combo_box_active];
    path = gtk_tree_path_new_from_string (path_str);

    /* get toggled iter */
    gtk_tree_model_get_iter (model, &iter, path);
    gtk_tree_model_get (model, &iter, 2, &toggle_item, -1);

    /* do something with the value */
    toggle_item ^= 1;

    /* remove previously existing cpu widget from memory */
    if (toggle_item==FALSE)
    {
      //tacho = sd->sensors->tachos [gtk_combo_box_active][atoi(path_str)];
      // gtk_container_remove also destroys the widget when there are no more references!
      gtk_container_remove(GTK_CONTAINER(sd->sensors->widget_sensors),
                                      sd->sensors->tachos [gtk_combo_box_active][atoi(path_str)]);
      //gtk_widget_destroy(sd->sensors->tachos [gtk_combo_box_active][atoi(path_str)]);
      sd->sensors->tachos [gtk_combo_box_active][atoi(path_str)] = NULL;
     }
     //else
     //{
       ////if (sd->sensors->tachos [gtk_combo_box_active][atoi(path_str)]==NULL)
                //sd->sensors->tachos[gtk_combo_box_active][atoi(path_str)] = gtk_sensorstacho_new();
     //}

    /* set new value */
    gtk_tree_store_set (GTK_TREE_STORE (model), &iter, 2, toggle_item, -1);
    chip = (t_chip *) g_ptr_array_index(sd->sensors->chips, gtk_combo_box_active);

    chipfeature = (t_chipfeature *) g_ptr_array_index(chip->chip_features, atoi(path_str));

    chipfeature->show = toggle_item;

    /* clean up */
    gtk_tree_path_free (path);

    /* update tooltip and panel widget */
    refresh_view ((gpointer) sd);

    TRACE ("leaves list_cell_toggle");//~
}

void
list_cell_color_edited_ (GtkCellRendererText *cellrenderertext, gchar *path_str,
                       gchar *new_color, t_sensors_dialog *sd)
{
      gint gtk_combo_box_active;
    GtkTreeModel *model;
    GtkTreePath *path;
    GtkTreeIter iter;
    gboolean hexColor;
    t_chip *chip;
    t_chipfeature *chipfeature;
    //int res;

    TRACE ("enters list_cell_color_edited");

    /* store new color in appropriate array */
    hexColor = g_str_has_prefix (new_color, "#");

    if (hexColor && strlen(new_color) == 7) {
        int i;
        for (i=1; i<7; i++) {
            /* only save hex numbers! */
            if ( ! g_ascii_isxdigit (new_color[i]) )
                return;
        }

        gtk_combo_box_active =
            gtk_combo_box_get_active(GTK_COMBO_BOX (sd->myComboBox));

        /* get model and path */
        model = (GtkTreeModel *) sd->myListStore
            [gtk_combo_box_active];
        path = gtk_tree_path_new_from_string (path_str);

        /* get model iterator */
        gtk_tree_model_get_iter (model, &iter, path);

        /* set new value */
        gtk_tree_store_set (GTK_TREE_STORE (model), &iter, 3, new_color, -1);
        chip = (t_chip *) g_ptr_array_index(sd->sensors->chips, gtk_combo_box_active);

        chipfeature = (t_chipfeature *) g_ptr_array_index(chip->chip_features, atoi(path_str));
        g_free (chipfeature->color);
        chipfeature->color = g_strdup(new_color);

        /* clean up */
        gtk_tree_path_free (path);

        /* update panel */
        //sensors_show_panel ((gpointer) sd->sensors);
        //res = refresh_view ((gpointer) sd);
        if (sd->sensors->tachos [gtk_combo_box_active][atoi(path_str)]!=NULL)
            gtk_sensorstacho_set_color(GTK_SENSORSTACHO(sd->sensors->tachos[gtk_combo_box_active][atoi(path_str)]), new_color);

    }

    TRACE ("leaves list_cell_color_edited");
}

void
minimum_changed_ (GtkCellRendererText *cellrenderertext, gchar *path_str,
                 gchar *new_value, t_sensors_dialog *sd)
{
    gint gtk_combo_box_active;
    float  value;
    GtkTreeModel *model;
    GtkTreePath *path;
    GtkTreeIter iter;
    t_chip *chip;
    t_chipfeature *chipfeature;
    //int res;

    TRACE ("enters minimum_changed");

    value = atof (new_value);

    gtk_combo_box_active =
        gtk_combo_box_get_active(GTK_COMBO_BOX (sd->myComboBox));

    /* get model and path */
    model = (GtkTreeModel *) sd->myListStore
        [gtk_combo_box_active];
    path = gtk_tree_path_new_from_string (path_str);

    /* get model iterator */
    gtk_tree_model_get_iter (model, &iter, path);

    /* set new value according to chosen scale */
    gtk_tree_store_set (GTK_TREE_STORE (model), &iter, 4, value, -1);
    chip = (t_chip *) g_ptr_array_index(sd->sensors->chips, gtk_combo_box_active);

    chipfeature = (t_chipfeature *) g_ptr_array_index(chip->chip_features, atoi(path_str));
    if (sd->sensors->scale==FAHRENHEIT)
        value = (value -32 ) * 5/9;
    chipfeature->min_value = value;

    /* clean up */
    gtk_tree_path_free (path);

    /* update panel */
    if (sd->sensors->tachos [gtk_combo_box_active][atoi(path_str)]!=NULL)
        refresh_view ((gpointer) sd);

    TRACE ("leaves minimum_changed");
}

void
maximum_changed_ (GtkCellRendererText *cellrenderertext, gchar *path_str,
            gchar *new_value, t_sensors_dialog *sd)
{
    gint gtk_combo_box_active;
    float value;
    GtkTreeModel *model;
    GtkTreePath *path;
    GtkTreeIter iter;
    t_chip *chip;
    t_chipfeature *chipfeature;
    //int res;

    TRACE ("enters maximum_changed");

    value = atof (new_value);

    gtk_combo_box_active =
        gtk_combo_box_get_active(GTK_COMBO_BOX (sd->myComboBox));

    /* get model and path */
    model = (GtkTreeModel *) sd->myListStore
        [gtk_combo_box_active];
    path = gtk_tree_path_new_from_string (path_str);

    /* get model iterator */
    gtk_tree_model_get_iter (model, &iter, path);

    /* set new value according to chosen scale */
    gtk_tree_store_set (GTK_TREE_STORE (model), &iter, 5, value, -1);
    chip = (t_chip *) g_ptr_array_index(sd->sensors->chips, gtk_combo_box_active);

    chipfeature = (t_chipfeature *) g_ptr_array_index(chip->chip_features, atoi(path_str));
    if (sd->sensors->scale==FAHRENHEIT)
      value = (value -32 ) * 5/9;
    chipfeature->max_value = value;

    /* clean up */
    gtk_tree_path_free (path);

    /* update panel */
    if (sd->sensors->tachos [gtk_combo_box_active][atoi(path_str)]!=NULL)
      //res =
            refresh_view ((gpointer) sd);

    TRACE ("leaves maximum_changed");
}

void
adjustment_value_changed_ (GtkWidget *widget, t_sensors_dialog* sd)
{
    TRACE ("enters adjustment_value_changed ");

    sd->sensors->sensors_refresh_time =
        (gint) gtk_adjustment_get_value ( GTK_ADJUSTMENT (widget) );

    /* stop the timeout functions ... */
    g_source_remove (sd->sensors->timeout_id);
    /* ... and start them again */
    sd->sensors->timeout_id  = g_timeout_add (
        sd->sensors->sensors_refresh_time * 1000,
        (GSourceFunc) refresh_view, (gpointer) sd);

    TRACE ("leaves adjustment_value_changed ");
}

void
temperature_unit_change_ (GtkWidget *widget, t_sensors_dialog *sd)
{
    TRACE ("enters temperature_unit_change ");

    /* toggle celsius-fahrenheit by use of mathematics ;) */
    sd->sensors->scale = 1 - sd->sensors->scale;

    /* refresh the panel content */

    reload_listbox (sd);

    TRACE ("laeves temperature_unit_change ");
}
