/* sensors-interface-common.cc
 * Part of xfce4-sensors-plugin
 *
 * Copyright (c) 2004-2017 Fabian Nowak <timystery@arcor.de>
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

#include <gtk/gtk.h>
#include <libxfce4panel/libxfce4panel.h>
#include "xfce4++/util.h"

/* Local/package includes */
#define XFCE4_SENSORS_INTERFACE_COMMON_DEFINING
#include <configuration.h>
#include <middlelayer.h>
#include <sensors-interface-common.h>
#include <tacho.h>

/* -------------------------------------------------------------------------- */
Ptr0<t_sensors>
sensors_new (XfcePanelPlugin *plugin, const char *rc_file_orNull)
{
    auto sensors = xfce4::make<t_sensors>(plugin);

    if (rc_file_orNull)
        sensors->plugin_config_file = rc_file_orNull;

    /* get suppressmessages */
    sensors_read_preliminary_config (plugin, sensors);

    /* read all sensors from libraries */
    int result = initialize_all (sensors->chips, &sensors->suppressmessage);
    if (result == 0)
        return nullptr;

    /* error-handling for no sensors */
    if (sensors->chips.empty()) {
        auto chip = xfce4::make<t_chip>();
        {
            chip->sensorId = _("No sensors found!");
            chip->description = _("No sensors found!");

            auto feature = xfce4::make<t_chipfeature>();
            feature->address = 0;
            feature->name = "No sensor";
            feature->valid = true;
            feature->formatted_value = "0.0";
            feature->raw_value = 0.0;
            feature->min_value = 0;
            feature->max_value = 7000;
            feature->show = false;
            chip->chip_features.push_back(feature);
        }
        sensors->chips.push_back(chip);
    }

    return sensors;
}


/* -------------------------------------------------------------------------- */
std::string
format_sensor_value (t_tempscale temperature_scale, const Ptr<t_chipfeature> &feature, double feature_value)
{
    switch (feature->cls) {
        case TEMPERATURE:
            if (temperature_scale == FAHRENHEIT)
                return xfce4::sprintf (_("%.0f °F"), feature_value * 9 / 5 + 32);
            else
                return xfce4::sprintf (_("%.0f °C"), feature_value);

        case VOLTAGE:
            return xfce4::sprintf (_("%+.3f V"), feature_value);

        case CURRENT:
            return xfce4::sprintf (_("%+.3f A"), feature_value);

        case ENERGY:
            return xfce4::sprintf (_("%.0f mWh"), feature_value);

        case POWER:
           return xfce4::sprintf (_("%.3f W"), feature_value);

        case STATE:
            return (feature_value == 0.0 ? _("off") : _("on"));

        case SPEED:
            return xfce4::sprintf (_("%.0f rpm"), feature_value);

        default:
            return xfce4::sprintf ("%+.2f", feature_value);
    }
}


/* -------------------------------------------------------------------------- */
t_sensors::t_sensors(XfcePanelPlugin *_plugin) : plugin(_plugin) {
    tachos_color = MAX_HUE;
    tachos_alpha = ALPHA_CHANNEL_VALUE;
}


/* -------------------------------------------------------------------------- */
t_sensors::~t_sensors()
{
    g_info ("%s", __PRETTY_FUNCTION__);
}


/* -------------------------------------------------------------------------- */
t_sensors_dialog::t_sensors_dialog(const Ptr<t_sensors> &_sensors) : sensors(_sensors) {}


/* -------------------------------------------------------------------------- */
t_sensors_dialog::~t_sensors_dialog()
{
    g_info ("%s", __PRETTY_FUNCTION__);

    if (dialog)
        g_object_unref (dialog);
}
