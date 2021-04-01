/* File: callbacks.c
 *
 * Copyright 2008-2017 Fabian Nowak (timystery@arcor.de)
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
#include <stdlib.h>

/* Package includes */
#include <sensors-interface.h>
#include <tacho.h>

/* Local includes */
#include "actions.h"
#include "callbacks.h"


/* -------------------------------------------------------------------------- */
void on_font_set (GtkWidget *widget, gpointer data)
{
    g_free (font);
    font = g_strdup(gtk_font_chooser_get_font (GTK_FONT_CHOOSER (widget)));
    refresh_view (data); /* data is pointer to sensors_dialog */
}


/* -------------------------------------------------------------------------- */
void
on_main_window_response (GtkWidget *widget, int response, t_sensors_dialog *dialog)
{
    gtk_main_quit();
}


/* -------------------------------------------------------------------------- */
void
sensor_entry_changed_ (GtkWidget *combobox, t_sensors_dialog *dialog)
{
    gint active_combobox;
    t_chip *chip;

    active_combobox = gtk_combo_box_get_active (GTK_COMBO_BOX(combobox));

    chip = g_ptr_array_index (dialog->sensors->chips, active_combobox);

    gtk_label_set_label (GTK_LABEL (dialog->mySensorLabel), chip->description);

    gtk_tree_view_set_model (GTK_TREE_VIEW (dialog->myTreeView),
                             GTK_TREE_MODEL (dialog->myListStore[active_combobox]));
}

/**
 * Sets the value in the determined column for the given path to a cell in the
 *  GtkTree.
 * @param dialog: pointer to sensors dialog structure
 * @param ptr_str_cellpath: Path in GtkTree
 * @param col_treeview: column in treeview
 * @param value: pointer to value to store
 * @param ptr_ptr_chipfeature: out pointer of determined chipfeature
 * @return index of chipfeature for ptr_str_cellpath
 */
static gint
set_value_in_treemodel_and_return_index_and_feature(t_sensors_dialog *dialog, const gchar *cellpath, gint col_treeview, GValue *value, t_chipfeature **out_feature)
{
    gint active_combobox = -1;
    GtkTreeModel *treemodel;
    GtkTreePath *treepath;
    GtkTreeIter iter_treemodel;
    t_chip *chip;
    t_chipfeature *feature;

    active_combobox = gtk_combo_box_get_active (GTK_COMBO_BOX (dialog->myComboBox));

    treemodel = (GtkTreeModel*) dialog->myListStore [active_combobox];
    treepath = gtk_tree_path_new_from_string (cellpath);

    /* get model iterator */
    gtk_tree_model_get_iter (treemodel, &iter_treemodel, treepath);

    /* set new value */
    chip = g_ptr_array_index(dialog->sensors->chips, active_combobox);

    gtk_tree_store_set_value (GTK_TREE_STORE (treemodel), &iter_treemodel, col_treeview, value);
    chip = g_ptr_array_index(dialog->sensors->chips, active_combobox);

    feature = g_ptr_array_index (chip->chip_features, atoi (cellpath));

    /* clean up */
    gtk_tree_path_free (treepath);

    *out_feature = feature;

    return active_combobox;
}


/* -------------------------------------------------------------------------- */
void
list_cell_text_edited_ (GtkCellRendererText *cell_renderer_text,
                        gchar *cellpath, gchar *new_text, t_sensors_dialog *dialog)
{
    gint active_combobox;
    t_chipfeature *feature = NULL;
    GValue text_string = G_VALUE_INIT;
    GtkWidget *tacho;

    g_value_init(&text_string, G_TYPE_STRING);
    g_value_set_static_string(&text_string, new_text);
    active_combobox = set_value_in_treemodel_and_return_index_and_feature(
        dialog, cellpath, eTreeColumn_Name, &text_string,
        &feature);

    if (feature->name != NULL)
        g_free(feature->name);
    feature->name = g_strdup (new_text);

    /* update panel */
    tacho = dialog->sensors->tachos[active_combobox][atoi(cellpath)];

    if (tacho != NULL)
        gtk_sensorstacho_set_text (GTK_SENSORSTACHO (tacho), new_text);

    refresh_view (dialog);
}


/* -------------------------------------------------------------------------- */
void
list_cell_toggle_ (GtkCellRendererToggle *cell_renderer_toggle, gchar *cellpath,
                   t_sensors_dialog *dialog)
{
    t_chip *chip;
    t_chipfeature *feature;
    gint active_combobox;
    GtkTreeModel *tree_model;
    GtkTreePath *tree_path;
    GtkTreeIter tree_iter;
    gboolean toggle_item;

    active_combobox = gtk_combo_box_get_active(GTK_COMBO_BOX (dialog->myComboBox));

    tree_model = (GtkTreeModel*) dialog->myListStore[active_combobox];
    tree_path = gtk_tree_path_new_from_string (cellpath);

    /* get toggled iter */
    gtk_tree_model_get_iter (tree_model, &tree_iter, tree_path);
    gtk_tree_model_get (tree_model, &tree_iter, 2, &toggle_item, -1);

    /* do something with the value */
    toggle_item ^= 1;

    DBG("toggle item is %d.", toggle_item);

    /* set new value */
    gtk_tree_store_set (GTK_TREE_STORE (tree_model), &tree_iter, eTreeColumn_Show, toggle_item, -1);
    chip = g_ptr_array_index(dialog->sensors->chips, active_combobox);

    feature = g_ptr_array_index(chip->chip_features, atoi(cellpath));

    /* clean up */
    gtk_tree_path_free (tree_path);

    feature->show = toggle_item;

    /* update tooltip and panel widget */
    refresh_view (dialog);
}


/* -------------------------------------------------------------------------- */
void
list_cell_color_edited_ (GtkCellRendererText *cell_renderer_text, const gchar *cellpath,
                         const gchar *new_color, t_sensors_dialog *dialog)
{
    gint active_combobox;
    gboolean has_sharpprefix;
    t_chipfeature *feature;
    GValue color_string = G_VALUE_INIT;
    GtkWidget *tacho;

    /* store new color in appropriate array */
    has_sharpprefix = g_str_has_prefix (new_color, "#");

    if (has_sharpprefix && strlen(new_color) == 7) {
        int i;
        for (i=1; i<7; i++) {
            /* only save hex numbers! */
            if (!g_ascii_isxdigit (new_color[i]))
                return;
        }

        g_value_init (&color_string, G_TYPE_STRING);
        g_value_set_static_string (&color_string, new_color);
        active_combobox = set_value_in_treemodel_and_return_index_and_feature (
            dialog, cellpath, eTreeColumn_Color, &color_string,
            &feature);

        g_free (feature->color_orNull);
        feature->color_orNull = g_strdup (new_color);

        /* update color value */
        tacho = dialog->sensors->tachos[active_combobox][atoi(cellpath)];
        if (tacho)
            gtk_sensorstacho_set_color (GTK_SENSORSTACHO(tacho), new_color);
    }
    else if (strlen (new_color) == 0) {
        g_value_init (&color_string, G_TYPE_STRING);
        g_value_set_static_string (&color_string, new_color);
        active_combobox = set_value_in_treemodel_and_return_index_and_feature (
            dialog, cellpath, eTreeColumn_Color, &color_string,
            &feature);

        g_free (feature->color_orNull);
        feature->color_orNull = NULL;

        /* update color value */
        tacho = dialog->sensors->tachos[active_combobox][atoi(cellpath)];
        if (tacho)
            gtk_sensorstacho_unset_color (GTK_SENSORSTACHO(tacho));
    }
}


/* -------------------------------------------------------------------------- */
void
minimum_changed_ (GtkCellRendererText *cell_renderer_text, gchar *cellpath,
                  gchar *new_value, t_sensors_dialog *dialog)
{
    gint active_combobox;
    t_chipfeature *feature;
    gfloat value;
    GValue value_min = G_VALUE_INIT;

    value = atof (new_value);

    g_value_init(&value_min, G_TYPE_FLOAT);
    g_value_set_float(&value_min, value);
    active_combobox = set_value_in_treemodel_and_return_index_and_feature(
        dialog, cellpath, eTreeColumn_Min, &value_min,
        &feature);

    if (dialog->sensors->scale == FAHRENHEIT)
        value = (value - 32) * 5/9;
    feature->min_value = value;

    /* update panel */
    if (dialog->sensors->tachos[active_combobox][atoi(cellpath)] != NULL)
        refresh_view (dialog);
}


/* -------------------------------------------------------------------------- */
void
maximum_changed_ (GtkCellRendererText *cell_renderer_text, gchar *cellpath,
                  gchar *new_value, t_sensors_dialog *dialog)
{
    gint active_combobox;
    t_chipfeature *feature;
    gfloat value;
    GValue value_max = G_VALUE_INIT;

    value = atof (new_value);

    g_value_init(&value_max, G_TYPE_FLOAT);
    g_value_set_float(&value_max, value);
    active_combobox = set_value_in_treemodel_and_return_index_and_feature(
        dialog, cellpath, eTreeColumn_Max, &value_max,
        &feature);

    if (dialog->sensors->scale == FAHRENHEIT)
      value = (value - 32) * 5/9;
    feature->max_value = value;

    /* update panel */
    if (dialog->sensors->tachos[active_combobox][atoi(cellpath)] != NULL)
      refresh_view (dialog);
}


/* -------------------------------------------------------------------------- */
void
adjustment_value_changed_ (GtkWidget *adjustment_widget, t_sensors_dialog *dialog)
{
    dialog->sensors->sensors_refresh_time =
        (gint) gtk_adjustment_get_value (GTK_ADJUSTMENT (adjustment_widget));

    /* stop the timeout functions ... */
    g_source_remove (dialog->sensors->timeout_id);
    /* ... and start them again */
    dialog->sensors->timeout_id  = g_timeout_add (
        dialog->sensors->sensors_refresh_time * 1000,
        refresh_view_cb, dialog);
}


/* -------------------------------------------------------------------------- */
void
temperature_unit_change_ (GtkWidget *unused, t_sensors_dialog *dialog)
{
    /* toggle celsius-fahrenheit by use of mathematics ;) */
    dialog->sensors->scale = 1 - dialog->sensors->scale;

    /* refresh the panel content */
    reload_listbox (dialog);
}
