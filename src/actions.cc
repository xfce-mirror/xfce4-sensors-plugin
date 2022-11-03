/* actions.cc
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
refresh_sensor_data (const Ptr<t_sensors_dialog> &dialog)
{
    auto sensors = dialog->sensors;
    for (auto chip : sensors->chips)
    {
        for (auto feature : chip->chip_features)
        {
            if (feature->valid)
            {
                Optional<double> feature_value = sensor_get_value (chip, feature->address, &sensors->suppressmessage);
                if (feature_value.has_value())
                {
                    feature->formatted_value = format_sensor_value (sensors->scale, feature, feature_value.value());
                    feature->raw_value = feature_value.value();
                }
                else {
                    /* FIXME: either print nothing, or undertake appropriate action, or pop up a message box. */
                    g_printf ( _("Sensors Viewer:\n"
                                 "Seems like there was a problem reading a sensor feature "
                                 "value.\nProper proceeding cannot be guaranteed.\n") );
                    break;
                }
            }
        }
    }
}


/* -------------------------------------------------------------------------- */
/**
 * refreshes the tacho view of the application
 * @param ptr_sensors_dialog_structure: pointer to sensors dialog structure
 */
void
refresh_tacho_view (const Ptr<t_sensors_dialog> &dialog)
{
    auto sensors = dialog->sensors;

    GtkWidget *wdgt_table = sensors->widget_sensors;
    g_assert (wdgt_table != NULL);

    GtkAllocation allocation;
    gtk_widget_get_allocation(gtk_widget_get_parent(wdgt_table), &allocation);
    gint num_max_cols = (allocation.width - BORDER) / (DEFAULT_SIZE_TACHOS + BORDER);
    gint num_max_rows = (allocation.height - BORDER) / (DEFAULT_SIZE_TACHOS + BORDER);

    gint row_tacho_table = 0, col_tacho_table = 0;
    for (auto chip : sensors->chips)
    {
        for (auto feature : chip->chip_features)
        {
            if (row_tacho_table >= num_max_rows)
                return;

            auto tacho = sensors->tachos.find(feature);

            GtkWidget *sensorstachowidget;
            GtkSensorsTacho *sensorstacho;
            if (tacho != sensors->tachos.end())
            {
                sensorstachowidget = tacho->second;
                sensorstacho = GTK_SENSORSTACHO(sensorstachowidget);
            }
            else
            {
                sensorstachowidget = NULL;
                sensorstacho = NULL;
            }

            if (feature->valid && feature->show)
            {
                if (sensorstachowidget == NULL)
                {
                    DBG("Newly adding selected widget from container.");

                    SensorsTachoStyle tacho_style = style_MinGYR; /* default as has been for 10 years */
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

                    GtkOrientation orientation = (sensors->plugin_mode != XFCE_PANEL_PLUGIN_MODE_VERTICAL) ? GTK_ORIENTATION_HORIZONTAL : GTK_ORIENTATION_VERTICAL;
                    sensors->tachos[feature] = sensorstachowidget = gtk_sensorstacho_new(orientation, DEFAULT_SIZE_TACHOS, tacho_style);
                    sensorstacho = GTK_SENSORSTACHO(sensorstachowidget);

                    gtk_sensorstacho_set_color (sensorstacho, feature->color_orEmpty.c_str());
                    gtk_sensorstacho_set_text (sensorstacho, feature->name.c_str());
                }

                gdouble fill_degree = (feature->raw_value - feature->min_value) / ( feature->max_value - feature->min_value);
                if (fill_degree < 0.0)
                    fill_degree = 0.0;
                else if (fill_degree > 1.0)
                    fill_degree = 1.0;

                gtk_sensorstacho_set_value(sensorstacho, fill_degree);

                auto tooltip_text = xfce4::sprintf ("<b>%s</b>\n%s: %s", chip->sensorId.c_str(), feature->name.c_str(), feature->formatted_value.c_str());
                gtk_widget_set_tooltip_markup (GTK_WIDGET(sensors->tachos[feature]), tooltip_text.c_str());

                if (gtk_widget_get_parent(sensorstachowidget) == NULL)
                {
                  gtk_grid_attach(GTK_GRID(wdgt_table), sensorstachowidget, col_tacho_table, row_tacho_table, 1, 1);
                  gtk_widget_show (sensorstachowidget);
                }
                else if (gtk_grid_get_child_at(GTK_GRID(wdgt_table), col_tacho_table, row_tacho_table) != sensorstachowidget)
                {
                  g_object_ref (sensorstachowidget);
                  gtk_container_remove(GTK_CONTAINER(wdgt_table), sensorstachowidget);
                  gtk_grid_attach(GTK_GRID(wdgt_table), sensorstachowidget, col_tacho_table, row_tacho_table, 1, 1);
                  g_object_unref (sensorstachowidget);
                }

                col_tacho_table++;
                if (col_tacho_table >= num_max_cols) {
                    row_tacho_table++;
                    col_tacho_table = 0;
                }
            }
            else if (sensorstachowidget != NULL)
            {
                g_free (sensorstacho->color_orNull);
                g_free (sensorstacho->text);
                sensorstacho->color_orNull = NULL;
                sensorstacho->text = NULL;

                sensors->tachos.erase(tacho);

                if (gtk_widget_get_parent(sensorstachowidget) != NULL)
                    gtk_container_remove(GTK_CONTAINER(wdgt_table), sensorstachowidget);
            }

        }
    }

    /* TODO: Rearrange all widgets in order to distribute evenly */
}


/* -------------------------------------------------------------------------- */
void
refresh_view (const Ptr<t_sensors_dialog> &dialog)
{
    refresh_sensor_data (dialog);
    reload_listbox (dialog);
    refresh_tacho_view (dialog); /* refresh all the tacho elements in the notebook/table view */
}
