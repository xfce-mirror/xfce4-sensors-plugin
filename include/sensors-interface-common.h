/* File: sensors-interface-common.h
 *
 *  Copyright 2004-2017 Fabian Nowak (timystery@arcor.de)
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

#ifndef XFCE4_SENSORS_INTERFACE_COMMON_H
#define XFCE4_SENSORS_INTERFACE_COMMON_H

#ifdef HAVE_CONFIG_H
 #include <config.h>
#endif

/* Glib/Gtk includes */
#include <gtk/gtk.h>
#include <glib.h>
/* #include <glib/gprintf.h>  */


/* Package includes */
#include "types.h"
#include "sensors-interface-plugin.h"

/* Definitions */
#define BORDER 8
#define OUTER_BORDER 4
#define INNER_BORDER 2

#define COLOR_ERROR     "#F00000"
#define COLOR_WARN      "#F0F000"
#define COLOR_NORMAL    "#00C000"


/**
 * sensor panel dialog widget.
 * Contains pointers to all used major widgets and to the sensors plugin data
 * itself.
 */
typedef struct {
    /* the sensors structure */
    t_sensors *sensors;

    gboolean plugin_dialog;

    /* controls dialog */
    GtkWidget *dialog;

    /* Gtk stuff */
    GtkWidget *myComboBox;
    /* GtkWidget *myFrame; */
    GtkWidget *mySensorLabel;
    GtkWidget *myTreeView;
    GtkTreeStore *myListStore[10]; /* replace by GPtrArray as well */
    /* used to disable font size option when using graphical view */
    GtkWidget *font_Box;
    GtkWidget *fontSettings_Box;
    GtkWidget *fontSettings_Button;
    GtkWidget *unit_checkbox;
    GtkWidget *Lines_Box;
    GtkWidget *Lines_Spin_Box;
    GtkWidget *suppressmessage_checkbox;
    GtkWidget *suppresstooltip_checkbox;
    GtkWidget *smallspacing_checkbox;
    /* used to enable 'show labels' option when using graphical view */
    GtkWidget *labels_Box;
    GtkWidget *coloredBars_Box;
    GtkWidget *temperature_radio_group;
    GtkWidget *spin_button_update_time;

    /* double-click improvement */
    GtkWidget *myExecCommand_CheckBox;
    GtkWidget *myCommandName_Entry;
}
t_sensors_dialog;



/* Extern functions that need to be re-implemented in the sensors-viewer and
 * the panel code.
 * They kind of need to be registered at the library by any software
 * implementing them.
 */

/**
 * Notification that the adjustment value of the update timer box has changed
 * @param ptr_widget: Pointer to original widget, i.e, the update timer box
 * @param ptr_sensorsdialog: argument pointer to sensors dialog data
 */
void
(*adjustment_value_changed) (GtkWidget *ptr_widget,
                             t_sensors_dialog *ptr_sensorsdialog);

/**
 * Notification that another sensor has been selected.
 * Therefore, the list box has to be updated.
 * @param ptr_widget: Pointer to original widget, i.e, the sensor entry combobox
 * @param ptr_sensorsdialog: argument pointer to sensors dialog data
 */
void
(*sensor_entry_changed) (GtkWidget *ptr_widget,
                         t_sensors_dialog *ptr_sensorsdialog);

/**
 * Notification that the label of a sensor entry has been changed
 * @param ptr_cellrenderertext: Pointer to the CellRenderer for texts
 * @param ptr_str_path: pointer to the string with the path of the changed item
 * @param ptr_str_newtext: Pointer to the string containing the new label
 * @param ptr_sensorsdialog: argument pointer to sensors dialog data
 */
void
(*list_cell_text_edited) (GtkCellRendererText *ptr_cellrenderertext,
                          gchar *ptr_str_path, gchar *ptr_str_newtext,
                          t_sensors_dialog *ptr_sensorsdialog);

/**
 * Notification that a sensors feature has been (de)selected for display
 * @param ptr_cellrenderertoggle: Pointer to cellrenderer for toggle items
 * @param ptr_str_path: pointer to the string with the path of the changed item
 * @param ptr_sensorsdialog: argument pointer to sensors dialog data
 */
void
(*list_cell_toggle) (GtkCellRendererToggle *ptr_cellrenderertoggle, gchar *ptr_str_path,
                     t_sensors_dialog *ptr_sensorsdialog);

/**
 * Notification that the color for a sensors feature has been edited
 * @param ptr_cellrenderertext: Pointer to the CellRenderer for texts
 * @param ptr_str_path: pointer to the string with the path of the changed item
 * @param ptr_str_newcolor: Pointer to the string containing the new color in
 *                          hexadecimal rgb format #0011ff
 * @param ptr_sensorsdialog: argument pointer to sensors dialog data
 */
void
(*list_cell_color_edited) (GtkCellRendererText *ptr_cellrenderertext,
                           gchar *ptr_str_path, gchar *ptr_str_newcolor,
                           t_sensors_dialog *ptr_sensorsdialog);

/**
 * Notification that the minimum value for a sensor feature has been changed
 * @param ptr_cellrenderertext: Pointer to the CellRenderer for texts
 * @param ptr_str_path: pointer to the string with the path of the changed item
 * @param ptr_str_newmin: Pointer to the string containing the new minimum
 *                        temperature
 * @param ptr_sensorsdialog: argument pointer to sensors dialog data
 */
void
(*minimum_changed) (GtkCellRendererText *ptr_cellrenderertext, gchar *ptr_str_path,
                    gchar *ptr_str_newmin, t_sensors_dialog *ptr_sensorsdialog);

/**
 * Notification that the maximum value for a sensor feature has been changed
 * @param ptr_cellrenderertext: Pointer to the CellRenderer for texts
 * @param ptr_str_path: pointer to the string with the path of the changed item
 * @param ptr_str_newmax: Pointer to the string containing the new maximum
 *                        temperature
 * @param ptr_sensorsdialog: argument pointer to sensors dialog data
 */
void
(*maximum_changed) (GtkCellRendererText *ptr_cellrenderertext, gchar *ptr_str_path,
                    gchar *ptr_str_newmax, t_sensors_dialog *ptr_sensorsdialog);

/**
 * Notification that the termperature unit has changed, i.e, between Fahrenheit
 * and Celsius
 * @param ptr_widget: Pointer to original widget, i.e, the update timer box
 * @param ptr_sensorsdialog: argument pointer to sensors dialog data
 */
void
(*temperature_unit_change) (GtkWidget *ptr_widget,
                            t_sensors_dialog *ptr_sensorsdialog);


/* Internal functions */

/**
 * Internal function for properly formatting a sensor value
 * @param temperaturescale: temperature scale
 * @param ptr_chipfeature; Pointer to chipfeature; required for properly formatting
 * @param val_sensorfeature [in]: input value to be formatted
 * @param dptr_str_formattedvalue [out]:
 */
void format_sensor_value (t_tempscale temperaturescale, t_chipfeature *ptr_chipfeature,
                     double val_sensorfeature, gchar **dptr_str_formattedvalue);

#endif /* XFCE4_SENSORS_INTERFACE_COMMON_H */
