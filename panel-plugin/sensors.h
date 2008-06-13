/*  Copyright 2004-2007 Fabian Nowak (timystery@arcor.de)
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

#ifndef XFCE4_SENSORS_SENSORS_H
#define XFCE4_SENSORS_SENSORS_H

#ifdef HAVE_CONFIG_H
 #include <config.h>
#endif

/* #endif */

#include <glib/garray.h>
#include <glib/gprintf.h>

#include <libxfce4util/libxfce4util.h>
#include <libxfcegui4/libxfcegui4.h>
#include <libxfce4panel/xfce-panel-plugin.h>

#include <string.h>

#include "middlelayer.h"
#include "types.h"

#define APP_NAME N_("Sensors Plugin")

#define BORDER 8
#define OUTER_BORDER 4
#define INNER_BORDER 2

#define COLOR_ERROR     "#F00000"
#define COLOR_WARN      "#F0F000"
#define COLOR_NORMAL    "#00C000"


/*
 * compound widget displaying a progressbar and optional label
 */
typedef struct {
    /* the progress bar */
    GtkWidget *progressbar;

    /* the label */
    GtkWidget *label;

    /* the surrounding box */
    GtkWidget *databox;
} t_barpanel;


/*
 * Sensors module
 */
typedef struct {

    XfcePanelPlugin *plugin;

    /* eventbox to catch events */
    GtkWidget *eventbox;

    /* our XfceSensors widget */
    GtkWidget *widget_sensors;

    /* panel value display */
    GtkWidget *panel_label_data;

    /* optional label for plugin */
    GtkWidget *panel_label_text;

    /* update the tooltip */
    gint timeout_id;

    /* font size for display in panel */
    gchar* font_size;
    gint font_size_numerical;

    /* temperature scale for display in panel */
    t_tempscale scale;

    /* panel size to compute number of cols/columns */
    gint panel_size;

    /* panel orientation */
    gint orientation;

    /* if the bars have been initialized */
    gboolean bars_created;

    /* show title in panel */
    gboolean show_title;

    /* show labels in panel (GUI mode only) */
    gboolean show_labels;

    /* show units in textual view */
    gboolean show_units;

    /* show small spacings only in textual view */
    gboolean show_smallspacings;

    /* show colored bars (GUI mode only) */
    gboolean show_colored_bars;

    /* use the progress-bar UI */
    gboolean display_values_graphically;

    /* sensor update time */
    gint sensors_refresh_time;

    /* sensor relevant stuff */
    /* no problem if less than 11 sensors, else will have to enlarge the
        following arrays. NYI!! */
    gint num_sensorchips;

    /* gint sensorsCount[SENSORS]; */

    /* contains the progress bar panels */
    /* FIXME:    Might be replaced by GPtrArray as well */
    GtkWidget* panels[10][256];
    /*    GArray *panels_array; */

    /* contains structure from libsensors */
    /* const sensors_chip_name *chipName[SENSORS]; */

    /* formatted sensor chip names, e.g. 'asb-100-45' */
    /* gchar *sensorId[SENSORS]; */

    /* unformatted sensor feature names, e.g. 'Vendor' */
    /* gchar *sensorNames[SENSORS][FEATURES_PER_SENSOR]; */

    /* minimum and maximum values (GUI mode only) */
    /* glong sensorMinValues[SENSORS][FEATURES_PER_SENSOR]; */
    /* glong sensorMaxValues[SENSORS][FEATURES_PER_SENSOR]; */

    /* unformatted sensor feature values */
    /* double sensorRawValues[SENSORS][FEATURES_PER_SENSOR]; */

    /* formatted (%f5.2) sensor feature values */
    /* gchar *sensorValues[SENSORS][FEATURES_PER_SENSOR]; */

    /* TRUE if sensorNames are set */
    /* gboolean sensorValid[SENSORS][FEATURES_PER_SENSOR]; */

    /* show sensor in panel */
    /* gboolean sensorCheckBoxes[SENSORS][FEATURES_PER_SENSOR]; */

    /* sensor types to display values in appropriate format */
    /* sensor_type sensor_types[SENSORS][FEATURES_PER_SENSOR]; */
    GPtrArray *chips;

    /* sensor colors in panel */
    /* gchar *sensorColors[SENSORS][FEATURES_PER_SENSOR]; */

    /* number in list <--> number in array */
    /* gint sensorAddress[NUM_SENSOR_CHIPS][FEATURES_PER_SENSOR]; */

    /* double-click improvement as suggested on xfce4-goodies@berlios.de */
    /* whether to execute command on double click */
    gboolean exec_command;

    /* command to excute */
    gchar* command_name;

    /* callback_id for doubleclicks */
    gint doubleclick_id;

    /* hddtemp disks */
    GPtrArray *disklist;
    /* gint num_disks; */

    /* ACPI thermal zones */
    GPtrArray *acpi_zones;
    gint num_acpi_zones;
}
t_sensors;


/*
 * sensor panel widget
 */
typedef struct {
    /* the sensors structure */
    t_sensors *sensors;

    /* controls dialog */
    GtkWidget *dialog;

    /* sensors options  - What was this crap for??? */
    /* GtkWidget *type_menu; */

    /* Gtk stuff */
    GtkWidget *myComboBox;
    /* GtkWidget *myFrame; */
    GtkWidget *mySensorLabel;
    GtkWidget *myTreeView;
    GtkTreeStore *myListStore[10]; /* replace by GPtrArray as well */
    GtkWidget *font_Box; /* used to disable font size option when using graphical view */
    GtkWidget *unit_checkbox;
    GtkWidget *smallspacing_checkbox;
    GtkWidget *labels_Box; /* used to enable 'show labels' option when using graphical view */
    GtkWidget *coloredBars_Box;
    GtkWidget *temperature_radio_group;

    /* double-click improvement */
    GtkWidget *myExecCommand_CheckBox;
    GtkWidget *myCommandName_Entry;
}
t_sensors_dialog;



#endif /* XFCE4_SENSORS_SENSORS_H */
