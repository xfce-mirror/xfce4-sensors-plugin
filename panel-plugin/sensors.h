/*  Copyright 2004 Fabian Nowak (timystery@arcor.de)
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

/* This plugin requires libsensors-1 and its headers !*/

#ifndef XFCE4_SENSORS_H
#define XFCE4_SENSORS_H



#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib/gprintf.h>

#include <libxfce4util/i18n.h>
#include <libxfce4util/libxfce4util.h>
#include <libxfcegui4/xfce_clock.h>

#include <panel/controls.h>
#include <panel/global.h>
/* #include <panel/icons.h> */
#include <panel/plugins.h>

#include <sensors/sensors.h>

#include <unistd.h>


#define BORDER 6
#define SENSORS 10
#define FEATURES_PER_SENSOR 256


#define COLOR_ERROR	"#f00000"
#define COLOR_WARN	"#f0f000"
#define COLOR_NORMAL	"#00C000"


typedef enum {

   TEMPERATURE,
   VOLTAGE,
   SPEED,
   OTHER
} sensor_type;

typedef struct {
	/* the progress bar */
	GtkWidget *progressbar;

	/* the label */
	GtkWidget *label;

	/* the surrounding box */
	GtkWidget *databox;
} t_barpanel;


/*  Sensors module
 *  ------------
 */
typedef struct {
    /* eventbox to catch events */
    GtkWidget *eventbox;
    
    /* our XfceSensors widget */
    GtkWidget *sensors;
    
    /* panel value display */
    GtkWidget *panelValuesLabel;

    /* update the tooltip */
    gint timeout_id, timeout_id2;

    /* font size for display in panel */
    gchar* fontSize;
    gint fontSizeNumerical;

    /* temperature unit for display in panel */
    gint tempUnit;
    
    /* panel size to compute number of cols/columns */
    gint panelSize;

    /* panel orientation */
    gint orientation;

    /* if the bars have been initialized */
    gboolean barsCreated;
    
    /* show title in panel */
    gboolean showTitle;

    /* show labels in panel (GUI mode only) */
    gboolean showLabels;
    
    /* use the progress-bar UI */
    gboolean useBarUI;

    /* sensor update time */
    gint sensorUpdateTime;
                
    /* sensor relevant stuff */
    /* no problem if less than 11 sensors, else will have to enlarge the 
        following arrays. NYI!! */
    gint sensorNumber;
    gint sensorsCount[SENSORS];
    
    /* contains the progress bar panels */
    GtkWidget* panels[SENSORS][FEATURES_PER_SENSOR];
    
    /* contains structure from libsensors */
    const sensors_chip_name *chipName[SENSORS];
    
    /* formatted sensor chip names, e.g. 'asb-100-45' */
    gchar *sensorId[SENSORS];
    
    /* unformatted sensor feature names, e.g. 'Vendor' */
    gchar *sensorNames[SENSORS][FEATURES_PER_SENSOR];

    /* minimum and maximum values (GUI mode only) */
    glong sensorMinValues[SENSORS][FEATURES_PER_SENSOR];
    glong sensorMaxValues[SENSORS][FEATURES_PER_SENSOR];
    
    /* unformatted sensor feature values */
    double sensorRawValues[SENSORS][FEATURES_PER_SENSOR];

    /* formatted (%f5.2) sensor feature values */
    gchar *sensorValues[SENSORS][FEATURES_PER_SENSOR];

    /* TRUE if sensorNames are set */
    gboolean sensorValid[SENSORS][FEATURES_PER_SENSOR];
    
    /* show sensor in panel */
    gboolean sensorCheckBoxes[SENSORS][FEATURES_PER_SENSOR];
    
    /* sensor types to display values in appropriate format */
    sensor_type sensor_types[SENSORS][FEATURES_PER_SENSOR];
    
    /* sensor colors in panel */
    gchar *sensorColors[SENSORS][FEATURES_PER_SENSOR];
    
    /* number in list <--> number in array */
    gint sensorAddress[SENSORS][FEATURES_PER_SENSOR];
    
    /* double-click improvement as suggested on xfce4-goodies@berlios.de */
    /* whether to execute command on double click */
     gboolean execCommand; 
    
    /* command to excute */
     gchar* commandName; 
    
    /* callback_id for doubleclicks */
     gint doubleClick_id; 
    
}
t_sensors;


/* sensor panel widget
 * -------------------
 */
typedef struct {
    /* the sensors structure */
    t_sensors *sensors;

    /* controls dialog */
    GtkWidget *dialog;

    /* sensors options */
    GtkWidget *type_menu;

    /* Gtk stuff */
    GtkWidget *myComboBox;
    GtkWidget *myFrame;
    GtkWidget *mySensorLabel;
    GtkWidget *myTreeView;
    GtkTreeStore *myListStore[SENSORS];
    GtkWidget *fontBox;
    GtkWidget *labelsBox;
    GtkWidget *tempUnitBox;

    /* double-click improvement */  
    GtkWidget *myExecCommandCheckBox;
    GtkWidget *myCommandNameEntry; 
}
SensorsDialog;



#endif // XFCE4_SENSORS_H
