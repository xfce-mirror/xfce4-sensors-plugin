/* File: sensors-interface.h
 *
 *  Copyright 2008-2017 Fabian Nowak (timystery@arcor.de)
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

#ifndef XFCE4_SENSORS_INTERFACE_H
#define XFCE4_SENSORS_INTERFACE_H

/* Gtk includes */
#include <gtk/gtk.h>

/* Local includes */
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
 * @param ptr_treestore: Pointer to treestore that has to be filled
 * @param ptr_chip: Pointer to chip structure
 * @param tempscale: temperature scale
 * @param ptr_sensorsdialog: Pointer to sensors dialog data
 */
void fill_gtkTreeStore (GtkTreeStore *ptr_treestore, t_chip *ptr_chip, t_tempscale tempscale, t_sensors_dialog *ptr_sensorsdialog);

/**
 * Frees the allocated and added dialog widgets
 * @param ptr_sensorsdialog: Pointer to sensors dialog data
 */
void free_widgets (t_sensors_dialog *ptr_sensorsdialog);

/**
 * Initializes the widgets with the proper values
 * @param ptr_sensorsdialog: Pointer to sensors dialog data
 */
void init_widgets (t_sensors_dialog *ptr_sensorsdialog);

/**
 * Reloads the data in the listbox
 * @param ptr_sensorsdialog: Pointer to sensors dialog data
 */
void reload_listbox (t_sensors_dialog *ptr_sensorsdialog);


/* GUI builder functions */

/**
 * Adds the settings box to ptr_widget_vbox
 * @param ptr_widget_vbox: Pointer to vbox widget to add the settings box to
 * @param ptr_sensorsdialog: Pointer to sensors dialog data
 */
void add_sensor_settings_box ( GtkWidget *ptr_widget_vbox, t_sensors_dialog * ptr_sensorsdialog);

/**
 * Adds the sensors type chooser box to ptr_widget_vbox
 * @param ptr_widget_vbox: Pointer to vbox widget to add the type box to
 * @param ptr_sensorsdialog: Pointer to sensors dialog data
 */
void add_type_box (GtkWidget *ptr_widget_vbox, t_sensors_dialog * ptr_sensorsdialog);

/**
 * Ads the termperature unit chooser box
 * @param ptr_widget_vbox:  Pointer to vbox widget to add the unit box to
 * @param ptr_sensorsdialog: Pointer to sensors dialog data
 */
void add_temperature_unit_box (GtkWidget *ptr_widget_vbox, t_sensors_dialog *ptr_sensorsdialog);

/**
 * Adds the entire sensors settings frame to the options dialog as new notebook
 * @param ptr_widget_notebook: Pointer to notebook to attach the new content to
 * @param ptr_sensorsdialog: Pointer to sensors dialog data
 */
void add_sensors_frame (GtkWidget *ptr_widget_notebook, t_sensors_dialog * ptr_sensorsdialog);

/**
 * Adds the update tim box
 * @param ptr_widget_vbox:  Pointer to vbox widget to add the update time box to
 * @param ptr_sensorsdialog: Pointer to sensors dialog data
 */
void add_update_time_box (GtkWidget *ptr_widget_vbox, t_sensors_dialog * ptr_sensorsdialog);

#endif /* XFCE4_SENSORS_INTERFACE_H */
