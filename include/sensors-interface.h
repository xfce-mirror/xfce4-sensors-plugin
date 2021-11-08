/* File: sensors-interface.h
 *
 * Copyright 2008-2017 Fabian Nowak (timystery@arcor.de)
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

#ifndef XFCE4_SENSORS_INTERFACE_H
#define XFCE4_SENSORS_INTERFACE_H

#include <glib.h>
#include <gtk/gtk.h>
#include "sensors-interface-common.h"

/* Initializing and filling functions */

/**
 * Enumeration of the colums for the used GtkTreeModel.
 */
enum Enum_TreeColumn
{
    /// User chosen name of the chipfeature
    eTreeColumn_Name = 0,
    /// non-writable value of the chipfeature
    eTreeColumn_Value = 1,
    /// whether to show the chipfeature in the display or panel
    eTreeColumn_Show = 2,
    /// color to use for the font or bar
    eTreeColumn_Color = 3,
    /// expected minimum value, used also for calculating 0 percent
    eTreeColumn_Min = 4,
    /// expected maximum value, used also for calculating 100 percent
    eTreeColumn_Max = 5
};

/**
 * Populates the tree store from the obtained sensors data
 * @param treestore: Pointer to treestore that has to be filled
 * @param tempscale: temperature scale
 */
void fill_gtkTreeStore (GtkTreeStore *treestore, const Ptr<t_chip> &chip, t_tempscale tempscale,
                        const Ptr<t_sensors_dialog> &dialog);

/**
 * Frees the allocated and added dialog widgets
 */
void free_widgets (const Ptr<t_sensors_dialog> &dialog);

/**
 * Initializes the widgets with the proper values
 */
void init_widgets (const Ptr<t_sensors_dialog> &dialog);

/**
 * Reloads the data in the listbox
 */
void reload_listbox (const Ptr<t_sensors_dialog> &dialog);


/* GUI builder functions */

/**
 * Adds the settings box to vbox
 * @param vbox: Pointer to vbox widget to add the settings box to
 */
void add_sensor_settings_box (GtkWidget *vbox, const Ptr<t_sensors_dialog> &dialog);

/**
 * Adds the sensors type chooser box to vbox
 * @param vbox: Pointer to vbox widget to add the type box to
 */
void add_type_box (GtkWidget *vbox, const Ptr<t_sensors_dialog> &dialog);

/**
 * Ads the termperature unit chooser box
 * @param vbox:  Pointer to vbox widget to add the unit box to
 */
void add_temperature_unit_box (GtkWidget *vbox, const Ptr<t_sensors_dialog> &dialog);

/**
 * Adds the entire sensors settings frame to the options dialog as new notebook
 * @param notebook: Pointer to notebook to attach the new content to
 */
void add_sensors_frame (GtkWidget *notebook, const Ptr<t_sensors_dialog> &dialog);

/**
 * Adds the update tim box
 * @param vbox:  Pointer to vbox widget to add the update time box to
 */
void add_update_time_box (GtkWidget *vbox, const Ptr<t_sensors_dialog> &dialog);

#endif /* XFCE4_SENSORS_INTERFACE_H */
