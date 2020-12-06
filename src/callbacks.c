/* file: callbacks.c
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

/* Forward declarations */
gint set_value_in_treemodel_and_return_index_and_feature(t_sensors_dialog *ptr_sensorsdialog, gchar *ptr_str_cellpath, gint col_treeview, GValue *ptr_value, t_chipfeature **ptr_ptr_chipfeature);


/* -------------------------------------------------------------------------- */
void on_font_set (GtkWidget *ptr_widget, gpointer data)
{
    if (font!=NULL)
        g_free (font);

    font = g_strdup(gtk_font_chooser_get_font(GTK_FONT_CHOOSER(ptr_widget)));
    refresh_view(data); /* data is pointer to sensors_dialog */
}


/* -------------------------------------------------------------------------- */
void
on_main_window_response (GtkWidget *ptr_dialog, int response, t_sensors_dialog *ptr_sensorsdialog)
{
    TRACE ("enters on_main_window_response");

    gtk_main_quit();

    TRACE ("leaves on_main_window_response");
}


/* -------------------------------------------------------------------------- */
void
sensor_entry_changed_ (GtkWidget *ptr_comboboxwidget, t_sensors_dialog *ptr_sensorsdialog)
{
    gint idx_active_combobox;
    t_chip *ptr_chip;

    TRACE ("enters sensor_entry_changed");

    idx_active_combobox = gtk_combo_box_get_active(GTK_COMBO_BOX (ptr_comboboxwidget));

    ptr_chip = (t_chip *) g_ptr_array_index (ptr_sensorsdialog->sensors->chips,
                                         idx_active_combobox);

    gtk_label_set_label (GTK_LABEL(ptr_sensorsdialog->mySensorLabel), ptr_chip->description);

    gtk_tree_view_set_model (GTK_TREE_VIEW (ptr_sensorsdialog->myTreeView),
                    GTK_TREE_MODEL ( ptr_sensorsdialog->myListStore[idx_active_combobox] ) );

    TRACE ("leaves sensor_entry_changed");
}

/**
 * Sets the value in the determined column for the given path to a cell in the
 *  GtkTree.
 * @param ptr_sensorsdialog: pointer to sensors dialog structure
 * @param ptr_str_cellpath: Path in GtkTree
 * @param col_treeview: column in treeview
 * @param ptr_value: pointer to value to store
 * @param ptr_ptr_chipfeature: out pointer of determined chipfeature
 * @return index of chipfeature for ptr_str_cellpath
 */
gint set_value_in_treemodel_and_return_index_and_feature(t_sensors_dialog *ptr_sensorsdialog, gchar *ptr_str_cellpath, gint col_treeview, GValue *ptr_value, t_chipfeature **ptr_ptr_chipfeature)
{
    gint idx_active_combobox = -1;
    GtkTreeModel *ptr_treemodel;
    GtkTreePath *ptr_treepath;
    GtkTreeIter iter_treemodel;
    t_chip *ptr_chip;
    t_chipfeature *ptr_chipfeature;

    TRACE ("enters list_cell_text_edited");

    idx_active_combobox =
        gtk_combo_box_get_active(GTK_COMBO_BOX (ptr_sensorsdialog->myComboBox));

    ptr_treemodel =
        (GtkTreeModel *) ptr_sensorsdialog->myListStore [idx_active_combobox];
    ptr_treepath = gtk_tree_path_new_from_string (ptr_str_cellpath);

    /* get model iterator */
    gtk_tree_model_get_iter (ptr_treemodel, &iter_treemodel, ptr_treepath);

    /* set new value */
    ptr_chip = (t_chip *) g_ptr_array_index(ptr_sensorsdialog->sensors->chips, idx_active_combobox);

    gtk_tree_store_set_value (GTK_TREE_STORE (ptr_treemodel), &iter_treemodel, col_treeview, ptr_value);
    ptr_chip = (t_chip *) g_ptr_array_index(ptr_sensorsdialog->sensors->chips, idx_active_combobox);

    ptr_chipfeature = (t_chipfeature *) g_ptr_array_index (ptr_chip->chip_features,
                                                        atoi(ptr_str_cellpath));

    /* clean up */
    gtk_tree_path_free (ptr_treepath);

    *ptr_ptr_chipfeature = ptr_chipfeature;

    return idx_active_combobox;
}


/* -------------------------------------------------------------------------- */
void
list_cell_text_edited_ (GtkCellRendererText *ptr_cellrenderertext,
                      gchar *ptr_str_cellpath, gchar *ptr_str_newtext, t_sensors_dialog *ptr_sensorsdialog)
{
    gint idx_active_combobox;
    t_chipfeature *ptr_chipfeature = NULL;
    GValue val_textstring = G_VALUE_INIT;
    GtkWidget *ptr_tacho;

    TRACE ("enters list_cell_text_edited");

    g_value_init(&val_textstring, G_TYPE_STRING);
    g_value_set_static_string(&val_textstring, ptr_str_newtext);
    idx_active_combobox = set_value_in_treemodel_and_return_index_and_feature(
        ptr_sensorsdialog, ptr_str_cellpath, eTreeColumn_Name, &val_textstring,
        &ptr_chipfeature);

        if (ptr_chipfeature->name != NULL)
            g_free(ptr_chipfeature->name);
        ptr_chipfeature->name = g_strdup (ptr_str_newtext);
    /* } */

    /* update panel */
    ptr_tacho = ptr_sensorsdialog->sensors->tachos [idx_active_combobox][atoi(ptr_str_cellpath)];

    if (ptr_tacho!=NULL)
        gtk_sensorstacho_set_text (GTK_SENSORSTACHO(ptr_tacho), ptr_str_newtext);

    refresh_view ((gpointer) ptr_sensorsdialog);

    TRACE ("leaves list_cell_text_edited");
}


/* -------------------------------------------------------------------------- */
void
list_cell_toggle_ (GtkCellRendererToggle *ptr_cellrenderertoggle, gchar *ptr_str_cellpath,
                  t_sensors_dialog *ptr_sensorsdialog)
{
    t_chip *ptr_chip;
    t_chipfeature *ptr_chipfeature;
    gint idx_active_combobox;
    GtkTreeModel *ptr_treemodel;
    GtkTreePath *ptr_treepath;
    GtkTreeIter treeiter;
    gboolean toggle_item;

    TRACE ("enters list_cell_toggle");

    idx_active_combobox =
        gtk_combo_box_get_active(GTK_COMBO_BOX (ptr_sensorsdialog->myComboBox));

    ptr_treemodel = (GtkTreeModel *) ptr_sensorsdialog->myListStore[idx_active_combobox];
    ptr_treepath = gtk_tree_path_new_from_string (ptr_str_cellpath);

    /* get toggled iter */
    gtk_tree_model_get_iter (ptr_treemodel, &treeiter, ptr_treepath);
    gtk_tree_model_get (ptr_treemodel, &treeiter, 2, &toggle_item, -1);

    /* do something with the value */
    toggle_item ^= 1;

    DBG("toggle item is %d.", toggle_item);

    /* set new value */
    gtk_tree_store_set (GTK_TREE_STORE (ptr_treemodel), &treeiter, eTreeColumn_Show, toggle_item, -1);
    ptr_chip = (t_chip *) g_ptr_array_index(ptr_sensorsdialog->sensors->chips, idx_active_combobox);

    ptr_chipfeature = (t_chipfeature *) g_ptr_array_index(ptr_chip->chip_features, atoi(ptr_str_cellpath));

    /* clean up */
    gtk_tree_path_free (ptr_treepath);

    ptr_chipfeature->show = toggle_item;

    /* update tooltip and panel widget */
    refresh_view ((gpointer) ptr_sensorsdialog);

    TRACE ("leaves list_cell_toggle");
}


/* -------------------------------------------------------------------------- */
void
list_cell_color_edited_ (GtkCellRendererText *ptr_cellrenderertext, gchar *ptr_str_cellpath,
                       gchar *ptr_str_newcolor, t_sensors_dialog *ptr_sensorsdialog)
{
    gint idx_active_combobox;
    gboolean has_sharpprefix;
    t_chipfeature *ptr_chipfeature;
    GValue val_colorstring = G_VALUE_INIT;
    GtkWidget *ptr_tacho;

    TRACE ("enters list_cell_color_edited");

    /* store new color in appropriate array */
    has_sharpprefix = g_str_has_prefix (ptr_str_newcolor, "#");

    if (has_sharpprefix && strlen(ptr_str_newcolor) == 7) {
        int i;
        for (i=1; i<7; i++) {
            /* only save hex numbers! */
            if ( ! g_ascii_isxdigit (ptr_str_newcolor[i]) )
                return;
        }

        g_value_init(&val_colorstring, G_TYPE_STRING);
        g_value_set_static_string(&val_colorstring, ptr_str_newcolor);
        idx_active_combobox = set_value_in_treemodel_and_return_index_and_feature(
        ptr_sensorsdialog, ptr_str_cellpath, eTreeColumn_Color, &val_colorstring,
        &ptr_chipfeature);

        if (ptr_chipfeature->color!=NULL)
            g_free (ptr_chipfeature->color);

        ptr_chipfeature->color = g_strdup(ptr_str_newcolor);

        /* update color value */
        ptr_tacho = ptr_sensorsdialog->sensors->tachos [idx_active_combobox][atoi(ptr_str_cellpath)];

        if (ptr_tacho!=NULL)
            gtk_sensorstacho_set_color(GTK_SENSORSTACHO(ptr_sensorsdialog->sensors->tachos[idx_active_combobox][atoi(ptr_str_cellpath)]), ptr_str_newcolor);
    }

    TRACE ("leaves list_cell_color_edited");
}


/* -------------------------------------------------------------------------- */
void
minimum_changed_ (GtkCellRendererText *cellrenderertext, gchar *ptr_str_cellpath,
                 gchar *new_value, t_sensors_dialog *ptr_sensorsdialog)
{
    gint idx_active_combobox;
    t_chipfeature *ptr_chipfeature;
    gfloat val_float;
    GValue val_minimum = G_VALUE_INIT;

    TRACE ("enters minimum_changed");

    val_float = atof (new_value);

    g_value_init(&val_minimum, G_TYPE_FLOAT);
    g_value_set_float(&val_minimum, val_float);
    idx_active_combobox = set_value_in_treemodel_and_return_index_and_feature(
        ptr_sensorsdialog, ptr_str_cellpath, eTreeColumn_Min, &val_minimum,
        &ptr_chipfeature);

    if (ptr_sensorsdialog->sensors->scale==FAHRENHEIT)
        val_float = (val_float - 32) * 5/9;
    ptr_chipfeature->min_value = val_float;

    /* update panel */
    if (ptr_sensorsdialog->sensors->tachos [idx_active_combobox][atoi(ptr_str_cellpath)]!=NULL)
        refresh_view ((gpointer) ptr_sensorsdialog);

    TRACE ("leaves minimum_changed");
}


/* -------------------------------------------------------------------------- */
void
maximum_changed_ (GtkCellRendererText *cellrenderertext, gchar *ptr_str_cellpath,
            gchar *new_value, t_sensors_dialog *ptr_sensorsdialog)
{
    gint idx_active_combobox;
    t_chipfeature *ptr_chipfeature;
    gfloat val_float;
    GValue val_maximum = G_VALUE_INIT;

    TRACE ("enters maximum_changed");

    val_float = atof (new_value);

    g_value_init(&val_maximum, G_TYPE_FLOAT);
    g_value_set_float(&val_maximum, val_float);
    idx_active_combobox = set_value_in_treemodel_and_return_index_and_feature(
        ptr_sensorsdialog, ptr_str_cellpath, eTreeColumn_Max, &val_maximum,
        &ptr_chipfeature);

    if (ptr_sensorsdialog->sensors->scale==FAHRENHEIT)
      val_float = (val_float - 32) * 5/9;
    ptr_chipfeature->max_value = val_float;

    /* update panel */
    if (ptr_sensorsdialog->sensors->tachos [idx_active_combobox][atoi(ptr_str_cellpath)]!=NULL)
      refresh_view ((gpointer) ptr_sensorsdialog);

    TRACE ("leaves maximum_changed");
}


/* -------------------------------------------------------------------------- */
void
adjustment_value_changed_ (GtkWidget *ptr_adjustmentwidget, t_sensors_dialog* ptr_sensorsdialog)
{
    TRACE ("enters adjustment_value_changed ");

    ptr_sensorsdialog->sensors->sensors_refresh_time =
        (gint) gtk_adjustment_get_value ( GTK_ADJUSTMENT (ptr_adjustmentwidget) );

    /* stop the timeout functions ... */
    g_source_remove (ptr_sensorsdialog->sensors->timeout_id);
    /* ... and start them again */
    ptr_sensorsdialog->sensors->timeout_id  = g_timeout_add (
        ptr_sensorsdialog->sensors->sensors_refresh_time * 1000,
        refresh_view_cb, ptr_sensorsdialog);

    TRACE ("leaves adjustment_value_changed ");
}


/* -------------------------------------------------------------------------- */
void
temperature_unit_change_ (GtkWidget *ptr_unused, t_sensors_dialog *ptr_sensorsdialog)
{
    TRACE ("enters temperature_unit_change ");

    /* toggle celsius-fahrenheit by use of mathematics ;) */
    ptr_sensorsdialog->sensors->scale = 1 - ptr_sensorsdialog->sensors->scale;

    /* refresh the panel content */
    reload_listbox (ptr_sensorsdialog);

    TRACE ("laeves temperature_unit_change ");
}
