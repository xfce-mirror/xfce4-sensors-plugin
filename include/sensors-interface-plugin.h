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
#include <map>
#include <string>
#include <vector>

/* Package includes */
#include "sensors-interface-types.h"
#include "types.h"

/**
 * Sensors module
 */
struct t_sensors {
    /** pointer for Xfce Panel */
    XfcePanelPlugin *const plugin;

    /** eventbox to catch events */
    GtkWidget *eventbox;

    /** our XfceSensors widget */
    GtkWidget *widget_sensors;

    /** optional label for plugin */
    GtkWidget *panel_label_text;

    /** text UI style */
    struct {
        GtkWidget *draw_area;
        bool reset_size = true;
    } text;

    /** update the tooltip */
    guint timeout_id;

    /** font size for display in panel */
    std::string str_fontsize = "medium";
    gint val_fontsize = 2;

    /** temperature scale for display in panel */
    t_tempscale scale = CELSIUS;

    /** panel size to compute number of cols/columns */
    gint panel_size;

    /** Requested/allowed number of lines in text mode */
    gint lines_size = 3;

    /** panel orientation */
    XfcePanelPluginMode plugin_mode = XFCE_PANEL_PLUGIN_MODE_HORIZONTAL;

    /** automatic bar colors */
    bool automatic_bar_colors = false;

    /** The panel plugins can cover all rows/columns of the panel, but default is not to do so */
    bool cover_panel_rows = false;

    /** if the bars have been initialized */
    bool bars_created = false;

    /** if the tachos have been initialized */
    bool tachos_created = false;

    /** show title in panel */
    bool show_title = false;

    /** show labels in panel (GUI mode only) */
    bool show_labels = true;

    /** show units in textual view */
    bool show_units = true;

    /** show small spacings only in textual view */
    bool show_smallspacings = false;

    /**
     * suppress tooltip from overlapping widget and thereby crashing the plugin
     * or modifying the background
     */
    bool suppresstooltip = false;

    /**
     * double-click improvement as suggested on xfce4-goodies@berlios.de.
     * whether to execute command on double click
     */
    bool exec_command = true;

    /** suppress Hddtemp failure messages and any other messages */
    bool suppressmessage = false;

    /** use the progress-bar UI */
    e_displaystyles display_values_type = DISPLAY_TEXT;

    /** sensor update time, in seconds */
    gint sensors_refresh_time = 60;

    /* sensor relevant stuff */

    /** contains the progress bar panels */
    std::map<Ptr<t_chipfeature>, Ptr<t_labelledlevelbar>> panels;

    /** CSS provider for main dialog */
    GtkCssProvider *css_provider;

    /** contains the tacho panels */
    std::map<Ptr<t_chipfeature>, GtkWidget*> tachos;

    /** sensor types to display values in appropriate format */
    std::vector<Ptr<t_chip>> chips;

    /** command to execute */
    std::string command_name = "xfce4-sensors";

    /** callback_id for doubleclicks */
    gint doubleclick_id = 0;

    /** filepath of config file for plugin, or an empty string */
    std::string plugin_config_file;

    /** preferred dialog width */
    gint preferred_width = 675;

    /** preferred dialog height */
    gint preferred_height = 400;

    /** color value for the tachometers, useful for dark themes where lower brightness is required */
    gfloat tachos_color;

    /** desired alpha value for the tachometers */
    gfloat tachos_alpha;

    t_sensors(XfcePanelPlugin *plugin);
    ~t_sensors();
};


/* Regularly included functions in library */

/**
 * Create new sensors plugin data object
 * @param plugin: Pointer to panel plugin data
 * @param plugin_config_filename
 * @return pointer to newly allocated sensors object
 */
Ptr0<t_sensors> sensors_new (XfcePanelPlugin *plugin, const char *plugin_config_filename_orNull);

#endif /* XFCE4_SENSORS_INTERFACE_PLUGIN_H */
