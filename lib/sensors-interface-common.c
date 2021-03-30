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

/* Note for programmers and editors: Try to use 4 spaces instead of Tab! */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

/* Xfce includes */
#include <gtk/gtk.h>
#include <libxfce4panel/xfce-panel-plugin.h>

/* Local/package includes */
#define XFCE4_SENSORS_INTERFACE_COMMON_DEFINING
#include <configuration.h>
#include <sensors-interface-common.h>
#include <middlelayer.h>
#include <tacho.h>

/* -------------------------------------------------------------------------- */
t_sensors *
sensors_new (XfcePanelPlugin *ptr_xfcepanelplugin, gchar * ptr_plugin_config_filename)
{
    t_sensors *ptr_sensors;
    gint result;
    t_chip *ptr_chip;
    t_chipfeature *ptr_chipfeature;

    ptr_sensors = g_new0 (t_sensors, 1);
    ptr_sensors->plugin_config_file = ptr_plugin_config_filename; /* important as we check against NULL frequently */

    /* init xfce sensors stuff with default values */
    sensors_init_default_values (ptr_sensors, ptr_xfcepanelplugin);

    /* get suppressmessages */
    sensors_read_preliminary_config(ptr_xfcepanelplugin, ptr_sensors);

    /* read all sensors from libraries */
    result = initialize_all (&(ptr_sensors->chips), &(ptr_sensors->suppressmessage));
    if (result==0)
        return NULL;

    ptr_sensors->num_sensorchips = ptr_sensors->chips->len;

    /* error handling for no sensors */
    if (!ptr_sensors->chips || ptr_sensors->num_sensorchips <= 0) {
        if (!ptr_sensors->chips)
            ptr_sensors->chips = g_ptr_array_new ();

        ptr_chip = g_new ( t_chip, 1);
        g_ptr_array_add (ptr_sensors->chips, ptr_chip);
        ptr_chip->chip_features = g_ptr_array_new();
        ptr_chipfeature = g_new (t_chipfeature, 1);

        ptr_chipfeature->address = 0;
        ptr_chip->sensorId = g_strdup(_("No sensors found!"));
        ptr_chip->description = g_strdup(_("No sensors found!"));
        ptr_chip->num_features = 1;
        ptr_chipfeature->color = g_strdup("#000000");
        ptr_chipfeature->name = g_strdup("No sensor");
        ptr_chipfeature->valid = TRUE;
        ptr_chipfeature->formatted_value = g_strdup("0.0");
        ptr_chipfeature->raw_value = 0.0;
        ptr_chipfeature->min_value = 0;
        ptr_chipfeature->max_value = 7000;
        ptr_chipfeature->show = FALSE;

        g_ptr_array_add (ptr_chip->chip_features, ptr_chipfeature);
    }

    return ptr_sensors;
}


/* -------------------------------------------------------------------------- */
void
sensors_init_default_values  (t_sensors *ptr_sensors, XfcePanelPlugin *ptr_xfcepanelplugin)
{
    g_return_if_fail(ptr_sensors!=NULL);

    ptr_sensors->show_title = TRUE;
    ptr_sensors->show_labels = TRUE;
    ptr_sensors->display_values_type = DISPLAY_TEXT;
    ptr_sensors->bars_created = FALSE;
    ptr_sensors->tachos_created = FALSE;
    ptr_sensors->str_fontsize = g_strdup("medium");
    ptr_sensors->val_fontsize = 2;
    ptr_sensors->lines_size = 3;

    ptr_sensors->show_colored_bars = TRUE;
    ptr_sensors->sensors_refresh_time = 60;
    ptr_sensors->scale = CELSIUS;

    ptr_sensors->plugin = ptr_xfcepanelplugin; // we prefer storing NULL in here in case it is NULL.

    /* double-click improvement */
    ptr_sensors->exec_command = TRUE;
    ptr_sensors->command_name = g_strdup("xfce4-sensors");
    ptr_sensors->doubleclick_id = 0;

    /* show units */
    ptr_sensors->show_units = TRUE;

    ptr_sensors->suppressmessage = FALSE;

    ptr_sensors->show_smallspacings = FALSE;

    ptr_sensors->val_tachos_color = MAX_HUE;
    ptr_sensors->val_tachos_alpha = ALPHA_CHANNEL_VALUE;

    font = g_strdup("Sans 11");
}


/* -------------------------------------------------------------------------- */
void
format_sensor_value (t_tempscale temperaturescale, t_chipfeature *ptr_chipfeature,
                     double val_sensorfeature, gchar **dptr_str_formattedvalue)
{
    g_return_if_fail(ptr_chipfeature!=NULL);
    g_return_if_fail(dptr_str_formattedvalue!=NULL);

    switch (ptr_chipfeature->class) {
        case TEMPERATURE:
           if (temperaturescale == FAHRENHEIT) {
                *dptr_str_formattedvalue = g_strdup_printf(_("%.0f °F"),
                            (float) (val_sensorfeature * 9/5 + 32) );
           } else { /* Celsius */
                *dptr_str_formattedvalue = g_strdup_printf(_("%.0f °C"), val_sensorfeature);
           }
           break;

        case VOLTAGE:
               *dptr_str_formattedvalue = g_strdup_printf(_("%+.3f V"), val_sensorfeature);
               break;

        case CURRENT:
               *dptr_str_formattedvalue = g_strdup_printf(_("%+.3f A"), val_sensorfeature);
               break;

        case ENERGY:
               *dptr_str_formattedvalue = g_strdup_printf(_("%.0f mWh"), val_sensorfeature);
               break;

        case POWER:
               *dptr_str_formattedvalue = g_strdup_printf(_("%.3f W"), val_sensorfeature);
               break;

        case STATE:
                if (val_sensorfeature==0.0)
                    *dptr_str_formattedvalue = g_strdup (_("off"));
                else
                    *dptr_str_formattedvalue = g_strdup (_("on"));
               break;

        case SPEED:
               *dptr_str_formattedvalue = g_strdup_printf(_("%.0f rpm"), val_sensorfeature);
               break;

        default:
                *dptr_str_formattedvalue = g_strdup_printf("%+.2f", val_sensorfeature);
               break;
    } /* end switch */
}
