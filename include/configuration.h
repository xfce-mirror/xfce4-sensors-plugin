/* File: configuration.h
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

#ifndef XFCE4_SENSORS_CONFIGURATION_H
#define XFCE4_SENSORS_CONFIGURATION_H

#include <glib.h>

#include "sensors-interface-common.h"

G_BEGIN_DECLS

/**
 * Gets the internally used chipfeature index for the given parameters.
 * @param chip_number: number of the chip to search for
 * @param addr_chipfeature: address of the chipfeature
 * @param sensors: pointer to sensors structure
 * @return -1 on error; else chipfeature index for the parameters
 */
gint get_Id_from_address (gint chip_number, gint addr_chipfeature, t_sensors *sensors);


/**
 * Write the configuration, e.g., when exiting the plugin.
 * @param plugin: pointer to panel plugin structure
 * @param sensors: pointer to sensors structure
 */
void sensors_write_config (XfcePanelPlugin *plugin, const t_sensors *sensors);


/**
 * Read the general settings stuff.
 * @param rc: pointer to xfce settings resource
 * @param sensors: pointer to sensors structure
 */
void sensors_read_general_config (XfceRc *rc, t_sensors *sensors);


/**
 * Read the configuration file at init:
 * Open the resource file and read down to the per-feature settings.
 * @param plugin: pointer to panel plugin structure
 * @param sensors: pointer to sensors structure
 */
void sensors_read_config (XfcePanelPlugin *plugin, t_sensors *sensors);


/**
 * Read the preliminary config, i.e, "suppress hdd tooltips".
 * @param plugin: pointer to panel plugin structure
 * @param sensors: pointer to sensors structure
 */
void sensors_read_preliminary_config (XfcePanelPlugin *plugin, t_sensors *sensors);

G_END_DECLS

#endif  /* define XFCE4_SENSORS_CONFIGURATION_H */
