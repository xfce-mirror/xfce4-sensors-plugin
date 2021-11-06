/* sensors-plugin.cc
 * Part of xfce4-sensors-plugin
 *
 * Copyright (c) 2004-2017 Fabian Nowak <timystery@arcor.de>
 * Copyright (c) 2021 Jan Ziak <0xe2.0x9a.0x9b@xfce.org>
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

/* The fixes file has to be included before any other #include directives */
#include "xfce4++/util/fixes.h"

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
#include "plugin.h"
#include "sensors-plugin.h"

/* Definitions due to porting from Gtk2 to Gtk3 */
#define gtk_hbox_new(spacing) \
        gtk_box_new(GTK_ORIENTATION_HORIZONTAL, spacing)

#define gtk_vbox_new(spacing) \
        gtk_box_new(GTK_ORIENTATION_VERTICAL, spacing)

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
        if (ptr_gsource != NULL)
            g_source_destroy (ptr_gsource);
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
sensors_set_bar_color (t_labelledlevelbar *bar, const gchar *color_orNull,
                       const t_sensors *sensors)
{
    gchar css[1024] = "";
    GtkWidget *bar_widget;

    g_return_if_fail (bar != NULL);
    bar_widget = bar->progressbar;
    g_return_if_fail (G_IS_OBJECT(bar_widget));

    gtk_level_bar_add_offset_value (GTK_LEVEL_BAR(bar_widget), BAR_OFFSET_MARKER_NORMAL, 0.8);
    gtk_level_bar_add_offset_value (GTK_LEVEL_BAR(bar_widget), BAR_OFFSET_MARKER_WARN, 0.9);
    gtk_level_bar_add_offset_value (GTK_LEVEL_BAR(bar_widget), BAR_OFFSET_MARKER_ERROR, 1.0);

    if (sensors->automatic_bar_colors) {
        g_strlcat (css, "levelbar block." BAR_OFFSET_MARKER_NORMAL " {\n", sizeof(css));
        g_strlcat (css, "  background-color: " COLOR_NORMAL ";\n", sizeof(css));
        g_strlcat (css, "}\n", sizeof(css));

        g_strlcat (css, "levelbar block." BAR_OFFSET_MARKER_WARN " {\n", sizeof(css));
        g_strlcat (css, "  background-color: " COLOR_WARN ";\n", sizeof(css));
        g_strlcat (css, "}\n", sizeof(css));

        g_strlcat (css, "levelbar block." BAR_OFFSET_MARKER_ERROR " {\n", sizeof(css));
        g_strlcat (css, "  background-color: " COLOR_ERROR ";\n", sizeof(css));
        g_strlcat (css, "}\n", sizeof(css));
    }
    else if (color_orNull) {
        g_strlcat (css, "levelbar block." BAR_OFFSET_MARKER_NORMAL " {\n", sizeof(css));
        g_strlcat (css, "   background-color: ", sizeof(css));
        g_strlcat (css, color_orNull, sizeof(css));
        g_strlcat (css, ";\n", sizeof(css));
        g_strlcat (css, "}\n", sizeof(css));

        g_strlcat (css, "levelbar block." BAR_OFFSET_MARKER_WARN " {\n", sizeof(css));
        g_strlcat (css, "   background-color: ", sizeof(css));
        g_strlcat (css, color_orNull, sizeof(css));
        g_strlcat (css, ";\n", sizeof(css));
        g_strlcat (css, "}\n", sizeof(css));

        g_strlcat (css, "levelbar block." BAR_OFFSET_MARKER_ERROR " {\n", sizeof(css));
        g_strlcat (css, "   background-color: ", sizeof(css));
        g_strlcat (css, color_orNull, sizeof(css));
        g_strlcat (css, ";\n", sizeof(css));
        g_strlcat (css, "}\n", sizeof(css));
    }

    if (bar->css_data != css)
    {
        bar->css_data = css;
        gtk_css_provider_load_from_data (bar->css_provider, css, -1, NULL);
    }
}


/* -------------------------------------------------------------------------- */
static double
sensors_get_percentage (const Ptr<t_chipfeature> &feature)
{
    double val = feature->raw_value;
    double minval = feature->min_value;
    double maxval = feature->max_value;
    if (G_LIKELY (!isnan (val) && minval < maxval))
    {
        double percentage = (val - minval) / (maxval - minval);
        if (percentage < 0)
            percentage = 0;
        else if (percentage > 1)
            percentage = 1;
        return percentage;
    }
    else
        return 0;
}


/* -------------------------------------------------------------------------- */
static void
sensors_remove_bars_panel (t_sensors *sensors)
{
    g_return_if_fail(sensors != NULL);

    for (size_t idx_sensorchip = 0; idx_sensorchip < sensors->chips.size(); idx_sensorchip++) {
        auto chip = sensors->chips[idx_sensorchip];
        for (size_t idx_feature = 0; idx_feature < chip->chip_features.size(); idx_feature++) {
            auto feature = chip->chip_features[idx_feature];
            if (feature->show) {
                t_labelledlevelbar *bar = sensors->panels[idx_sensorchip][idx_feature];

                g_object_unref (bar->css_provider);
                bar->css_provider = NULL;

                if (sensors->show_labels) {
                    gtk_widget_hide (bar->label);
                    gtk_widget_destroy (bar->label);
                }
                gtk_widget_hide (bar->progressbar);
                gtk_widget_destroy (bar->progressbar);
                gtk_widget_hide (bar->databox);
                gtk_widget_destroy (bar->databox);

                delete bar;
                sensors->panels[idx_sensorchip][idx_feature] = NULL;
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
    g_return_if_fail (sensors != NULL);

    for (auto chip : sensors->chips) {
        for (auto feature : chip->chip_features) {
            auto tacho = sensors->tachos.find(feature);
            if (tacho != sensors->tachos.end()) {
                GtkWidget *w = tacho->second;
                sensors->tachos.erase(tacho);
                gtk_widget_hide (w);
                gtk_widget_destroy (w);
            }
        }
    }
    sensors->tachos_created = FALSE;
    gtk_widget_hide (sensors->panel_label_text);
}


/* -------------------------------------------------------------------------- */
static void
sensors_update_bars_panel (const t_sensors *sensors)
{
    for (size_t idx_sensorchip = 0; idx_sensorchip < sensors->chips.size(); idx_sensorchip++) {
        auto chip = sensors->chips[idx_sensorchip];
        for (size_t idx_feature = 0; idx_feature < chip->chip_features.size(); idx_feature++) {
            auto feature = chip->chip_features[idx_feature];
            if (feature->show) {
                GtkWidget *level_bar;
                t_labelledlevelbar *labelled_level_bar;
                double percentage;

                labelled_level_bar = sensors->panels[idx_sensorchip][idx_feature];

                level_bar = labelled_level_bar->progressbar;
                g_return_if_fail (G_IS_OBJECT(level_bar));

                sensors_set_levelbar_size (level_bar, (int) sensors->panel_size, sensors->plugin_mode);
                percentage = sensors_get_percentage (feature);
                sensors_set_bar_color (labelled_level_bar, feature->color_orEmpty.c_str(), sensors);

                gtk_level_bar_set_value(GTK_LEVEL_BAR(level_bar), percentage);
            }
        }
    }
}


/* -------------------------------------------------------------------------- */
static void
sensors_update_tacho_panel (const t_sensors *sensors)
{
    gint panel_size = sensors->panel_size;

    if (!sensors->cover_panel_rows && xfce_panel_plugin_get_mode(sensors->plugin) != XFCE_PANEL_PLUGIN_MODE_DESKBAR)
        panel_size /= xfce_panel_plugin_get_nrows (sensors->plugin);

    for (auto chip : sensors->chips) {
        for (auto feature : chip->chip_features) {
            auto tacho = sensors->tachos.find(feature);
            if (tacho != sensors->tachos.end()) {
                GtkWidget *w = tacho->second;
                double percentage = sensors_get_percentage (feature);
                gtk_sensorstacho_set_size(GTK_SENSORSTACHO(w), panel_size);
                gtk_sensorstacho_set_color(GTK_SENSORSTACHO(w), feature->color_orEmpty.c_str());
                gtk_sensorstacho_set_value(GTK_SENSORSTACHO(w), percentage);
            }
        }
    }

    gtk_widget_queue_draw (GTK_WIDGET(sensors->eventbox));
}


/* -------------------------------------------------------------------------- */
static void
sensors_add_bars_display (t_sensors *sensors)
{
    bool has_bars = false;
    gint size_panel = sensors->panel_size;

    g_return_if_fail (sensors != NULL);

    if (!sensors->cover_panel_rows && xfce_panel_plugin_get_mode(sensors->plugin) != XFCE_PANEL_PLUGIN_MODE_DESKBAR)
        size_panel /= xfce_panel_plugin_get_nrows (sensors->plugin);

    gtk_label_set_markup (GTK_LABEL(sensors->panel_label_text), _("<span><b>Sensors</b></span>"));

    gtk_container_set_border_width (GTK_CONTAINER(sensors->widget_sensors), 0);
    for (size_t idx_sensorchip = 0; idx_sensorchip < sensors->chips.size(); idx_sensorchip++) {
        auto chip = sensors->chips[idx_sensorchip];

        for (size_t idx_feature = 0; idx_feature < chip->chip_features.size(); idx_feature++) {
            auto feature = chip->chip_features[idx_feature];
            if (feature->show) {
                GtkWidget *widget_progbar, *widget_databox;

                has_bars = true;

                widget_progbar = gtk_level_bar_new();

                if (sensors->plugin_mode == XFCE_PANEL_PLUGIN_MODE_HORIZONTAL) {
                    gtk_orientable_set_orientation (GTK_ORIENTABLE (widget_progbar),
                                                    GTK_ORIENTATION_VERTICAL);
                    gtk_level_bar_set_inverted(GTK_LEVEL_BAR(widget_progbar), TRUE);
                    widget_databox = gtk_hbox_new (0);
                }
                else {
                    gtk_orientable_set_orientation (GTK_ORIENTABLE (widget_progbar),
                                                    GTK_ORIENTATION_HORIZONTAL);
                    gtk_level_bar_set_inverted(GTK_LEVEL_BAR(widget_progbar), FALSE);
                    widget_databox = gtk_vbox_new (0);
                }

                sensors_set_levelbar_size (widget_progbar, size_panel,
                                           sensors->plugin_mode);

                /* save the panel elements */
                auto bar = new t_labelledlevelbar();
                bar->progressbar = widget_progbar;

                bar->css_data = "";
                bar->css_provider = gtk_css_provider_new ();

                gtk_style_context_add_provider (gtk_widget_get_style_context (widget_progbar),
                                                GTK_STYLE_PROVIDER (bar->css_provider),
                                                GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

                /* create the label stuff only if needed - saves some memory! */
                if (sensors->show_labels) {
                    std::string bar_label_text = feature->name;
                    GtkWidget *widget_label;

                    widget_label = gtk_label_new (bar_label_text.c_str());
                    if (bar_label_text.size() >= 10) {
                        gtk_label_set_ellipsize (GTK_LABEL(widget_label), PANGO_ELLIPSIZE_END);
                        gtk_label_set_width_chars (GTK_LABEL(widget_label), 10);
                    }
                    else if (sensors->plugin_mode == XFCE_PANEL_PLUGIN_MODE_DESKBAR) {
                        gtk_label_set_ellipsize (GTK_LABEL(widget_label), PANGO_ELLIPSIZE_END);
                    }

                    if (sensors->plugin_mode == XFCE_PANEL_PLUGIN_MODE_VERTICAL)
                        gtk_label_set_angle (GTK_LABEL(widget_label), 270);
                    else
                        gtk_label_set_angle (GTK_LABEL(widget_label), 0);

                    bar->label = widget_label;

                    gtk_box_pack_start (GTK_BOX(widget_databox), widget_label, FALSE, FALSE, INNER_BORDER);
                }

                gtk_box_pack_start (GTK_BOX(widget_databox), widget_progbar, FALSE, FALSE, 0);

                bar->databox = widget_databox;
                sensors->panels[idx_sensorchip][idx_feature] = bar;

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

    gtk_widget_hide (sensors->text.draw_area);

    sensors->bars_created = TRUE;
}


/* -------------------------------------------------------------------------- */
static void
sensors_add_tacho_display (t_sensors *sensors)
{
    bool has_tachos = false;

    g_return_if_fail (sensors != NULL);

    gint panel_size = sensors->panel_size;

    if (!sensors->cover_panel_rows && xfce_panel_plugin_get_mode(sensors->plugin) != XFCE_PANEL_PLUGIN_MODE_DESKBAR)
        panel_size /= xfce_panel_plugin_get_nrows (sensors->plugin);

    gtk_label_set_markup (GTK_LABEL(sensors->panel_label_text), _("<span><b>Sensors</b></span>"));

    gtk_container_set_border_width (GTK_CONTAINER(sensors->widget_sensors), 0);
    for (auto chip : sensors->chips) {
        for (auto feature : chip->chip_features) {
            if (feature->show) {
                has_tachos = true;

                SensorsTachoStyle style;
                switch (feature->cls) {
                    case VOLTAGE:
                    case POWER:
                    case CURRENT:
                    //case TEMPERATURE: // activate for developping only
                        style = style_MediumYGB;
                        break;
                    case ENERGY:
                        style = style_MaxRYG;
                        break;
                    default:
                        style = style_MinGYR; /* default as has been for 10 years */
                }

                GtkOrientation orientation = (sensors->plugin_mode != XFCE_PANEL_PLUGIN_MODE_VERTICAL) ? GTK_ORIENTATION_HORIZONTAL : GTK_ORIENTATION_VERTICAL;
                GtkWidget *tacho = gtk_sensorstacho_new(orientation, panel_size, style);

                /* create the label stuff only if needed - saves some memory! */
                if (sensors->show_labels) {
                    gtk_sensorstacho_set_color (GTK_SENSORSTACHO(tacho), feature->color_orEmpty.c_str());
                    gtk_sensorstacho_set_text (GTK_SENSORSTACHO(tacho), feature->name.c_str());
                }
                else {
                    gtk_sensorstacho_unset_color (GTK_SENSORSTACHO(tacho));
                    gtk_sensorstacho_unset_text (GTK_SENSORSTACHO(tacho));
                }

                sensors->tachos[feature] = tacho;

                gtk_widget_show (tacho);
                gtk_box_pack_start (GTK_BOX (sensors->widget_sensors), tacho, FALSE, FALSE, INNER_BORDER);
            }
        }
    }

    if (has_tachos && !sensors->show_title)
        gtk_widget_hide (sensors->panel_label_text);
    else
        gtk_widget_show (sensors->panel_label_text);

    gtk_widget_hide (sensors->text.draw_area);

    sensors->tachos_created = TRUE;
}


/* -------------------------------------------------------------------------- */
static void
sensors_show_bars_display (t_sensors *sensors)
{
    const gchar *localcssfilewohome = "/.config/" DATASUBPATH "/" XFCE4SENSORSPLUGINCSSFILE;
    const gchar *globalcssfile = DATADIR "/" DATASUBPATH "/plugins/" XFCE4SENSORSPLUGINCSSFILE;

    if (!sensors->bars_created) {
        GdkScreen *screen;
        GFile *cssdatafile = NULL;

        sensors->css_provider = gtk_css_provider_new ();

        auto local_css_file = xfce4::sprintf ("%s/%s", getenv("HOME"), localcssfilewohome);
        if (g_file_test(local_css_file.c_str(), G_FILE_TEST_EXISTS)) {
            DBG("found CSS file: %s\n", local_css_file.c_str());
            cssdatafile = g_file_new_for_path(local_css_file.c_str());
        }
        else if (g_file_test(globalcssfile, G_FILE_TEST_EXISTS)) {
            DBG("found CSS file: %s\n", globalcssfile);
            cssdatafile = g_file_new_for_path(globalcssfile);
        }

        if (cssdatafile) {
            gtk_css_provider_load_from_file(GTK_CSS_PROVIDER(sensors->css_provider), cssdatafile, NULL);
        }
        else {
            const gchar *cssstring =
                "levelbar block {\n"
                "  min-height : 0px;\n"
                "  min-width : 0px;\n"
                "  border: 0px none;\n"
                "  margin: 0px;\n"
                "  padding: 0px;\n"
                "}\n";

            gtk_css_provider_load_from_data (GTK_CSS_PROVIDER(sensors->css_provider), cssstring, -1, NULL);
        }

        screen = gdk_display_get_default_screen (gdk_display_get_default ());
        gtk_style_context_add_provider_for_screen (screen,
            GTK_STYLE_PROVIDER (sensors->css_provider),
            GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

        g_object_unref (sensors->css_provider);
        sensors->css_provider = NULL;

        sensors_add_bars_display (sensors);
    }

    sensors_update_bars_panel (sensors);
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
static gint
determine_number_of_rows (const t_sensors *sensors)
{
    gint num_rows = -1;

    g_return_val_if_fail(sensors != NULL, num_rows);
    g_return_val_if_fail(sensors->text.draw_area != NULL, num_rows);

    if (sensors->plugin_mode != XFCE_PANEL_PLUGIN_MODE_DESKBAR)
    {
        PangoContext *pango_context = gtk_widget_get_pango_context (sensors->text.draw_area);
        PangoLayout *pango_layout = pango_layout_new (pango_context);
        PangoRectangle extents;
        gfloat font_size;
        gint panel_size;

        /* "jŽ" is used to get an estimate of the maximum height of a line given the font size. */
        std::string text = "<span size=\"" + sensors->str_fontsize + "\">jŽ</span>";
        pango_layout_set_markup (pango_layout, text.c_str(), -1);

        pango_layout_get_extents (pango_layout, &extents, NULL);
        font_size = (gfloat) extents.height / PANGO_SCALE;

        g_object_unref(pango_layout);

        panel_size = sensors->panel_size;
        if (!sensors->cover_panel_rows && xfce_panel_plugin_get_mode(sensors->plugin) != XFCE_PANEL_PLUGIN_MODE_DESKBAR)
            panel_size /= xfce_panel_plugin_get_nrows (sensors->plugin);

        num_rows = floor (panel_size / font_size);
    }
    else
        num_rows = G_MAXINT;

    /* fail-safe */
    if (G_UNLIKELY (num_rows <= 0))
        num_rows = 1;

    return num_rows;
}


/* -------------------------------------------------------------------------- */
static int
determine_number_of_cols (gint num_rows, gint num_items_to_display)
{
    gint num_cols = 1;

    if (num_rows > 1)
    {
        if (num_items_to_display > num_rows)
            num_cols = (gint) ceil (num_items_to_display / (float)num_rows);
    }
    else
        num_cols = num_items_to_display;

    return num_cols;
}


/* -------------------------------------------------------------------------- */
static int
count_number_checked_sensor_features (const t_sensors *sensors)
{
    gint num_items_to_display = 0;

    for (auto chip : sensors->chips)
        for (auto feature : chip->chip_features)
            if (feature->valid && feature->show)
                num_items_to_display++;

    return num_items_to_display;
}


static void
draw_text_area (GtkWidget *widget, cairo_t *cr, t_sensors *sensors)
{
    GtkAllocation alloc;
    GdkRGBA color;
    PangoLayout *layout;
    PangoContext *pango_context;
    PangoRectangle extents;
    GtkStyleContext *style_context;
    gint idx_currentcolumn = 0, idx_currentrow = 0;
    gint num_items_to_display, num_rows, num_cols;

    if (G_UNLIKELY (!sensors))
        return;
    if (G_UNLIKELY (!sensors->text.draw_area))
        return;

    /* count number of checked sensors to display.
       TODO: this could also be done by every toggle/untoggle action
             by putting this variable into t_sensors */
    num_items_to_display = count_number_checked_sensor_features (sensors);
    num_rows = MIN (sensors->lines_size, determine_number_of_rows (sensors));
    num_cols = determine_number_of_cols (num_rows, num_items_to_display);

    cairo_save (cr);

    gtk_widget_get_allocation (widget, &alloc);
    pango_context = gtk_widget_get_pango_context (widget);
    style_context = gtk_widget_get_style_context (widget);

    gtk_style_context_get_color (style_context,
                                 gtk_style_context_get_state (style_context),
                                 &color);
    gdk_cairo_set_source_rgba (cr, &color);

    std::string markup = xfce4::sprintf ("<span size=\"%s\">", sensors->str_fontsize.c_str());

    for (auto chip : sensors->chips)
    {
        for (auto feature : chip->chip_features)
        {
            if (feature->show)
            {
                if (idx_currentcolumn == 0 && idx_currentrow > 0)
                    markup += (sensors->plugin_mode == XFCE_PANEL_PLUGIN_MODE_DESKBAR) ? "\n" : " \n";

                if(sensors->show_labels)
                {
                    if (!feature->color_orEmpty.empty())
                        markup += xfce4::sprintf ("<span foreground=\"%s\">%s:</span> ", feature->color_orEmpty.c_str(), feature->name.c_str());
                    else
                        markup += xfce4::sprintf ("<span>%s:</span> ", feature->name.c_str());
                }

                if (sensors->show_units)
                {
                    if (!feature->color_orEmpty.empty())
                        markup += xfce4::sprintf ("<span foreground=\"%s\">%s</span>", feature->color_orEmpty.c_str(), feature->formatted_value.c_str());
                    else
                        markup += xfce4::sprintf ("<span>%s</span>", feature->formatted_value.c_str());
                }
                else
                {
                    if (!feature->color_orEmpty.empty())
                        markup += xfce4::sprintf ("<span foreground=\"%s\">%.1f</span>", feature->color_orEmpty.c_str(), feature->raw_value);
                    else
                        markup += xfce4::sprintf ("<span>%.1f</span>", feature->raw_value);
                }

                if (sensors->plugin_mode == XFCE_PANEL_PLUGIN_MODE_DESKBAR)
                {
                    idx_currentcolumn = 0;
                    idx_currentrow++;
                }
                else if (idx_currentcolumn < num_cols-1)
                {
                    markup += sensors->show_smallspacings ? "  " : " \t";
                    idx_currentcolumn++;
                }
                else
                {
                    idx_currentcolumn = 0;
                    idx_currentrow++;
                }
            }
        }
    }

    markup += "</span>";

    gtk_widget_show (sensors->text.draw_area);

    layout = pango_layout_new (pango_context);
    pango_layout_set_markup (layout, markup.c_str(), markup.size());

    if (sensors->plugin_mode != XFCE_PANEL_PLUGIN_MODE_VERTICAL)
    {
        double x0, x1, y0, y1;

        pango_layout_get_extents (layout, NULL, &extents);
        x0 = (double) extents.x / PANGO_SCALE;
        x1 = alloc.width / 2.0 - extents.width / 2.0 / PANGO_SCALE;
        y0 = (double) extents.y / PANGO_SCALE;
        y1 = alloc.height / 2.0 - extents.height / 2.0 / PANGO_SCALE;
        cairo_translate (cr, x1 - x0, 0);
        cairo_translate (cr, 0, y1 - y0);

        /* Update width ONLY if it is smaller to avoid panel resizing/jumping */
        sensors->text.reset_size |= alloc.width < PANGO_PIXELS_CEIL (extents.width);
    }
    else
    {
        PangoRectangle non_transformed_extents;
        double x0, x1, y0, y1;

        /* rotate by 90° */
        cairo_rotate (cr, M_PI_2);
        cairo_translate (cr, 0, -alloc.width);
        pango_cairo_update_layout (cr, layout);

        pango_layout_get_extents (layout, NULL, &non_transformed_extents);
        extents.x = non_transformed_extents.y;
        extents.y = non_transformed_extents.x;
        extents.width = non_transformed_extents.height;
        extents.height = non_transformed_extents.width;
        x0 = (double) extents.x / PANGO_SCALE;
        x1 = alloc.width / 2.0 - extents.width / 2.0 / PANGO_SCALE;
        y0 = (double) extents.y / PANGO_SCALE;
        y1 = alloc.height / 2.0 - extents.height / 2.0 / PANGO_SCALE;
        /* cairo_translate: X and Y are swapped because of the rotation by 90° */
        cairo_translate (cr, 0, x1 - x0);
        cairo_translate (cr, y1 - y0, 0);

        /* Update height ONLY if is smaller to avoid panel resizing/jumping */
        sensors->text.reset_size |= alloc.height < PANGO_PIXELS_CEIL (extents.height);
    }

    if (sensors->text.reset_size)
    {
        gtk_widget_set_size_request (widget,
                                     PANGO_PIXELS_CEIL (extents.width),
                                     PANGO_PIXELS_CEIL (extents.height));
        sensors->text.reset_size = false;
    }

    pango_cairo_show_layout (cr, layout);

    g_object_unref (layout);
    cairo_restore (cr);
}


/* -------------------------------------------------------------------------- */
/* draw label with sensor values into panel's vbox */
static void
sensors_show_text_display (const t_sensors *sensors)
{
    /* count number of checked sensors to display.
       TODO: this could also be done by every toggle/untoggle action
             by putting this variable into t_sensors */
    int num_items_to_display = count_number_checked_sensor_features (sensors);

    gtk_widget_set_visible (sensors->panel_label_text, sensors->show_title || num_items_to_display == 0);
    if (num_items_to_display != 0)
    {
        gtk_widget_show (sensors->text.draw_area);
        gtk_widget_queue_draw (sensors->text.draw_area);
    }
    else
    {
        gtk_widget_hide (sensors->text.draw_area);
    }
}


/* -------------------------------------------------------------------------- */
/* Updates the sensor values */
static void
sensors_update_values (t_sensors *sensors)
{
    for (auto chip : sensors->chips) {
        for (auto feature : chip->chip_features) {
            if (feature->valid && feature->show)
            {
                double feature_value;
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

                feature->formatted_value = format_sensor_value (sensors->scale, feature, feature_value);
                feature->raw_value = feature_value;
            }
        }
    }
}

/* -------------------------------------------------------------------------- */
/* create tooltip,see lines 440 and following */
static void
sensors_create_tooltip (const t_sensors *sensors)
{
    bool first_textline = true;

    std::string tooltip = _("No sensors selected!");

    for (auto chip : sensors->chips) {
        bool chipname_already_prepended = false;

        for (auto feature : chip->chip_features) {
            if (feature->valid && feature->show)
            {
                if (!chipname_already_prepended)
                {
                    if (first_textline) {
                        tooltip = "<b>" + chip->sensorId + "</b>";
                        first_textline = false;
                    }
                    else {
                        tooltip = tooltip + " \n<b>" + chip->sensorId + "</b>";
                    }

                    chipname_already_prepended = true;
                }

                tooltip = tooltip + "\n  " + feature->name + ": " + feature->formatted_value;
            }
        }
    }

    gtk_widget_set_tooltip_markup (GTK_WIDGET(sensors->eventbox), tooltip.c_str());
}


/* -------------------------------------------------------------------------- */
static void
sensors_update_panel (t_sensors *sensors, bool update_layout)
{
    sensors_update_values (sensors);

    sensors->text.reset_size |= update_layout;

    switch (sensors->display_values_type)
    {
      case DISPLAY_TACHO:
        sensors_show_tacho_display (sensors);
        break;
      case DISPLAY_BARS:
        sensors_show_bars_display (sensors);
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
}


/* -------------------------------------------------------------------------- */
/* initialize box and label to pack them together */
static void
create_panel_widget (t_sensors *sensors)
{
    /* initialize a new vbox widget */
    sensors->widget_sensors = gtk_box_new ((sensors->plugin_mode == XFCE_PANEL_PLUGIN_MODE_HORIZONTAL) ? GTK_ORIENTATION_HORIZONTAL : GTK_ORIENTATION_VERTICAL, 0);

    /* initialize value label widget */
    sensors->panel_label_text = gtk_widget_new (GTK_TYPE_LABEL, "label", _("<span><b>Sensors</b></span>"), "use-markup", TRUE, "xalign", 0.0, "yalign", 0.5, "margin", INNER_BORDER, NULL);

    gtk_widget_show (sensors->panel_label_text);

    sensors->text.draw_area = gtk_drawing_area_new ();
    sensors->text.reset_size = true;
    gtk_widget_set_halign (sensors->text.draw_area, GTK_ALIGN_CENTER);
    gtk_widget_set_valign (sensors->text.draw_area, GTK_ALIGN_CENTER);
    gtk_widget_set_size_request (sensors->text.draw_area, 1, 1);
    xfce4::connect_draw (sensors->text.draw_area, [sensors](GtkWidget *w, cairo_t *cr) {
        draw_text_area (w, cr, sensors);
        return xfce4::PROPAGATE;
    });
    gtk_widget_show (sensors->text.draw_area);

    /* add newly created label to box */
    gtk_box_pack_start (GTK_BOX (sensors->widget_sensors), sensors->panel_label_text, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (sensors->widget_sensors), sensors->text.draw_area, TRUE, TRUE, 0);

    /* create 'valued' label */
    sensors_update_panel (sensors, true);

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
            sensors_remove_bars_panel (sensors);
            break;
        case DISPLAY_TACHO:
            sensors_remove_tacho_panel (sensors);
            break;
        case DISPLAY_TEXT:
            break;
    }

    gtk_widget_destroy (sensors->text.draw_area);
    gtk_widget_destroy (sensors->panel_label_text);
    gtk_widget_destroy (sensors->widget_sensors);
    sensors->text.draw_area = NULL;
    sensors->panel_label_text = NULL;
    sensors->widget_sensors = NULL;

    create_panel_widget (sensors);

    gtk_container_add (GTK_CONTAINER (sensors->eventbox), sensors->widget_sensors);
}


/* -------------------------------------------------------------------------- */
/* double-click improvement */
static xfce4::Propagation
execute_command (GdkEventButton *event, t_sensors *sensors)
{
    if (event->type == GDK_2BUTTON_PRESS && sensors && sensors->exec_command) {
        // screen NULL, command, terminal=no, startup=yes, error=NULL
        xfce_spawn_command_line_on_screen (NULL, sensors->command_name.c_str(), FALSE, TRUE, NULL);
        return xfce4::STOP;
    }
    else {
        return xfce4::PROPAGATE;
    }
}


/* -------------------------------------------------------------------------- */
static void
sensors_free (t_sensors *sensors)
{
    g_return_if_fail (sensors != NULL);

    /* stop association to libsensors and others*/
    cleanup_interfaces();

    /* remove timeout functions */
    remove_gsource (sensors->timeout_id);

    /* double-click improvement */
    remove_gsource (sensors->doubleclick_id);

    delete sensors;
}


/* -------------------------------------------------------------------------- */
static xfce4::PluginSize
sensors_set_size (XfcePanelPlugin *plugin, guint size, t_sensors *sensors)
{
    sensors->panel_size = size;

    /* when the orientation has toggled, maybe the size as well? */
    if (sensors->cover_panel_rows || xfce_panel_plugin_get_mode(plugin) == XFCE_PANEL_PLUGIN_MODE_DESKBAR)
        xfce_panel_plugin_set_small(plugin, FALSE);
    else
        xfce_panel_plugin_set_small(plugin, TRUE);

    sensors_update_panel (sensors, true);

    return xfce4::RECTANGLE;
}


/* -------------------------------------------------------------------------- */
static void
show_title_toggled (GtkToggleButton *button, t_sensors_dialog *dialog)
{
    if (dialog->sensors->display_values_type == DISPLAY_BARS)
        sensors_remove_bars_panel (dialog->sensors);
    else if (dialog->sensors->display_values_type == DISPLAY_TACHO)
        sensors_remove_tacho_panel (dialog->sensors);

    dialog->sensors->show_title = gtk_toggle_button_get_active (button);

    sensors_update_panel (dialog->sensors, true);
}


/* -------------------------------------------------------------------------- */
static void
show_labels_toggled (GtkToggleButton *button, t_sensors_dialog *dialog)
{
    if (dialog->sensors->display_values_type == DISPLAY_BARS)
        sensors_remove_bars_panel (dialog->sensors);
    else if (dialog->sensors->display_values_type == DISPLAY_TACHO)
        sensors_remove_tacho_panel (dialog->sensors);

    dialog->sensors->show_labels = gtk_toggle_button_get_active (button);

    sensors_update_panel (dialog->sensors, true);
}


/* -------------------------------------------------------------------------- */
static void
auto_bar_colors_toggled (GtkToggleButton *button, t_sensors_dialog *dialog)
{
    if (dialog->sensors->display_values_type == DISPLAY_BARS)
        sensors_remove_bars_panel (dialog->sensors);

    dialog->sensors->automatic_bar_colors = gtk_toggle_button_get_active (button);

    sensors_update_panel (dialog->sensors, true);
}


/* -------------------------------------------------------------------------- */
static void
display_style_changed_text (GtkToggleButton *button, t_sensors_dialog *dialog)
{
    if (!gtk_toggle_button_get_active (button))
        return;

    if (dialog->sensors->display_values_type == DISPLAY_BARS)
        sensors_remove_bars_panel (dialog->sensors);
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

    sensors_update_panel (dialog->sensors, true);
}


/* -------------------------------------------------------------------------- */
static void
display_style_changed_bars (GtkToggleButton *button, t_sensors_dialog *dialog)
{
    if (!gtk_toggle_button_get_active (button))
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

    sensors_update_panel (dialog->sensors, true);
}


/* -------------------------------------------------------------------------- */
static void
suppresstooltip_changed (t_sensors *sensors)
{
    sensors->suppresstooltip = !sensors->suppresstooltip;
    gtk_widget_set_has_tooltip(sensors->eventbox, !sensors->suppresstooltip);
    if (!sensors->suppresstooltip)
        sensors_create_tooltip (sensors);
}


/* -------------------------------------------------------------------------- */
static void
display_style_changed_tacho (GtkToggleButton *button, t_sensors_dialog *dialog)
{
    if (!gtk_toggle_button_get_active (button))
        return;

    if (dialog->sensors->display_values_type == DISPLAY_BARS)
        sensors_remove_bars_panel(dialog->sensors);

    gtk_widget_hide (dialog->coloredBars_Box);
    gtk_widget_show (dialog->fontSettings_Box);
    gtk_widget_hide (dialog->font_Box);
    gtk_widget_hide (dialog->Lines_Box);
    gtk_widget_hide (dialog->unit_checkbox);
    gtk_widget_hide (dialog->smallspacing_checkbox);
    gtk_widget_show (dialog->colorvalue_slider_box);
    gtk_widget_show (dialog->alpha_slider_box);

    dialog->sensors->display_values_type = DISPLAY_TACHO;

    sensors_update_panel (dialog->sensors, true);
}


/* -------------------------------------------------------------------------- */
void
sensor_entry_changed_ (GtkWidget *widget, t_sensors_dialog *dialog)
{
    gint combo_box_active = gtk_combo_box_get_active (GTK_COMBO_BOX (widget));

    auto chip = dialog->sensors->chips[combo_box_active];
    gtk_label_set_label (GTK_LABEL (dialog->mySensorLabel), chip->description.c_str());

    gtk_tree_view_set_model (GTK_TREE_VIEW (dialog->myTreeView), GTK_TREE_MODEL (dialog->myListStore[combo_box_active]));
}


/* -------------------------------------------------------------------------- */
static void
str_fontsize_change (GtkComboBox *widget, t_sensors_dialog *dialog)
{
    t_sensors *sensors = dialog->sensors;

    switch (gtk_combo_box_get_active (widget))
    {
        case 0: sensors->str_fontsize = "x-small"; break;
        case 1: sensors->str_fontsize = "small"; break;
        case 3: sensors->str_fontsize = "large"; break;
        case 4: sensors->str_fontsize = "x-large"; break;
        default: sensors->str_fontsize = "medium";
    }

    sensors->val_fontsize = gtk_combo_box_get_active (GTK_COMBO_BOX (widget));

    int rows = determine_number_of_rows (sensors);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (dialog->Lines_Spin_Button), (gdouble) rows);

    sensors_update_panel (sensors, true);
}


/* -------------------------------------------------------------------------- */
static void
lines_size_change (GtkSpinButton *button, t_sensors *sensors)
{
    sensors->lines_size = (int) gtk_spin_button_get_value(button);
    sensors_update_panel (sensors, true);
}


/* -------------------------------------------------------------------------- */
static void
cover_rows_toggled(GtkToggleButton *button, t_sensors_dialog *dialog)
{
    t_sensors *const sensors = dialog->sensors;

    sensors->cover_panel_rows = gtk_toggle_button_get_active (button);

    if (sensors->cover_panel_rows || xfce_panel_plugin_get_mode (sensors->plugin) == XFCE_PANEL_PLUGIN_MODE_DESKBAR)
        xfce_panel_plugin_set_small (sensors->plugin, FALSE);
    else
        xfce_panel_plugin_set_small (sensors->plugin, TRUE);

    sensors_update_panel (sensors, true);
}


/* -------------------------------------------------------------------------- */
void
temperature_unit_change_ (GtkToggleButton*, t_sensors_dialog *dialog)
{
    t_sensors *sensors = dialog->sensors;
    switch (sensors->scale)
    {
        case CELSIUS:
            sensors->scale = FAHRENHEIT;
            break;
        case FAHRENHEIT:
            sensors->scale = CELSIUS;
            break;
    }
    sensors_update_panel (sensors, true);
    reload_listbox (dialog);
}

/* -------------------------------------------------------------------------- */
void
adjustment_value_changed_ (GtkAdjustment *adjustment, t_sensors_dialog *dialog)
{
    t_sensors *sensors = dialog->sensors;

    auto refresh_time = (gint) gtk_adjustment_get_value (adjustment);
    sensors->sensors_refresh_time = refresh_time;

    remove_gsource (sensors->timeout_id);
    sensors->timeout_id  = xfce4::timeout_add (refresh_time * 1000, [sensors]() {
        sensors_update_panel (sensors, false);
        return xfce4::TIMEOUT_AGAIN;
    });
}


static void
draw_units_changed (t_sensors *sensors)
{
    sensors->show_units = !sensors->show_units;
    sensors_update_panel (sensors, true);
}


/* -------------------------------------------------------------------------- */
static void
draw_smallspacings_changed (t_sensors *sensors)
{
    sensors->show_smallspacings = !sensors->show_smallspacings;
    sensors_update_panel (sensors, true);
}


/* -------------------------------------------------------------------------- */
static void
suppressmessage_changed (t_sensors *sensors)
{
    sensors->suppressmessage = !sensors->suppressmessage;
    sensors_update_panel (sensors, true);
}


/* -------------------------------------------------------------------------- */
/* double-click improvement */
static void
execCommand_toggled (GtkToggleButton *button, t_sensors *sensors)
{
    sensors->exec_command = gtk_toggle_button_get_active (button);

    if (sensors->exec_command )
        g_signal_handler_unblock (sensors->eventbox, sensors->doubleclick_id);
    else
        g_signal_handler_block (sensors->eventbox, sensors->doubleclick_id);
}


/* -------------------------------------------------------------------------- */
void
minimum_changed_ (GtkCellRendererText *cell_renderer_text, gchar *path_str,
                  gchar *newmin, t_sensors_dialog *dialog)
{
    t_sensors *const sensors = dialog->sensors;
    GtkTreeIter iter;

    float value = atof (newmin);

    gint combo_box_active = gtk_combo_box_get_active(GTK_COMBO_BOX (dialog->myComboBox));

    /* get model and path */
    auto model = (GtkTreeModel*) dialog->myListStore[combo_box_active];
    GtkTreePath *path = gtk_tree_path_new_from_string (path_str);

    /* get model iterator */
    gtk_tree_model_get_iter (model, &iter, path);

    /* set new value according to chosen scale */
    gtk_tree_store_set (GTK_TREE_STORE (model), &iter, eTreeColumn_Min, value, -1);
    auto chip = sensors->chips[combo_box_active];

    auto feature = chip->chip_features[atoi(path_str)];
    if (sensors->scale == FAHRENHEIT)
      value = (value - 32) * 5 / 9;
    feature->min_value = value;

    /* clean up */
    gtk_tree_path_free (path);

    if (sensors->display_values_type == DISPLAY_BARS)
        sensors_remove_bars_panel (sensors);
    else if (sensors->display_values_type == DISPLAY_TACHO)
        sensors_remove_tacho_panel (sensors);

    sensors_update_panel (sensors, true);
}


/* -------------------------------------------------------------------------- */
void
maximum_changed_ (GtkCellRendererText *cell_renderer_text, gchar *path_str,
                  gchar *newmax, t_sensors_dialog *dialog)
{
    t_sensors *const sensors = dialog->sensors;
    GtkTreeIter iter;

    float value = atof (newmax);

    gint combo_box_active = gtk_combo_box_get_active(GTK_COMBO_BOX (dialog->myComboBox));

    /* get model and path */
    auto model = (GtkTreeModel*) dialog->myListStore[combo_box_active];
    GtkTreePath *path = gtk_tree_path_new_from_string (path_str);

    /* get model iterator */
    gtk_tree_model_get_iter (model, &iter, path);

    /* set new value according to chosen scale */
    gtk_tree_store_set (GTK_TREE_STORE (model), &iter, eTreeColumn_Max, value, -1);
    auto chip = sensors->chips[combo_box_active];

    auto feature = chip->chip_features[atoi(path_str)];
    if (sensors->scale == FAHRENHEIT)
      value = (value - 32) * 5 / 9;
    feature->max_value = value;

    /* clean up */
    gtk_tree_path_free (path);

    if (sensors->display_values_type == DISPLAY_BARS)
        sensors_remove_bars_panel (sensors);
    else if (sensors->display_values_type == DISPLAY_TACHO)
        sensors_remove_tacho_panel (sensors);

    sensors_update_panel (sensors, true);
}


/* -------------------------------------------------------------------------- */
void
list_cell_color_edited_ (GtkCellRendererText *cell_renderer_text, const gchar *path_str,
                         const gchar *new_color, t_sensors_dialog *dialog)
{
    t_sensors *const sensors = dialog->sensors;
    GtkTreeIter iter;

    /* store new color in appropriate array */
    gboolean hexColor = g_str_has_prefix (new_color, "#");

    if (hexColor && strlen (new_color) == 7) {
        int i;
        for (i=1; i<7; i++) {
            /* only save hex numbers! */
            if ( ! g_ascii_isxdigit (new_color[i]) )
                return;
        }

        gint combo_box_active = gtk_combo_box_get_active (GTK_COMBO_BOX (dialog->myComboBox));

        /* get model and path */
        auto model = (GtkTreeModel*) dialog->myListStore[combo_box_active];
        GtkTreePath *path = gtk_tree_path_new_from_string (path_str);

        /* get model iterator */
        gtk_tree_model_get_iter (model, &iter, path);

        /* set new value */
        gtk_tree_store_set (GTK_TREE_STORE (model), &iter, eTreeColumn_Color, new_color, -1);
        auto chip = sensors->chips[combo_box_active];

        auto feature = chip->chip_features[atoi(path_str)];
        feature->color_orEmpty = new_color;

        /* clean up */
        gtk_tree_path_free (path);

        sensors_update_panel (sensors, true);
    }
    else if (strlen (new_color) == 0) {
        gint combo_box_active = gtk_combo_box_get_active (GTK_COMBO_BOX (dialog->myComboBox));

        /* get model and path */
        auto model = (GtkTreeModel*) dialog->myListStore[combo_box_active];
        GtkTreePath *path = gtk_tree_path_new_from_string (path_str);

        /* get model iterator */
        gtk_tree_model_get_iter (model, &iter, path);

        /* set new value */
        gtk_tree_store_set (GTK_TREE_STORE (model), &iter, eTreeColumn_Color, new_color, -1);
        auto chip = sensors->chips[combo_box_active];

        auto feature = chip->chip_features[atoi(path_str)];
        feature->color_orEmpty = "";

        /* clean up */
        gtk_tree_path_free (path);

        sensors_update_panel (sensors, true);
    }
}


/* -------------------------------------------------------------------------- */
void
list_cell_text_edited_ (GtkCellRendererText *cell_renderer_text, gchar *path_str,
                        gchar *new_text, t_sensors_dialog *dialog)
{
    t_sensors *const sensors = dialog->sensors;
    GtkTreeIter iter;

    if (sensors->display_values_type == DISPLAY_BARS) {
        sensors_remove_bars_panel (sensors);
    }
    else  if (sensors->display_values_type == DISPLAY_TACHO) {
        sensors_remove_tacho_panel (sensors);
    }
    gint combo_box_active = gtk_combo_box_get_active (GTK_COMBO_BOX (dialog->myComboBox));

    auto model = (GtkTreeModel*) dialog->myListStore [combo_box_active];
    GtkTreePath *path = gtk_tree_path_new_from_string (path_str);

    /* get model iterator */
    gtk_tree_model_get_iter (model, &iter, path);

    /* set new value */
    gtk_tree_store_set (GTK_TREE_STORE (model), &iter, eTreeColumn_Name, new_text, -1);
    auto chip = sensors->chips[combo_box_active];

    auto feature = chip->chip_features[atoi(path_str)];
    feature->name = new_text;

    /* clean up */
    gtk_tree_path_free (path);

    sensors_update_panel (sensors, true);
}


/* -------------------------------------------------------------------------- */
void
list_cell_toggle_ (GtkCellRendererToggle *cell, gchar *path_str,  t_sensors_dialog *dialog)
{
    t_sensors *const sensors = dialog->sensors;
    GtkTreeIter iter;
    gboolean show;

    if (sensors->display_values_type == DISPLAY_BARS) {
        sensors_remove_bars_panel (sensors);
    }
    else if (sensors->display_values_type == DISPLAY_TACHO) {
        sensors_remove_tacho_panel (sensors);
    }
    gint combo_box_active = gtk_combo_box_get_active (GTK_COMBO_BOX (dialog->myComboBox));

    auto model = (GtkTreeModel*) dialog->myListStore[combo_box_active];
    GtkTreePath *path = gtk_tree_path_new_from_string (path_str);

    /* get toggled iter */
    gtk_tree_model_get_iter (model, &iter, path);
    gtk_tree_model_get (model, &iter, 2, &show, -1);

    /* do something with the value */
    show = !show;

    /* set new value */
    gtk_tree_store_set (GTK_TREE_STORE (model), &iter, eTreeColumn_Show, show, -1);
    auto chip = sensors->chips[combo_box_active];

    auto feature = chip->chip_features[atoi(path_str)];
    feature->show = show;

    /* clean up */
    gtk_tree_path_free (path);

    /* update tooltip and panel widget */
    sensors_update_panel (sensors, true);
}


/* -------------------------------------------------------------------------- */
static void
on_font_set (GtkFontButton *widget, t_sensors *sensors)
{
    gchar *new_font = gtk_font_chooser_get_font(GTK_FONT_CHOOSER(widget));
    if (new_font)
    {
        font = new_font;
        g_free (new_font);
    }
    else
    {
        font = default_font;
    }

    sensors_update_panel (sensors, true);
}


/* -------------------------------------------------------------------------- */
static xfce4::Propagation
tachos_colorvalue_changed_ (gdouble value, t_sensors *sensors)
{
    g_assert (sensors!=NULL);
    sensors->val_tachos_color = val_colorvalue = value; // TODO: gtk_scale_button_get_value(GTK_SCALE_BUTTON(widget));
    sensors_update_panel (sensors, true);
    return xfce4::PROPAGATE;
}


/* -------------------------------------------------------------------------- */
static xfce4::Propagation
tachos_alpha_changed_ (gdouble value, t_sensors *sensors)
{
    g_assert (sensors != NULL);
    sensors->val_tachos_alpha = val_alpha = value; // TODO: gtk_scale_button_get_value(GTK_SCALE_BUTTON(widget));
    sensors_update_panel (sensors, true);
    return xfce4::PROPAGATE;
}


/* -------------------------------------------------------------------------- */
static void
add_ui_style_box (GtkWidget *vbox, t_sensors_dialog *dialog)
{
    GtkWidget *hbox = gtk_hbox_new (BORDER);
    gtk_widget_show (hbox);

    GtkWidget *label = gtk_label_new (_("UI style:"));
    GtkWidget *radioText = gtk_radio_button_new_with_mnemonic(NULL, _("_text"));
    GtkWidget *radioBars = gtk_radio_button_new_with_mnemonic_from_widget (GTK_RADIO_BUTTON(radioText), _("_progress bars"));
    GtkWidget *radioTachos = gtk_radio_button_new_with_mnemonic_from_widget (GTK_RADIO_BUTTON(radioText), _("_tachos"));

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

    xfce4::connect_toggled (GTK_TOGGLE_BUTTON (radioText), [dialog](GtkToggleButton *button) {
        display_style_changed_text (button, dialog);
    });
    xfce4::connect_toggled (GTK_TOGGLE_BUTTON (radioBars), [dialog](GtkToggleButton *button) {
        display_style_changed_bars (button, dialog);
    });
    xfce4::connect_toggled (GTK_TOGGLE_BUTTON (radioTachos), [dialog](GtkToggleButton *button) {
        display_style_changed_tacho (button, dialog);
    });
}


/* -------------------------------------------------------------------------- */
static void
add_labels_box (GtkWidget *vbox, t_sensors_dialog *dialog)
{
    GtkWidget *hbox = gtk_hbox_new (BORDER);
    gtk_widget_show (hbox);
    dialog->labels_Box = hbox;

    GtkWidget *checkButton = gtk_check_button_new_with_mnemonic (_("Show _labels"));
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(checkButton), dialog->sensors->show_labels);
    gtk_widget_show (checkButton);

    gtk_box_pack_start (GTK_BOX (hbox), checkButton, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);

    xfce4::connect_toggled (GTK_TOGGLE_BUTTON (checkButton), [dialog](GtkToggleButton *button) {
        show_labels_toggled (button, dialog);
    });
}


/* -------------------------------------------------------------------------- */
static void
add_auto_bar_colors_box (GtkWidget *vbox, t_sensors_dialog *dialog)
{
    GtkWidget *hbox = gtk_hbox_new (BORDER);

    gtk_widget_show (hbox);
    dialog->coloredBars_Box = hbox;

    GtkWidget *checkButton = gtk_check_button_new_with_mnemonic (_("_Automatic bar colors"));
    gtk_widget_set_tooltip_text (checkButton,
                                 _("If enabled, bar colors depend on their values (normal, high, very high).\n"
                                   "If disabled, bars use the user-defined sensor colors.\n"
                                   "If a particular user-defined sensor color is unspecified,\n"
                                   "the bar color is derived from the current UI style."));
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(checkButton), dialog->sensors->automatic_bar_colors);

    gtk_widget_show (checkButton);

    gtk_box_pack_start (GTK_BOX (hbox), checkButton, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);

    if (dialog->sensors->display_values_type != DISPLAY_BARS)
        gtk_widget_hide(dialog->coloredBars_Box);

    xfce4::connect_toggled (GTK_TOGGLE_BUTTON (checkButton), [dialog](GtkToggleButton *button) {
        auto_bar_colors_toggled (button, dialog);
    });
}


/* -------------------------------------------------------------------------- */
static void
add_title_box (GtkWidget *vbox, t_sensors_dialog *dialog)
{
    GtkWidget *hbox = gtk_hbox_new (BORDER);
    gtk_widget_show (hbox);

    GtkWidget *checkButton = gtk_check_button_new_with_mnemonic (_("_Show title"));
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(checkButton), dialog->sensors->show_title);
    gtk_widget_show (checkButton);

    gtk_box_pack_start (GTK_BOX (hbox), checkButton, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);

    xfce4::connect_toggled (GTK_TOGGLE_BUTTON (checkButton), [dialog](GtkToggleButton *button) {
        show_title_toggled (button, dialog);
    });
}


/* -------------------------------------------------------------------------- */
static void
add_lines_box (GtkWidget *vbox, t_sensors_dialog * dialog)
{
    GtkWidget *myLinesLabel = gtk_label_new_with_mnemonic (_("_Number of text lines:"));
    GtkWidget *myLinesBox = gtk_hbox_new (BORDER);
    GtkWidget *myLinesSizeSpinButton = gtk_spin_button_new_with_range  (1.0, 10.0, 1.0);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(myLinesSizeSpinButton), (gdouble) dialog->sensors->lines_size);

    dialog->Lines_Box = myLinesBox;
    dialog->Lines_Spin_Button = myLinesSizeSpinButton;

    gtk_box_pack_start (GTK_BOX (myLinesBox), myLinesLabel, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (myLinesBox), myLinesSizeSpinButton, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), myLinesBox, FALSE, FALSE, 0);

    gtk_widget_show_all (myLinesBox);

    if (dialog->sensors->display_values_type != DISPLAY_TEXT)
        gtk_widget_hide(dialog->Lines_Box);

    xfce4::connect_value_changed (GTK_SPIN_BUTTON (myLinesSizeSpinButton), [dialog](GtkSpinButton *button) {
        lines_size_change (button, dialog->sensors);
    });
}


/* -------------------------------------------------------------------------- */
static void
add_cover_rows_box (GtkWidget *vbox, t_sensors_dialog *dialog)
{
    if (xfce_panel_plugin_get_mode(dialog->sensors->plugin) != XFCE_PANEL_PLUGIN_MODE_DESKBAR)
    {
        // The Xfce 4 panel can have several rows or columns. With such a mode,
        //  the plugins are allowed to span over all available rows/columns.
        //  When translating, "cover" might be replaced by "use" or "span".
        GtkWidget *myCheckBox = gtk_check_button_new_with_mnemonic (_("_Cover all panel rows/columns"));
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(myCheckBox), dialog->sensors->cover_panel_rows);

        gtk_box_pack_start (GTK_BOX (vbox), myCheckBox, FALSE, FALSE, 0);

        gtk_widget_show (myCheckBox);

        xfce4::connect_toggled (GTK_TOGGLE_BUTTON (myCheckBox), [dialog](GtkToggleButton *button) {
            cover_rows_toggled (button, dialog);
        });
    }
}


/* -------------------------------------------------------------------------- */
static void
add_str_fontsize_box (GtkWidget *vbox, t_sensors_dialog *dialog)
{
    GtkWidget *myFontLabel = gtk_label_new_with_mnemonic (_("F_ont size:"));
    GtkWidget *myFontBox = gtk_hbox_new (BORDER);
    GtkWidget *myFontSizeComboBox = gtk_combo_box_text_new();

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

    gtk_widget_show_all (myFontBox);

    if (dialog->sensors->display_values_type != DISPLAY_TEXT)
        gtk_widget_hide(dialog->font_Box);

    xfce4::connect_changed (GTK_COMBO_BOX (myFontSizeComboBox), [dialog](GtkComboBox *widget) {
        str_fontsize_change (widget, dialog);
    });
}


/* -------------------------------------------------------------------------- */
static void
add_font_settings_box (GtkWidget *vbox, t_sensors_dialog *dialog)
{
    GtkWidget *myFontLabel = gtk_label_new_with_mnemonic (_("F_ont:"));
    GtkWidget *myFontSettingsBox = gtk_hbox_new (BORDER);
    GtkWidget *myFontSettingsButton = gtk_font_button_new_with_font (font.c_str());
    gtk_font_button_set_use_font (GTK_FONT_BUTTON(myFontSettingsButton), TRUE);

    dialog->fontSettings_Box = myFontSettingsBox;

    gtk_box_pack_start (GTK_BOX (myFontSettingsBox), myFontLabel, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (myFontSettingsBox), myFontSettingsButton, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), myFontSettingsBox, FALSE, FALSE, 0);

    gtk_widget_show_all (myFontSettingsBox);

    if (dialog->sensors->display_values_type != DISPLAY_TACHO)
        gtk_widget_hide(dialog->fontSettings_Box);

    xfce4::connect_font_set (GTK_FONT_BUTTON (myFontSettingsButton), [dialog](GtkFontButton *button) {
        on_font_set (button, dialog->sensors);
    });
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

    xfce4::connect_toggled (GTK_TOGGLE_BUTTON (dialog->unit_checkbox), [dialog](GtkToggleButton*) {
        draw_units_changed (dialog->sensors);
    });
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

    xfce4::connect_toggled (GTK_TOGGLE_BUTTON (dialog->smallspacing_checkbox), [dialog](GtkToggleButton*) {
        draw_smallspacings_changed (dialog->sensors);
    });
}


/* -------------------------------------------------------------------------- */
static void
add_tachos_appearance_boxes(GtkWidget *vbox, t_sensors_dialog *dialog)
{
    GtkWidget *widget_hscale;
    GtkWidget *widget_label;

    GtkSizeGroup *sg = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

    // Alpha value of the tacho coloring
    dialog->alpha_slider_box = gtk_hbox_new (INNER_BORDER);
    widget_label = gtk_label_new(_("Tacho color alpha value:"));
    gtk_size_group_add_widget (sg, widget_label);
    gtk_label_set_xalign (GTK_LABEL (widget_label), 0.0);
    gtk_box_pack_start (GTK_BOX (dialog->alpha_slider_box), widget_label, FALSE, TRUE, 0);
    widget_hscale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.0, 1.0, 0.01);
    gtk_range_set_value(GTK_RANGE(widget_hscale), dialog->sensors->val_tachos_alpha);
    gtk_scale_set_value_pos(GTK_SCALE(widget_hscale), GTK_POS_RIGHT);
    xfce4::connect_change_value (GTK_RANGE (widget_hscale), [dialog](GtkRange*, GtkScrollType*, double value) {
        return tachos_alpha_changed_ (value, dialog->sensors);
    });
    gtk_box_pack_start (GTK_BOX (dialog->alpha_slider_box), widget_hscale, TRUE, TRUE, 0);
    gtk_widget_show_all (dialog->alpha_slider_box);

    // The value from HSV color model
    dialog->colorvalue_slider_box = gtk_hbox_new (INNER_BORDER);
    widget_label = gtk_label_new(_("Tacho color value:"));
    gtk_size_group_add_widget (sg, widget_label);
    gtk_label_set_xalign (GTK_LABEL (widget_label), 0.0);
    gtk_box_pack_start (GTK_BOX (dialog->colorvalue_slider_box), widget_label, FALSE, TRUE, 0);
    widget_hscale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.0, 1.0, 0.01);
    gtk_range_set_value(GTK_RANGE(widget_hscale), dialog->sensors->val_tachos_color);
    gtk_scale_set_value_pos(GTK_SCALE(widget_hscale), GTK_POS_RIGHT);
    xfce4::connect_change_value (GTK_RANGE (widget_hscale), [dialog](GtkRange*, GtkScrollType*, double value) {
        return tachos_colorvalue_changed_ (value, dialog->sensors);
    });
    gtk_box_pack_start (GTK_BOX (dialog->colorvalue_slider_box), widget_hscale, TRUE, TRUE, 0);
    gtk_widget_show_all (dialog->colorvalue_slider_box);

    gtk_box_pack_start (GTK_BOX (vbox), dialog->alpha_slider_box, FALSE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), dialog->colorvalue_slider_box, FALSE, TRUE, 0);

    if (dialog->sensors->display_values_type != DISPLAY_TACHO)
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

    xfce4::connect_toggled (GTK_TOGGLE_BUTTON (dialog->suppressmessage_checkbox), [dialog](GtkToggleButton*) {
        suppressmessage_changed (dialog->sensors);
    });
}


/* -------------------------------------------------------------------------- */
static void
add_suppresstooltips_box (GtkWidget *vbox, t_sensors_dialog *dialog)
{
    dialog->suppresstooltip_checkbox  = gtk_check_button_new_with_mnemonic(_("Suppress tooltip"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dialog->suppresstooltip_checkbox), dialog->sensors->suppresstooltip);

    gtk_widget_show (dialog->suppresstooltip_checkbox);

    gtk_box_pack_start (GTK_BOX (vbox), dialog->suppresstooltip_checkbox, FALSE, TRUE, 0);

    xfce4::connect_toggled (GTK_TOGGLE_BUTTON (dialog->suppresstooltip_checkbox), [dialog](GtkToggleButton*) {
        suppresstooltip_changed (dialog->sensors);
    });
}


/* -------------------------------------------------------------------------- */
/* double-click improvement */
static void
add_command_box (GtkWidget *vbox,  t_sensors_dialog *dialog)
{
    GtkWidget *myBox = gtk_hbox_new (BORDER);

    dialog->myExecCommand_CheckBox = gtk_check_button_new_with_mnemonic (_("E_xecute on double click:"));
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->myExecCommand_CheckBox), dialog->sensors->exec_command);

    dialog->myCommandName_Entry = gtk_entry_new ();
    gtk_widget_set_size_request (dialog->myCommandName_Entry, 160, 25);

    gtk_entry_set_text( GTK_ENTRY(dialog->myCommandName_Entry), dialog->sensors->command_name.c_str());

    gtk_box_pack_start (GTK_BOX (myBox), dialog->myExecCommand_CheckBox, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (myBox), dialog->myCommandName_Entry, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), myBox, FALSE, FALSE, 0);

    gtk_widget_show_all (myBox);

    xfce4::connect_toggled (GTK_TOGGLE_BUTTON (dialog->myExecCommand_CheckBox), [dialog](GtkToggleButton *button) {
        execCommand_toggled (button, dialog->sensors);
    });
}


/* -------------------------------------------------------------------------- */
static void
add_view_frame (GtkWidget *notebook, t_sensors_dialog *dialog)
{
    GtkWidget *vbox = gtk_vbox_new (OUTER_BORDER);
    gtk_widget_show (vbox);

    GtkWidget *label = gtk_label_new_with_mnemonic(_("_View"));
    gtk_widget_show (label);

    gtk_container_set_border_width (GTK_CONTAINER (vbox), BORDER<<1);

    gtk_notebook_append_page (GTK_NOTEBOOK(notebook), vbox, label);

    add_title_box (vbox, dialog);

    add_ui_style_box (vbox, dialog);
    add_labels_box (vbox, dialog);
    add_cover_rows_box(vbox, dialog);

    GtkWidget *frame = gtk_frame_new(_("UI style options"));
    gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
    gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
    gtk_widget_show (frame);

    vbox = gtk_vbox_new (OUTER_BORDER);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), OUTER_BORDER);
    gtk_container_add(GTK_CONTAINER(frame), vbox);
    gtk_widget_show (vbox);

    add_str_fontsize_box (vbox, dialog);
    add_font_settings_box (vbox, dialog);
    add_lines_box (vbox, dialog);
    add_auto_bar_colors_box (vbox, dialog);
    add_units_box (vbox, dialog);
    add_smallspacings_box(vbox, dialog);
    add_tachos_appearance_boxes(vbox, dialog);
}


/* -------------------------------------------------------------------------- */
static void
add_miscellaneous_frame (GtkWidget *notebook, t_sensors_dialog *dialog)
{
    GtkWidget *vbox = gtk_vbox_new (OUTER_BORDER);
    gtk_widget_show (vbox);

    GtkWidget *label = gtk_label_new_with_mnemonic (_("_Miscellaneous"));
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
on_optionsDialog_response (GtkDialog *dlg, gint response, t_sensors_dialog *sd)
{
    if (response == GTK_RESPONSE_OK || response == GTK_RESPONSE_DELETE_EVENT) {
        /* FIXME: save most of the content in this function,
           remove those toggle functions where possible. NYI */
        /* sensors_apply_options (sd); */
        sd->sensors->command_name = gtk_entry_get_text(GTK_ENTRY(sd->myCommandName_Entry));

        if (sd->sensors->plugin_config_file.empty())
        {
            gchar *save_location = xfce_panel_plugin_save_location (sd->sensors->plugin, TRUE);
            sd->sensors->plugin_config_file = save_location;
            g_free (save_location);
        }

        if (!sd->sensors->plugin_config_file.empty())
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
    GtkWidget *dlg;

    xfce_panel_plugin_block_menu (plugin);

    dlg = xfce_titled_dialog_new_with_buttons(
                _("Sensors Plugin"),
                GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (plugin))),
                (GtkDialogFlags) 0,
                "gtk-close",
                GTK_RESPONSE_OK,
                NULL
            );

    xfce_titled_dialog_set_subtitle (XFCE_TITLED_DIALOG (dlg), _("Properties"));
    gtk_window_set_icon_name(GTK_WINDOW(dlg),"xfce-sensors");

    gtk_container_set_border_width (GTK_CONTAINER (dlg), 2);

    GtkWidget *vbox = gtk_dialog_get_content_area(GTK_DIALOG(dlg));

    t_sensors_dialog *sd = g_new0 (t_sensors_dialog, 1);

    sd->sensors = sensors;
    sd->dialog = dlg;
    sd->plugin_dialog = true;

    sd->myComboBox = gtk_combo_box_text_new();

    init_widgets (sd);

    gtk_combo_box_set_active (GTK_COMBO_BOX(sd->myComboBox), 0);

    GtkWidget *notebook = gtk_notebook_new ();

    gtk_box_pack_start (GTK_BOX(vbox), notebook, TRUE, TRUE, 0);
    gtk_widget_show(notebook);

    add_sensors_frame (notebook, sd);

    gchar *myToolTipText = g_strdup(_("You can change a feature's properties such as name, colours, min/max value by double-clicking the entry, editing the content, and pressing \"Return\" or selecting a different field."));
    gtk_widget_set_tooltip_text (sd->myTreeView, myToolTipText);
    g_free (myToolTipText);

    add_view_frame (notebook, sd);
    add_miscellaneous_frame (notebook, sd);

    gtk_window_set_default_size (GTK_WINDOW(dlg), sensors->preferred_width, sensors->preferred_height);

    xfce4::connect_response (GTK_DIALOG (dlg), [sd](GtkDialog *d, gint response) {
        on_optionsDialog_response (d, response, sd);
    });

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
    xfce4::connect_button_press (sensors->eventbox, [sensors](GtkWidget*, GdkEventButton *event) {
        return execute_command (event, sensors);
    });
}


/* -------------------------------------------------------------------------- */
/**
 * Create sensors panel control
 * @param plugin Panel plugin proxy to create sensors plugin in
 */
static t_sensors *
create_sensors_control (XfcePanelPlugin *plugin)
{
    gchar *rc_file = xfce_panel_plugin_lookup_rc_file (plugin);

    t_sensors *sensors = sensors_new (plugin, rc_file);
    sensors->plugin_mode = xfce_panel_plugin_get_mode (plugin);
    sensors->panel_size = xfce_panel_plugin_get_size (plugin);

    add_event_box (sensors);

    /* fill panel widget with boxes, strings, values, ... */
    create_panel_widget (sensors);

    /* finally add panel "sensors" to eventbox */
    gtk_container_add (GTK_CONTAINER (sensors->eventbox), sensors->widget_sensors);

    g_free (rc_file);
    return sensors;
}


static void
sensors_show_about (XfcePanelPlugin *plugin)
{
   /* List of authors (in alphabetical order) */
   const gchar *auth[] = {
      "Benedikt Meurer",
      "Fabian Nowak <timystery@xfce.org>",
      "Jan Ziak <0xe2.0x9a.0x9b@xfce.org>",
      "Stefan Ott",
      NULL };

   GdkPixbuf *icon = xfce_panel_pixbuf_from_source ("xfce-sensors", NULL, 48);

   gtk_show_about_dialog (NULL,
      "logo", icon,
      "license", xfce_get_license_text (XFCE_LICENSE_TEXT_GPL),
      "version", PACKAGE_VERSION,
      "program-name", PACKAGE_NAME,
      "comments", _("Show sensor values from LM sensors, ACPI, hard disks, NVIDIA"),
      "website", "https://docs.xfce.org/panel-plugins/xfce4-sensors-plugin",
      "copyright", _("Copyright (c) 2004-2021\n"),
      "authors", auth,
      NULL);

   if(icon)
      g_object_unref (G_OBJECT(icon));
}


/* -------------------------------------------------------------------------- */
void
sensors_plugin_construct (XfcePanelPlugin *plugin)
{
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

    t_sensors *sensors = create_sensors_control (plugin);

    gchar *rc_file = xfce_panel_plugin_lookup_rc_file (plugin);
    if (rc_file)
    {
        sensors->plugin_config_file = rc_file;
        g_free (rc_file);
        rc_file = NULL;
    }
    sensors_read_config (plugin, sensors);

    /* use values from config file */
    gtk_widget_set_has_tooltip(sensors->eventbox, !sensors->suppresstooltip);

    if (sensors->cover_panel_rows || xfce_panel_plugin_get_mode(plugin) == XFCE_PANEL_PLUGIN_MODE_DESKBAR)
        xfce_panel_plugin_set_small(plugin, FALSE);
    else
        xfce_panel_plugin_set_small(plugin, TRUE);

    /* Try to resize the ptr_sensorsstruct to fit the user settings.
       Do also modify the tooltip text. */
    sensors_update_panel (sensors, true);

    sensors->timeout_id = xfce4::timeout_add (sensors->sensors_refresh_time * 1000, [sensors]() {
        sensors_update_panel (sensors, false);
        return xfce4::TIMEOUT_AGAIN;
    });

    xfce4::connect_free_data (plugin, [sensors](XfcePanelPlugin*) {
        sensors_free (sensors);
    });

    gchar *save_location = xfce_panel_plugin_save_location (plugin, TRUE);
    sensors->plugin_config_file = save_location;
    g_free (save_location);
    save_location = NULL;

    /* saving seems to cause problems when closing the panel on fast multi-core CPUs; writing when closing the config dialog should suffice */
    if (false)
    {
        xfce4::connect_save (plugin, [sensors](XfcePanelPlugin *p) {
            sensors_write_config (p, sensors);
        });
    }

    xfce_panel_plugin_menu_show_configure (plugin);

    xfce_panel_plugin_menu_show_about(plugin);
    xfce4::connect_about (plugin, sensors_show_about);
    xfce4::connect_configure_plugin (plugin, [sensors](XfcePanelPlugin *p) {
        sensors_create_options (p, sensors);
    });
    xfce4::connect_mode_changed (plugin, [sensors](XfcePanelPlugin *p, XfcePanelPluginMode mode) {
        sensors_set_mode (p, mode, sensors);
    });
    xfce4::connect_size_changed (plugin, [sensors](XfcePanelPlugin *p, guint size) {
        return sensors_set_size (p, size, sensors);
    });

    gtk_container_add (GTK_CONTAINER(plugin), sensors->eventbox);

    xfce_panel_plugin_add_action_widget (plugin, sensors->eventbox);

    gtk_widget_show (sensors->eventbox);
}
