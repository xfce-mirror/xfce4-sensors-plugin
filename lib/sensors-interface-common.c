/*  Copyright 2004-2008 Fabian Nowak (timystery@arcor.de)
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

/* Xfce includes */
#include <libxfce4panel/xfce-panel-plugin.h>

/* Local/package includes */
#include <y.h>
#include <sensors-interface-common.h>

t_sensors * sensors_new (XfcePanelPlugin *plugin)
{
    t_sensors *sensors;
    gint result;
    t_chip *chip;
    t_chipfeature *chipfeature;

    TRACE ("enters sensors_new");

    sensors = g_new (t_sensors, 1);

    /* init xfce sensors stuff width default values */
    sensors_init_default_values (sensors, plugin);

    /* get suppressmessages */
    sensors_read_preliminary_config(plugin, sensors);

    /* read all sensors from libraries */
    result = initialize_all (&(sensors->chips), &(sensors->suppressmessage));

    sensors->num_sensorchips = sensors->chips->len;

    /* error handling for no sensors */
    if (!sensors->chips || sensors->num_sensorchips <= 0) {
        if (!sensors->chips)
            sensors->chips = g_ptr_array_new ();

        chip = g_new ( t_chip, 1);
        g_ptr_array_add (sensors->chips, chip);
        chip->chip_features = g_ptr_array_new();
        chipfeature = g_new (t_chipfeature, 1);

        chipfeature->address = 0;
        chip->sensorId = g_strdup(_("No sensors found!"));
        chip->num_features = 1;
        chipfeature->color = g_strdup("#000000");
        g_free (chipfeature->name);
        chipfeature->name = g_strdup("No sensor");
        chipfeature->valid = TRUE;
        g_free (chipfeature->formatted_value);
        chipfeature->formatted_value = g_strdup("0.0");
        chipfeature->raw_value = 0.0;
        chipfeature->min_value = 0;
        chipfeature->max_value = 7000;
        chipfeature->show = FALSE;

        g_ptr_array_add (chip->chip_features, chipfeature);
    }


    /* Add tooltip to show extended current sensors status */
    sensors_create_tooltip ((gpointer) sensors);

    /* fill panel widget with boxes, strings, values, ... */
    create_panel_widget (sensors);

    /* finally add panel "sensors" to eventbox */
    gtk_container_add (GTK_CONTAINER (sensors->eventbox),
                       sensors->widget_sensors);


    /* #if GTK_VERSION >= 2.11
     * g_signal_connect(G_OBJECT(sensors->eventbox),
                                    "query-tooltip",
                                    G_CALLBACK(handle_tooltip_query),
                                    (gpointer) sensors); */


    TRACE ("leaves sensors_new");

    return sensors;
}
