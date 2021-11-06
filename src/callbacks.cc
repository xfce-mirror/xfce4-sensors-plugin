/* callbacks.cc
 * Part of xfce4-sensors-plugin
 *
 * Copyright (c) 2008-2017 Fabian Nowak <timystery@arcor.de>
 * Copyright (c) 2021 Jan Ziak <0xe2.0x9a.0x9b@xfce.org>
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

/* The fixes file has to be included before any other #include directives */
#include "xfce4++/util/fixes.h"

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
void
on_font_set (GtkFontButton *widget, t_sensors_dialog *dialog)
{
    gchar *new_font = gtk_font_chooser_get_font (GTK_FONT_CHOOSER (widget));
    if (new_font)
    {
        font = new_font;
        g_free (new_font);
    }
    else
    {
        font = default_font;
    }
    refresh_view (dialog);
}


/* -------------------------------------------------------------------------- */
void
sensor_entry_changed_ (GtkWidget *combobox, t_sensors_dialog *dialog)
{
    gint active_combobox = gtk_combo_box_get_active (GTK_COMBO_BOX(combobox));

    auto chip = dialog->sensors->chips[active_combobox];

    gtk_label_set_label (GTK_LABEL (dialog->mySensorLabel), chip->description.c_str());

    gtk_tree_view_set_model (GTK_TREE_VIEW (dialog->myTreeView),
                             GTK_TREE_MODEL (dialog->myListStore[active_combobox]));
}

/**
 * Sets the value in the determined column for the given path to a cell in the
 *  GtkTree.
 * @param dialog: pointer to sensors dialog structure
 * @param cellpath: Path in GtkTree
 * @param col_treeview: column in treeview
 * @param value: pointer to value to store
 * @param out_chipfeature: out pointer of determined chipfeature
 * @return index of chipfeature for ptr_str_cellpath
 */
static gint
set_value_in_treemodel_and_return_index_and_feature(t_sensors_dialog *dialog, const gchar *cellpath, gint col_treeview, GValue *value, Ptr0<t_chipfeature> *out_feature)
{
    GtkTreeIter iter_treemodel;

    gint active_combobox = gtk_combo_box_get_active (GTK_COMBO_BOX (dialog->myComboBox));

    auto treemodel = (GtkTreeModel*) dialog->myListStore [active_combobox];
    GtkTreePath *treepath = gtk_tree_path_new_from_string (cellpath);

    /* set value */
    if (gtk_tree_model_get_iter (treemodel, &iter_treemodel, treepath))
        gtk_tree_store_set_value (GTK_TREE_STORE (treemodel), &iter_treemodel, col_treeview, value);

    auto chip = dialog->sensors->chips[active_combobox];
    auto feature = chip->chip_features[atoi(cellpath)];

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
    Ptr0<t_chipfeature> feature;
    GValue text_string = G_VALUE_INIT;

    g_value_init(&text_string, G_TYPE_STRING);
    g_value_set_static_string(&text_string, new_text);

    set_value_in_treemodel_and_return_index_and_feature(dialog, cellpath, eTreeColumn_Name, &text_string, &feature);

    feature->name = new_text;

    /* update panel */
    GtkWidget *tacho = dialog->sensors->tachos[feature.toPtr()];

    if (tacho)
        gtk_sensorstacho_set_text (GTK_SENSORSTACHO (tacho), new_text);

    refresh_view (dialog);
}


/* -------------------------------------------------------------------------- */
void
list_cell_toggle_ (GtkCellRendererToggle *cell_renderer_toggle, gchar *cellpath,
                   t_sensors_dialog *dialog)
{
    GtkTreeIter tree_iter;
    gboolean toggle_item;

    gint active_combobox = gtk_combo_box_get_active(GTK_COMBO_BOX (dialog->myComboBox));

    auto tree_model = (GtkTreeModel*) dialog->myListStore[active_combobox];
    GtkTreePath *tree_path = gtk_tree_path_new_from_string (cellpath);

    /* get toggled iter */
    gtk_tree_model_get_iter (tree_model, &tree_iter, tree_path);
    gtk_tree_model_get (tree_model, &tree_iter, 2, &toggle_item, -1);

    /* do something with the value */
    toggle_item = !toggle_item;

    DBG("toggle item is %d.", toggle_item);

    /* set new value */
    gtk_tree_store_set (GTK_TREE_STORE (tree_model), &tree_iter, eTreeColumn_Show, toggle_item, -1);

    auto chip = dialog->sensors->chips[active_combobox];
    auto feature = chip->chip_features[atoi(cellpath)];

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
    Ptr0<t_chipfeature> feature;
    GValue color_string = G_VALUE_INIT;

    /* store new color in appropriate array */
    gboolean has_sharpprefix = g_str_has_prefix (new_color, "#");

    if (has_sharpprefix && strlen(new_color) == 7) {
        for (size_t i = 1; i < 7; i++) {
            /* only save hex numbers! */
            if (!g_ascii_isxdigit (new_color[i]))
                return;
        }

        g_value_init (&color_string, G_TYPE_STRING);
        g_value_set_static_string (&color_string, new_color);
        set_value_in_treemodel_and_return_index_and_feature (dialog, cellpath, eTreeColumn_Color, &color_string, &feature);

        feature->color_orEmpty = new_color;

        /* update color value */
        GtkWidget *tacho = dialog->sensors->tachos[feature.toPtr()];
        if (tacho)
            gtk_sensorstacho_set_color (GTK_SENSORSTACHO(tacho), new_color);
    }
    else if (strlen (new_color) == 0) {
        g_value_init (&color_string, G_TYPE_STRING);
        g_value_set_static_string (&color_string, new_color);
        set_value_in_treemodel_and_return_index_and_feature (dialog, cellpath, eTreeColumn_Color, &color_string, &feature);

        feature->color_orEmpty = "";

        /* update color value */
        GtkWidget *tacho = dialog->sensors->tachos[feature.toPtr()];
        if (tacho)
            gtk_sensorstacho_unset_color (GTK_SENSORSTACHO(tacho));
    }
}


/* -------------------------------------------------------------------------- */
void
minimum_changed_ (GtkCellRendererText *cell_renderer_text, gchar *cellpath,
                  gchar *new_value, t_sensors_dialog *dialog)
{
    Ptr0<t_chipfeature> feature;
    GValue value_min = G_VALUE_INIT;

    gfloat value = atof (new_value);

    g_value_init(&value_min, G_TYPE_FLOAT);
    g_value_set_float(&value_min, value);
    set_value_in_treemodel_and_return_index_and_feature (dialog, cellpath, eTreeColumn_Min, &value_min, &feature);

    if (dialog->sensors->scale == FAHRENHEIT)
        value = (value - 32) * 5/9;

    feature->min_value = value;

    /* update panel */
    if (dialog->sensors->tachos.count(feature.toPtr()) != 0)
        refresh_view (dialog);
}


/* -------------------------------------------------------------------------- */
void
maximum_changed_ (GtkCellRendererText *cell_renderer_text, gchar *cellpath,
                  gchar *new_value, t_sensors_dialog *dialog)
{
    Ptr0<t_chipfeature> feature;
    GValue value_max = G_VALUE_INIT;

    gfloat value = atof (new_value);

    g_value_init(&value_max, G_TYPE_FLOAT);
    g_value_set_float(&value_max, value);
    set_value_in_treemodel_and_return_index_and_feature (dialog, cellpath, eTreeColumn_Max, &value_max, &feature);

    if (dialog->sensors->scale == FAHRENHEIT)
      value = (value - 32) * 5/9;

    feature->max_value = value;

    /* update panel */
    if (dialog->sensors->tachos.count(feature.toPtr()) != 9)
      refresh_view (dialog);
}


/* -------------------------------------------------------------------------- */
void
adjustment_value_changed_ (GtkAdjustment *adjustment, t_sensors_dialog *dialog)
{
    auto refresh_time = (gint) gtk_adjustment_get_value (adjustment);

    dialog->sensors->sensors_refresh_time = refresh_time;

    /* restart timeout functions */
    g_source_remove (dialog->sensors->timeout_id);
    dialog->sensors->timeout_id = xfce4::timeout_add (refresh_time * 1000, [dialog]() {
        refresh_view (dialog);
        return xfce4::TIMEOUT_AGAIN;
    });
}


/* -------------------------------------------------------------------------- */
void
temperature_unit_change_ (GtkToggleButton*, t_sensors_dialog *dialog)
{
    /* toggle celsius-fahrenheit */
    switch (dialog->sensors->scale)
    {
        case CELSIUS:
            dialog->sensors->scale = FAHRENHEIT;
            break;
        case FAHRENHEIT:
            dialog->sensors->scale = CELSIUS;
            break;
    }

    /* refresh the panel content */
    reload_listbox (dialog);
}
