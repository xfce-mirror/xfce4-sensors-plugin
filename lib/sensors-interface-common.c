/* $Id$ */
/*  Copyright 2004-2010 Fabian Nowak (timystery@arcor.de)
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
// #include <libxfce4panel/xfce-panel-enums.h>
#include <gtk/gtk.h>
#include <libxfce4panel/xfce-panel-plugin.h>

/* Local/package includes */
#include <configuration.h>
#include <sensors-interface-common.h>
#include <middlelayer.h>
#include <cpu.h>

t_sensors *
sensors_new (XfcePanelPlugin *plugin, gchar *plugin_config_file)
{
    t_sensors *sensors;
    gint result;
    t_chip *chip;
    t_chipfeature *chipfeature;

    TRACE ("enters sensors_new");

    sensors = g_new0 (t_sensors, 1);
    sensors->plugin_config_file = plugin_config_file; /* important as we check against NULL frequently */

    /* init xfce sensors stuff with default values */
    sensors_init_default_values (sensors, plugin);

    /* get suppressmessages */
    sensors_read_preliminary_config(plugin, sensors);

    /* read all sensors from libraries */
    result = initialize_all (&(sensors->chips), &(sensors->suppressmessage));
    if (result==0)
      return NULL;

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
        chip->description = g_strdup(_("No sensors found!"));
        chip->num_features = 1;
        chipfeature->color = g_strdup("#000000");
        /* g_free (chipfeature->name); */
        chipfeature->name = g_strdup("No sensor");
        chipfeature->valid = TRUE;
        /* g_free (chipfeature->formatted_value); */
        chipfeature->formatted_value = g_strdup("0.0");
        chipfeature->raw_value = 0.0;
        chipfeature->min_value = 0;
        chipfeature->max_value = 7000;
        chipfeature->show = FALSE;

        g_ptr_array_add (chip->chip_features, chipfeature);
    }

    TRACE ("leaves sensors_new");

    return sensors;
}



void
sensors_init_default_values  (t_sensors *sensors, XfcePanelPlugin *plugin)
{
    TRACE ("enters sensors_init_default_values");

    sensors->show_title = TRUE;
    sensors->show_labels = TRUE;
    sensors->display_values_type = DISPLAY_TEXT;
    sensors->bars_created = FALSE;
    sensors->font_size = g_strdup("medium");
    sensors->font_size_numerical = 2;
    sensors->lines_size = 3;

    sensors->show_colored_bars = TRUE;
    sensors->sensors_refresh_time = 60;
    sensors->scale = CELSIUS;

    sensors->plugin = plugin; // we prefer storing NULL in here in case it is NULL.


    /* double-click improvement */
    sensors->exec_command = TRUE;
    sensors->command_name = g_strdup("xfce4-sensors");
    sensors->doubleclick_id = 0;

    /* show units */
    sensors->show_units = TRUE;

    sensors->suppressmessage = FALSE;

    sensors->show_smallspacings = FALSE;

    font = NULL;

    TRACE ("leaves sensors_init_default_values");
}


void
format_sensor_value (t_tempscale scale, t_chipfeature *chipfeature,
                     double sensorFeature, gchar **help)
{
    /* TRACE ("enters format_sensor_value"); */

    switch (chipfeature->class) {
        case TEMPERATURE:
           if (scale == FAHRENHEIT) {
                *help = g_strdup_printf(_("%.0f °F"),
                            (float) (sensorFeature * 9/5 + 32) );
           } else { /* Celsius */
                *help = g_strdup_printf(_("%.0f °C"), sensorFeature);
           }
           break;

        case VOLTAGE:
               *help = g_strdup_printf(_("%+.3f V"), sensorFeature);
               break;

        case ENERGY:
               *help = g_strdup_printf(_("%.0f mWh"), sensorFeature);
               break;

        case STATE:
                if (sensorFeature==0.0)
                    *help = g_strdup (_("off"));
                else
                    *help = g_strdup (_("on"));
               break;

        case SPEED:
               *help = g_strdup_printf(_("%.0f rpm"), sensorFeature);
               break;

        default:
                *help = g_strdup_printf("%+.2f", sensorFeature);
               break;
    } /* end switch */

    /* TRACE ("leaves format_sensor_value"); */
}
