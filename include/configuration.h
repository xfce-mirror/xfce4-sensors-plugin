/* $Id$ */
/* File configuration.h
 *
 *  Copyright 2004-2010 Fabian Nowak (timystery@arcor.de)
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

#ifndef XFCE4_SENSORS_CONFIGURATION_H
 #define XFCE4_SENSORS_CONFIGURATION_H

/* Gtk/Glib includes */
#include <glib.h>

/* Local includes */
#include "sensors-interface-common.h"

/**
 * Gets the internally used chipfeature index for the given parameters.
 * @Param chipnumber: number of the chip to search for
 * @Param addr: address of the chipfeature
 * @Param sensors: pointer to sensors structure
 * @Return: -1 on error; else chipfeature index for the parameters
 */
gint get_Id_from_address (gint chipnumber, gint addr, t_sensors *sensors);

/**
 * .
 * @Param :
 * @Param :
 */
void sensors_write_config (XfcePanelPlugin *plugin, t_sensors *sensors);

/**
 * .
 * @Param :
 * @Param :
 */
void sensors_read_general_config (XfceRc *rc, t_sensors *sensors);

/**
 * .
 * @Param :
 * @Param :
 */
void sensors_read_config (XfcePanelPlugin *plugin, t_sensors *sensors);

/**
 * .
 * @Param :
 * @Param :
 */
void sensors_read_preliminary_config (XfcePanelPlugin *plugin, t_sensors *sensors);

#endif  /* define XFCE4_SENSORS_CONFIGURATION_H */
