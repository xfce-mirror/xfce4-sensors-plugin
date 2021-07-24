/* File: sensors-interface-common.c
 *
 * Copyright 2004-2017 Fabian Nowak (timystery@arcor.de)
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

#include <gtk/gtk.h>
#include <libxfce4panel/xfce-panel-plugin.h>

/* Local/package includes */
#define XFCE4_SENSORS_INTERFACE_COMMON_DEFINING
#include <configuration.h>
#include <middlelayer.h>
#include <sensors-interface-common.h>
#include <tacho.h>

/* -------------------------------------------------------------------------- */
t_sensors *
sensors_new (XfcePanelPlugin *plugin, gchar * plugin_config_filename)
{
    t_sensors *sensors;
    gint result;
    t_chip *chip;
    t_chipfeature *feature;

    sensors = g_new0 (t_sensors, 1);
    sensors->plugin_config_file = plugin_config_filename; /* important as we check against NULL frequently */

    /* init xfce sensors stuff with default values */
    sensors_init_default_values (sensors, plugin);

    /* get suppressmessages */
    sensors_read_preliminary_config(plugin, sensors);

    /* read all sensors from libraries */
    result = initialize_all (&sensors->chips, &sensors->suppressmessage);
    if (result==0)
        return NULL;

    sensors->num_sensorchips = sensors->chips->len;

    /* error-handling for no sensors */
    if (!sensors->chips || sensors->num_sensorchips <= 0) {
        if (!sensors->chips)
            sensors->chips = g_ptr_array_new ();

        chip = g_new0 (t_chip, 1);
        g_ptr_array_add (sensors->chips, chip);
        chip->chip_features = g_ptr_array_new();
        feature = g_new0 (t_chipfeature, 1);

        feature->address = 0;
        chip->sensorId = g_strdup(_("No sensors found!"));
        chip->description = g_strdup(_("No sensors found!"));
        chip->num_features = 1;
        feature->name = g_strdup("No sensor");
        feature->valid = TRUE;
        feature->formatted_value = g_strdup("0.0");
        feature->raw_value = 0.0;
        feature->min_value = 0;
        feature->max_value = 7000;
        feature->show = FALSE;

        g_ptr_array_add (chip->chip_features, feature);
    }

    return sensors;
}


/* -------------------------------------------------------------------------- */
void
sensors_init_default_values  (t_sensors *sensors, XfcePanelPlugin *plugin)
{
    g_return_if_fail (sensors != NULL);

    sensors->show_title = TRUE;
    sensors->show_labels = TRUE;
    sensors->display_values_type = DISPLAY_TEXT;
    sensors->bars_created = FALSE;
    sensors->tachos_created = FALSE;
    sensors->str_fontsize = g_strdup ("medium");
    sensors->val_fontsize = 2;
    sensors->lines_size = 3;
    sensors->text.reset_size = true;

    sensors->automatic_bar_colors = FALSE;
    sensors->sensors_refresh_time = 60;
    sensors->scale = CELSIUS;

    sensors->plugin = plugin; // we prefer storing NULL in here in case it is NULL.

    /* double-click improvement */
    sensors->exec_command = TRUE;
    sensors->command_name = g_strdup ("xfce4-sensors");
    sensors->doubleclick_id = 0;

    sensors->show_units = TRUE;

    sensors->suppressmessage = FALSE;

    sensors->show_smallspacings = FALSE;

    sensors->val_tachos_color = MAX_HUE;
    sensors->val_tachos_alpha = ALPHA_CHANNEL_VALUE;

    font = g_strdup ("Sans 11");
}


/* -------------------------------------------------------------------------- */
void
format_sensor_value (t_tempscale temperature_scale, t_chipfeature *feature,
                     double feature_value, gchar **formatted_value)
{
    g_return_if_fail (feature != NULL);
    g_return_if_fail (formatted_value != NULL);

    switch (feature->class) {
        case TEMPERATURE:
            if (temperature_scale == FAHRENHEIT)
                *formatted_value = g_strdup_printf (_("%.0f °F"), feature_value * 9 / 5 + 32);
            else
                *formatted_value = g_strdup_printf (_("%.0f °C"), feature_value);
            break;

        case VOLTAGE:
            *formatted_value = g_strdup_printf (_("%+.3f V"), feature_value);
            break;

        case CURRENT:
            *formatted_value = g_strdup_printf (_("%+.3f A"), feature_value);
            break;

        case ENERGY:
            *formatted_value = g_strdup_printf (_("%.0f mWh"), feature_value);
            break;

        case POWER:
           *formatted_value = g_strdup_printf (_("%.3f W"), feature_value);
            break;

        case STATE:
            *formatted_value = g_strdup (feature_value == 0.0 ? _("off") : _("on"));
            break;

        case SPEED:
            *formatted_value = g_strdup_printf (_("%.0f rpm"), feature_value);
            break;

        default:
            *formatted_value = g_strdup_printf ("%+.2f", feature_value);
            break;
    }
}
