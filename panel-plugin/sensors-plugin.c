/* $Id$ */
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

/* This plugin requires libsensors-3 and its headers in order to monitor
 * ordinary mainboard sensors!
 *
 * It also works with solely ACPI or hddtemp, but then with more limited
 * functionality.
 */

/* Note for programmers and editors: Try to use 4 spaces instead of Tab! */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

/* Global includes */
#include <math.h>
#include <stdlib.h>
#include <string.h>

/* Gtk/Glib includes */
#include <glib.h>
#include <glib/gprintf.h> /* ain't included in glib.h! */

/* Xfce includes */
#include <libxfce4util/libxfce4util.h>
#include <libxfcegui4/libxfcegui4.h>
#include <libxfce4panel/xfce-panel-plugin.h>

/* Package includes */
#include <configuration.h>
#include <sensors-interface.h>
#include <sensors-interface-common.h>
#include <middlelayer.h>

/* Local includes */
#include "sensors-plugin.h"


/*
 * Tooltips to display for any part of this plugin
 */
extern GtkTooltips *tooltips;


static void
sensors_set_bar_size (GtkWidget *bar, int size, int orientation)
{
    //TRACE ("enters sensors_set_bar_size");

    /* check arguments */
    g_return_if_fail (G_IS_OBJECT(bar));

    if (orientation == GTK_ORIENTATION_HORIZONTAL) {
        gtk_widget_set_size_request (bar, BORDER, size-BORDER);
    } else {
        gtk_widget_set_size_request (bar, size-BORDER, BORDER);
    }

    //TRACE ("leaves sensors_set_bar_size");
}


static void
sensors_set_bar_color (GtkWidget *bar, double fraction, gchar* user_bar_color,
                       t_sensors *sensors)
{
    GdkColor color;

    //TRACE ("enters sensors_set_bar_color");

    g_return_if_fail (G_IS_OBJECT(bar));

    if (fraction >= 1 || fraction<=0)
        gdk_color_parse(COLOR_ERROR, &color);

    else if ((fraction < .2) || (fraction > .8))
        gdk_color_parse(COLOR_WARN, &color);

    else {
        if (sensors->show_colored_bars)
            gdk_color_parse(user_bar_color, &color);
        else
            gdk_color_parse(COLOR_NORMAL, &color);
    }

    gtk_widget_modify_bg (bar, GTK_STATE_PRELIGHT, &color);
    gtk_widget_modify_bg (bar, GTK_STATE_SELECTED, &color);
    gtk_widget_modify_base (bar, GTK_STATE_SELECTED, &color);

    //TRACE ("leaves sensors_set_bar_color");
}

static double
sensors_get_percentage (t_chipfeature *chipfeature)
{
    double value, min, max, percentage;

    //TRACE ("enters sensors_get_percentage");

    value = chipfeature->raw_value;
    min = chipfeature->min_value;
    max = chipfeature->max_value;
    percentage = (value - min) / (max - min);

    if (percentage > 1.0)
        percentage = 1.0;

    else if (percentage <= 0.0)
        percentage = 0.0;

    //TRACE ("leaves sensors_get_percentage with %f", percentage);
    return percentage;
}


static void
sensors_remove_graphical_panel (t_sensors *sensors)
{
    int chipNum, feature;
    t_chip *chip;
    t_chipfeature *chipfeature;
    t_barpanel *panel;

    TRACE ("enters sensors_remove_graphical_panel");

    for (chipNum=0; chipNum < sensors->num_sensorchips; chipNum++) {
        chip = (t_chip *) g_ptr_array_index(sensors->chips, chipNum);
        g_assert (chip != NULL);

        for (feature=0; feature<chip->num_features; feature++) {
            chipfeature = g_ptr_array_index(chip->chip_features, feature);
            g_assert (chipfeature != NULL);

            if (chipfeature->show == TRUE) {
                panel = (t_barpanel*) sensors->panels[chipNum][feature];

        if (sensors->show_labels == TRUE) /* FN: FIXME; this value is already updated! */
                    gtk_widget_destroy (panel->label);

                gtk_widget_destroy (panel->progressbar);
                gtk_widget_destroy (panel->databox);

                g_free (panel);
            }
        }
    }
    sensors->bars_created = FALSE;
    gtk_widget_hide (sensors->panel_label_text);

    TRACE ("leaves sensors_remove_graphical_panel");
}


static void
sensors_update_graphical_panel (t_sensors *sensors)
{
    int chipNum, feature;
    t_chip *chip;
    t_chipfeature *chipfeature;
    t_barpanel *panel;
    double fraction;
    GtkWidget *bar;

    //TRACE("enters sensors_update_graphical_panel");

    for (chipNum=0; chipNum < sensors->num_sensorchips; chipNum++) {
        chip = (t_chip *) g_ptr_array_index(sensors->chips, chipNum);
        g_assert (chip != NULL);

        for (feature=0; feature<chip->num_features; feature++) {
            chipfeature = g_ptr_array_index(chip->chip_features, feature);
            g_assert (chipfeature != NULL);

            if (chipfeature->show == TRUE) {
                panel = (t_barpanel*) sensors->panels[chipNum][feature];

                bar = panel->progressbar;
                g_return_if_fail (G_IS_OBJECT(bar));

                sensors_set_bar_size (bar, (int) sensors->panel_size,
                                      sensors->orientation);
                fraction = sensors_get_percentage (chipfeature);
                sensors_set_bar_color (bar, fraction, chipfeature->color,
                                       sensors);
                gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(bar),
                                               fraction);
            }
        }
    }

    //TRACE("leaves sensors_update_graphical_panel");
}


static void
sensors_add_graphical_display (t_sensors *sensors)
{
    int chipNum, feature;
    t_chip *chip;
    t_chipfeature *chipfeature;
    t_barpanel *panel;
    gboolean has_bars = FALSE;
    GtkWidget *progbar, *databox, *label;
    gchar *caption, *text;

    TRACE ("enters sensors_add_graphical_display");

    text = g_strdup (_("<span foreground=\"#000000\">"
                                     "<b>Sensors</b></span>"));
    gtk_label_set_markup (GTK_LABEL(sensors->panel_label_text), text);
    g_free (text);

    for (chipNum=0; chipNum < sensors->num_sensorchips; chipNum++) {
        chip = (t_chip *) g_ptr_array_index(sensors->chips, chipNum);
        g_assert (chip != NULL);

        for (feature=0; feature<chip->num_features; feature++) {
            chipfeature = g_ptr_array_index (chip->chip_features, feature);
            g_assert (chipfeature != NULL);

            if (chipfeature->show == TRUE) {
                has_bars = TRUE;

                progbar = gtk_progress_bar_new();

                if (sensors->orientation == GTK_ORIENTATION_HORIZONTAL)
                    gtk_progress_bar_set_orientation (
                                                GTK_PROGRESS_BAR (progbar),
                                                GTK_PROGRESS_BOTTOM_TO_TOP);
                else
                    gtk_progress_bar_set_orientation (
                                                GTK_PROGRESS_BAR (progbar),
                                                GTK_PROGRESS_LEFT_TO_RIGHT);

                sensors_set_bar_size (progbar, (int) sensors->panel_size,
                                      sensors->orientation);
                gtk_widget_show (progbar);

                if (sensors->orientation == GTK_ORIENTATION_HORIZONTAL)
                    databox = gtk_hbox_new (FALSE, 0);

                else
                    databox = gtk_vbox_new (FALSE, 0);

                gtk_widget_show (databox);

                /* save the panel elements */
                panel = g_new (t_barpanel, 1);
                panel->progressbar = progbar;

                /* create the label stuff only if needed - saves some memory! */
                if (sensors->show_labels == TRUE) {
                    caption = g_strdup (chipfeature->name);
                    label = gtk_label_new (caption);
                    g_free(caption);
                    gtk_label_set_max_width_chars (GTK_LABEL(label), 12);
                    gtk_label_set_ellipsize (GTK_LABEL(label),
                                             PANGO_ELLIPSIZE_END);
                    gtk_widget_show (label);
                    panel->label = label;

                    gtk_box_pack_start (GTK_BOX(databox), label, FALSE, FALSE, INNER_BORDER);
                }
                else
                    panel->label = NULL;

                gtk_box_pack_start (GTK_BOX(databox), progbar, FALSE, FALSE, 0);

                panel->databox = databox;
                sensors->panels[chipNum][feature] = (GtkWidget*) panel;

                gtk_container_set_border_width (GTK_CONTAINER(sensors->widget_sensors), 0);
                gtk_box_pack_start (GTK_BOX (sensors->widget_sensors),
                                    databox, FALSE, FALSE, INNER_BORDER);
            }
        }
    }
    if (has_bars && !sensors->show_title)
        gtk_widget_hide (sensors->panel_label_text);
    else
        gtk_widget_show (sensors->panel_label_text);

    gtk_widget_hide (sensors->panel_label_data);

    sensors->bars_created = TRUE;
    sensors_update_graphical_panel (sensors);

    TRACE ("leaves sensors_add_graphical_display");
}


static gboolean
sensors_show_graphical_display (t_sensors *sensors)
{
    TRACE ("enters sensors_show_graphical_display");

    if (sensors->bars_created == TRUE)
        sensors_update_graphical_panel (sensors);
    else
        sensors_add_graphical_display (sensors);

    TRACE ("leaves sensors_show_graphical_display");

    return TRUE;
}


static int
determine_number_of_rows (t_sensors *sensors)
{
    gint numRows, gtk_font_size, additional_offset, avail_height;
    gdouble divisor;
    GtkStyle *style;
    PangoFontDescription *descr;
    PangoFontMask mask;
    gboolean is_absolute;

    TRACE ("enters determine_number_of_rows");

    style = gtk_widget_get_style (sensors->panel_label_data);

    descr = style->font_desc; /* Note that stupid GTK guys forgot the 'r' */

    is_absolute = FALSE;
    mask = pango_font_description_get_set_fields (descr);
    if (mask>=PANGO_FONT_MASK_SIZE) {
        is_absolute = pango_font_description_get_size_is_absolute (descr);
        if (!is_absolute) {
            gtk_font_size = pango_font_description_get_size (descr) / 1000;
        }
    }

    if (mask<PANGO_FONT_MASK_SIZE || is_absolute) {
        gtk_font_size = 10; /* not many people will want a bigger font size,
                                and so only few rows are gonna be displayed. */
    }

    g_assert (gtk_font_size!=0);

    if (sensors->orientation == GTK_ORIENTATION_HORIZONTAL) {
        switch (gtk_font_size) {
            case 8:
                switch (sensors->font_size_numerical) {
                    case 0: additional_offset=10; divisor = 12; break;
                    case 1: additional_offset=11; divisor = 12; break;
                    case 2: additional_offset=12; divisor = 12; break;
                    case 3: additional_offset=13; divisor = 13; break;
                    default: additional_offset=16; divisor = 17;
                }
                break;

            case 9:
                switch (sensors->font_size_numerical) {
                    case 0: additional_offset=12; divisor = 13; break;
                    case 1: additional_offset=13; divisor = 13; break;
                    case 2: additional_offset=14; divisor = 14; break;
                    case 3: additional_offset=14; divisor = 17; break;
                    default: additional_offset=16; divisor = 20;
                }
                break;

            default: /* case 10 */
                 switch (sensors->font_size_numerical) {
                    case 0: additional_offset=13; divisor = 14; break;
                    case 1: additional_offset=14; divisor = 14; break;
                    case 2: additional_offset=14; divisor = 14; break;
                    case 3: additional_offset=16; divisor = 17; break;
                    default: additional_offset=20; divisor = 20;
                }
        }
        avail_height = sensors->panel_size - additional_offset;
        if (avail_height < 0)
            avail_height = 0;

        numRows = (int) floor (avail_height / divisor);
        if (numRows < 0)
            numRows = 0;

        numRows++;
    }
    else numRows = 1 << 30; /* that's enough, MAXINT would be nicer */

    /* fail-safe */
    if (numRows<=0)
        numRows = 1;

    TRACE ("leaves determine_number_of_rows with rows=%d", numRows);

    return numRows;
}


static int
determine_number_of_cols (gint numRows, gint itemsToDisplay)
{
    gint numCols;

    TRACE ("enters determine_number_of_cols");

    if (numRows > 1) {
        if (itemsToDisplay > numRows)
            numCols = (gint) ceil (itemsToDisplay/ (float)numRows);
        else
            numCols = (gint) 1;
    }
    else
        numCols = itemsToDisplay;

    TRACE ("leaves determine_number_of_cols width cols=%d", numCols);

    return numCols;
}


static void
sensors_set_text_panel_label (t_sensors *sensors, gint numCols, gint itemsToDisplay)
{
    gint currentColumn, chipNum, feature;
    t_chip *chip;
    t_chipfeature *chipfeature;
    gchar *myLabelText, *tmpstring;

    TRACE ("enters sensors_set_text_panel_label");

    if (itemsToDisplay==0) {
        gtk_widget_hide (sensors->panel_label_data);
        return;
    }

    currentColumn = 0;
    myLabelText = g_strdup (""); /* don't use NULL because of g_strconcat */

    for (chipNum=0; chipNum < sensors->num_sensorchips; chipNum++) {
        chip = (t_chip *) g_ptr_array_index (sensors->chips, chipNum);
        g_assert (chip != NULL);

        for (feature=0; feature<chip->num_features; feature++) {
            chipfeature = g_ptr_array_index (chip->chip_features, feature);
            g_assert (chipfeature != NULL);

            if (chipfeature->show == TRUE) {
                if (sensors->show_units) {
                    tmpstring = g_strconcat (myLabelText,
                                            "<span foreground=\"",
                                            chipfeature->color, "\" size=\"",
                                            sensors->font_size, "\">",
                                            chipfeature->formatted_value,
                                            "</span>", NULL);
                }
                else {
                    tmpstring = g_strdup_printf("%s<span foreground=\"%s\" size=\"%s\">%.1f</span>", myLabelText,
                            chipfeature->color, sensors->font_size,
                            chipfeature->raw_value);
                    myLabelText = g_strconcat (myLabelText, tmpstring, NULL);
                }

                g_free (myLabelText);

                myLabelText = tmpstring;

                if (sensors->orientation == GTK_ORIENTATION_VERTICAL) {
                    if (itemsToDisplay > 1)
                        myLabelText = g_strconcat (myLabelText, "\n", NULL);
                }
                else if (currentColumn < numCols-1) {
                    if (sensors->show_smallspacings)
                        myLabelText = g_strconcat (myLabelText, "  ", NULL);
                    else
                        myLabelText = g_strconcat (myLabelText, " \t", NULL);
                    currentColumn++;
                }
                else if (itemsToDisplay > 1) { /* do NOT add \n if last item */
                    myLabelText = g_strconcat (myLabelText, " \n", NULL);
                    currentColumn = 0;
                }
                itemsToDisplay--;
            }
        }
    }

    g_assert (itemsToDisplay==0);

    gtk_label_set_markup (GTK_LABEL(sensors->panel_label_data), myLabelText);
    /* if (sensors->show_units) */
        g_free(myLabelText);
    /* else: with sprintf, we cannot free the string. how bad. */

    if (sensors->orientation== GTK_ORIENTATION_HORIZONTAL)
        gtk_misc_set_alignment(GTK_MISC(sensors->panel_label_data), 0.0, 0.5);
    else
        gtk_misc_set_alignment(GTK_MISC(sensors->panel_label_data), 0.5, 0.5);

    gtk_widget_show (sensors->panel_label_data);

    TRACE ("leaves sensors_set_text_panel_label");
}


static int
count_number_checked_sensor_features (t_sensors *sensors)
{
    gint chipNum, feature, itemsToDisplay;
    t_chipfeature *chipfeature;
    t_chip *chip;

    TRACE ("enters count_number_checked_sensor_features");

    itemsToDisplay = 0;


    for (chipNum=0; chipNum < sensors->num_sensorchips; chipNum++) {
        chip = (t_chip *) g_ptr_array_index (sensors->chips, chipNum);
        g_assert (chip!=NULL);

        for (feature=0; feature<chip->num_features; /* none */ ) {
            chipfeature = g_ptr_array_index (chip->chip_features, feature);
            g_assert (chipfeature!=NULL);

            if (chipfeature->show == TRUE)
                itemsToDisplay++;

            if (chipfeature->valid == TRUE)
                feature++;
        }
    }

    TRACE ("leaves count_number_checked_sensor_features with %d", itemsToDisplay);

    return itemsToDisplay;
}


/* draw label with sensor values into panel's vbox */
static gboolean
sensors_show_text_display (t_sensors *sensors)
{
    gint itemsToDisplay, numRows, numCols;

    TRACE ("enters sensors_show_text_display");

    /* count number of checked sensors to display.
       this could also be done by every toggle/untoggle action
       by putting this variable into t_sensors */
    itemsToDisplay = count_number_checked_sensor_features (sensors);

    numRows = determine_number_of_rows (sensors);

    if (sensors->show_title == TRUE || itemsToDisplay==0)
        gtk_widget_show (sensors->panel_label_text);
    else
        gtk_widget_hide (sensors->panel_label_text);

    numCols = determine_number_of_cols (numRows, itemsToDisplay);

    sensors_set_text_panel_label (sensors, numCols, itemsToDisplay);

    TRACE ("leaves sensors_show_text_display\n");

    return TRUE;
}


/* create tooltip
Updates the sensor values, see lines 440 and following */
static gboolean
sensors_create_tooltip (gpointer data)
{
    TRACE ("enters sensors_create_tooltip");

    t_sensors *sensors;
    GtkWidget *widget;
    int i, index_feature, res;
    double sensorFeature;
    gboolean first, prependedChipName;
    gchar *myToolTipText, *myToolTipText2, *tmp;
    t_chipfeature *chipfeature;
    t_chip *chip;

    g_return_val_if_fail (data != NULL, FALSE);

    sensors = (t_sensors *) data;
    widget = sensors->eventbox;
    myToolTipText = g_strdup (_("No sensors selected!"));
    first = TRUE;

    for (i=0; i < sensors->num_sensorchips; i++) {
        chip = (t_chip *) g_ptr_array_index (sensors->chips, i);
        g_assert (chip!=NULL);

        prependedChipName = FALSE;

        for (index_feature = 0; index_feature<chip->num_features; index_feature++) {
            chipfeature = g_ptr_array_index (chip->chip_features, index_feature);
            g_assert (chipfeature!=NULL);

            if ( chipfeature->valid == TRUE && chipfeature->show == TRUE ) {

                if (prependedChipName != TRUE) {

                    if (first == TRUE) {
                        myToolTipText = g_strdup (chip->sensorId);
                        first = FALSE;
                    }
                    else {
                        myToolTipText2 = g_strconcat (myToolTipText, " \n",
                                                     chip->sensorId, NULL);
                        g_free (myToolTipText);
                        myToolTipText = myToolTipText2;
                    }

                    prependedChipName = TRUE;
                }

                res = sensor_get_value (chip, chipfeature->address,
                                                    &sensorFeature,
                                                    &(sensors->suppressmessage));

                if ( res!=0 ) {
                    /* FIXME: either print nothing, or undertake appropriate action,
                     * or pop up a message box. */
                    g_printf ( _("Sensors Plugin:\n"
                    "Seems like there was a problem reading a sensor feature "
                    "value.\nProper proceeding cannot be guaranteed.\n") );
                    break;
                }
                tmp = g_new (gchar, 0);
                format_sensor_value (sensors->scale, chipfeature,
                                     sensorFeature, &tmp);

                myToolTipText2 = g_strconcat (myToolTipText, "\n  ",
                                             chipfeature->name, ": ", tmp,
                                             NULL);
                g_free (myToolTipText);
                myToolTipText = myToolTipText2;

                g_free (chipfeature->formatted_value);
                chipfeature->formatted_value = g_strdup (tmp);
                chipfeature->raw_value = sensorFeature;

                g_free (tmp);
            } /* end if chipfeature->valid */
        }
    }

    if (!tooltips)
      tooltips = gtk_tooltips_new();

    /* #if GTK_VERSION < 2.11 */
    gtk_tooltips_set_tip (tooltips, GTK_WIDGET(sensors->eventbox),
                          myToolTipText, NULL);
    g_free (myToolTipText);

    TRACE ("leaves sensors_create_tooltip");

    return TRUE;
}


static gboolean
sensors_show_panel (gpointer data)
{
    t_sensors *sensors;
    gboolean result;

    TRACE ("enters sensors_show_panel");

    g_return_val_if_fail (data != NULL, FALSE);

    sensors = (t_sensors *) data;

    sensors_create_tooltip ((gpointer) sensors);

    if (sensors->display_values_graphically == FALSE)
        result = sensors_show_text_display (sensors);
    else
        result = sensors_show_graphical_display (sensors);

    TRACE ("leaves sensors_show_panel\n");
    return result;
}


static void
sensors_set_orientation (XfcePanelPlugin *plugin, GtkOrientation orientation,
                         t_sensors *sensors)
{
    GtkWidget *newBox;
    int i, feature;
    t_chip *chip;
    t_chipfeature *chipfeature;
    t_barpanel *panel;

    TRACE ("enters sensors_set_orientation: %d", orientation);

    if (orientation == sensors->orientation) // || !sensors->display_values_graphically)
        return;

    sensors->orientation = orientation; /* now assign the new orientation */

    newBox = orientation==GTK_ORIENTATION_HORIZONTAL ? gtk_hbox_new(FALSE, 0) : gtk_vbox_new(FALSE, 0);
    gtk_widget_show (newBox);

    gtk_widget_reparent (sensors->panel_label_text, newBox);
    gtk_widget_reparent (sensors->panel_label_data, newBox);

    if (sensors->display_values_graphically)
    {
        for (i=0; i < sensors->num_sensorchips; i++) {
            chip = (t_chip *) g_ptr_array_index (sensors->chips, i);
            g_assert (chip!=NULL);

            for (feature = 0; feature<chip->num_features; feature++)
            {
                chipfeature = g_ptr_array_index (chip->chip_features, feature);
                g_assert (chipfeature!=NULL);

                if (chipfeature->show == TRUE) {
                    panel = (t_barpanel*) sensors->panels[i][feature];
                    gtk_widget_reparent (panel->databox, newBox);
                }
            }
        }
    }
    else
    {
        /* we don't need a separate function for just one call. */
    }

    gtk_widget_destroy (sensors->widget_sensors);
    sensors->widget_sensors = newBox;

    gtk_container_add (GTK_CONTAINER (sensors->eventbox),
                   sensors->widget_sensors);

    if (sensors->display_values_graphically)
        sensors_remove_graphical_panel (sensors);

    sensors_show_panel (sensors);

    TRACE ("leaves sensors_set_orientation");
}




/* initialize box and label to pack them together */
static void
create_panel_widget (t_sensors * sensors)
{
    gchar *myLabelText;
    TRACE ("enters create_panel_widget");

    /* initialize a new vbox widget */
    if (sensors->orientation == GTK_ORIENTATION_HORIZONTAL)
        sensors->widget_sensors = gtk_hbox_new (FALSE, 0);
    else
        sensors->widget_sensors = gtk_vbox_new (FALSE, 0);

    gtk_widget_show (sensors->widget_sensors);

    /* initialize value label widget */
    sensors->panel_label_text = gtk_label_new (NULL);
    gtk_misc_set_padding (GTK_MISC(sensors->panel_label_text), INNER_BORDER, 0);
    gtk_misc_set_alignment(GTK_MISC(sensors->panel_label_text), 0.0, 0.5);

    myLabelText = g_strdup (_("<span foreground=\"#000000\"><b>Sensors"
                                    "</b></span>"));
    gtk_label_set_markup(GTK_LABEL(sensors->panel_label_text), myLabelText);
    gtk_widget_show (sensors->panel_label_text);
    g_free(myLabelText);

    sensors->panel_label_data = gtk_label_new (NULL);
    gtk_widget_show (sensors->panel_label_data);

    /* create 'valued' label */
    sensors_show_panel (sensors);

    /* add newly created label to box */
    gtk_box_pack_start (GTK_BOX (sensors->widget_sensors),
                        sensors->panel_label_text, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (sensors->widget_sensors),
                        sensors->panel_label_data, TRUE, TRUE, 0);


    TRACE ("leaves create_panel_widget");
}


/* double-click improvement */
static gboolean
execute_command (GtkWidget *widget, GdkEventButton *event, gpointer data)
{
    t_sensors *sensors;

    TRACE ("enters execute_command");

    g_return_val_if_fail (data != NULL, FALSE);

    if (event->type == GDK_2BUTTON_PRESS) {

        sensors = (t_sensors *) data;

        g_return_val_if_fail ( sensors->exec_command, FALSE);

        xfce_exec (sensors->command_name, FALSE, FALSE, NULL);

        TRACE ("leaves execute_command with TRUE");

        return TRUE;
    }
    else {
        TRACE ("leaves execute_command with FALSE");
        return FALSE;
    }
}



/* #if GTK_VERSION >= 2.11
 * static gboolean
handle_tooltip_query (GtkWidget  *widget,
                                         gint        x, gint        y,
                                         GtkTooltip *tooltip,
                                         gpointer    data)
{
    t_sensors *sensors;
    gchar *buffer;

    g_assert (data!=NULL);

    sensors = (t_sensors *) data;

    buffer = g_strdup("Tooltip placeholder");

    gtk_tooltip_set_markup (tooltip, buffer);

    return TRUE;
} */




static void
sensors_free (XfcePanelPlugin *plugin, t_sensors *sensors)
{
    TRACE ("enters sensors_free");

    g_return_if_fail (sensors != NULL);

    /* stop association to libsensors and others*/
    sensor_interface_cleanup();

    /* remove timeout functions */
    if (sensors->timeout_id)
        g_source_remove (sensors->timeout_id);

    /* double-click improvement */
    g_source_remove (sensors->doubleclick_id);

    /* free structures and arrays */
    g_ptr_array_foreach (sensors->chips, free_chip, NULL);
    g_ptr_array_free (sensors->chips, TRUE);

    g_free (sensors->plugin_config_file);
    g_free (sensors);

    TRACE ("leaves sensors_free");
}


static void
sensors_set_size (XfcePanelPlugin *plugin, int size, t_sensors *sensors)
{
    TRACE ("enters sensors_set_size");

    sensors->panel_size = (gint) size;

    /* update the panel widget */
    sensors_show_panel ((gpointer) sensors);

    TRACE ("leaves sensors_set_size");
}


static void
show_title_toggled (GtkWidget *widget, t_sensors_dialog *sd)
{
    TRACE ("enters show_title_toggled");

    if (sd->sensors->display_values_graphically == TRUE) {
        sensors_remove_graphical_panel(sd->sensors);
    }
    sd->sensors->show_title = gtk_toggle_button_get_active
        ( GTK_TOGGLE_BUTTON(widget) );

    sensors_show_panel ((gpointer) sd->sensors);

    TRACE ("leaves show_title_toggled");
}


static void
show_labels_toggled (GtkWidget *widget, t_sensors_dialog *sd)
{
    TRACE ("enters show_labels_toggled");

    if (sd->sensors->display_values_graphically == TRUE) {
        sensors_remove_graphical_panel(sd->sensors);
    }

    sd->sensors->show_labels = gtk_toggle_button_get_active
        ( GTK_TOGGLE_BUTTON(widget) );

    sensors_show_panel ((gpointer) sd->sensors);

    TRACE ("leaves show_labels_toggled");
}

static void
show_colored_bars_toggled (GtkWidget *widget, t_sensors_dialog *sd)
{
    TRACE ("enters show_colored_bars_toggled");

    if (sd->sensors->display_values_graphically == TRUE) {
        sensors_remove_graphical_panel(sd->sensors);
    }

    sd->sensors->show_colored_bars = gtk_toggle_button_get_active
        ( GTK_TOGGLE_BUTTON(widget) );

    sensors_show_panel ((gpointer) sd->sensors);

    TRACE ("leaves show_colored_bars_toggled");
}

static void
display_style_changed (GtkWidget *widget, t_sensors_dialog *sd)
{
    TRACE ("enters display_style_changed");

    if (sd->sensors->display_values_graphically == TRUE) {
        sensors_remove_graphical_panel(sd->sensors);
        gtk_widget_hide(sd->labels_Box);
        gtk_widget_hide(sd->coloredBars_Box);
        gtk_widget_show(sd->font_Box);
        gtk_widget_show (sd->unit_checkbox);
        gtk_widget_show (sd->smallspacing_checkbox);
    }
    else {
        gtk_widget_show(sd->labels_Box);
        gtk_widget_show(sd->coloredBars_Box);
        gtk_widget_hide(sd->font_Box);
        gtk_widget_hide (sd->unit_checkbox);
        gtk_widget_hide (sd->smallspacing_checkbox);
    }

    sd->sensors->display_values_graphically = gtk_toggle_button_get_active
        ( GTK_TOGGLE_BUTTON(widget) );

    sensors_show_panel ((gpointer) sd->sensors);

    TRACE ("leaves display_style_changed");
}


void
sensor_entry_changed (GtkWidget *widget, t_sensors_dialog *sd)
{
    gint gtk_combo_box_active;
    t_chip *chip;

    TRACE ("enters sensor_entry_changed");

    gtk_combo_box_active = gtk_combo_box_get_active(GTK_COMBO_BOX (widget));

    chip = (t_chip *) g_ptr_array_index (sd->sensors->chips,
                                         gtk_combo_box_active);
    gtk_label_set_label (GTK_LABEL(sd->mySensorLabel), chip->description);

    gtk_tree_view_set_model (GTK_TREE_VIEW (sd->myTreeView),
                    GTK_TREE_MODEL ( sd->myListStore[gtk_combo_box_active] ) );

    TRACE ("leaves sensor_entry_changed");
}


static void
font_size_change (GtkWidget *widget, t_sensors_dialog *sd)
{
    TRACE ("enters font_size_change");

    switch ( gtk_combo_box_get_active(GTK_COMBO_BOX(widget)) ) {

        case 0: sd->sensors->font_size = "x-small"; break;
        case 1: sd->sensors->font_size = "small"; break;
        case 3: sd->sensors->font_size = "large"; break;
        case 4: sd->sensors->font_size = "x-large"; break;
        default: sd->sensors->font_size = "medium";
    }

    sd->sensors->font_size_numerical =
        gtk_combo_box_get_active(GTK_COMBO_BOX(widget));

    /* refresh the panel content */
    sensors_show_panel ((gpointer) sd->sensors);

    TRACE ("leaves font_size_change");
}


void
temperature_unit_change (GtkWidget *widget, t_sensors_dialog *sd)
{
    TRACE ("enters temperature_unit_change ");

    /* toggle celsius-fahrenheit by use of mathematics ;) */
    sd->sensors->scale = 1 - sd->sensors->scale;

    /* refresh the panel content */
    sensors_show_panel ((gpointer) sd->sensors);

    reload_listbox (sd);

    TRACE ("laeves temperature_unit_change ");
}


static void
adjustment_value_changed (GtkWidget *widget, t_sensors_dialog* sd)
{
    TRACE ("enters adjustment_value_changed ");

    sd->sensors->sensors_refresh_time =
        (gint) gtk_adjustment_get_value ( GTK_ADJUSTMENT (widget) );

    /* stop the timeout functions ... */
    g_source_remove (sd->sensors->timeout_id);
    /* ... and start them again */
    sd->sensors->timeout_id  = g_timeout_add (
        sd->sensors->sensors_refresh_time * 1000,
        (GtkFunction) sensors_show_panel, (gpointer) sd->sensors);

    TRACE ("leaves adjustment_value_changed ");
}


static void
draw_units_changed (GtkWidget *widget, t_sensors_dialog* sd)
{
    TRACE ("enters draw_units_changed");

    sd->sensors->show_units = ! sd->sensors->show_units;

    sensors_show_text_display (sd->sensors);

    TRACE ("leaves draw_units_changed");
}


static void
draw_smallspacings_changed (GtkWidget *widget, t_sensors_dialog* sd)
{
    TRACE ("enters draw_smallspacings_changed");

    sd->sensors->show_smallspacings = ! sd->sensors->show_smallspacings;

    sensors_show_text_display (sd->sensors);

    TRACE ("leaves draw_smallspacings_changed");
}


static void
suppressmessage_changed (GtkWidget *widget, t_sensors_dialog* sd)
{
    TRACE ("enters suppressmessage_changed");

    sd->sensors->suppressmessage = ! sd->sensors->suppressmessage;

    TRACE ("leaves suppressmessage_changed");
}


/* double-click improvement */
static void
execCommand_toggled (GtkWidget *widget, t_sensors_dialog* sd)
{
    TRACE ("enters execCommand_toggled");

    sd->sensors->exec_command =
        gtk_toggle_button_get_active ( GTK_TOGGLE_BUTTON (widget) );

    if ( sd->sensors->exec_command )
        g_signal_handler_unblock (sd->sensors->eventbox,
            sd->sensors->doubleclick_id);
    else
        g_signal_handler_block (sd->sensors->eventbox,
            sd->sensors->doubleclick_id );

    TRACE ("leaves execCommand_toggled");
}


void
minimum_changed (GtkCellRendererText *cellrenderertext, gchar *path_str,
                 gchar *new_value, t_sensors_dialog *sd)
{
    gint gtk_combo_box_active;
    float  value;
    GtkTreeModel *model;
    GtkTreePath *path;
    GtkTreeIter iter;
    t_chip *chip;
    t_chipfeature *chipfeature;

    TRACE ("enters minimum_changed");

    value = atof (new_value);

    gtk_combo_box_active =
        gtk_combo_box_get_active(GTK_COMBO_BOX (sd->myComboBox));

    /* get model and path */
    model = (GtkTreeModel *) sd->myListStore
        [gtk_combo_box_active];
    path = gtk_tree_path_new_from_string (path_str);

    /* get model iterator */
    gtk_tree_model_get_iter (model, &iter, path);

    /* set new value according to chosen scale */
    gtk_tree_store_set (GTK_TREE_STORE (model), &iter, 4, value, -1);
    chip = (t_chip *) g_ptr_array_index(sd->sensors->chips, gtk_combo_box_active);

    chipfeature = (t_chipfeature *) g_ptr_array_index(chip->chip_features, atoi(path_str));
    if (sd->sensors->scale==FAHRENHEIT)
      value = (value -32 ) * 5/9;
    chipfeature->min_value = value;

    /* clean up */
    gtk_tree_path_free (path);

    if (sd->sensors->display_values_graphically == TRUE) {
        sensors_remove_graphical_panel (sd->sensors);
    }

    /* update panel */
    sensors_show_panel ((gpointer) sd->sensors);

    TRACE ("leaves minimum_changed");
}


void
maximum_changed (GtkCellRendererText *cellrenderertext, gchar *path_str,
            gchar *new_value, t_sensors_dialog *sd)
{
    gint gtk_combo_box_active;
    float value;
    GtkTreeModel *model;
    GtkTreePath *path;
    GtkTreeIter iter;
    t_chip *chip;
    t_chipfeature *chipfeature;

    TRACE ("enters maximum_changed");

    value = atof (new_value);

    gtk_combo_box_active =
        gtk_combo_box_get_active(GTK_COMBO_BOX (sd->myComboBox));

    /* get model and path */
    model = (GtkTreeModel *) sd->myListStore
        [gtk_combo_box_active];
    path = gtk_tree_path_new_from_string (path_str);

    /* get model iterator */
    gtk_tree_model_get_iter (model, &iter, path);

    /* set new value according to chosen scale */
    gtk_tree_store_set (GTK_TREE_STORE (model), &iter, 5, value, -1);
    chip = (t_chip *) g_ptr_array_index(sd->sensors->chips, gtk_combo_box_active);

    chipfeature = (t_chipfeature *) g_ptr_array_index(chip->chip_features, atoi(path_str));
    if (sd->sensors->scale==FAHRENHEIT)
      value = (value -32 ) * 5/9;
    chipfeature->max_value = value;

    /* clean up */
    gtk_tree_path_free (path);

    if (sd->sensors->display_values_graphically == TRUE) {
        sensors_remove_graphical_panel (sd->sensors);
    }

    /* update panel */
    sensors_show_panel ((gpointer) sd->sensors);

    TRACE ("leaves maximum_changed");
}


void
list_cell_color_edited (GtkCellRendererText *cellrenderertext, gchar *path_str,
                       gchar *new_color, t_sensors_dialog *sd)
{
    gint gtk_combo_box_active;
    GtkTreeModel *model;
    GtkTreePath *path;
    GtkTreeIter iter;
    gboolean hexColor;
    t_chip *chip;
    t_chipfeature *chipfeature;

    TRACE ("enters list_cell_color_edited");

    /* store new color in appropriate array */
    hexColor = g_str_has_prefix (new_color, "#");

    if (hexColor && strlen(new_color) == 7) {
        int i;
        for (i=1; i<7; i++) {
            /* only save hex numbers! */
            if ( ! g_ascii_isxdigit (new_color[i]) )
                return;
        }

        gtk_combo_box_active =
            gtk_combo_box_get_active(GTK_COMBO_BOX (sd->myComboBox));

        /* get model and path */
        model = (GtkTreeModel *) sd->myListStore
            [gtk_combo_box_active];
        path = gtk_tree_path_new_from_string (path_str);

        /* get model iterator */
        gtk_tree_model_get_iter (model, &iter, path);

        /* set new value */
        gtk_tree_store_set (GTK_TREE_STORE (model), &iter, 3, new_color, -1);
        chip = (t_chip *) g_ptr_array_index(sd->sensors->chips, gtk_combo_box_active);

        chipfeature = (t_chipfeature *) g_ptr_array_index(chip->chip_features, atoi(path_str));
        g_free (chipfeature->color);
        chipfeature->color = g_strdup(new_color);

        /* clean up */
        gtk_tree_path_free (path);

        /* update panel */
        sensors_show_panel ((gpointer) sd->sensors);
    }

    TRACE ("leaves list_cell_color_edited");
}


void
list_cell_text_edited (GtkCellRendererText *cellrenderertext,
                      gchar *path_str, gchar *new_text, t_sensors_dialog *sd)
{
    gint gtk_combo_box_active;
    GtkTreeModel *model;
    GtkTreePath *path;
    GtkTreeIter iter;
    t_chip *chip;
    t_chipfeature *chipfeature;

    TRACE ("enters list_cell_text_edited");

    if (sd->sensors->display_values_graphically == TRUE) {
        DBG("removing graphical panel");
        sensors_remove_graphical_panel(sd->sensors);
    DBG("done removing grap. panel");
    }
    gtk_combo_box_active =
        gtk_combo_box_get_active(GTK_COMBO_BOX (sd->myComboBox));

    model =
        (GtkTreeModel *) sd->myListStore [gtk_combo_box_active];
    path = gtk_tree_path_new_from_string (path_str);

    /* get model iterator */
    gtk_tree_model_get_iter (model, &iter, path);

    /* set new value */
    gtk_tree_store_set (GTK_TREE_STORE (model), &iter, 0, new_text, -1);
    chip = (t_chip *) g_ptr_array_index(sd->sensors->chips, gtk_combo_box_active);

    /* if (chip->type!=ACPI) { */ /* No Bug. The cryptic filesystem names are
                                  needed for the update in ACPI and hddtemp. */
        chipfeature = (t_chipfeature *) g_ptr_array_index (chip->chip_features,
                                                            atoi(path_str));
        g_free(chipfeature->name);
        chipfeature->name = g_strdup (new_text);
    /* } */

    /* clean up */
    gtk_tree_path_free (path);

    /* update panel */
    sensors_show_panel ((gpointer) sd->sensors);

    TRACE ("leaves list_cell_text_edited");
}


void
list_cell_toggle (GtkCellRendererToggle *cell, gchar *path_str,
                  t_sensors_dialog *sd)
{
    t_chip *chip;
    t_chipfeature *chipfeature;
    gint gtk_combo_box_active;
    GtkTreeModel *model;
    GtkTreePath *path;
    GtkTreeIter iter;
    gboolean toggle_item;

    TRACE ("enters list_cell_toggle");

    if (sd->sensors->display_values_graphically == TRUE) {
        sensors_remove_graphical_panel(sd->sensors);
    }
    gtk_combo_box_active =
        gtk_combo_box_get_active(GTK_COMBO_BOX (sd->myComboBox));

    model = (GtkTreeModel *) sd->myListStore[gtk_combo_box_active];
    path = gtk_tree_path_new_from_string (path_str);

    /* get toggled iter */
    gtk_tree_model_get_iter (model, &iter, path);
    gtk_tree_model_get (model, &iter, 2, &toggle_item, -1);

    /* do something with the value */
    toggle_item ^= 1;

    /* set new value */
    gtk_tree_store_set (GTK_TREE_STORE (model), &iter, 2, toggle_item, -1);
    chip = (t_chip *) g_ptr_array_index(sd->sensors->chips, gtk_combo_box_active);

    chipfeature = (t_chipfeature *) g_ptr_array_index(chip->chip_features, atoi(path_str));

    /* help = chip->sensorAddress [ gtk_combo_box_active ] [atoi(path_str)];
       chipfeature = (t_chipfeature *) g_ptr_array_index(chip->chip_features, help); */

    chipfeature->show = toggle_item;

    /* clean up */
    gtk_tree_path_free (path);

    /* update tooltip and panel widget */
    sensors_show_panel ((gpointer) sd->sensors);

    TRACE ("leaves list_cell_toggle");
}





static void
add_ui_style_box (GtkWidget * vbox, t_sensors_dialog * sd)
{
    GtkWidget *hbox, *label, *radioText, *radioBars; /* *checkButton,  */

    TRACE ("enters add_ui_style_box");

    hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_widget_show (hbox);

    label = gtk_label_new (_("UI style:"));
    radioText = gtk_radio_button_new_with_mnemonic(NULL, _("_text"));
    radioBars = gtk_radio_button_new_with_mnemonic(
           gtk_radio_button_group(GTK_RADIO_BUTTON(radioText)), _("g_raphical"));

    gtk_widget_show(radioText);
    gtk_widget_show(radioBars);
    gtk_widget_show(label);

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radioText),
                    sd->sensors->display_values_graphically == FALSE);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radioBars),
                    sd->sensors->display_values_graphically == TRUE);

    gtk_box_pack_start(GTK_BOX (hbox), label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX (hbox), radioText, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX (hbox), radioBars, FALSE, FALSE, 0);

    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);

    g_signal_connect (G_OBJECT (radioBars), "toggled",
                      G_CALLBACK (display_style_changed), sd );

    TRACE ("leaves add_ui_style_box");
}


static void
add_labels_box (GtkWidget * vbox, t_sensors_dialog * sd)
{
    GtkWidget *hbox, *checkButton;

    TRACE ("enters add_labels_box");

    hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_widget_show (hbox);
    sd->labels_Box = hbox;
    /* gtk_widget_set_sensitive(hbox, sd->sensors->display_values_graphically); */

    checkButton = gtk_check_button_new_with_mnemonic (
         _("Show _labels in graphical UI"));
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(checkButton),
                                  sd->sensors->show_labels);
    gtk_widget_show (checkButton);

    gtk_box_pack_start (GTK_BOX (hbox), checkButton, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);

    if (sd->sensors->display_values_graphically==FALSE)
        gtk_widget_hide(sd->labels_Box);

    g_signal_connect (G_OBJECT (checkButton), "toggled",
                      G_CALLBACK (show_labels_toggled), sd );

    TRACE ("leaves add_labels_box");
}

static void
add_colored_bars_box (GtkWidget *vbox, t_sensors_dialog *sd)
{
    GtkWidget *hbox, *checkButton;

    TRACE ("enters add_colored_bars_box");

    hbox = gtk_hbox_new (FALSE, BORDER);

    gtk_widget_show (hbox);
    sd->coloredBars_Box = hbox;

    checkButton = gtk_check_button_new_with_mnemonic (
         _("Show colored _bars"));
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(checkButton),
                                  sd->sensors->show_colored_bars);

    gtk_widget_show (checkButton);

    gtk_box_pack_start (GTK_BOX (hbox), checkButton, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);

    if (sd->sensors->display_values_graphically==FALSE)
        gtk_widget_hide(sd->coloredBars_Box);

    g_signal_connect (G_OBJECT (checkButton), "toggled",
                      G_CALLBACK (show_colored_bars_toggled), sd );

    TRACE ("leaves add_colored_bars_box");
}

static void
add_title_box (GtkWidget * vbox, t_sensors_dialog * sd)
{
    GtkWidget *hbox, *checkButton;

    TRACE ("enters add_title_box");

    hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_widget_show (hbox);

    checkButton = gtk_check_button_new_with_mnemonic (_("_Show title"));
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(checkButton),
                                  sd->sensors->show_title);
    gtk_widget_show (checkButton);

    gtk_box_pack_start (GTK_BOX (hbox), checkButton, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);

    g_signal_connect (G_OBJECT (checkButton), "toggled",
                      G_CALLBACK (show_title_toggled), sd );

    TRACE ("leaves add_title_box");
}




static void
add_font_size_box (GtkWidget * vbox, t_sensors_dialog * sd)
{
    GtkWidget *myFontLabel;
    GtkWidget *myFontBox;
    GtkWidget *myFontSizeComboBox;

    TRACE ("enters add_font_size_box");

    myFontLabel = gtk_label_new_with_mnemonic (_("F_ont size:"));
    myFontBox = gtk_hbox_new(FALSE, BORDER);
    myFontSizeComboBox = gtk_combo_box_new_text();

    sd->font_Box = myFontBox;
    /* gtk_widget_set_sensitive(myFontBox, !sd->sensors->display_values_graphically); */

    gtk_combo_box_append_text (GTK_COMBO_BOX(myFontSizeComboBox), _("x-small"));
    gtk_combo_box_append_text (GTK_COMBO_BOX(myFontSizeComboBox), _("small")  );
    gtk_combo_box_append_text (GTK_COMBO_BOX(myFontSizeComboBox), _("medium") );
    gtk_combo_box_append_text (GTK_COMBO_BOX(myFontSizeComboBox), _("large")  );
    gtk_combo_box_append_text (GTK_COMBO_BOX(myFontSizeComboBox), _("x-large"));
    gtk_combo_box_set_active  (GTK_COMBO_BOX(myFontSizeComboBox),
        sd->sensors->font_size_numerical);

    gtk_box_pack_start (GTK_BOX (myFontBox), myFontLabel, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (myFontBox), myFontSizeComboBox, FALSE, FALSE,
        0);
    gtk_box_pack_start (GTK_BOX (vbox), myFontBox, FALSE, FALSE, 0);

    gtk_widget_show (myFontLabel);
    gtk_widget_show (myFontSizeComboBox);
    gtk_widget_show (myFontBox);

    if (sd->sensors->display_values_graphically==TRUE)
        gtk_widget_hide(sd->font_Box);

    g_signal_connect   (G_OBJECT (myFontSizeComboBox), "changed",
                        G_CALLBACK (font_size_change), sd );

    TRACE ("leaves add_font_size_box");
}


static void
add_units_box (GtkWidget * vbox, t_sensors_dialog * sd)
{
    TRACE ("enters add_units_box");

    sd->unit_checkbox = gtk_check_button_new_with_mnemonic(_("Show _Units"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(sd->unit_checkbox), sd->sensors->show_units);

    gtk_widget_show (sd->unit_checkbox);

    gtk_box_pack_start (GTK_BOX (vbox), sd->unit_checkbox, FALSE, TRUE, 0);

    if (sd->sensors->display_values_graphically==TRUE)
        gtk_widget_hide(sd->unit_checkbox);

    g_signal_connect   (G_OBJECT (sd->unit_checkbox), "toggled",
                        G_CALLBACK (draw_units_changed), sd );

    TRACE ("leaves add_units_box");
}

static void
add_smallspacings_box (GtkWidget * vbox, t_sensors_dialog * sd)
{
    TRACE ("enters add_smallspacings_box");

    sd->smallspacing_checkbox  = gtk_check_button_new_with_mnemonic(_("Small horizontal s_pacing"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(sd->smallspacing_checkbox), sd->sensors->show_smallspacings);

    gtk_widget_show (sd->smallspacing_checkbox);

    gtk_box_pack_start (GTK_BOX (vbox), sd->smallspacing_checkbox, FALSE, TRUE, 0);

    if (sd->sensors->display_values_graphically==TRUE)
        gtk_widget_hide(sd->smallspacing_checkbox);

    g_signal_connect   (G_OBJECT (sd->smallspacing_checkbox), "toggled",
                        G_CALLBACK (draw_smallspacings_changed), sd );

    TRACE ("leaves add_smallspacings_box");
}


static void
add_suppressmessage_box (GtkWidget * vbox, t_sensors_dialog * sd)
{
    TRACE ("enters add_suppressmessage_box");

    sd->suppressmessage_checkbox  = gtk_check_button_new_with_mnemonic(_("Suppress messages"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(sd->suppressmessage_checkbox), sd->sensors->suppressmessage);

    gtk_widget_show (sd->suppressmessage_checkbox);

    gtk_box_pack_start (GTK_BOX (vbox), sd->suppressmessage_checkbox, FALSE, TRUE, 0);

    g_signal_connect   (G_OBJECT (sd->suppressmessage_checkbox), "toggled",
                        G_CALLBACK (suppressmessage_changed), sd );

    TRACE ("leaves add_suppressmessage_box");
}


static void
add_update_time_box (GtkWidget * vbox, t_sensors_dialog * sd)
{
    GtkWidget *spinner, *myLabel, *myBox;
    GtkAdjustment *spinner_adj;

    TRACE ("enters add_update_time_box");

    spinner_adj = (GtkAdjustment *) gtk_adjustment_new (
/* TODO: restore original */
        sd->sensors->sensors_refresh_time, 1.0, 990.0, 1.0, 60.0, 60.0);

    /* creates the spinner, with no decimal places */
    spinner = gtk_spin_button_new (spinner_adj, 10.0, 0);

    myLabel = gtk_label_new_with_mnemonic ( _("U_pdate interval (seconds):"));
    gtk_label_set_mnemonic_widget (GTK_LABEL(myLabel), spinner);

    myBox = gtk_hbox_new(FALSE, BORDER);

    gtk_box_pack_start (GTK_BOX (myBox), myLabel, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (myBox), spinner, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), myBox, FALSE, FALSE, 0);

    gtk_widget_show (myLabel);
    gtk_widget_show (spinner);
    gtk_widget_show (myBox);

    g_signal_connect   (G_OBJECT (spinner_adj), "value_changed",
                        G_CALLBACK (adjustment_value_changed), sd );

    TRACE ("leaves add_update_time_box");
}

/* double-click improvement */
static void
add_command_box (GtkWidget * vbox,  t_sensors_dialog * sd)
{
    GtkWidget *myBox;

    TRACE ("enters add_command_box");

    myBox = gtk_hbox_new(FALSE, BORDER);

    sd->myExecCommand_CheckBox = gtk_check_button_new_with_mnemonic
        (_("E_xecute on double click:"));
    gtk_toggle_button_set_active
        ( GTK_TOGGLE_BUTTON (sd->myExecCommand_CheckBox),
        sd->sensors->exec_command );

    sd->myCommandName_Entry = gtk_entry_new ();
    gtk_widget_set_size_request (sd->myCommandName_Entry, 160, 25);


    gtk_entry_set_text( GTK_ENTRY(sd->myCommandName_Entry),
        sd->sensors->command_name );

    gtk_box_pack_start (GTK_BOX (myBox), sd->myExecCommand_CheckBox, FALSE,
                        FALSE, 0);
    gtk_box_pack_start (GTK_BOX (myBox), sd->myCommandName_Entry, FALSE,
                        FALSE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), myBox, FALSE, FALSE, 0);

    gtk_widget_show (sd->myExecCommand_CheckBox);
    gtk_widget_show (sd->myCommandName_Entry);
    gtk_widget_show (myBox);

    g_signal_connect  (G_OBJECT (sd->myExecCommand_CheckBox), "toggled",
                    G_CALLBACK (execCommand_toggled), sd );

    /* g_signal_connect  (G_OBJECT (sd->myCommandNameEntry), "focus-out-event",
                    G_CALLBACK (execCommandName_activate), sd ); */

    TRACE ("leaves add_command_box");
}


static void
add_view_frame (GtkWidget * notebook, t_sensors_dialog * sd)
{
    GtkWidget *_vbox, *_label;

    TRACE ("enters add_view_frame");

    _vbox = gtk_vbox_new (FALSE, 4);
    gtk_container_set_border_width (GTK_CONTAINER(_vbox), 4);
    gtk_widget_show (_vbox);

    _label = gtk_label_new_with_mnemonic(_("_View"));
    gtk_widget_show (_label);

    gtk_container_set_border_width (GTK_CONTAINER (_vbox), BORDER<<1);

    gtk_notebook_append_page (GTK_NOTEBOOK(notebook), _vbox, _label);

    add_title_box (_vbox, sd);

    add_ui_style_box (_vbox, sd);
    add_labels_box (_vbox, sd);
    add_colored_bars_box (_vbox, sd);
    add_font_size_box (_vbox, sd);
    add_units_box (_vbox, sd);
    add_smallspacings_box(_vbox, sd);

    TRACE ("leaves add_view_frame");
}



static void
add_miscellaneous_frame (GtkWidget * notebook, t_sensors_dialog * sd)
{
    GtkWidget *_vbox, *_label;

    TRACE ("enters add_miscellaneous_frame");

    _vbox = gtk_vbox_new (FALSE, 4);
    gtk_container_set_border_width (GTK_CONTAINER(_vbox), 4);
    gtk_widget_show (_vbox);

    _label = gtk_label_new_with_mnemonic (_("_Miscellaneous"));
    gtk_widget_show (_label);

    gtk_container_set_border_width (GTK_CONTAINER (_vbox), BORDER<<1);

    gtk_notebook_append_page (GTK_NOTEBOOK(notebook), _vbox, _label);

    add_update_time_box (_vbox, sd);

    add_suppressmessage_box(_vbox, sd);

    add_command_box (_vbox, sd);

    TRACE ("leaves add_miscellaneous_frame");
}


static void
on_optionsDialog_response (GtkWidget *dlg, int response, t_sensors_dialog *sd)
{
    TRACE ("enters on_optionsDialog_response");

    if (response==GTK_RESPONSE_OK) {
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
    gtk_widget_destroy (sd->dialog);

    xfce_panel_plugin_unblock_menu (sd->sensors->plugin);

    /* g_free(sd->coloredBars_Box);
    g_free(sd->font_Box);
    g_free(sd->labels_Box);
    g_free(sd->myComboBox);
    g_free(sd->myCommandName_Entry);
    g_free(sd->myExecCommand_CheckBox);
    g_free(sd->mySensorLabel);
    g_free(sd->smallspacing_checkbox);
    g_free(sd->suppressmessage_checkbox);
    g_free(sd->temperature_radio_group);
    g_free(sd->unit_checkbox); */

    g_free(sd);

    TRACE ("leaves on_optionsDialog_response");
}


/* create sensor options box */
static void
sensors_create_options (XfcePanelPlugin *plugin, t_sensors *sensors)
{
    GtkWidget *dlg, *vbox, *notebook;
    t_sensors_dialog *sd;
    gchar *myToolTipText;

    TRACE ("enters sensors_create_options");

    xfce_panel_plugin_block_menu (plugin);

    //dlg = gtk_dialog_new_with_buttons (
    dlg = xfce_titled_dialog_new_with_buttons(
                _("Sensors Plugin"),
                GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (plugin))),
                GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_NO_SEPARATOR,
                GTK_STOCK_CLOSE,
                GTK_RESPONSE_OK,
                NULL
            );
    gtk_window_set_icon_name(GTK_WINDOW(dlg),"xfce-sensors");

    gtk_container_set_border_width (GTK_CONTAINER (dlg), 2);

    /*header = xfce_create_header (NULL, _("Sensors Plugin"));
    gtk_widget_set_size_request (GTK_BIN (header)->child, -1, 32);
    gtk_container_set_border_width (GTK_CONTAINER (header), BORDER-2);
    gtk_widget_show (header);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), header,
                        FALSE, TRUE, 0);*/

    vbox = GTK_DIALOG (dlg)->vbox;

    sd = g_new0 (t_sensors_dialog, 1);

    sd->sensors = sensors;
    sd->dialog = dlg;
    sd->plugin_dialog = TRUE;

    sd->myComboBox = gtk_combo_box_new_text();

    init_widgets (sd);

    gtk_combo_box_set_active (GTK_COMBO_BOX(sd->myComboBox), 0);

    notebook = gtk_notebook_new ();

    gtk_box_pack_start (GTK_BOX(vbox), notebook, TRUE, TRUE, 0);
    gtk_widget_show(notebook);

    add_sensors_frame (notebook, sd);

    if (!tooltips)
      tooltips = gtk_tooltips_new();

    /* #if GTK_VERSION < 2.11 */
    myToolTipText = g_strdup(_("You can change a feature's properties such as name, colours, min/max value by double-clicking the entry, editing the content, and pressing \"Return\" or selecting a different field."));
    gtk_tooltips_set_tip (tooltips, GTK_WIDGET(sd->myTreeView),
                          myToolTipText, NULL);
    g_free (myToolTipText);
    /* g_signal_connect (G_OBJECT (sd->myComboBox), "changed",
                      //G_CALLBACK (sensor_entry_changed), sd ); */

    add_view_frame (notebook, sd);
    add_miscellaneous_frame (notebook, sd);

    gtk_widget_set_size_request (vbox, 400, 400);

    g_signal_connect (dlg, "response",
            G_CALLBACK(on_optionsDialog_response), sd);

    gtk_widget_show (dlg);

    TRACE ("leaves sensors_create_options");
}


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
    gtk_widget_show (sensors->eventbox);

    /* double-click improvement */
    sensors->doubleclick_id = g_signal_connect (G_OBJECT(sensors->eventbox),
                                                 "button-press-event",
                                                 G_CALLBACK (execute_command),
                                                 (gpointer) sensors);
}


/**
 * Create sensors panel control
 * @param plugin Panel plugin proxy to create sensors plugin in
 */
static t_sensors *
create_sensors_control (XfcePanelPlugin *plugin)
{
    t_sensors *sensors;
    gchar *tmp;

    TRACE ("enters create_sensors_control");

    tmp = xfce_panel_plugin_lookup_rc_file(plugin);

    sensors = sensors_new (plugin, tmp);
    /* need/want to encapsulate/wrap that ? */
    sensors->orientation = xfce_panel_plugin_get_orientation (plugin);
    sensors->panel_size = xfce_panel_plugin_get_size (plugin);

    add_event_box (sensors);

        /* Add tooltip to show extended current sensors status */
    sensors_create_tooltip ((gpointer) sensors);

    /* fill panel widget with boxes, strings, values, ... */
    create_panel_widget (sensors);

    /* finally add panel "sensors" to eventbox */
    gtk_container_add (GTK_CONTAINER (sensors->eventbox),
                       sensors->widget_sensors);

    /* #if GTK_VERSION >= 2.11
     * g_signal_connect(G_OBJECT(sensors->eventbox),
                                    "query-tooltip",
                                    G_CALLBACK(handle_tooltip_query),
                                    (gpointer) sensors); */

    /* sensors_set_size (control, settings.size); */

    TRACE ("leaves create_sensors_control");

    return sensors;
}


static void
sensors_plugin_construct (XfcePanelPlugin *plugin)
{
    t_sensors *sensors;

    TRACE ("enters sensors_plugin_construct");

    xfce_textdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");

    sensors = create_sensors_control (plugin);

    sensors->plugin_config_file = xfce_panel_plugin_lookup_rc_file(plugin);
    sensors_read_config (plugin, sensors);

    /* Try to resize the sensors to fit the user settings.
       Do also modify the tooltip text. */
    sensors_show_panel ((gpointer) sensors);

    sensors->timeout_id = g_timeout_add (sensors->sensors_refresh_time * 1000,
                                         (GtkFunction) sensors_show_panel,
                                         (gpointer) sensors);

    g_signal_connect (plugin, "free-data", G_CALLBACK (sensors_free), sensors);

    sensors->plugin_config_file = xfce_panel_plugin_save_location (plugin, TRUE);
    g_signal_connect (plugin, "save", G_CALLBACK (sensors_write_config),
                      sensors);

    xfce_panel_plugin_menu_show_configure (plugin);

    g_signal_connect (plugin, "configure-plugin",
                      G_CALLBACK (sensors_create_options), sensors);

    g_signal_connect (plugin, "size-changed", G_CALLBACK (sensors_set_size),
                         sensors);

    g_signal_connect (plugin, "orientation-changed",
                      G_CALLBACK (sensors_set_orientation), sensors);

    gtk_container_add (GTK_CONTAINER(plugin), sensors->eventbox);

    xfce_panel_plugin_add_action_widget (plugin, sensors->eventbox);

    TRACE ("leaves sensors_plugin_construct");
}

XFCE_PANEL_PLUGIN_REGISTER_EXTERNAL (sensors_plugin_construct);
