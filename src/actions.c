/* File: actions.c
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

/* Note for programmers and editors: Try to use 4 spaces instead of Tab! */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

/* Gtk includes */
#include <glib/gprintf.h>
#include <gtk/gtk.h>

/* Xfce includes */
#include <libxfce4ui/libxfce4ui.h>

/* Package includes */
#include <middlelayer.h>
#include <sensors-interface.h>

/* Local includes */
#include "actions.h"
#include "interface.h"

/* Local definitions */
/**
 *  Default size of the tacho widgets
 */
#define DEFAULT_SIZE_TACHOS 48


/* actual implementations */

/* -------------------------------------------------------------------------- */
/** Internal function to refresh all sensors data for updating the displayed
 * tachos
 * @param ptr_sensors_dialog_structure: Pointer to sensors dialog structure
 */
static void
refresh_sensor_data (t_sensors_dialog *ptr_sensors_dialog_structure)
{
    t_sensors *ptr_sensors_structure;
    int idx_chip, idx_feature, result;
    double val_sensor_feature;
    gchar *str_formatted_sensor_value;
    t_chipfeature *ptr_chipfeature_structure;
    t_chip *ptr_chip_structure;

    ptr_sensors_structure = ptr_sensors_dialog_structure->sensors;

    for (idx_chip=0; idx_chip < ptr_sensors_structure->num_sensorchips; idx_chip++) {
        ptr_chip_structure = (t_chip *) g_ptr_array_index (ptr_sensors_structure->chips, idx_chip);
        g_assert (ptr_chip_structure!=NULL);

        for (idx_feature = 0; idx_feature<ptr_chip_structure->num_features; idx_feature++) {
            ptr_chipfeature_structure = g_ptr_array_index (ptr_chip_structure->chip_features, idx_feature);
            g_assert (ptr_chipfeature_structure!=NULL);

            if ( ptr_chipfeature_structure->valid == TRUE)
            {
                result = sensor_get_value (ptr_chip_structure, ptr_chipfeature_structure->address,
                                                    &val_sensor_feature,
                                                    &(ptr_sensors_structure->suppressmessage));

                if ( result!=0 ) {
                    /* FIXME: either print nothing, or undertake appropriate action,
                     * or pop up a message box. */
                    g_printf ( _("Sensors Viewer:\n"
                    "Seems like there was a problem reading a sensor feature "
                    "value.\nProper proceeding cannot be guaranteed.\n") );
                    break;
                }
                format_sensor_value (ptr_sensors_structure->scale, ptr_chipfeature_structure,
                                     val_sensor_feature, &str_formatted_sensor_value);

                if (ptr_chipfeature_structure->formatted_value != NULL)
                    g_free (ptr_chipfeature_structure->formatted_value);

                ptr_chipfeature_structure->formatted_value = str_formatted_sensor_value;
                ptr_chipfeature_structure->raw_value = val_sensor_feature;

            } /* end if ptr_chipfeature_structure->valid */
        }
    }
}


/* -------------------------------------------------------------------------- */
/**
 * refreshes the tacho view of the application
 * @param ptr_sensors_dialog_structure: pointer to sensors dialog structure
 */
static void
refresh_tacho_view (t_sensors_dialog *ptr_sensors_dialog_structure)
{
    gint idx_chip, idx_feature, row_tacho_table=0, col_tacho_table=0;
    t_sensors *ptr_sensors_structure;
    t_chipfeature *ptr_chipfeature_structure;
    t_chip *ptr_chip_structure;
    GtkWidget *ptr_wdgt_table;
    GtkAllocation allocation;
    gdouble val_fill_degree;
    gchar str_widget_tooltip_text[128];
    gint num_max_cols, num_max_rows;
    SensorsTachoStyle tacho_style = style_MinGYR; /* default as has been for 10 years */

    g_return_if_fail (ptr_sensors_dialog_structure!=NULL);

    ptr_sensors_structure = ptr_sensors_dialog_structure->sensors;

    ptr_wdgt_table = ptr_sensors_structure->widget_sensors;
    g_assert (ptr_wdgt_table != NULL);

    gtk_widget_get_allocation(gtk_widget_get_parent(ptr_wdgt_table), &allocation);
    num_max_cols = (allocation.width - BORDER) / (DEFAULT_SIZE_TACHOS + BORDER);
    num_max_rows = (allocation.height - BORDER) / (DEFAULT_SIZE_TACHOS + BORDER);
    DBG("using max cols/rows: %d/%d.", num_max_cols, num_max_rows);

    for (idx_chip=0; idx_chip < ptr_sensors_structure->num_sensorchips; idx_chip++)
    {
        ptr_chip_structure = (t_chip *) g_ptr_array_index (ptr_sensors_structure->chips, idx_chip);
        g_assert (ptr_chip_structure!=NULL);

        for (idx_feature = 0; idx_feature<ptr_chip_structure->num_features; idx_feature++)
        {
            GtkWidget *ptr_sensorstachowidget = ptr_sensors_structure->tachos[idx_chip][idx_feature];
            GtkSensorsTacho *ptr_sensorstacho = GTK_SENSORSTACHO(ptr_sensorstachowidget);

            if (row_tacho_table>=num_max_rows)
                return;

            ptr_chipfeature_structure = g_ptr_array_index (ptr_chip_structure->chip_features, idx_feature);
            g_assert (ptr_chipfeature_structure!=NULL);

            if (ptr_chipfeature_structure->valid == TRUE && ptr_chipfeature_structure->show == TRUE)
            {

                if (ptr_sensorstachowidget == NULL)
                {
                    GtkOrientation orientation;

                    DBG("Newly adding selected widget from container.");

                    switch (ptr_chipfeature_structure->class) {
                        case VOLTAGE:
                        case POWER:
                        case CURRENT:
                            tacho_style = style_MediumYGB;
                            break;
                        case ENERGY:
                            tacho_style = style_MaxRYG;
                            break;
                        default: // tacho_style = style_MinGYR; // already set per default
                            break;
                    }

                    orientation = (ptr_sensors_structure->plugin_mode != XFCE_PANEL_PLUGIN_MODE_VERTICAL) ? GTK_ORIENTATION_HORIZONTAL : GTK_ORIENTATION_VERTICAL;
                    ptr_sensors_structure->tachos[idx_chip][idx_feature] = ptr_sensorstachowidget = gtk_sensorstacho_new(orientation, DEFAULT_SIZE_TACHOS, tacho_style);
                    ptr_sensorstacho = GTK_SENSORSTACHO(ptr_sensorstachowidget);

                    gtk_sensorstacho_set_text(ptr_sensorstacho, ptr_chipfeature_structure->name);

                    gtk_sensorstacho_set_color(ptr_sensorstacho, ptr_chipfeature_structure->color);
                }

                val_fill_degree = (ptr_chipfeature_structure->raw_value - ptr_chipfeature_structure->min_value) / ( ptr_chipfeature_structure->max_value - ptr_chipfeature_structure->min_value);
                if (val_fill_degree<0.0)
                    val_fill_degree=0.0;
                else if (val_fill_degree>1.0)
                    val_fill_degree=1.0;

                gtk_sensorstacho_set_value(ptr_sensorstacho, val_fill_degree);
                snprintf(str_widget_tooltip_text, 128, "<b>%s</b>\n%s: %s", ptr_chip_structure->sensorId, ptr_chipfeature_structure->name, ptr_chipfeature_structure->formatted_value);
                gtk_widget_set_tooltip_markup (GTK_WIDGET(ptr_sensors_structure->tachos [idx_chip][idx_feature]), str_widget_tooltip_text);


                if ( gtk_widget_get_parent(ptr_sensorstachowidget) == NULL )
                {
                  while (gtk_grid_get_child_at(GTK_GRID(ptr_wdgt_table), col_tacho_table, row_tacho_table) != NULL)
                  {
                    col_tacho_table++;
                    if (col_tacho_table>=num_max_cols) {
                        row_tacho_table++;
                        col_tacho_table = 0;
                    }
                  }
                    gtk_grid_attach(GTK_GRID(ptr_wdgt_table), ptr_sensorstachowidget, col_tacho_table, row_tacho_table, 1, 1);
                    gtk_widget_show (ptr_sensorstachowidget);
                }

                    col_tacho_table++;
                if (col_tacho_table>=num_max_cols) {
                    row_tacho_table++;
                    col_tacho_table = 0;
                }

            }
            else if ( ptr_sensorstachowidget != NULL &&
                gtk_widget_get_parent(ptr_sensorstachowidget) != NULL )
            {
                DBG("Removing deselected widget from container.");
                gtk_container_remove(GTK_CONTAINER(ptr_wdgt_table), ptr_sensorstachowidget);
                if (ptr_sensorstacho->text) {
                    g_free(ptr_sensorstacho->text);
                    ptr_sensorstacho->text = NULL;
                }

                if (ptr_sensorstacho->color) {
                    g_free(ptr_sensorstacho->color);
                    ptr_sensorstacho->color = NULL;
                }
                ptr_sensors_structure->tachos[idx_chip][idx_feature] = NULL;
            }

        }
    }

    /* TODO: Rearrange all widgets in order to distribute evenly */
}


/* -------------------------------------------------------------------------- */
void
refresh_view (t_sensors_dialog *ptr_sensors_dialog)
{
    refresh_sensor_data (ptr_sensors_dialog);
    reload_listbox (ptr_sensors_dialog);
    refresh_tacho_view (ptr_sensors_dialog); /* refresh all the tacho elements in the notebook/table view */
}

gboolean
refresh_view_cb (gpointer user_data)
{
    t_sensors_dialog *ptr_sensors_dialog = user_data;

    refresh_view (ptr_sensors_dialog);
    return TRUE;
}
