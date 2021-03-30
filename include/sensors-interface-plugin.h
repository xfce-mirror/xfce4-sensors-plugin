/* File: sensors-interface-plugin.h
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

#ifndef XFCE4_SENSORS_INTERFACE_PLUGIN_H
#define XFCE4_SENSORS_INTERFACE_PLUGIN_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>
#include <gtk/gtk.h>
#include <libxfce4panel/libxfce4panel.h>
#include <stdbool.h>

/* Package includes */
#include "sensors-interface-types.h"
#include "types.h"

#define MAX_NUM_CHIPS 10
#define MAX_NUM_FEATURES 256


/**
 * Sensors module
 */
typedef struct {

    /** pointer for Xfce Panel */
    XfcePanelPlugin *plugin;

    /** eventbox to catch events */
    GtkWidget *eventbox;

    /** our XfceSensors widget */
    GtkWidget *widget_sensors;

    /** panel value display */
    GtkWidget *panel_label_data;

    /** optional label for plugin */
    GtkWidget *panel_label_text;

    /** update the tooltip */
    gint timeout_id;

    /** font size for display in panel */
    gchar *str_fontsize;
    gint val_fontsize;

    /** temperature scale for display in panel */
    t_tempscale scale;

    /** panel size to compute number of cols/columns */
    gint panel_size;

    /** Requested/allowed number of lines in text mode */
    gint lines_size;

    /** panel orientation */
    XfcePanelPluginMode plugin_mode;

    /** The panel plugins can cover all rows/columns of the panel, but default is to not do so */
    bool cover_panel_rows:1;

    /** if the bars have been initialized */
    bool bars_created:1;

    /** if the tachos have been initialized */
    bool tachos_created:1;

    /** show title in panel */
    bool show_title:1;

    /** show labels in panel (GUI mode only) */
    bool show_labels:1;

    /** show units in textual view */
    bool show_units:1;

    /** show small spacings only in textual view */
    bool show_smallspacings:1;

    /** show colored bars (GUI mode only) */
    bool show_colored_bars:1;

    /**
     * suppress tooltip from overlapping widget and thereby crashing the plugin
     * or modifying the background
     */
    bool suppresstooltip:1;

    /**
     * double-click improvement as suggested on xfce4-goodies@berlios.de.
     * whether to execute command on double click
     */
    bool exec_command:1;

    /** suppress Hddtemp failure messages and any other messages */
    gboolean suppressmessage;

    /** use the progress-bar UI */
    e_displaystyles display_values_type;

    /** sensor update time */
    gint sensors_refresh_time;

    /* sensor relevant stuff */
    /**
     * no problem if less than 11 sensors, else will have to enlarge the
     * following arrays. NYI!!
     */
    gint num_sensorchips;

    /** contains the progress bar panels */
    /* FIXME:    Might be replaced by GPtrArray as well */
    t_labelledlevelbar *panels[MAX_NUM_CHIPS][MAX_NUM_FEATURES];
    /*    GArray *panels_array; */

    /** CSS provider for main dialog */
    GtkCssProvider *css_provider;

    /** contains the tacho panels */
    /* FIXME:    Might be replaced by GPtrArray as well */
    GtkWidget *tachos[MAX_NUM_CHIPS][MAX_NUM_FEATURES];

    /** sensor types to display values in appropriate format */
    GPtrArray *chips;

    /** command to excute */
    gchar *command_name;

    /** callback_id for doubleclicks */
    gint doubleclick_id;

    /** file name of config file for plugin */
    gchar *plugin_config_file;

    /** preferred dialog width */
    gint preferred_width;

    /** preferred dialog height */
    gint preferred_height;

    /** color value for the tachometers, useful for dark themes where lower brightness is required */
    gfloat val_tachos_color;

    /** desired alpha value for the tachometers */
    gfloat val_tachos_alpha;

}
t_sensors;


/* Regularly included functions in library */

/**
 * Create new sensors plugin data object
 * @param plugin: Pointer to panel plugin data
 * @param plugin_config_filename
 * @return pointer to newly allocated sensors object
 */
t_sensors* sensors_new (XfcePanelPlugin *plugin, gchar *plugin_config_filename);

/**
 * Initialize sensors structure with default values
 * @param sensors: pointer to sensors plugin data
 * @param plugin: Pointer to panel plugin data
 */
void sensors_init_default_values (t_sensors *sensors, XfcePanelPlugin *plugin);

#endif /* XFCE4_SENSORS_INTERFACE_PLUGIN_H */
