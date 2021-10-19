/* actions.cc
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib/gprintf.h>
#include <gtk/gtk.h>
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
 * @param dialog: Pointer to sensors dialog structure
 */
static void
refresh_sensor_data (t_sensors_dialog *dialog)
{
    t_sensors *sensors = dialog->sensors;

    for (gint idx_chip=0; idx_chip < sensors->num_sensorchips; idx_chip++) {
        auto chip = (t_chip*) g_ptr_array_index (sensors->chips, idx_chip);
        g_assert (chip!=NULL);

        for (gint idx_feature = 0; idx_feature<chip->num_features; idx_feature++) {
            auto feature =  (t_chipfeature*) g_ptr_array_index (chip->chip_features, idx_feature);
            g_assert (feature!=NULL);

            if (feature->valid)
            {
                double feature_value;
                int result = sensor_get_value (chip, feature->address, &feature_value, &sensors->suppressmessage);

                if ( result!=0 ) {
                    /* FIXME: either print nothing, or undertake appropriate action,
                     * or pop up a message box. */
                    g_printf ( _("Sensors Viewer:\n"
                    "Seems like there was a problem reading a sensor feature "
                    "value.\nProper proceeding cannot be guaranteed.\n") );
                    break;
                }

                feature->formatted_value = format_sensor_value (sensors->scale, feature, feature_value);
                feature->raw_value = feature_value;

            }
        }
    }
}


/* -------------------------------------------------------------------------- */
/**
 * refreshes the tacho view of the application
 * @param ptr_sensors_dialog_structure: pointer to sensors dialog structure
 */
static void
refresh_tacho_view (t_sensors_dialog *dialog)
{
    gint row_tacho_table = 0, col_tacho_table = 0;
    t_sensors *sensors;
    GtkWidget *wdgt_table;
    GtkAllocation allocation;
    gint num_max_cols, num_max_rows;
    SensorsTachoStyle tacho_style = style_MinGYR; /* default as has been for 10 years */

    g_return_if_fail (dialog != NULL);

    sensors = dialog->sensors;

    wdgt_table = sensors->widget_sensors;
    g_assert (wdgt_table != NULL);

    gtk_widget_get_allocation(gtk_widget_get_parent(wdgt_table), &allocation);
    num_max_cols = (allocation.width - BORDER) / (DEFAULT_SIZE_TACHOS + BORDER);
    num_max_rows = (allocation.height - BORDER) / (DEFAULT_SIZE_TACHOS + BORDER);
    DBG("using max cols/rows: %d/%d.", num_max_cols, num_max_rows);

    for (gint idx_chip=0; idx_chip < sensors->num_sensorchips; idx_chip++)
    {
        auto chip = (t_chip*) g_ptr_array_index (sensors->chips, idx_chip);
        g_assert (chip!=NULL);

        for (gint idx_feature = 0; idx_feature<chip->num_features; idx_feature++)
        {
            GtkWidget *ptr_sensorstachowidget = sensors->tachos[idx_chip][idx_feature];
            GtkSensorsTacho *ptr_sensorstacho = GTK_SENSORSTACHO(ptr_sensorstachowidget);

            if (row_tacho_table >= num_max_rows)
                return;

            auto feature = (t_chipfeature*) g_ptr_array_index (chip->chip_features, idx_feature);
            g_assert (feature!=NULL);

            if (feature->valid && feature->show)
            {
                gdouble fill_degree;
                gchar widget_tooltip_text[128];

                if (ptr_sensorstachowidget == NULL)
                {
                    GtkOrientation orientation;

                    DBG("Newly adding selected widget from container.");

                    switch (feature->cls) {
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

                    orientation = (sensors->plugin_mode != XFCE_PANEL_PLUGIN_MODE_VERTICAL) ? GTK_ORIENTATION_HORIZONTAL : GTK_ORIENTATION_VERTICAL;
                    sensors->tachos[idx_chip][idx_feature] = ptr_sensorstachowidget = gtk_sensorstacho_new(orientation, DEFAULT_SIZE_TACHOS, tacho_style);
                    ptr_sensorstacho = GTK_SENSORSTACHO(ptr_sensorstachowidget);

                    gtk_sensorstacho_set_color (ptr_sensorstacho, feature->color_orEmpty.c_str());
                    gtk_sensorstacho_set_text (ptr_sensorstacho, feature->name.c_str());
                }

                fill_degree = (feature->raw_value - feature->min_value) / ( feature->max_value - feature->min_value);
                if (fill_degree<0.0)
                    fill_degree=0.0;
                else if (fill_degree>1.0)
                    fill_degree=1.0;

                gtk_sensorstacho_set_value(ptr_sensorstacho, fill_degree);
                snprintf(widget_tooltip_text, sizeof (widget_tooltip_text), "<b>%s</b>\n%s: %s",
                         chip->sensorId.c_str(), feature->name.c_str(), feature->formatted_value.c_str());
                gtk_widget_set_tooltip_markup (GTK_WIDGET(sensors->tachos [idx_chip][idx_feature]), widget_tooltip_text);


                if (gtk_widget_get_parent(ptr_sensorstachowidget) == NULL)
                {
                  while (gtk_grid_get_child_at(GTK_GRID(wdgt_table), col_tacho_table, row_tacho_table) != NULL)
                  {
                    col_tacho_table++;
                    if (col_tacho_table>=num_max_cols) {
                        row_tacho_table++;
                        col_tacho_table = 0;
                    }
                  }
                    gtk_grid_attach(GTK_GRID(wdgt_table), ptr_sensorstachowidget, col_tacho_table, row_tacho_table, 1, 1);
                    gtk_widget_show (ptr_sensorstachowidget);
                }

                col_tacho_table++;
                if (col_tacho_table>=num_max_cols) {
                    row_tacho_table++;
                    col_tacho_table = 0;
                }
            }
            else if (ptr_sensorstachowidget != NULL && gtk_widget_get_parent(ptr_sensorstachowidget) != NULL)
            {
                DBG("Removing deselected widget from container.");
                gtk_container_remove(GTK_CONTAINER(wdgt_table), ptr_sensorstachowidget);

                g_free (ptr_sensorstacho->color_orNull);
                g_free (ptr_sensorstacho->text);
                ptr_sensorstacho->color_orNull = NULL;
                ptr_sensorstacho->text = NULL;

                sensors->tachos[idx_chip][idx_feature] = NULL;
            }

        }
    }

    /* TODO: Rearrange all widgets in order to distribute evenly */
}


/* -------------------------------------------------------------------------- */
void
refresh_view (t_sensors_dialog *dialog)
{
    refresh_sensor_data (dialog);
    reload_listbox (dialog);
    refresh_tacho_view (dialog); /* refresh all the tacho elements in the notebook/table view */
}

gboolean
refresh_view_cb (gpointer user_data)
{
    auto dialog = (t_sensors_dialog*) user_data;
    refresh_view (dialog);
    return TRUE;
}
