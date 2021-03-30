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

/* Gtk/Glib includes */
#include <glib.h>

/* Local includes */
#include "sensors-interface-common.h"

/**
 * Gets the internally used chipfeature index for the given parameters.
 * @param chipnumber: number of the chip to search for
 * @param addr_chipfeature: address of the chipfeature
 * @param ptr_sensors: pointer to sensors structure
 * @return -1 on error; else chipfeature index for the parameters
 */
gint get_Id_from_address (gint chipnumber, gint addr_chipfeature,
                          t_sensors *ptr_sensors);


/**
 * Write the configuration, e.g., when exiting the plugin.
 * @param ptr_panelplugin: pointer to panel plugin structure
 * @param ptr_sensors: pointer to sensors structure
 */
void sensors_write_config (XfcePanelPlugin *ptr_panelplugin,t_sensors *ptr_sensors);


/**
 * Read the general settings stuff.
 * @param ptr_xfcerc: pointer to xfce settings resource
 * @param ptr_sensors: pointer to sensors structure
 */
void sensors_read_general_config (XfceRc *ptr_xfcerc, t_sensors *ptr_sensors);


/**
 * Read the configuration file at init:
 * Open the resource file and read down to the per-feature settings.
 * @param ptr_panelplugin: pointer to panel plugin structure
 * @param ptr_sensors: pointer to sensors structure
 */
void sensors_read_config (XfcePanelPlugin *ptr_panelplugin,
                          t_sensors *ptr_sensors);


/**
 * Read the preliminary config, i.e, "suppress hdd tooltips".
 * @param ptr_panelplugin: pointer to panel plugin structure
 * @param ptr_sensors: pointer to sensors structure
 */
void sensors_read_preliminary_config (XfcePanelPlugin *ptr_panelplugin,
                                      t_sensors *ptr_sensors);

#endif  /* define XFCE4_SENSORS_CONFIGURATION_H */
