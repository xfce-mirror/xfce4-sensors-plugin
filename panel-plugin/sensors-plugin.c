/* File: sensors-plugin.c
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

/* This plugin requires libsensors-3 and its headers in order to monitor
 * ordinary mainboard sensors!
 *
 * It also works with solely ACPI or hddtemp, but then with more limited
 * functionality.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gio/gio.h>
#include <glib.h>
#include <glib/gprintf.h> /* ain't included in glib.h! */
#include <gtk/gtk.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

/* Xfce includes */
#include <libxfce4util/libxfce4util.h>
#include <libxfce4ui/libxfce4ui.h>
#include <libxfce4panel/xfce-panel-plugin.h>

/* Package includes */
#include <configuration.h>
#include <middlelayer.h>
#include <sensors-interface.h>
#include <sensors-interface-plugin.h>
#include <tacho.h>

/* Local includes */
#include "sensors-plugin.h"

/* Definitions due to porting from Gtk2 to Gtk3 */
#define gtk_hbox_new(homogeneous, spacing) \
        gtk_box_new(GTK_ORIENTATION_HORIZONTAL, spacing)

#define gtk_vbox_new(homogeneous, spacing) \
        gtk_box_new(GTK_ORIENTATION_VERTICAL, spacing)

#define gtk_widget_reparent(wdgt, nwprnt) \
        gtk_container_remove(GTK_CONTAINER(gtk_widget_get_parent(wdgt)), wdgt); \
        gtk_container_add(GTK_CONTAINER(nwprnt), wdgt)

#ifndef DATADIR
# define DATADIR "/usr/local/share/"
#endif

#define DATASUBPATH "/xfce4/panel/"

#define XFCE4SENSORSPLUGINCSSFILE "xfce4-sensors-plugin.css"

/* -------------------------------------------------------------------------- */
static void
remove_gsource (guint gsource_id)
{
    GSource *ptr_gsource;
    if (gsource_id != 0) {
        ptr_gsource = g_main_context_find_source_by_id (NULL, gsource_id);
        if (ptr_gsource != NULL) {
            g_source_destroy (ptr_gsource);
            gsource_id = 0;
        }
    }
}

#define BAR_SIZE 8

/* -------------------------------------------------------------------------- */
static void
sensors_set_levelbar_size (GtkWidget *level_bar, int panel_height, XfcePanelPluginMode plugin_mode)
{
    g_return_if_fail (G_IS_OBJECT(level_bar));

    if (plugin_mode == XFCE_PANEL_PLUGIN_MODE_HORIZONTAL)
        gtk_widget_set_size_request (level_bar, BAR_SIZE+2, panel_height-BAR_SIZE);
    else
        gtk_widget_set_size_request (level_bar, panel_height-BAR_SIZE, BAR_SIZE+2);
}


/* -------------------------------------------------------------------------- */
static void
sensors_set_bar_color (t_labelledlevelbar *labelled_level_bar, const gchar *color_orNull,
                       const t_sensors *sensors)
{
    GtkWidget *level_bar;
    gchar gtk_css_data[256] = "levelbar block.";
    gchar level_bar_id[32];
    //gchar section_levelbarid[64];

    g_return_if_fail(labelled_level_bar != NULL);
    level_bar = labelled_level_bar->progressbar;

    g_return_if_fail (G_IS_OBJECT(level_bar));

    g_snprintf(level_bar_id, 32, "warn-high%lX", (unsigned long int) level_bar);

    g_strlcat(gtk_css_data, level_bar_id, sizeof(gtk_css_data));
    //g_strlcpy(section_levelbarid, str_gtkcssdata, sizeof(section_levelbarid));

    g_strlcat(gtk_css_data, " {\n", sizeof(gtk_css_data));

    if (sensors->show_colored_bars && color_orNull) {
        g_strlcat(gtk_css_data, "   background-color: ", sizeof(gtk_css_data));
        g_strlcat(gtk_css_data, color_orNull, sizeof(gtk_css_data));
        g_strlcat(gtk_css_data, ";\n", sizeof(gtk_css_data));
    }

    g_strlcat(gtk_css_data,   "   padding: 0px;\n"
                                "   border: 1px none black;\n"
                                "}", sizeof(gtk_css_data));

    gtk_level_bar_add_offset_value (GTK_LEVEL_BAR(level_bar),
                                  GTK_LEVEL_BAR_OFFSET_LOW,
                                  0.1);

    gtk_level_bar_add_offset_value (GTK_LEVEL_BAR(level_bar),
                                  "warn-low",
                                  0.2);

    gtk_level_bar_add_offset_value (GTK_LEVEL_BAR(level_bar),
                                  level_bar_id,
                                  0.8);

    gtk_level_bar_add_offset_value (GTK_LEVEL_BAR(level_bar),
                                  GTK_LEVEL_BAR_OFFSET_HIGH,
                                  0.9);

    gtk_css_provider_load_from_data (GTK_CSS_PROVIDER(labelled_level_bar->css_provider),
                                   gtk_css_data, -1, NULL);

    //DBG("unreferencing section '%s'.", section_levelbarid);
    //gtk_css_section_unref(section_levelbarid);
}


/* -------------------------------------------------------------------------- */
static double
sensors_get_percentage (const t_chipfeature *feature)
{
    double feature_value, mival, maxval, percentage;

    g_return_val_if_fail(feature != NULL, 0.0);

    feature_value = feature->raw_value;
    mival = feature->min_value;
    maxval = feature->max_value;
    percentage = (feature_value - mival) / (maxval - mival);

    if (percentage > 1.0) {
        percentage = 1.0;
    }
    else if (percentage <= 0.0) {
        percentage = 0.0;
    }

    return percentage;
}


/* -------------------------------------------------------------------------- */
static void
sensors_remove_graphical_panel (t_sensors *sensors)
{
    gint idx_sensorchip, idx_feature;

    g_return_if_fail(sensors != NULL);

    for (idx_sensorchip=0; idx_sensorchip < sensors->num_sensorchips; idx_sensorchip++) {
        t_chip *chip = g_ptr_array_index(sensors->chips, idx_sensorchip);
        g_assert (chip != NULL);

        for (idx_feature=0; idx_feature < chip->num_features; idx_feature++) {
            t_chipfeature *feature = g_ptr_array_index(chip->chip_features, idx_feature);
            g_assert (feature != NULL);

            if (feature->show) {
                t_labelledlevelbar *labelled_level_bar;

                labelled_level_bar = sensors->panels[idx_sensorchip][idx_feature];

                g_object_unref (labelled_level_bar->css_provider);
                labelled_level_bar->css_provider = NULL;

                if (sensors->show_labels) {
                    gtk_widget_hide (labelled_level_bar->label);
                    gtk_widget_destroy (labelled_level_bar->label);
                }
                gtk_widget_hide (labelled_level_bar->progressbar);
                gtk_widget_destroy (labelled_level_bar->progressbar);
                gtk_widget_hide (labelled_level_bar->databox);
                gtk_widget_destroy (labelled_level_bar->databox);

                g_free (labelled_level_bar);
                labelled_level_bar = NULL;
            }
        }
    }
    sensors->bars_created = FALSE;
    gtk_widget_hide (sensors->panel_label_text);
}


/* -------------------------------------------------------------------------- */
static void
sensors_remove_tacho_panel (t_sensors *sensors)
{
    gint idx_sensorchip, idx_feature;

    g_return_if_fail (sensors != NULL);

    for (idx_sensorchip=0; idx_sensorchip < sensors->num_sensorchips; idx_sensorchip++) {
        t_chip *chip = g_ptr_array_index(sensors->chips, idx_sensorchip);
        g_assert (chip != NULL);

        for (idx_feature=0; idx_feature < chip->num_features; idx_feature++) {
            t_chipfeature *feature = g_ptr_array_index (chip->chip_features, idx_feature);
            g_assert (feature != NULL);

            if (feature->show) {
                GtkWidget *tacho = sensors->tachos[idx_sensorchip][idx_feature];
                gtk_widget_hide (tacho);
                gtk_widget_destroy (tacho);
            }
        }
    }
    sensors->tachos_created = FALSE;
    gtk_widget_hide (sensors->panel_label_text);
}


/* -------------------------------------------------------------------------- */
static void
sensors_update_graphical_panel (const t_sensors *sensors)
{
    gint idx_sensorchips, idx_feature;

    for (idx_sensorchips=0; idx_sensorchips < sensors->num_sensorchips; idx_sensorchips++) {
        t_chip *chip = g_ptr_array_index(sensors->chips, idx_sensorchips);
        g_assert (chip != NULL);

        for (idx_feature=0; idx_feature < chip->num_features; idx_feature++) {
            t_chipfeature *feature = g_ptr_array_index(chip->chip_features, idx_feature);
            g_assert (feature != NULL);

            if (feature->show) {
                GtkWidget *level_bar;
                t_labelledlevelbar *labelled_level_bar;
                double percentage;

                labelled_level_bar = sensors->panels[idx_sensorchips][idx_feature];

                level_bar = labelled_level_bar->progressbar;
                g_return_if_fail (G_IS_OBJECT(level_bar));

                sensors_set_levelbar_size (level_bar, (int) sensors->panel_size, sensors->plugin_mode);
                percentage = sensors_get_percentage (feature);
                sensors_set_bar_color (labelled_level_bar, feature->color_orNull, sensors);

                gtk_level_bar_set_value(GTK_LEVEL_BAR(level_bar), percentage);
            }
        }
    }
}


/* -------------------------------------------------------------------------- */
static void
sensors_update_tacho_panel (const t_sensors *sensors)
{
    gint idx_sensorchips = 0, idx_feature = 0;
    gint panel_size = sensors->panel_size;

    if (!sensors->cover_panel_rows && xfce_panel_plugin_get_mode(sensors->plugin) != XFCE_PANEL_PLUGIN_MODE_DESKBAR)
        panel_size /= xfce_panel_plugin_get_nrows (sensors->plugin);

    for (idx_sensorchips=0; idx_sensorchips < sensors->num_sensorchips; idx_sensorchips++) {
        t_chip *chip = g_ptr_array_index(sensors->chips, idx_sensorchips);
        g_assert (chip != NULL);

        for (idx_feature=0; idx_feature < chip->num_features; idx_feature++) {
            t_chipfeature *feature = g_ptr_array_index(chip->chip_features, idx_feature);
            g_assert (feature != NULL);

            if (feature->show) {
                double percentage;
                GtkWidget *tacho = sensors->tachos[idx_sensorchips][idx_feature];
                g_assert(tacho != NULL);

                percentage = sensors_get_percentage (feature);
                gtk_sensorstacho_set_size(GTK_SENSORSTACHO(tacho), panel_size);
                gtk_sensorstacho_set_color(GTK_SENSORSTACHO(tacho), feature->color_orNull);
                gtk_sensorstacho_set_value(GTK_SENSORSTACHO(tacho), percentage);
            }
        }
    }

    gtk_widget_queue_draw (GTK_WIDGET(sensors->eventbox));
}


/* -------------------------------------------------------------------------- */
static void
sensors_add_graphical_display (t_sensors *sensors)
{
    gint idx_sensorchip, idx_feature;
    gboolean has_bars = FALSE;
    gchar *panel_label_text;
    gint size_panel = sensors->panel_size;

    g_return_if_fail (sensors != NULL);

    if (!sensors->cover_panel_rows && xfce_panel_plugin_get_mode(sensors->plugin) != XFCE_PANEL_PLUGIN_MODE_DESKBAR)
        size_panel /= xfce_panel_plugin_get_nrows (sensors->plugin);

    panel_label_text = g_strdup (_("<span><b>Sensors</b></span>"));
    gtk_label_set_markup (GTK_LABEL(sensors->panel_label_text), panel_label_text);
    g_free (panel_label_text);
    panel_label_text = NULL;

    gtk_container_set_border_width (GTK_CONTAINER(sensors->widget_sensors), 0);
    for (idx_sensorchip=0; idx_sensorchip < sensors->num_sensorchips; idx_sensorchip++) {
        t_chip *chip =  g_ptr_array_index (sensors->chips, idx_sensorchip);
        g_assert (chip != NULL);

        for (idx_feature=0; idx_feature < chip->num_features; idx_feature++) {
            t_chipfeature *feature = g_ptr_array_index (chip->chip_features, idx_feature);
            g_assert (feature != NULL);

            if (feature->show) {
                GdkDisplay *display;
                GdkScreen *screen;
                GtkWidget *widget_progbar, *widget_databox;
                t_labelledlevelbar *labelled_level_bar;

                has_bars = TRUE;

                widget_progbar = gtk_level_bar_new();

                if (sensors->plugin_mode == XFCE_PANEL_PLUGIN_MODE_HORIZONTAL) {
                    gtk_orientable_set_orientation (GTK_ORIENTABLE (widget_progbar),
                                                    GTK_ORIENTATION_VERTICAL);
                    gtk_level_bar_set_inverted(GTK_LEVEL_BAR(widget_progbar), TRUE);
                    widget_databox = gtk_hbox_new (FALSE, 0);
                }
                else {
                    gtk_orientable_set_orientation (GTK_ORIENTABLE (widget_progbar),
                                                    GTK_ORIENTATION_HORIZONTAL);
                    gtk_level_bar_set_inverted(GTK_LEVEL_BAR(widget_progbar), FALSE);
                    widget_databox = gtk_vbox_new (FALSE, 0);
                }

                sensors_set_levelbar_size (widget_progbar, size_panel,
                                           sensors->plugin_mode);

                /* save the panel elements */
                labelled_level_bar = g_new0 (t_labelledlevelbar, 1);
                labelled_level_bar->progressbar = widget_progbar;

                labelled_level_bar->css_provider = gtk_css_provider_new ();
                display = gdk_display_get_default ();
                screen = gdk_display_get_default_screen (display);

                gtk_style_context_add_provider_for_screen (screen,
                                 GTK_STYLE_PROVIDER (labelled_level_bar->css_provider),
                                 GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

                /* create the label stuff only if needed - saves some memory! */
                if (sensors->show_labels) {
                    gchar *bar_label_text = g_strdup (feature->name);
                    GtkWidget *widget_label;

                    widget_label = gtk_label_new (bar_label_text);
                    if (strlen (bar_label_text) >= 10) {
                        gtk_label_set_ellipsize (GTK_LABEL(widget_label), PANGO_ELLIPSIZE_END);
                        gtk_label_set_width_chars (GTK_LABEL(widget_label), 10);
                    }
                    else if (sensors->plugin_mode == XFCE_PANEL_PLUGIN_MODE_DESKBAR) {
                        gtk_label_set_ellipsize (GTK_LABEL(widget_label), PANGO_ELLIPSIZE_END);
                    }
                    g_free (bar_label_text);
                    bar_label_text = NULL;

                    if (sensors->plugin_mode == XFCE_PANEL_PLUGIN_MODE_VERTICAL)
                        gtk_label_set_angle (GTK_LABEL(widget_label), 270);
                    else
                        gtk_label_set_angle (GTK_LABEL(widget_label), 0);

                    labelled_level_bar->label = widget_label;

                    gtk_box_pack_start (GTK_BOX(widget_databox), widget_label, FALSE, FALSE, INNER_BORDER);
                }

                gtk_box_pack_start (GTK_BOX(widget_databox), widget_progbar, FALSE, FALSE, 0);

                labelled_level_bar->databox = widget_databox;
                sensors->panels[idx_sensorchip][idx_feature] = labelled_level_bar;

                gtk_widget_show_all (widget_databox);
                gtk_box_pack_start (GTK_BOX (sensors->widget_sensors), widget_databox, FALSE, FALSE, INNER_BORDER/2);
            }
        }
    }

    if (has_bars && !sensors->show_title) {
        gtk_widget_hide (sensors->panel_label_text);
    }
    else {
        gtk_widget_show (sensors->panel_label_text);
    }

    gtk_widget_hide (sensors->panel_label_data);

    sensors->bars_created = TRUE;
}


/* -------------------------------------------------------------------------- */
static void
sensors_add_tacho_display (t_sensors *sensors)
{
    int idx_sensorchips, idx_feature;
    gboolean has_tachos = FALSE;
    gchar *panel_label_text;
    gint panel_size;

    g_return_if_fail (sensors != NULL);

    panel_size = sensors->panel_size;

    if (!sensors->cover_panel_rows && xfce_panel_plugin_get_mode(sensors->plugin) != XFCE_PANEL_PLUGIN_MODE_DESKBAR)
        panel_size /= xfce_panel_plugin_get_nrows (sensors->plugin);

    panel_label_text = g_strdup (_("<span><b>Sensors</b></span>"));
    gtk_label_set_markup (GTK_LABEL(sensors->panel_label_text), panel_label_text);
    g_free (panel_label_text);
    panel_label_text = NULL;

    gtk_container_set_border_width (GTK_CONTAINER(sensors->widget_sensors), 0);
    for (idx_sensorchips=0; idx_sensorchips < sensors->num_sensorchips; idx_sensorchips++) {
        t_chip *chip = g_ptr_array_index(sensors->chips, idx_sensorchips);
        g_assert (chip != NULL);

        for (idx_feature=0; idx_feature < chip->num_features; idx_feature++) {
            t_chipfeature *feature;
            SensorsTachoStyle style;

            feature = g_ptr_array_index (chip->chip_features, idx_feature);
            g_assert (feature != NULL);

            style = style_MinGYR; /* default as has been for 10 years */

            if (feature->show) {
                GtkOrientation orientation;
                GtkWidget *tacho;

                has_tachos = TRUE;

                switch (feature->class) {
                    case VOLTAGE:
                    case POWER:
                    case CURRENT:
                    //case TEMPERATURE: // activate for developping only
                        style = style_MediumYGB;
                        break;
                    case ENERGY:
                        style = style_MaxRYG;
                        break;
                    default: // tacho_style = style_MinGYR; // already set per default
                        break;
                }

                orientation = (sensors->plugin_mode != XFCE_PANEL_PLUGIN_MODE_VERTICAL) ? GTK_ORIENTATION_HORIZONTAL : GTK_ORIENTATION_VERTICAL;
                tacho = gtk_sensorstacho_new(orientation, panel_size, style);

                /* create the label stuff only if needed - saves some memory! */
                if (sensors->show_labels) {
                    gtk_sensorstacho_set_color (GTK_SENSORSTACHO(tacho), feature->color_orNull);
                    gtk_sensorstacho_set_text (GTK_SENSORSTACHO(tacho), feature->name);
                }
                else {
                    gtk_sensorstacho_unset_color (GTK_SENSORSTACHO(tacho));
                    gtk_sensorstacho_unset_text (GTK_SENSORSTACHO(tacho));
                }

                sensors->tachos[idx_sensorchips][idx_feature] = tacho;

                gtk_widget_show (tacho);
                gtk_box_pack_start (GTK_BOX (sensors->widget_sensors), tacho, FALSE, FALSE, INNER_BORDER);
            }
        }
    }

    if (has_tachos && !sensors->show_title) {
        gtk_widget_hide (sensors->panel_label_text);
    }
    else {
        gtk_widget_show (sensors->panel_label_text);
    }

    gtk_widget_hide (sensors->panel_label_data);

    sensors->tachos_created = TRUE;
}


/* -------------------------------------------------------------------------- */
static void
sensors_show_graphical_display (t_sensors *sensors)
{
    const gchar *localcssfilewohome = "/.config/"
                                      DATASUBPATH
                                      "/"
                                      XFCE4SENSORSPLUGINCSSFILE;
    const gchar *globalcssfile = DATADIR
                                 "/"
                                 DATASUBPATH
                                 "/plugins/"
                                 XFCE4SENSORSPLUGINCSSFILE;

    if (!sensors->bars_created) {
        GdkDisplay *display;
        GdkScreen *screen;
        GFile *cssdatafile = NULL;
        gchar local_css_file[128];

        sensors->css_provider = gtk_css_provider_new ();
        display = gdk_display_get_default ();
        screen = gdk_display_get_default_screen (display);

        gtk_style_context_add_provider_for_screen (screen,
                                     GTK_STYLE_PROVIDER (sensors->css_provider),
                                     GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

        g_snprintf(local_css_file, sizeof(local_css_file), "%s/%s", getenv("HOME"), localcssfilewohome);

        if (g_file_test(local_css_file, G_FILE_TEST_EXISTS)) {
            DBG("found CSS file: %s\n", local_css_file);
            cssdatafile = g_file_new_for_path(local_css_file);
        }
        else if (g_file_test(globalcssfile, G_FILE_TEST_EXISTS)) {
            DBG("found CSS file: %s\n", globalcssfile);
            cssdatafile = g_file_new_for_path(globalcssfile);
        }

        if (NULL != cssdatafile) {
            gtk_css_provider_load_from_file(GTK_CSS_PROVIDER(sensors->css_provider), cssdatafile, NULL);
        }
        else {
            gchar *cssstring =  "levelbar block {\n"
                                "   min-height : 0px;\n"
                                "   min-width : 0px;\n"
                                "   border: 0px none;\n"
                                "   margin: 0px;\n"
                                "   padding: 0px;\n"
                                "}\n"
                                "levelbar block.full {\n"
                                "   background-color: "
                                COLOR_ERROR
                                ";\n"
                                "}\n"
                                "levelbar block.high {\n"
                                "   background-color: "
                                COLOR_WARN
                                ";\n"
                                "}\n"
                                "levelbar block.warn-low {\n"
                                "   background-color: "
                                COLOR_WARN
                                ";\n"
                                "}\n"
                                "levelbar block.low {\n"
                                "   background-color: "
                                COLOR_ERROR
                                ";\n"
                                "}\n";

            gtk_css_provider_load_from_data (GTK_CSS_PROVIDER(sensors->css_provider), cssstring,
                                   strlen(cssstring), NULL);
        }
        g_object_unref (sensors->css_provider);
        sensors->css_provider = NULL;

        sensors_add_graphical_display (sensors);
    }

    sensors_update_graphical_panel (sensors);
}


/* -------------------------------------------------------------------------- */
static void
sensors_show_tacho_display (t_sensors *sensors)
{
    if (!sensors->tachos_created) {
        val_colorvalue = sensors->val_tachos_color;
        val_alpha = sensors->val_tachos_alpha;

        sensors_add_tacho_display (sensors);
    }

    sensors_update_tacho_panel (sensors);
}


/* -------------------------------------------------------------------------- */
static int
determine_number_of_rows (const t_sensors *sensors)
{
    gint num_rows = -1, font_size;
    GtkStyleContext *style_context;
    PangoFontDescription *font_desc = NULL;
    PangoFontMask font_mask;
    gboolean is_absolute;

    g_return_val_if_fail(sensors != NULL, num_rows);

    style_context = gtk_widget_get_style_context(sensors->panel_label_data);

    gtk_style_context_get(style_context, GTK_STATE_FLAG_NORMAL, "font", font_desc, NULL);

    is_absolute = FALSE;
    font_mask = pango_font_description_get_set_fields (font_desc);
    if (font_mask>=PANGO_FONT_MASK_SIZE) {
        is_absolute = pango_font_description_get_size_is_absolute (font_desc);
        if (!is_absolute) {
            font_size = pango_font_description_get_size (font_desc) / 1000;
        }
    }

    if (font_mask<PANGO_FONT_MASK_SIZE || is_absolute) {
        font_size = 10; /* not many people will want a bigger font size,
                                and so only few rows are gonna be displayed. */
    }

    g_assert (font_size!=0);

    if (sensors->plugin_mode != XFCE_PANEL_PLUGIN_MODE_DESKBAR) {
        gint additional_offset, available_height;
        gdouble divisor;

        switch (font_size) {
            case 8:
                switch (sensors->val_fontsize) {
                    case 0: additional_offset=10; divisor = 12; break;
                    case 1: additional_offset=11; divisor = 12; break;
                    case 2: additional_offset=12; divisor = 12; break;
                    case 3: additional_offset=13; divisor = 13; break;
                    default: additional_offset=16; divisor = 17;
                }
                break;

            case 9:
                switch (sensors->val_fontsize) {
                    case 0: additional_offset=12; divisor = 13; break;
                    case 1: additional_offset=13; divisor = 13; break;
                    case 2: additional_offset=14; divisor = 14; break;
                    case 3: additional_offset=14; divisor = 17; break;
                    default: additional_offset=16; divisor = 20;
                }
                break;

            default: /* case 10 */
                 switch (sensors->val_fontsize) {
                    case 0: additional_offset=13; divisor = 14; break;
                    case 1: additional_offset=14; divisor = 14; break;
                    case 2: additional_offset=14; divisor = 14; break;
                    case 3: additional_offset=16; divisor = 17; break;
                    default: additional_offset=20; divisor = 20;
                }
        }
        available_height = sensors->panel_size - additional_offset;
        if (available_height < 0)
            available_height = 0;

        num_rows = (int) floor (available_height / divisor);
        if (num_rows < 0)
            num_rows = 0;

        num_rows++;
    }
    else num_rows = 1 << 30; /* that's enough, MAXINT would be nicer */

    /* fail-safe */
    if (num_rows<=0)
        num_rows = 1;

    return num_rows;
}


/* -------------------------------------------------------------------------- */
static int
determine_number_of_cols (gint num_rows, gint num_items_to_display)
{
    gint num_cols = 1;

    if (num_rows > 1) {
        if (num_items_to_display > num_rows)
            num_cols = (gint) ceil (num_items_to_display/ (float)num_rows);
    }
    else
        num_cols = num_items_to_display;

    return num_cols;
}


/* -------------------------------------------------------------------------- */
static void
sensors_set_text_panel_label (const t_sensors *sensors, gint num_cols, gint num_items_to_display)
{
    gint idx_currentcolumn, idx_sensorchip, idx_feature;
    gchar *label_text;

    if (sensors == NULL)
        return;
    if (num_items_to_display==0) {
        gtk_widget_hide (sensors->panel_label_data);
        return;
    }

    idx_currentcolumn = 0;
    label_text = g_strdup (""); /* don't use NULL because of g_strconcat */

    for (idx_sensorchip=0; idx_sensorchip < sensors->num_sensorchips; idx_sensorchip++) {
        t_chip *chip = g_ptr_array_index (sensors->chips, idx_sensorchip);
        g_assert (chip != NULL);

        for (idx_feature=0; idx_feature < chip->num_features; idx_feature++) {
            t_chipfeature *feature;
            gchar *help;

            feature = g_ptr_array_index (chip->chip_features, idx_feature);
            g_assert (feature != NULL);

            if (feature->show)
            {
                if(sensors->show_labels)
                {
                    if (feature->color_orNull)
                        help = g_strconcat (label_text, "<span foreground=\"", feature->color_orNull, "\" size=\"", sensors->str_fontsize, "\">", feature->name, NULL);
                    else
                        help = g_strconcat (label_text, "<span size=\"", sensors->str_fontsize, "\">", feature->name, NULL);

                    g_free(label_text);
                    label_text = g_strconcat (help, ":</span> ", NULL);
                    g_free(help);
                    help = NULL;
                }

                if (sensors->show_units)
                {
                    if (feature->color_orNull)
                        help = g_strconcat (label_text, "<span foreground=\"", feature->color_orNull, "\" size=\"", sensors->str_fontsize, "\">", feature->formatted_value, NULL);
                    else
                        help = g_strconcat (label_text, "<span size=\"", sensors->str_fontsize, "\">", feature->formatted_value, NULL);

                    g_free(label_text);
                    label_text = g_strconcat (help, "</span>", NULL);

                    g_free (help);
                    help = NULL;
                }
                else {
                    if (feature->color_orNull)
                        help = g_strdup_printf ("%s<span foreground=\"%s\" size=\"%s\">%.1f</span>", label_text, feature->color_orNull, sensors->str_fontsize, feature->raw_value);
                    else
                        help = g_strdup_printf ("%s<span size=\"%s\">%.1f</span>", label_text, sensors->str_fontsize, feature->raw_value);
                    g_free(label_text);
                    label_text = help;
                }


                if (sensors->plugin_mode == XFCE_PANEL_PLUGIN_MODE_DESKBAR) {
                    if (num_items_to_display > 1) {
                        help = g_strconcat (label_text, "\n", NULL);
                        g_free(label_text);
                        label_text = help;
                    }
                }
                else if (idx_currentcolumn < num_cols-1) {
                    if (sensors->show_smallspacings) {
                        help = g_strconcat (label_text, "  ", NULL);
                        g_free(label_text);
                        label_text = help;
                    }
                    else {
                        help = g_strconcat (label_text, " \t", NULL);
                        g_free(label_text);
                        label_text = help;
                    }
                    idx_currentcolumn++;
                }
                else if (num_items_to_display > 1) { /* do NOT add \n if last item */
                    help = g_strconcat (label_text, " \n", NULL);
                    g_free(label_text);
                    label_text = help;
                    idx_currentcolumn = 0;
                }
                num_items_to_display--;
            }
        }
    }

    g_assert (num_items_to_display==0);

    gtk_label_set_markup (GTK_LABEL(sensors->panel_label_data), label_text);

    gtk_widget_show (sensors->panel_label_data);

    if (sensors->plugin_mode == XFCE_PANEL_PLUGIN_MODE_VERTICAL)
    {
        gtk_widget_set_halign(sensors->panel_label_data, GTK_ALIGN_CENTER);
        gtk_label_set_angle(GTK_LABEL(sensors->panel_label_data), 270.0);
    }
    else
    {
        gtk_widget_set_valign(sensors->panel_label_data, GTK_ALIGN_CENTER);
        gtk_label_set_angle(GTK_LABEL(sensors->panel_label_data), 0.0);
    }

    g_free(label_text);
}


/* -------------------------------------------------------------------------- */
static int
count_number_checked_sensor_features (const t_sensors *sensors)
{
    gint idx_sensorchips, idx_feature, num_items_to_display;

    num_items_to_display = 0;

    for (idx_sensorchips=0; idx_sensorchips < sensors->num_sensorchips; idx_sensorchips++) {
        t_chip *chip = g_ptr_array_index (sensors->chips, idx_sensorchips);
        g_assert (chip!=NULL);

        for (idx_feature=0; idx_feature < chip->num_features; idx_feature++) {
            t_chipfeature *feature = g_ptr_array_index (chip->chip_features, idx_feature);
            g_assert (feature!=NULL);

            if (feature->valid && feature->show)
                num_items_to_display++;
        }
    }

    return num_items_to_display;
}


/* -------------------------------------------------------------------------- */
/* draw label with sensor values into panel's vbox */
static void
sensors_show_text_display (const t_sensors *sensors)
{
    gint num_items_to_display, num_rows, num_cols;

    /* count number of checked sensors to display.
       this could also be done by every toggle/untoggle action
       by putting this variable into t_sensors */
    num_items_to_display = count_number_checked_sensor_features (sensors);

    num_rows = sensors->lines_size; /* determine_number_of_rows (sensors); */

    if (sensors->show_title || num_items_to_display == 0)
        gtk_widget_show (sensors->panel_label_text);
    else
        gtk_widget_hide (sensors->panel_label_text);

    num_cols = determine_number_of_cols (num_rows, num_items_to_display);

    sensors_set_text_panel_label (sensors, num_cols, num_items_to_display);
}


/* -------------------------------------------------------------------------- */
/* Updates the sensor values */
static void
sensors_update_values (t_sensors *sensors)
{
    gint idx_sensorchip, idx_feature;

    for (idx_sensorchip=0; idx_sensorchip < sensors->num_sensorchips; idx_sensorchip++) {
        t_chip *chip = g_ptr_array_index (sensors->chips, idx_sensorchip);
        g_assert (chip!=NULL);

        for (idx_feature = 0; idx_feature<chip->num_features; idx_feature++) {
            t_chipfeature *feature = g_ptr_array_index (chip->chip_features, idx_feature);
            g_assert (feature!=NULL);

            if (feature->valid && feature->show)
            {
                double feature_value;
                gchar *tmp;
                gint result;

                result = sensor_get_value (chip, feature->address,
                                           &feature_value,
                                           &sensors->suppressmessage);

                if (result != 0) {
                    /* output to stdout on command line, not very useful for user, except for tracing problems */
                    g_printf ( _("Sensors Plugin:\n"
                    "Seems like there was a problem reading a sensor feature "
                    "value.\nProper proceeding cannot be guaranteed.\n") );
                    break;
                }
                tmp = g_new (gchar, 0);
                format_sensor_value (sensors->scale, feature,
                                     feature_value, &tmp);

                if (feature->formatted_value != NULL)
                    g_free (feature->formatted_value);

                feature->formatted_value = g_strdup (tmp);
                feature->raw_value = feature_value;

                g_free (tmp);
            }
        }
    }
}

/* -------------------------------------------------------------------------- */
/* create tooltip,see lines 440 and following */
static void
sensors_create_tooltip (const t_sensors *sensors)
{
    int idx_sensorchip, idx_feature;
    gchar *tooltip;
    gboolean first_textline = TRUE;

    tooltip = g_strdup (_("No sensors selected!"));

    for (idx_sensorchip=0; idx_sensorchip < sensors->num_sensorchips; idx_sensorchip++) {
        t_chip *chip;
        gboolean chipname_already_prepended = FALSE;

        chip = g_ptr_array_index (sensors->chips, idx_sensorchip);
        g_assert (chip!=NULL);

        for (idx_feature = 0; idx_feature<chip->num_features; idx_feature++)
        {
            t_chipfeature *feature = g_ptr_array_index (chip->chip_features, idx_feature);
            g_assert (feature!=NULL);

            if (feature->valid && feature->show)
            {
                gchar *tmp;

                if (!chipname_already_prepended)
                {
                    if (first_textline) {
                        g_free (tooltip);
                        tooltip = g_strconcat ("<b>", chip->sensorId, "</b>", NULL);
                        first_textline = FALSE;
                    }
                    else {
                        tmp = g_strconcat (tooltip, " \n<b>", chip->sensorId, "</b>", NULL);
                        g_free (tooltip);
                        tooltip = tmp;
                    }

                    chipname_already_prepended = TRUE;
                }

                tmp = g_strconcat (tooltip, "\n  ",
                                   feature->name, ": ", feature->formatted_value,
                                   NULL);
                g_free (tooltip);
                tooltip = tmp;
            }
        }
    }

    gtk_widget_set_tooltip_markup (GTK_WIDGET(sensors->eventbox), tooltip);

    g_free (tooltip);
}


/* -------------------------------------------------------------------------- */
static gboolean
sensors_show_panel (t_sensors *sensors)
{
    sensors_update_values (sensors);

    switch (sensors->display_values_type)
    {
      case DISPLAY_TACHO:
        sensors_show_tacho_display (sensors);
        break;
      case DISPLAY_BARS:
        sensors_show_graphical_display (sensors);
        break;
      case DISPLAY_TEXT:
        sensors_show_text_display (sensors);
	break;
    }

    if (sensors->plugin_mode == XFCE_PANEL_PLUGIN_MODE_VERTICAL)
    {
        gtk_label_set_angle(GTK_LABEL(sensors->panel_label_text), 270.0);
        gtk_widget_set_halign(sensors->panel_label_text, GTK_ALIGN_CENTER);
    }
    else
    {
        gtk_label_set_angle(GTK_LABEL(sensors->panel_label_text), 0);
        gtk_widget_set_valign(sensors->panel_label_text, GTK_ALIGN_CENTER);
    }

    if (!sensors->suppresstooltip)
        sensors_create_tooltip (sensors);

    return TRUE;
}


/* -------------------------------------------------------------------------- */
/* initialize box and label to pack them together */
static void
create_panel_widget (t_sensors *sensors)
{
    /* initialize a new vbox widget */
    sensors->widget_sensors = (sensors->plugin_mode == XFCE_PANEL_PLUGIN_MODE_HORIZONTAL) ? gtk_hbox_new (FALSE, 0) : gtk_vbox_new (FALSE, 0);

    /* initialize value label widget */
    sensors->panel_label_text = gtk_widget_new (GTK_TYPE_LABEL, "label", _("<span><b>Sensors</b></span>"), "use-markup", TRUE, "xalign", 0.0, "yalign", 0.5, "margin", INNER_BORDER, NULL);

    gtk_widget_show (sensors->panel_label_text);

    sensors->panel_label_data = gtk_label_new (NULL);
    gtk_widget_show (sensors->panel_label_data);

    /* add newly created label to box */
    gtk_box_pack_start (GTK_BOX (sensors->widget_sensors), sensors->panel_label_text, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (sensors->widget_sensors), sensors->panel_label_data, TRUE, TRUE, 0);

    /* create 'valued' label */
    sensors_show_panel (sensors);

    gtk_widget_show (sensors->widget_sensors);
}


/* -------------------------------------------------------------------------- */
static void
sensors_set_mode (XfcePanelPlugin *plugin, XfcePanelPluginMode plugin_mode, t_sensors *sensors)
{
    g_return_if_fail (plugin!=NULL && sensors!=NULL);
    g_return_if_fail (plugin_mode != sensors->plugin_mode);

    if (sensors->cover_panel_rows || plugin_mode == XFCE_PANEL_PLUGIN_MODE_DESKBAR)
        xfce_panel_plugin_set_small(plugin, FALSE);
    else
        xfce_panel_plugin_set_small(plugin, TRUE);

    sensors->plugin_mode = plugin_mode; /* now assign the new orientation */

    switch (sensors->display_values_type) {
        case DISPLAY_BARS:
            sensors_remove_graphical_panel (sensors);
            break;
        case DISPLAY_TACHO:
            sensors_remove_tacho_panel (sensors);
            break;
        case DISPLAY_TEXT:
            break;
    }

    gtk_widget_destroy (sensors->panel_label_data);
    gtk_widget_destroy (sensors->panel_label_text);
    gtk_widget_destroy (sensors->widget_sensors);
    sensors->panel_label_data = NULL;
    sensors->panel_label_text = NULL;
    sensors->widget_sensors = NULL;

    create_panel_widget (sensors);

    gtk_container_add (GTK_CONTAINER (sensors->eventbox), sensors->widget_sensors);
}


/* -------------------------------------------------------------------------- */
/* double-click improvement */
static gboolean
execute_command (GtkWidget *widget, GdkEventButton *event, gpointer data)
{
    g_return_val_if_fail (data != NULL, FALSE);

    if (event->type == GDK_2BUTTON_PRESS) {
        t_sensors *sensors = data;

        g_return_val_if_fail (sensors->exec_command, FALSE);

        // screen NULL, command, terminal=no, startup=yes, error=NULL
        xfce_spawn_command_line_on_screen (NULL, sensors->command_name, FALSE, TRUE, NULL);

        return TRUE;
    }
    else {
        return FALSE; // with FALSE, the event will not have been accepted by the handler and will be propagated further
    }
}


/* -------------------------------------------------------------------------- */
static void
sensors_free (XfcePanelPlugin *plugin, t_sensors *sensors)
{
    g_return_if_fail (sensors != NULL);

    /* stop association to libsensors and others*/
    cleanup_interfaces();

    /* remove timeout functions */
    remove_gsource (sensors->timeout_id);

    /* double-click improvement */
    remove_gsource (sensors->doubleclick_id);

    /* free structures and arrays */
    g_ptr_array_foreach (sensors->chips, free_chip, NULL);
    g_ptr_array_free (sensors->chips, TRUE);

    g_free (sensors->plugin_config_file);
    sensors->plugin_config_file = NULL;
    g_free (sensors->command_name);
    sensors->command_name = NULL;

    if (font)
    {
        g_free (font);
        font = NULL;
    }

    g_free (sensors->str_fontsize);
    sensors->str_fontsize = NULL;
    g_free (sensors);
    sensors = NULL;
}


/* -------------------------------------------------------------------------- */
static void
sensors_set_size (XfcePanelPlugin *plugin, int size, t_sensors *sensors)
{
    sensors->panel_size = size;

    /* when the orientation has toggled, maybe the size as well? */
    if (sensors->cover_panel_rows || xfce_panel_plugin_get_mode(plugin) == XFCE_PANEL_PLUGIN_MODE_DESKBAR)
        xfce_panel_plugin_set_small(plugin, FALSE);
    else
        xfce_panel_plugin_set_small(plugin, TRUE);

    /* update the panel widget */
    sensors_show_panel (sensors);
}


/* -------------------------------------------------------------------------- */
static void
show_title_toggled (GtkWidget *widget, t_sensors_dialog *dialog)
{
    if (dialog->sensors->display_values_type == DISPLAY_BARS)
        sensors_remove_graphical_panel (dialog->sensors);
    else if (dialog->sensors->display_values_type == DISPLAY_TACHO)
        sensors_remove_tacho_panel (dialog->sensors);

    dialog->sensors->show_title = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));

    sensors_show_panel (dialog->sensors);
}


/* -------------------------------------------------------------------------- */
static void
show_labels_toggled (GtkWidget *widget, t_sensors_dialog *dialog)
{
    if (dialog->sensors->display_values_type == DISPLAY_BARS)
        sensors_remove_graphical_panel (dialog->sensors);
    else if (dialog->sensors->display_values_type == DISPLAY_TACHO)
        sensors_remove_tacho_panel (dialog->sensors);

    dialog->sensors->show_labels = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));

    sensors_show_panel (dialog->sensors);
}


/* -------------------------------------------------------------------------- */
static void
show_colored_bars_toggled (GtkWidget *widget, t_sensors_dialog *dialog)
{
    if (dialog->sensors->display_values_type == DISPLAY_BARS)
        sensors_remove_graphical_panel (dialog->sensors);

    dialog->sensors->show_colored_bars = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));

    sensors_show_panel (dialog->sensors);
}


/* -------------------------------------------------------------------------- */
static void
display_style_changed_text (GtkWidget *widget, t_sensors_dialog *dialog)
{
    if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
        return;

    if (dialog->sensors->display_values_type == DISPLAY_BARS)
        sensors_remove_graphical_panel (dialog->sensors);
    else if (dialog->sensors->display_values_type == DISPLAY_TACHO)
        sensors_remove_tacho_panel (dialog->sensors);

    gtk_widget_hide (dialog->coloredBars_Box);
    gtk_widget_hide (dialog->fontSettings_Box);
    gtk_widget_show (dialog->font_Box);
    gtk_widget_show (dialog->Lines_Box);
    gtk_widget_show (dialog->unit_checkbox);
    gtk_widget_show (dialog->smallspacing_checkbox);
    gtk_widget_hide (dialog->colorvalue_slider_box);
    gtk_widget_hide (dialog->alpha_slider_box);

    dialog->sensors->display_values_type = DISPLAY_TEXT;

    sensors_show_panel (dialog->sensors);
}


/* -------------------------------------------------------------------------- */
static void
display_style_changed_bars (GtkWidget *widget, t_sensors_dialog *dialog)
{
    if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
        return;

    if (dialog->sensors->display_values_type == DISPLAY_TACHO)
        sensors_remove_tacho_panel (dialog->sensors);

    gtk_widget_show (dialog->coloredBars_Box);
    gtk_widget_hide (dialog->fontSettings_Box);
    gtk_widget_hide (dialog->font_Box);
    gtk_widget_hide (dialog->Lines_Box);
    gtk_widget_hide (dialog->unit_checkbox);
    gtk_widget_hide (dialog->smallspacing_checkbox);
    gtk_widget_hide (dialog->colorvalue_slider_box);
    gtk_widget_hide (dialog->alpha_slider_box);

    dialog->sensors->display_values_type = DISPLAY_BARS;

    sensors_show_panel (dialog->sensors);
}


/* -------------------------------------------------------------------------- */
static void
suppresstooltip_changed (GtkWidget *widget, t_sensors_dialog* dialog)
{
    dialog->sensors->suppresstooltip = !dialog->sensors->suppresstooltip;

    gtk_widget_set_has_tooltip(dialog->sensors->eventbox, !dialog->sensors->suppresstooltip);

    if (! dialog->sensors->suppresstooltip)
        sensors_create_tooltip (dialog->sensors);
}


/* -------------------------------------------------------------------------- */
static void
display_style_changed_tacho (GtkWidget *widget, t_sensors_dialog *dialog)
{
    if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
        return;

    if (dialog->sensors->display_values_type == DISPLAY_BARS)
        sensors_remove_graphical_panel(dialog->sensors);

    gtk_widget_hide (dialog->coloredBars_Box);
    gtk_widget_show (dialog->fontSettings_Box);
    gtk_widget_hide (dialog->font_Box);
    gtk_widget_hide (dialog->Lines_Box);
    gtk_widget_hide (dialog->unit_checkbox);
    gtk_widget_hide (dialog->smallspacing_checkbox);
    gtk_widget_show (dialog->colorvalue_slider_box);
    gtk_widget_show (dialog->alpha_slider_box);

    dialog->sensors->display_values_type = DISPLAY_TACHO;

    sensors_show_panel (dialog->sensors);
}


/* -------------------------------------------------------------------------- */
void
sensor_entry_changed_ (GtkWidget *widget, t_sensors_dialog *dialog)
{
    gint combo_box_active;
    t_chip *chip;

    combo_box_active = gtk_combo_box_get_active (GTK_COMBO_BOX (widget));

    chip = (t_chip*) g_ptr_array_index (dialog->sensors->chips,
                                        combo_box_active);
    gtk_label_set_label (GTK_LABEL (dialog->mySensorLabel), chip->description);

    gtk_tree_view_set_model (GTK_TREE_VIEW (dialog->myTreeView),
                    GTK_TREE_MODEL (dialog->myListStore[combo_box_active]));
}


/* -------------------------------------------------------------------------- */
static void
str_fontsize_change (GtkWidget *widget, t_sensors_dialog *dialog)
{
    int rows;

    g_free(dialog->sensors->str_fontsize);
    switch (gtk_combo_box_get_active (GTK_COMBO_BOX (widget)))
    {
        case 0: dialog->sensors->str_fontsize = g_strdup("x-small"); break;
        case 1: dialog->sensors->str_fontsize = g_strdup("small"); break;
        case 3: dialog->sensors->str_fontsize = g_strdup("large"); break;
        case 4: dialog->sensors->str_fontsize = g_strdup("x-large"); break;
        default: dialog->sensors->str_fontsize = g_strdup("medium");
    }

    dialog->sensors->val_fontsize = gtk_combo_box_get_active (GTK_COMBO_BOX (widget));

    rows = determine_number_of_rows (dialog->sensors);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (dialog->Lines_Spin_Box), (gdouble) rows);

    /* refresh the panel content */
    sensors_show_panel (dialog->sensors);
}


/* -------------------------------------------------------------------------- */
static void
lines_size_change (GtkWidget *widget, t_sensors_dialog *dialog)
{
    dialog->sensors->lines_size = (int) gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget));

    /* refresh the panel content */
    sensors_show_panel (dialog->sensors);
}


/* -------------------------------------------------------------------------- */
static void
cover_rows_toggled(GtkWidget *widget, t_sensors_dialog *dialog)
{
    dialog->sensors->cover_panel_rows = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));

    if (dialog->sensors->cover_panel_rows || xfce_panel_plugin_get_mode (dialog->sensors->plugin) == XFCE_PANEL_PLUGIN_MODE_DESKBAR)
        xfce_panel_plugin_set_small (dialog->sensors->plugin, FALSE);
    else
        xfce_panel_plugin_set_small (dialog->sensors->plugin, TRUE);
}


/* -------------------------------------------------------------------------- */
void
temperature_unit_change_ (GtkWidget *widget, t_sensors_dialog *dialog)
{
    /* toggle celsius-fahrenheit by use of mathematics ;) */
    dialog->sensors->scale = 1 - dialog->sensors->scale;

    /* refresh the panel content */
    sensors_show_panel (dialog->sensors);

    reload_listbox (dialog);
}

static gboolean
sensors_show_panel_cb (gpointer user_data)
{
    t_sensors *sensors = user_data;
    sensors_show_panel (sensors);
    return TRUE;
}

/* -------------------------------------------------------------------------- */
void
adjustment_value_changed_ (GtkWidget *widget, t_sensors_dialog* dialog)
{
    dialog->sensors->sensors_refresh_time = (gint) gtk_adjustment_get_value (GTK_ADJUSTMENT (widget));

    /* stop the timeout functions ... */
    remove_gsource (dialog->sensors->timeout_id);
    /* ... and start them again */
    dialog->sensors->timeout_id  = g_timeout_add (
        dialog->sensors->sensors_refresh_time * 1000,
        sensors_show_panel_cb, dialog->sensors);
}


static void
draw_units_changed (GtkWidget *widget, t_sensors_dialog* dialog)
{
    dialog->sensors->show_units = !dialog->sensors->show_units;
    sensors_show_text_display (dialog->sensors);
}


/* -------------------------------------------------------------------------- */
static void
draw_smallspacings_changed (GtkWidget *widget, t_sensors_dialog *dialog)
{
    dialog->sensors->show_smallspacings = !dialog->sensors->show_smallspacings;
    sensors_show_text_display (dialog->sensors);
}


/* -------------------------------------------------------------------------- */
static void
suppressmessage_changed (GtkWidget *widget, t_sensors_dialog *dialog)
{
    dialog->sensors->suppressmessage = !dialog->sensors->suppressmessage;
}


/* -------------------------------------------------------------------------- */
/* double-click improvement */
static void
execCommand_toggled (GtkWidget *widget, t_sensors_dialog *dialog)
{
    dialog->sensors->exec_command = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));

    if ( dialog->sensors->exec_command )
        g_signal_handler_unblock (dialog->sensors->eventbox, dialog->sensors->doubleclick_id);
    else
        g_signal_handler_block (dialog->sensors->eventbox, dialog->sensors->doubleclick_id);
}


/* -------------------------------------------------------------------------- */
void
minimum_changed_ (GtkCellRendererText *cell_renderer_text, gchar *path_str,
                  gchar *newmin, t_sensors_dialog *dialog)
{
    gint combo_box_active;
    float  value;
    GtkTreeModel *model;
    GtkTreePath *path;
    GtkTreeIter iter;
    t_chip *chip;
    t_chipfeature *feature;

    value = atof (newmin);

    combo_box_active = gtk_combo_box_get_active(GTK_COMBO_BOX (dialog->myComboBox));

    /* get model and path */
    model = (GtkTreeModel*) dialog->myListStore[combo_box_active];
    path = gtk_tree_path_new_from_string (path_str);

    /* get model iterator */
    gtk_tree_model_get_iter (model, &iter, path);

    /* set new value according to chosen scale */
    gtk_tree_store_set (GTK_TREE_STORE (model), &iter, eTreeColumn_Min, value, -1);
    chip = (t_chip*) g_ptr_array_index(dialog->sensors->chips, combo_box_active);

    feature = (t_chipfeature*) g_ptr_array_index(chip->chip_features, atoi(path_str));
    if (dialog->sensors->scale == FAHRENHEIT)
      value = (value - 32) * 5 / 9;
    feature->min_value = value;

    /* clean up */
    gtk_tree_path_free (path);

    if (dialog->sensors->display_values_type == DISPLAY_BARS)
        sensors_remove_graphical_panel (dialog->sensors);
    else if (dialog->sensors->display_values_type == DISPLAY_TACHO)
        sensors_remove_tacho_panel (dialog->sensors);

    /* update panel */
    sensors_show_panel (dialog->sensors);
}


/* -------------------------------------------------------------------------- */
void
maximum_changed_ (GtkCellRendererText *cell_renderer_text, gchar *path_str,
                  gchar *newmax, t_sensors_dialog *dialog)
{
    gint combo_box_active;
    float value;
    GtkTreeModel *model;
    GtkTreePath *path;
    GtkTreeIter iter;
    t_chip *chip;
    t_chipfeature *feature;

    value = atof (newmax);

    combo_box_active = gtk_combo_box_get_active(GTK_COMBO_BOX (dialog->myComboBox));

    /* get model and path */
    model = (GtkTreeModel*) dialog->myListStore[combo_box_active];
    path = gtk_tree_path_new_from_string (path_str);

    /* get model iterator */
    gtk_tree_model_get_iter (model, &iter, path);

    /* set new value according to chosen scale */
    gtk_tree_store_set (GTK_TREE_STORE (model), &iter, eTreeColumn_Max, value, -1);
    chip = (t_chip*) g_ptr_array_index(dialog->sensors->chips, combo_box_active);

    feature = (t_chipfeature*) g_ptr_array_index(chip->chip_features, atoi(path_str));
    if (dialog->sensors->scale == FAHRENHEIT)
      value = (value - 32) * 5 / 9;
    feature->max_value = value;

    /* clean up */
    gtk_tree_path_free (path);

    if (dialog->sensors->display_values_type == DISPLAY_BARS)
        sensors_remove_graphical_panel (dialog->sensors);
    else if (dialog->sensors->display_values_type == DISPLAY_TACHO)
        sensors_remove_tacho_panel (dialog->sensors);

    /* update panel */
    sensors_show_panel (dialog->sensors);
}


/* -------------------------------------------------------------------------- */
void
list_cell_color_edited_ (GtkCellRendererText *cell_renderer_text, const gchar *path_str,
                         const gchar *new_color, t_sensors_dialog *dialog)
{
    gint combo_box_active;
    GtkTreeModel *model;
    GtkTreePath *path;
    GtkTreeIter iter;
    gboolean hexColor;
    t_chip *chip;
    t_chipfeature *feature;

    /* store new color in appropriate array */
    hexColor = g_str_has_prefix (new_color, "#");

    if (hexColor && strlen (new_color) == 7) {
        int i;
        for (i=1; i<7; i++) {
            /* only save hex numbers! */
            if ( ! g_ascii_isxdigit (new_color[i]) )
                return;
        }

        combo_box_active = gtk_combo_box_get_active (GTK_COMBO_BOX (dialog->myComboBox));

        /* get model and path */
        model = (GtkTreeModel*) dialog->myListStore[combo_box_active];
        path = gtk_tree_path_new_from_string (path_str);

        /* get model iterator */
        gtk_tree_model_get_iter (model, &iter, path);

        /* set new value */
        gtk_tree_store_set (GTK_TREE_STORE (model), &iter, eTreeColumn_Color, new_color, -1);
        chip = (t_chip*) g_ptr_array_index(dialog->sensors->chips, combo_box_active);

        feature = (t_chipfeature*) g_ptr_array_index(chip->chip_features, atoi(path_str));
        g_free (feature->color_orNull);
        feature->color_orNull = g_strdup (new_color);

        /* clean up */
        gtk_tree_path_free (path);

        /* update panel */
        sensors_show_panel (dialog->sensors);
    }
    else if (strlen (new_color) == 0) {
        combo_box_active = gtk_combo_box_get_active (GTK_COMBO_BOX (dialog->myComboBox));

        /* get model and path */
        model = (GtkTreeModel*) dialog->myListStore[combo_box_active];
        path = gtk_tree_path_new_from_string (path_str);

        /* get model iterator */
        gtk_tree_model_get_iter (model, &iter, path);

        /* set new value */
        gtk_tree_store_set (GTK_TREE_STORE (model), &iter, eTreeColumn_Color, new_color, -1);
        chip = (t_chip*) g_ptr_array_index(dialog->sensors->chips, combo_box_active);

        feature = (t_chipfeature*) g_ptr_array_index(chip->chip_features, atoi(path_str));
        g_free (feature->color_orNull);
        feature->color_orNull = NULL;

        /* clean up */
        gtk_tree_path_free (path);

        /* update panel */
        sensors_show_panel (dialog->sensors);
    }
}


/* -------------------------------------------------------------------------- */
void
list_cell_text_edited_ (GtkCellRendererText *cell_renderer_text, gchar *path_str,
                        gchar *new_text, t_sensors_dialog *dialog)
{
    gint combo_box_active;
    GtkTreeModel *model;
    GtkTreePath *path;
    GtkTreeIter iter;
    t_chip *chip;
    t_chipfeature *feature;

    if (dialog->sensors->display_values_type == DISPLAY_BARS) {
        sensors_remove_graphical_panel (dialog->sensors);
    }
    else  if (dialog->sensors->display_values_type == DISPLAY_TACHO) {
        sensors_remove_tacho_panel (dialog->sensors);
    }
    combo_box_active = gtk_combo_box_get_active (GTK_COMBO_BOX (dialog->myComboBox));

    model = (GtkTreeModel*) dialog->myListStore [combo_box_active];
    path = gtk_tree_path_new_from_string (path_str);

    /* get model iterator */
    gtk_tree_model_get_iter (model, &iter, path);

    /* set new value */
    gtk_tree_store_set (GTK_TREE_STORE (model), &iter, eTreeColumn_Name, new_text, -1);
    chip = (t_chip*) g_ptr_array_index(dialog->sensors->chips, combo_box_active);

    feature = (t_chipfeature*) g_ptr_array_index (chip->chip_features, atoi (path_str));
    g_free(feature->name);
    feature->name = g_strdup (new_text);

    /* clean up */
    gtk_tree_path_free (path);

    /* update panel */
    sensors_show_panel (dialog->sensors);
}


/* -------------------------------------------------------------------------- */
void
list_cell_toggle_ (GtkCellRendererToggle *cell, gchar *path_str,  t_sensors_dialog *dialog)
{
    t_chip *chip;
    t_chipfeature *feature;
    gint combo_box_active;
    GtkTreeModel *model;
    GtkTreePath *path;
    GtkTreeIter iter;
    gboolean toggle_item;

    if (dialog->sensors->display_values_type == DISPLAY_BARS) {
        sensors_remove_graphical_panel (dialog->sensors);
    }
    else if (dialog->sensors->display_values_type == DISPLAY_TACHO) {
        sensors_remove_tacho_panel (dialog->sensors);
    }
    combo_box_active = gtk_combo_box_get_active (GTK_COMBO_BOX (dialog->myComboBox));

    model = (GtkTreeModel*) dialog->myListStore[combo_box_active];
    path = gtk_tree_path_new_from_string (path_str);

    /* get toggled iter */
    gtk_tree_model_get_iter (model, &iter, path);
    gtk_tree_model_get (model, &iter, 2, &toggle_item, -1);

    /* do something with the value */
    toggle_item ^= 1;

    if (!toggle_item)
    {
        GtkWidget *tacho = dialog->sensors->tachos[combo_box_active][atoi(path_str)];
        gtk_container_remove (GTK_CONTAINER(dialog->sensors->widget_sensors), tacho);
        gtk_widget_destroy(tacho);
        dialog->sensors->tachos[combo_box_active][atoi(path_str)] = NULL;
    }

    /* set new value */
    gtk_tree_store_set (GTK_TREE_STORE (model), &iter, eTreeColumn_Show, toggle_item, -1);
    chip = (t_chip*) g_ptr_array_index(dialog->sensors->chips, combo_box_active);

    feature = (t_chipfeature*) g_ptr_array_index(chip->chip_features, atoi(path_str));

    feature->show = toggle_item;

    /* clean up */
    gtk_tree_path_free (path);

    /* update tooltip and panel widget */
    sensors_show_panel (dialog->sensors);
}


/* -------------------------------------------------------------------------- */
static void
on_font_set (GtkWidget *widget, gpointer data)
{
    t_sensors *sensors = data;
    gchar *new_font, *tmp;

    new_font = g_strdup(gtk_font_chooser_get_font(GTK_FONT_CHOOSER(widget)));
    if (new_font)
    {
        tmp = font;
        font = new_font;
        g_free (tmp);
    }

    if (sensors->display_values_type == DISPLAY_TACHO)
    {
        /* refresh the panel content */
        sensors_update_tacho_panel (sensors);
    }
}


/* -------------------------------------------------------------------------- */
static void
tachos_colorvalue_changed_ (GtkWidget *widget, GtkScrollType type, gdouble value, t_sensors_dialog *dialog)
{
    t_sensors *sensors = dialog->sensors;
    g_assert (sensors!=NULL);

    sensors->val_tachos_color = val_colorvalue = value; //gtk_scale_button_get_value(GTK_SCALE_BUTTON(widget));
    DBG("new color value is %f.", val_colorvalue);

    if (sensors->display_values_type == DISPLAY_TACHO)
    {
        /* refresh the panel content */
        sensors_update_tacho_panel (sensors);
    }
}


/* -------------------------------------------------------------------------- */
static void
tachos_alpha_changed_ (GtkWidget *widget, GtkScrollType type, gdouble value, t_sensors_dialog *dialog)
{
    t_sensors *sensors = dialog->sensors;
    g_assert (sensors!=NULL);

    sensors->val_tachos_alpha = val_alpha = value; //gtk_scale_button_get_value(GTK_SCALE_BUTTON(widget));
    DBG("new alpha value is %f.", val_alpha);

    if (sensors->display_values_type==DISPLAY_TACHO)
    {
        /* refresh the panel content */
        sensors_update_tacho_panel (sensors);
    }
}


/* -------------------------------------------------------------------------- */
static void
add_ui_style_box (GtkWidget *vbox, t_sensors_dialog *dialog)
{
    GtkWidget *hbox, *label, *radioText, *radioBars, *radioTachos;

    hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_widget_show (hbox);

    label = gtk_label_new (_("UI style:"));
    radioText = gtk_radio_button_new_with_mnemonic(NULL, _("_text"));
    radioBars = gtk_radio_button_new_with_mnemonic_from_widget (GTK_RADIO_BUTTON(radioText), _("_progress bars"));
    radioTachos = gtk_radio_button_new_with_mnemonic_from_widget (GTK_RADIO_BUTTON(radioText), _("_tachos"));

    gtk_widget_show(radioText);
    gtk_widget_show(radioBars);
    gtk_widget_show(radioTachos);
    gtk_widget_show(label);

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radioText), dialog->sensors->display_values_type == DISPLAY_TEXT);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radioBars), dialog->sensors->display_values_type == DISPLAY_BARS);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radioTachos), dialog->sensors->display_values_type == DISPLAY_TACHO);

    gtk_box_pack_start(GTK_BOX (hbox), label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX (hbox), radioText, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX (hbox), radioBars, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX (hbox), radioTachos, FALSE, FALSE, 0);

    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);

    g_signal_connect (G_OBJECT (radioText), "toggled", G_CALLBACK (display_style_changed_text), dialog);
    g_signal_connect (G_OBJECT (radioBars), "toggled", G_CALLBACK (display_style_changed_bars), dialog);
    g_signal_connect (G_OBJECT (radioTachos), "toggled", G_CALLBACK (display_style_changed_tacho), dialog);
}


/* -------------------------------------------------------------------------- */
static void
add_labels_box (GtkWidget *vbox, t_sensors_dialog *dialog)
{
    GtkWidget *hbox, *checkButton;

    hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_widget_show (hbox);
    dialog->labels_Box = hbox;

    checkButton = gtk_check_button_new_with_mnemonic (_("Show _labels"));
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(checkButton), dialog->sensors->show_labels);
    gtk_widget_show (checkButton);

    gtk_box_pack_start (GTK_BOX (hbox), checkButton, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);

    g_signal_connect (G_OBJECT (checkButton), "toggled",  G_CALLBACK (show_labels_toggled), dialog);
}


/* -------------------------------------------------------------------------- */
static void
add_colored_bars_box (GtkWidget *vbox, t_sensors_dialog *dialog)
{
    GtkWidget *hbox, *checkButton;

    hbox = gtk_hbox_new (FALSE, BORDER);

    gtk_widget_show (hbox);
    dialog->coloredBars_Box = hbox;

    checkButton = gtk_check_button_new_with_mnemonic (_("Show colored _bars"));
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(checkButton), dialog->sensors->show_colored_bars);

    gtk_widget_show (checkButton);

    gtk_box_pack_start (GTK_BOX (hbox), checkButton, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);

    if (dialog->sensors->display_values_type != DISPLAY_BARS)
        gtk_widget_hide(dialog->coloredBars_Box);

    g_signal_connect (G_OBJECT (checkButton), "toggled", G_CALLBACK (show_colored_bars_toggled), dialog);
}


/* -------------------------------------------------------------------------- */
static void
add_title_box (GtkWidget *vbox, t_sensors_dialog *dialog)
{
    GtkWidget *hbox, *checkButton;

    hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_widget_show (hbox);

    checkButton = gtk_check_button_new_with_mnemonic (_("_Show title"));
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(checkButton), dialog->sensors->show_title);
    gtk_widget_show (checkButton);

    gtk_box_pack_start (GTK_BOX (hbox), checkButton, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);

    g_signal_connect (G_OBJECT (checkButton), "toggled", G_CALLBACK (show_title_toggled), dialog);
}


/* -------------------------------------------------------------------------- */
static void
add_lines_box (GtkWidget *vbox, t_sensors_dialog * dialog)
{
    GtkWidget *myLinesLabel;
    GtkWidget *myLinesBox;
    GtkWidget *myLinesSizeSpinBox;

    myLinesLabel = gtk_label_new_with_mnemonic (_("_Number of text lines:"));
    myLinesBox = gtk_hbox_new(FALSE, BORDER);
    myLinesSizeSpinBox = gtk_spin_button_new_with_range  (1.0, 10.0, 1.0);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(myLinesSizeSpinBox), (gdouble) dialog->sensors->lines_size);

    dialog->Lines_Box = myLinesBox;
    dialog->Lines_Spin_Box = myLinesSizeSpinBox;

    gtk_box_pack_start (GTK_BOX (myLinesBox), myLinesLabel, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (myLinesBox), myLinesSizeSpinBox, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), myLinesBox, FALSE, FALSE, 0);

    gtk_widget_show (myLinesLabel);
    gtk_widget_show (myLinesSizeSpinBox);
    gtk_widget_show (myLinesBox);

    if (dialog->sensors->display_values_type != DISPLAY_TEXT)
        gtk_widget_hide(dialog->Lines_Box);

    g_signal_connect (G_OBJECT (myLinesSizeSpinBox), "value-changed", G_CALLBACK (lines_size_change), dialog);
}


/* -------------------------------------------------------------------------- */
static void
add_cover_rows_box (GtkWidget *vbox, t_sensors_dialog *dialog)
{
    GtkWidget *myCheckBox;

    if (xfce_panel_plugin_get_mode(dialog->sensors->plugin) != XFCE_PANEL_PLUGIN_MODE_DESKBAR)
    {
        // The Xfce 4 panel can have several rows or columns. With such a mode,
        //  the plugins are allowed to span over all available rows/columns.
        //  When translating, "cover" might be replaced by "use" or "span".
        myCheckBox = gtk_check_button_new_with_mnemonic (_("_Cover all panel rows/columns"));
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(myCheckBox), dialog->sensors->cover_panel_rows);

        gtk_box_pack_start (GTK_BOX (vbox), myCheckBox, FALSE, FALSE, 0);

        gtk_widget_show (myCheckBox);

        g_signal_connect (G_OBJECT (myCheckBox), "toggled", G_CALLBACK (cover_rows_toggled), dialog);
    }
}


/* -------------------------------------------------------------------------- */
static void
add_str_fontsize_box (GtkWidget *vbox, t_sensors_dialog *dialog)
{
    GtkWidget *myFontLabel;
    GtkWidget *myFontBox;
    GtkWidget *myFontSizeComboBox;

    myFontLabel = gtk_label_new_with_mnemonic (_("F_ont size:"));
    myFontBox = gtk_hbox_new(FALSE, BORDER);
    myFontSizeComboBox = gtk_combo_box_text_new();

    dialog->font_Box = myFontBox;

    gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT(myFontSizeComboBox), _("x-small"));
    gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT(myFontSizeComboBox), _("small")  );
    gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT(myFontSizeComboBox), _("medium") );
    gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT(myFontSizeComboBox), _("large")  );
    gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT(myFontSizeComboBox), _("x-large"));
    gtk_combo_box_set_active (GTK_COMBO_BOX(myFontSizeComboBox), dialog->sensors->val_fontsize);

    gtk_box_pack_start (GTK_BOX (myFontBox), myFontLabel, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (myFontBox), myFontSizeComboBox, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), myFontBox, FALSE, FALSE, 0);

    gtk_widget_show (myFontLabel);
    gtk_widget_show (myFontSizeComboBox);
    gtk_widget_show (myFontBox);

    if (dialog->sensors->display_values_type != DISPLAY_TEXT)
        gtk_widget_hide(dialog->font_Box);

    g_signal_connect   (G_OBJECT (myFontSizeComboBox), "changed", G_CALLBACK (str_fontsize_change), dialog);
}


/* -------------------------------------------------------------------------- */
static void
add_font_settings_box (GtkWidget *vbox, t_sensors_dialog *dialog)
{
    GtkWidget *myFontLabel;
    GtkWidget *myFontSettingsBox;
    GtkWidget *myFontSettingsButton;

    myFontLabel = gtk_label_new_with_mnemonic (_("F_ont:"));
    myFontSettingsBox = gtk_hbox_new (FALSE, BORDER);
    myFontSettingsButton = gtk_font_button_new();
    gtk_font_chooser_set_font(GTK_FONT_CHOOSER(myFontSettingsButton), font);
    gtk_font_button_set_use_font(GTK_FONT_BUTTON(myFontSettingsButton), TRUE);

    dialog->fontSettings_Box = myFontSettingsBox;

    gtk_box_pack_start (GTK_BOX (myFontSettingsBox), myFontLabel, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (myFontSettingsBox), myFontSettingsButton, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), myFontSettingsBox, FALSE, FALSE, 0);

    gtk_widget_show (myFontLabel);
    gtk_widget_show (myFontSettingsButton);
    gtk_widget_show (myFontSettingsBox);

    if (dialog->sensors->display_values_type != DISPLAY_TACHO)
        gtk_widget_hide(dialog->fontSettings_Box);

    g_signal_connect (G_OBJECT(myFontSettingsButton), "font-set", G_CALLBACK(on_font_set), dialog->sensors);
}


/* -------------------------------------------------------------------------- */
static void
add_units_box (GtkWidget *vbox, t_sensors_dialog *dialog)
{
    dialog->unit_checkbox = gtk_check_button_new_with_mnemonic(_("Show _Units"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dialog->unit_checkbox), dialog->sensors->show_units);

    gtk_widget_show (dialog->unit_checkbox);

    gtk_box_pack_start (GTK_BOX (vbox), dialog->unit_checkbox, FALSE, TRUE, 0);

    if (dialog->sensors->display_values_type!=DISPLAY_TEXT)
        gtk_widget_hide(dialog->unit_checkbox);

    g_signal_connect (G_OBJECT (dialog->unit_checkbox), "toggled",  G_CALLBACK (draw_units_changed), dialog);
}


/* -------------------------------------------------------------------------- */
static void
add_smallspacings_box (GtkWidget *vbox, t_sensors_dialog *dialog)
{
    dialog->smallspacing_checkbox  = gtk_check_button_new_with_mnemonic(_("Small horizontal s_pacing"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dialog->smallspacing_checkbox), dialog->sensors->show_smallspacings);

    gtk_widget_show (dialog->smallspacing_checkbox);

    gtk_box_pack_start (GTK_BOX (vbox), dialog->smallspacing_checkbox, FALSE, TRUE, 0);

    if (dialog->sensors->display_values_type!=DISPLAY_TEXT)
        gtk_widget_hide(dialog->smallspacing_checkbox);

    g_signal_connect (G_OBJECT (dialog->smallspacing_checkbox), "toggled", G_CALLBACK (draw_smallspacings_changed), dialog);
}


/* -------------------------------------------------------------------------- */
static void
add_tachos_appearance_boxes(GtkWidget *vbox, t_sensors_dialog *dialog)
{
    GtkWidget *widget_hscale;
    GtkWidget *widget_label;

    dialog->alpha_slider_box = gtk_hbox_new(FALSE, INNER_BORDER);
    // Alpha value of the tacho coloring
    widget_label = gtk_label_new(_("Tacho color alpha value:"));
    gtk_widget_show (widget_label);
    gtk_box_pack_start (GTK_BOX (dialog->alpha_slider_box), widget_label, FALSE, TRUE, 0);
    widget_hscale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.0, 1.0, 0.01);
    gtk_range_set_value(GTK_RANGE(widget_hscale), dialog->sensors->val_tachos_alpha);
    gtk_widget_show (widget_hscale);
    g_signal_connect (G_OBJECT (widget_hscale), "change-value", G_CALLBACK (tachos_alpha_changed_), dialog);
    gtk_box_pack_start (GTK_BOX (dialog->alpha_slider_box), widget_hscale, TRUE, TRUE, 0);
    gtk_widget_show (dialog->alpha_slider_box);


    dialog->colorvalue_slider_box = gtk_hbox_new(FALSE, INNER_BORDER);
    // The value from HSV color model
    widget_label = gtk_label_new(_("Tacho color value:"));
    gtk_widget_show (widget_label);
    gtk_box_pack_start (GTK_BOX (dialog->colorvalue_slider_box), widget_label, FALSE, TRUE, 0);
    widget_hscale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.0, 1.0, 0.01);
    gtk_range_set_value(GTK_RANGE(widget_hscale), dialog->sensors->val_tachos_color);
    gtk_widget_show (widget_hscale);
    g_signal_connect (G_OBJECT (widget_hscale), "change-value", G_CALLBACK (tachos_colorvalue_changed_), dialog);
    gtk_box_pack_start (GTK_BOX (dialog->colorvalue_slider_box), widget_hscale, TRUE, TRUE, 0);
    gtk_widget_show (dialog->colorvalue_slider_box);

    gtk_box_pack_start (GTK_BOX (vbox), dialog->alpha_slider_box, FALSE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), dialog->colorvalue_slider_box, FALSE, TRUE, 0);

    if (dialog->sensors->display_values_type!=DISPLAY_TACHO)
    {
        gtk_widget_hide(dialog->alpha_slider_box);
        gtk_widget_hide(dialog->colorvalue_slider_box);
    }
}


/* -------------------------------------------------------------------------- */
static void
add_suppressmessage_box (GtkWidget *vbox, t_sensors_dialog *dialog)
{
    dialog->suppressmessage_checkbox  = gtk_check_button_new_with_mnemonic(_("Suppress messages"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dialog->suppressmessage_checkbox), dialog->sensors->suppressmessage);

    gtk_widget_show (dialog->suppressmessage_checkbox);

    gtk_box_pack_start (GTK_BOX (vbox), dialog->suppressmessage_checkbox, FALSE, TRUE, 0);

    g_signal_connect (G_OBJECT (dialog->suppressmessage_checkbox), "toggled", G_CALLBACK (suppressmessage_changed), dialog);
}


/* -------------------------------------------------------------------------- */
static void
add_suppresstooltips_box (GtkWidget *vbox, t_sensors_dialog *dialog)
{
    dialog->suppresstooltip_checkbox  = gtk_check_button_new_with_mnemonic(_("Suppress tooltip"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dialog->suppresstooltip_checkbox), dialog->sensors->suppresstooltip);

    gtk_widget_show (dialog->suppresstooltip_checkbox);

    gtk_box_pack_start (GTK_BOX (vbox), dialog->suppresstooltip_checkbox, FALSE, TRUE, 0);

    g_signal_connect (G_OBJECT (dialog->suppresstooltip_checkbox), "toggled", G_CALLBACK (suppresstooltip_changed), dialog);
}


/* -------------------------------------------------------------------------- */
/* double-click improvement */
static void
add_command_box (GtkWidget *vbox,  t_sensors_dialog *dialog)
{
    GtkWidget *myBox;

    myBox = gtk_hbox_new(FALSE, BORDER);

    dialog->myExecCommand_CheckBox = gtk_check_button_new_with_mnemonic (_("E_xecute on double click:"));
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->myExecCommand_CheckBox), dialog->sensors->exec_command);

    dialog->myCommandName_Entry = gtk_entry_new ();
    gtk_widget_set_size_request (dialog->myCommandName_Entry, 160, 25);

    gtk_entry_set_text( GTK_ENTRY(dialog->myCommandName_Entry), dialog->sensors->command_name );

    gtk_box_pack_start (GTK_BOX (myBox), dialog->myExecCommand_CheckBox, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (myBox), dialog->myCommandName_Entry, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), myBox, FALSE, FALSE, 0);

    gtk_widget_show (dialog->myExecCommand_CheckBox);
    gtk_widget_show (dialog->myCommandName_Entry);
    gtk_widget_show (myBox);

    g_signal_connect (G_OBJECT (dialog->myExecCommand_CheckBox), "toggled", G_CALLBACK (execCommand_toggled), dialog);
}


/* -------------------------------------------------------------------------- */
static void
add_view_frame (GtkWidget *notebook, t_sensors_dialog *dialog)
{
    GtkWidget *vbox, *label, *frame;

    vbox = gtk_vbox_new (FALSE, OUTER_BORDER);
    gtk_widget_show (vbox);

    label = gtk_label_new_with_mnemonic(_("_View"));
    gtk_widget_show (label);

    gtk_container_set_border_width (GTK_CONTAINER (vbox), BORDER<<1);

    gtk_notebook_append_page (GTK_NOTEBOOK(notebook), vbox, label);

    add_title_box (vbox, dialog);

    add_ui_style_box (vbox, dialog);
    add_labels_box (vbox, dialog);
    add_cover_rows_box(vbox, dialog);

    frame = gtk_frame_new(_("UI style options"));
    gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
    gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
    gtk_widget_show (frame);

    vbox = gtk_vbox_new (FALSE, OUTER_BORDER);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), OUTER_BORDER);
    gtk_container_add(GTK_CONTAINER(frame), vbox);
    gtk_widget_show (vbox);

    add_str_fontsize_box (vbox, dialog);
    add_font_settings_box (vbox, dialog);
    add_lines_box (vbox, dialog);
    add_colored_bars_box (vbox, dialog);
    add_units_box (vbox, dialog);
    add_smallspacings_box(vbox, dialog);
    add_tachos_appearance_boxes(vbox, dialog);
}


/* -------------------------------------------------------------------------- */
static void
add_miscellaneous_frame (GtkWidget *notebook, t_sensors_dialog *dialog)
{
    GtkWidget *vbox, *label;

    vbox = gtk_vbox_new (FALSE, OUTER_BORDER);
    gtk_widget_show (vbox);

    label = gtk_label_new_with_mnemonic (_("_Miscellaneous"));
    gtk_widget_show (label);

    gtk_container_set_border_width (GTK_CONTAINER (vbox), BORDER<<1);

    gtk_notebook_append_page (GTK_NOTEBOOK(notebook), vbox, label);

    add_update_time_box (vbox, dialog);
    add_suppressmessage_box(vbox, dialog);
    add_suppresstooltips_box(vbox, dialog);
    add_command_box (vbox, dialog);
}


/* -------------------------------------------------------------------------- */
static void
on_optionsDialog_response (GtkWidget *dlg, int response, t_sensors_dialog *sd)
{
    if (response==GTK_RESPONSE_OK || response==GTK_RESPONSE_DELETE_EVENT) {
        /* FIXME: save most of the content in this function,
           remove those toggle functions where possible. NYI */
        /* sensors_apply_options (sd); */
        g_free(sd->sensors->command_name);
        sd->sensors->command_name =
            g_strdup ( gtk_entry_get_text(GTK_ENTRY(sd->myCommandName_Entry)) );

            if (! sd->sensors->plugin_config_file)
                sd->sensors->plugin_config_file = xfce_panel_plugin_save_location (sd->sensors->plugin, TRUE);

            if (sd->sensors->plugin_config_file)
                sensors_write_config (sd->sensors->plugin, sd->sensors);
    }
    gtk_window_get_size (GTK_WINDOW(dlg), &sd->sensors->preferred_width, &sd->sensors->preferred_height);
    gtk_widget_destroy (sd->dialog);
    sd->dialog = NULL;

    xfce_panel_plugin_unblock_menu (sd->sensors->plugin);

    g_free(sd);
}


/* -------------------------------------------------------------------------- */
/* create sensor options box */
static void
sensors_create_options (XfcePanelPlugin *plugin, t_sensors *sensors)
{
    GtkWidget *dlg, *vbox, *notebook;
    t_sensors_dialog *sd;
    gchar *myToolTipText;

    xfce_panel_plugin_block_menu (plugin);

    dlg = xfce_titled_dialog_new_with_buttons(
                _("Sensors Plugin"),
                GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (plugin))),
                0,
                "gtk-close",
                GTK_RESPONSE_OK,
                NULL
            );

    xfce_titled_dialog_set_subtitle (XFCE_TITLED_DIALOG (dlg), _("Properties"));
    gtk_window_set_icon_name(GTK_WINDOW(dlg),"xfce-sensors");

    gtk_container_set_border_width (GTK_CONTAINER (dlg), 2);

    vbox = gtk_dialog_get_content_area(GTK_DIALOG(dlg));

    sd = g_new0 (t_sensors_dialog, 1);

    sd->sensors = sensors;
    sd->dialog = dlg;
    sd->plugin_dialog = TRUE;

    sd->myComboBox = gtk_combo_box_text_new();

    init_widgets (sd);

    gtk_combo_box_set_active (GTK_COMBO_BOX(sd->myComboBox), 0);

    notebook = gtk_notebook_new ();

    gtk_box_pack_start (GTK_BOX(vbox), notebook, TRUE, TRUE, 0);
    gtk_widget_show(notebook);

    add_sensors_frame (notebook, sd);

    myToolTipText = g_strdup(_("You can change a feature's properties such as name, colours, min/max value by double-clicking the entry, editing the content, and pressing \"Return\" or selecting a different field."));
    gtk_widget_set_tooltip_text (GTK_WIDGET(sd->myTreeView), myToolTipText);
    g_free (myToolTipText);

    add_view_frame (notebook, sd);
    add_miscellaneous_frame (notebook, sd);

    gtk_window_set_default_size (GTK_WINDOW(dlg), sensors->preferred_width, sensors->preferred_height);

    g_signal_connect (dlg, "response", G_CALLBACK(on_optionsDialog_response), sd);

    gtk_widget_show (dlg);
}


/* -------------------------------------------------------------------------- */
/**
 * Add event box to sensors panel
 * @param sensors Pointer to t_sensors structure
 */
static void
add_event_box (t_sensors *sensors)
{
    /* create eventbox to catch events on widget */
    sensors->eventbox = gtk_event_box_new ();
    gtk_widget_set_name (sensors->eventbox, "xfce_sensors");

    /* double-click improvement */
    sensors->doubleclick_id = g_signal_connect (G_OBJECT(sensors->eventbox),
                                                "button-press-event",
                                                G_CALLBACK (execute_command),
                                                (gpointer) sensors);
}


/* -------------------------------------------------------------------------- */
/**
 * Create sensors panel control
 * @param plugin Panel plugin proxy to create sensors plugin in
 */
static t_sensors *
create_sensors_control (XfcePanelPlugin *plugin)
{
    t_sensors *ptr_sensorsstruct;
    gchar *str_name_rc_file;

    str_name_rc_file = xfce_panel_plugin_lookup_rc_file(plugin);

    ptr_sensorsstruct = sensors_new (plugin, str_name_rc_file);

    ptr_sensorsstruct->plugin_mode = xfce_panel_plugin_get_mode (plugin);
    ptr_sensorsstruct->panel_size = xfce_panel_plugin_get_size (plugin);

    add_event_box (ptr_sensorsstruct);

    /* fill panel widget with boxes, strings, values, ... */
    create_panel_widget (ptr_sensorsstruct);

    /* finally add panel "sensors" to eventbox */
    gtk_container_add (GTK_CONTAINER (ptr_sensorsstruct->eventbox), ptr_sensorsstruct->widget_sensors);

    return ptr_sensorsstruct;
}


static void
sensors_show_about(XfcePanelPlugin *plugin, t_sensors *sensors)
{
   GdkPixbuf *icon;
   const gchar *auth[] = {"Fabian Nowak <timystery@xfce.org>",
                          "Stefan Ott",
                          NULL };
   icon = xfce_panel_pixbuf_from_source("xfce-sensors", NULL, 48);
   gtk_show_about_dialog(NULL,
      "logo", icon,
      "license", xfce_get_license_text (XFCE_LICENSE_TEXT_GPL),
      "version", PACKAGE_VERSION,
      "program-name", PACKAGE_NAME,
      "comments", _("Show sensor values from LM sensors, ACPI, hard disks, NVIDIA"),
      "website", "https://docs.xfce.org/panel-plugins/xfce4-sensors-plugin",
      "copyright", _("Copyright (c) 2004-2021\n"),
      "authors", auth, NULL);

   if(icon)
      g_object_unref(G_OBJECT(icon));
}


/* -------------------------------------------------------------------------- */
static void
sensors_plugin_construct (XfcePanelPlugin *plugin)
{
    t_sensors *sensors;

    xfce_textdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");

    /* declare callback functions for libxfce4sensors */
    adjustment_value_changed = &adjustment_value_changed_;
    sensor_entry_changed = &sensor_entry_changed_;
    list_cell_text_edited= &list_cell_text_edited_;
    list_cell_toggle = &list_cell_toggle_;
    list_cell_color_edited = &list_cell_color_edited_;
    minimum_changed = &minimum_changed_;
    maximum_changed = &maximum_changed_;
    temperature_unit_change = &temperature_unit_change_;

    sensors = create_sensors_control (plugin);

    sensors->plugin_config_file = xfce_panel_plugin_lookup_rc_file(plugin);
    sensors_read_config (plugin, sensors);

    /* use values from config file */
    gtk_widget_set_has_tooltip(sensors->eventbox, !sensors->suppresstooltip);

    if (sensors->cover_panel_rows || xfce_panel_plugin_get_mode(plugin) == XFCE_PANEL_PLUGIN_MODE_DESKBAR)
        xfce_panel_plugin_set_small(plugin, FALSE);
    else
        xfce_panel_plugin_set_small(plugin, TRUE);

    /* Try to resize the ptr_sensorsstruct to fit the user settings.
       Do also modify the tooltip text. */
    sensors_show_panel (sensors);

    sensors->timeout_id = g_timeout_add (sensors->sensors_refresh_time * 1000, (GSourceFunc) sensors_show_panel, sensors);

    g_signal_connect (plugin, "free-data", G_CALLBACK (sensors_free), sensors);

    sensors->plugin_config_file = xfce_panel_plugin_save_location (plugin, TRUE);
    /* saving seens to cause problems when closing the panel on fast multi-core CPUs; writing when closing the config dialog should suffice */
    /*g_signal_connect (plugin, "save", G_CALLBACK (sensors_write_config),
                      ptr_sensorsstruct);*/

    xfce_panel_plugin_menu_show_configure (plugin);

    g_signal_connect (plugin, "configure-plugin", G_CALLBACK (sensors_create_options), sensors);

    xfce_panel_plugin_menu_show_about(plugin);
    g_signal_connect (plugin, "about", G_CALLBACK (sensors_show_about), sensors);

    g_signal_connect (plugin, "size-changed", G_CALLBACK (sensors_set_size), sensors);

    g_signal_connect (plugin, "mode-changed", G_CALLBACK (sensors_set_mode), sensors);

    gtk_container_add (GTK_CONTAINER(plugin), sensors->eventbox);

    xfce_panel_plugin_add_action_widget (plugin, sensors->eventbox);

    gtk_widget_show (sensors->eventbox);
}

XFCE_PANEL_PLUGIN_REGISTER (sensors_plugin_construct);
