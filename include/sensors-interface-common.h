/* $Id$ */
/*  Copyright 2004-2010 Fabian Nowak (timystery@arcor.de)
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
 * sensor panel widget
 */
typedef struct {
    /* the sensors structure */
    t_sensors *sensors;

    gboolean plugin_dialog;

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
    GtkWidget *fontSettings_Box;
    GtkWidget *fontSettings_Button;
    GtkWidget *unit_checkbox;
    GtkWidget *Lines_Box;
    GtkWidget *Lines_Spin_Box;
    GtkWidget *suppressmessage_checkbox;
    GtkWidget *suppresstooltip_checkbox;
    GtkWidget *smallspacing_checkbox;
    GtkWidget *labels_Box; /* used to enable 'show labels' option when using graphical view */
    GtkWidget *coloredBars_Box;
    GtkWidget *temperature_radio_group;

    /* double-click improvement */
    GtkWidget *myExecCommand_CheckBox;
    GtkWidget *myCommandName_Entry;
}
t_sensors_dialog;



/* Extern functions that need to be re-implemented in the sensors-viewer and
 * the panel code. */
extern void
adjustment_value_changed  (GtkWidget *widget, t_sensors_dialog *sd); // for update timer box

extern void
sensor_entry_changed (GtkWidget *widget, t_sensors_dialog *sd);

extern void
list_cell_text_edited (GtkCellRendererText *cellrenderertext,
                      gchar *path_str, gchar *new_text, t_sensors_dialog *sd);

extern void
list_cell_toggle (GtkCellRendererToggle *cell, gchar *path_str,
                  t_sensors_dialog *sd);

extern void
list_cell_color_edited (GtkCellRendererText *cellrenderertext, gchar *path_str,
                       gchar *new_color, t_sensors_dialog *sd);

extern void
minimum_changed (GtkCellRendererText *cellrenderertext, gchar *path_str,
                 gchar *new_value, t_sensors_dialog *sd);

extern void
maximum_changed (GtkCellRendererText *cellrenderertext, gchar *path_str,
            gchar *new_value, t_sensors_dialog *sd);

extern void
temperature_unit_change (GtkWidget *widget, t_sensors_dialog *sd);

void format_sensor_value (t_tempscale scale, t_chipfeature *chipfeature,
                     double sensorFeature, gchar **help);

#endif /* XFCE4_SENSORS_INTERFACE_COMMON_H */
