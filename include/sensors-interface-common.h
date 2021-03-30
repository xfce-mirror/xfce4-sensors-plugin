/* File: sensors-interface-common.h
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
// Newer Gnome 2 spacing?
//#define BORDER 18
//#define OUTER_BORDER 12
//#define INNER_BORDER 6

// Old Xfce 4.2 spacing
//#define BORDER 8
//#define OUTER_BORDER 4
//#define INNER_BORDER 2

// somewhere in between :)
#define BORDER 12
#define OUTER_BORDER 12
#define INNER_BORDER 6

#define COLOR_ERROR     "#F00000"
#define COLOR_WARN      "#F0F000"
#define COLOR_NORMAL    "#00C000"


/**
 * sensor panel dialog widget.
 * Contains pointers to all used major widgets and to the sensors plugin data
 * itself.
 */
typedef struct {
    /** the sensors structure */
    t_sensors *sensors;

    /** is it the dialog of the panel plugin? */
    gboolean plugin_dialog;

    /** controls dialog */
    GtkWidget *dialog;

    /* Gtk stuff */
    /** pointer to combobox for choosing sensor chip */
    GtkWidget *myComboBox;

    /** pointer to GtkLabel for displaying the chip's name */
    GtkWidget *mySensorLabel;

    /** pointer to GtkTreeView widget */
    GtkWidget *myTreeView;

    /**
     * array of pointers to tree stores, limited to 10 as well
     * TODO: replace by GPtrArray as well
     */
    GtkTreeStore *myListStore[MAX_NUM_CHIPS];

    /**
     * box with font settings for text view; used to disable font size option
     * when using graphical/tacho view
     */
    GtkWidget *font_Box;

    /** GtkBox for font settings in visual displays */
    GtkWidget *fontSettings_Box;

    /** button for font settings in visual displays */
    GtkWidget *fontSettings_Button;

    /** pointer to GtkCheckbox whether to display units in text mode */
    GtkWidget *unit_checkbox;

    /** pointer to surrounding GtkBox for number of text lines in text mode */
    GtkWidget *Lines_Box;

    /** pointer to GtkSpinButton for number of text lines */
    GtkWidget *Lines_Spin_Box;

    /** pointer to GtkCheckbox for opting to suppress notifications */
    GtkWidget *suppressmessage_checkbox;

    /** pointer to GtkCheckbox for opting to suppress tooltips */
    GtkWidget *suppresstooltip_checkbox;

    /** pointer to GtkCheckbox for text mode to have smaller spacings */
    GtkWidget *smallspacing_checkbox;

    /** used to enable 'show labels' option when using graphical view */
    GtkWidget *labels_Box;

    /** used to customize colorvalue value when using graphical view */
    GtkWidget *colorvalue_slider_box;

    /** used to customize alpha value when using graphical view */
    GtkWidget *alpha_slider_box;

    /**
     * pointer to GtkCheckbox whether to colorize the checkboxes over the Gtk
     * theme
     */
    GtkWidget *coloredBars_Box;

    /** pointer to GtkSpinButton for selecting the refresh interval in seconds */
    GtkWidget *spin_button_update_time;

    /** double-click improvement: check to activate */
    GtkWidget *myExecCommand_CheckBox;

    /** double-click improvement: entry for command */
    GtkWidget *myCommandName_Entry;
}
t_sensors_dialog;



#ifdef XFCE4_SENSORS_INTERFACE_COMMON_DEFINING
#define EXTERN
#else
#define EXTERN extern
#endif

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
EXTERN void
(*adjustment_value_changed) (GtkWidget *ptr_widget,
                             t_sensors_dialog *ptr_sensorsdialog);

/**
 * Notification that another sensor has been selected.
 * Therefore, the list box has to be updated.
 * @param ptr_widget: Pointer to original widget, i.e, the sensor entry combobox
 * @param ptr_sensorsdialog: argument pointer to sensors dialog data
 */
EXTERN void
(*sensor_entry_changed) (GtkWidget *ptr_widget,
                         t_sensors_dialog *ptr_sensorsdialog);

/**
 * Notification that the label of a sensor entry has been changed
 * @param ptr_cellrenderertext: Pointer to the CellRenderer for texts
 * @param ptr_str_path: pointer to the string with the path of the changed item
 * @param ptr_str_newtext: Pointer to the string containing the new label
 * @param ptr_sensorsdialog: argument pointer to sensors dialog data
 */
EXTERN void
(*list_cell_text_edited) (GtkCellRendererText *ptr_cellrenderertext,
                          gchar *ptr_str_path, gchar *ptr_str_newtext,
                          t_sensors_dialog *ptr_sensorsdialog);

/**
 * Notification that a sensors feature has been (de)selected for display
 * @param ptr_cellrenderertoggle: Pointer to cellrenderer for toggle items
 * @param ptr_str_path: pointer to the string with the path of the changed item
 * @param ptr_sensorsdialog: argument pointer to sensors dialog data
 */
EXTERN void
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
EXTERN void
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
EXTERN void
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
EXTERN void
(*maximum_changed) (GtkCellRendererText *ptr_cellrenderertext, gchar *ptr_str_path,
                    gchar *ptr_str_newmax, t_sensors_dialog *ptr_sensorsdialog);

/**
 * Notification that the termperature unit has changed, i.e, between Fahrenheit
 * and Celsius
 * @param ptr_widget: Pointer to original widget, i.e, the update timer box
 * @param ptr_sensorsdialog: argument pointer to sensors dialog data
 */
EXTERN void
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
