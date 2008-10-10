/*  Copyright 2008 Fabian Nowak (timystery@arcor.de)
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

/* Definitions */

/* Global includes */
/* #include <stdlib.h> */

/* Glib/Gtk includes */
#include <glib.h>
#include <gtk/gtk.h>

/* Xfce includes */
#include <libxfcegui4/libxfcegui4.h>

/* Package includes */
#include <sensors-interface-common.h>
#include <sensors-interface.h>
#include <middlelayer.h>


void
fill_gtkTreeStore (GtkTreeStore *model, t_chip *chip, t_tempscale scale)
{
    int featureindex, res;
    double sensorFeature;
    t_chipfeature *chipfeature;
    GtkTreeIter *iter;

    TRACE ("enters fill_gtkTreeStore");

    for (featureindex=0; featureindex < chip->num_features; featureindex++)
    {
        chipfeature = (t_chipfeature *) g_ptr_array_index (chip->chip_features, featureindex);
        g_assert (chipfeature!=NULL);

        iter = g_new0 (GtkTreeIter, 1);

        if ( chipfeature->valid == TRUE ) {
            res = sensor_get_value
                    (chip, chipfeature->address, &sensorFeature);
            if ( res!=0) {
                DBG ( _("Xfce Hardware Sensors Plugin:\n"
                    "Seems like there was a problem reading a sensor "
                    "feature value.\nProper proceeding cannot be "
                    "guaranteed.\n") );
                /* FIXME: Better popup a window or DBG message or quit plugin. */
                break;
            }
            g_free (chipfeature->formatted_value);
            chipfeature->formatted_value = g_new (gchar, 0);
            format_sensor_value (scale, chipfeature, sensorFeature,
                                 &(chipfeature->formatted_value));
            chipfeature->raw_value = sensorFeature;
            gtk_tree_store_append (model, iter, NULL);
            gtk_tree_store_set ( model, iter,
                                 0, chipfeature->name,
                                1, chipfeature->formatted_value,
                                2, chipfeature->show,
                                3, chipfeature->color,
                                4, chipfeature->min_value,
                                5, chipfeature->max_value,
                                 -1);
        } /* end if sensors-valid */
        /* g_free(iter); ??? */
    }

    TRACE ("leaves fill_gtkTreeStore");
}



void
add_type_box (GtkWidget * vbox, t_sensors_dialog * sd)
{
    GtkWidget *hbox, *label;
    t_chip *chip;
    gint gtk_combo_box_active;

    TRACE ("enters add_type_box");

    hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_widget_show (hbox);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

    label = gtk_label_new_with_mnemonic (_("Sensors t_ype:"));
    gtk_widget_show (label);
    gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

    gtk_widget_show (sd->myComboBox);
    gtk_box_pack_start (GTK_BOX (hbox), sd->myComboBox, FALSE, FALSE, 0);

    gtk_label_set_mnemonic_widget(GTK_LABEL(label), sd->myComboBox);

    gtk_combo_box_active =
        gtk_combo_box_get_active(GTK_COMBO_BOX(sd->myComboBox));

    chip = g_ptr_array_index (sd->sensors->chips, gtk_combo_box_active);

    /* if (sd->sensors->num_sensorchips > 0)
        sd->mySensorLabel = gtk_label_new
            ( sensors_get_adapter_name_wrapper
                ( chip->chip_name->bus) );
    else */

    hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_widget_show (hbox);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

    label = gtk_label_new_with_mnemonic (_("Description:"));
    gtk_widget_show (label);
    gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

    sd->mySensorLabel =
            gtk_label_new (chip->description);

    gtk_widget_show (sd->mySensorLabel);
    gtk_box_pack_start (GTK_BOX (hbox), sd->mySensorLabel, FALSE, FALSE, 0);

    g_signal_connect (G_OBJECT (sd->myComboBox), "changed",
                      G_CALLBACK (sensor_entry_changed), sd );

    TRACE ("leaves add_type_box");
}


void
add_sensor_settings_box ( GtkWidget * vbox, t_sensors_dialog * sd)
{
    GtkTreeViewColumn *aTreeViewColumn;
    GtkCellRenderer *myCellRendererText, *myCellRendererToggle;
    GtkWidget *myScrolledWindow;
    gint gtk_combo_box_active;

    TRACE ("enters add_sensor_settings_box");

    gtk_combo_box_active =
        gtk_combo_box_get_active(GTK_COMBO_BOX(sd->myComboBox));

    sd->myTreeView = gtk_tree_view_new_with_model
        ( GTK_TREE_MODEL ( sd->myListStore[ gtk_combo_box_active ] ) );

    myCellRendererText = gtk_cell_renderer_text_new ();
    g_object_set ( (gpointer*) myCellRendererText, "editable", TRUE, NULL );

    aTreeViewColumn = gtk_tree_view_column_new_with_attributes (_("Name"),
                        myCellRendererText, "text", 0, NULL);
    g_signal_connect    (G_OBJECT (myCellRendererText), "edited",
                        G_CALLBACK (list_cell_text_edited), sd);
    gtk_tree_view_column_set_expand (aTreeViewColumn, TRUE);
    gtk_tree_view_append_column (GTK_TREE_VIEW (sd->myTreeView),
                        GTK_TREE_VIEW_COLUMN (aTreeViewColumn));

    myCellRendererText = gtk_cell_renderer_text_new ();
    aTreeViewColumn = gtk_tree_view_column_new_with_attributes (_("Value"),
                        myCellRendererText, "text", 1, NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (sd->myTreeView),
                        GTK_TREE_VIEW_COLUMN (aTreeViewColumn));

    myCellRendererToggle = gtk_cell_renderer_toggle_new();
    aTreeViewColumn = gtk_tree_view_column_new_with_attributes (_("Show"),
                        myCellRendererToggle, "active", 2, NULL);
    g_signal_connect    (G_OBJECT (myCellRendererToggle), "toggled",
                        G_CALLBACK (list_cell_toggle), sd );
    gtk_tree_view_append_column (GTK_TREE_VIEW (sd->myTreeView),
                        GTK_TREE_VIEW_COLUMN (aTreeViewColumn));

    myCellRendererText = gtk_cell_renderer_text_new ();
    g_object_set ( (gpointer*) myCellRendererText, "editable", TRUE, NULL );
    aTreeViewColumn = gtk_tree_view_column_new_with_attributes (_("Color"),
                        myCellRendererText, "text", 3, NULL);
    g_signal_connect    (G_OBJECT (myCellRendererText), "edited",
                        G_CALLBACK (list_cell_color_edited), sd);
    gtk_tree_view_append_column (GTK_TREE_VIEW (sd->myTreeView),
                        GTK_TREE_VIEW_COLUMN (aTreeViewColumn));

    myCellRendererText = gtk_cell_renderer_text_new ();
    g_object_set ( (gpointer*) myCellRendererText, "editable", TRUE, NULL );
    aTreeViewColumn = gtk_tree_view_column_new_with_attributes
                    (_("Min"), myCellRendererText, "text", 4, NULL);
    g_signal_connect(G_OBJECT(myCellRendererText), "edited",
                        G_CALLBACK(minimum_changed), sd);
    gtk_tree_view_append_column(GTK_TREE_VIEW(sd->myTreeView),
                        GTK_TREE_VIEW_COLUMN(aTreeViewColumn));

    myCellRendererText = gtk_cell_renderer_text_new ();
    g_object_set ( (gpointer*) myCellRendererText, "editable", TRUE, NULL );
    aTreeViewColumn = gtk_tree_view_column_new_with_attributes
                    (_("Max"), myCellRendererText, "text", 5, NULL);
    g_signal_connect(G_OBJECT(myCellRendererText), "edited",
                        G_CALLBACK(maximum_changed), sd);
    gtk_tree_view_append_column(GTK_TREE_VIEW(sd->myTreeView),
                        GTK_TREE_VIEW_COLUMN(aTreeViewColumn));

    myScrolledWindow = gtk_scrolled_window_new (NULL, NULL);
        gtk_scrolled_window_set_policy (
            GTK_SCROLLED_WINDOW (myScrolledWindow), GTK_POLICY_AUTOMATIC,
            GTK_POLICY_AUTOMATIC);
        gtk_container_set_border_width (GTK_CONTAINER (myScrolledWindow), 0);
        /* gtk_scrolled_window_add_with_viewport (
            GTK_SCROLLED_WINDOW (myScrolledWindow), sd->myTreeView); */
        gtk_container_add (GTK_CONTAINER (myScrolledWindow), sd->myTreeView);

    gtk_box_pack_start (GTK_BOX (vbox), myScrolledWindow, TRUE, TRUE, BORDER);

    gtk_widget_show (sd->myTreeView);
    gtk_widget_show (myScrolledWindow);

    TRACE ("leaves add_sensor_settings_box");
}


void
add_temperature_unit_box (GtkWidget *vbox, t_sensors_dialog *sd)
{
    GtkWidget *hbox, *label, *radioCelsius, *radioFahrenheit;

    TRACE ("enters add_temperature_unit_box");

    hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_widget_show (hbox);

    label = gtk_label_new ( _("Temperature scale:"));
    radioCelsius = gtk_radio_button_new_with_mnemonic (NULL,
                                                              _("_Celsius"));
    radioFahrenheit = gtk_radio_button_new_with_mnemonic(
      gtk_radio_button_get_group(GTK_RADIO_BUTTON(radioCelsius)), _("_Fahrenheit"));

    gtk_widget_show(radioCelsius);
    gtk_widget_show(radioFahrenheit);
    gtk_widget_show(label);

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radioCelsius),
                    sd->sensors->scale == CELSIUS);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radioFahrenheit),
                    sd->sensors->scale == FAHRENHEIT);

    gtk_box_pack_start(GTK_BOX (hbox), label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX (hbox), radioCelsius, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX (hbox), radioFahrenheit, FALSE, FALSE, 0);

    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);

    g_signal_connect (G_OBJECT (radioCelsius), "toggled",
                      G_CALLBACK (temperature_unit_change), sd );

    TRACE ("leaves add_temperature_unit_box");
}



void
add_sensors_frame (GtkWidget * notebook, t_sensors_dialog * sd)
{
    GtkWidget *_vbox, *_label;

    TRACE ("enters add_sensors_frame");

    _vbox = gtk_vbox_new (FALSE, 4);
    gtk_container_set_border_width (GTK_CONTAINER(_vbox), 4);
    gtk_widget_show (_vbox);

    _label = gtk_label_new_with_mnemonic(_("_Sensors"));
    gtk_widget_show (_label);

    gtk_container_set_border_width (GTK_CONTAINER (_vbox), BORDER<<1);

    gtk_notebook_append_page (GTK_NOTEBOOK(notebook), _vbox, _label);

    add_type_box (_vbox, sd);

    add_sensor_settings_box (_vbox, sd);

    add_temperature_unit_box (_vbox, sd);

    TRACE ("leaves add_sensors_frame");
}


void
init_widgets (t_sensors_dialog *sd)
{
    int chipindex;
    t_chip *chip;
    t_chipfeature *chipfeature;
    GtkTreeIter *iter;
    t_sensors *sensors;
    GtkTreeStore *model;

    TRACE ("enters init_widgets");

    sensors = sd->sensors;

    for (chipindex=0; chipindex < sensors->num_sensorchips; chipindex++) {
        sd->myListStore[chipindex] = gtk_tree_store_new (6, G_TYPE_STRING,
                        G_TYPE_STRING, G_TYPE_BOOLEAN, G_TYPE_STRING,
                        G_TYPE_FLOAT, G_TYPE_FLOAT);

        chip = (t_chip *) g_ptr_array_index (sensors->chips, chipindex);
        gtk_combo_box_append_text ( GTK_COMBO_BOX(sd->myComboBox),
                                    chip->sensorId );
        model = GTK_TREE_STORE (sd->myListStore[chipindex]);

        fill_gtkTreeStore (model, chip, sensors->scale);
    }

    if(sd->sensors->num_sensorchips == 0) {
        chip = (t_chip *) g_ptr_array_index(sensors->chips, 0);
        g_assert (chip!=NULL);
        gtk_combo_box_append_text ( GTK_COMBO_BOX(sd->myComboBox),
                                chip->sensorId );
        sd->myListStore[0] = gtk_tree_store_new (6, G_TYPE_STRING,
                                                G_TYPE_STRING, G_TYPE_BOOLEAN,
                                                G_TYPE_STRING, G_TYPE_DOUBLE,
                                                G_TYPE_DOUBLE);
        chipfeature = (t_chipfeature *) g_ptr_array_index (chip->chip_features, 0);
        g_assert (chipfeature!=NULL);

        g_free(chipfeature->formatted_value);
        chipfeature->formatted_value = g_strdup ("0.0");
        chipfeature->raw_value = 0.0;

        iter = g_new0 (GtkTreeIter, 1);
        gtk_tree_store_append ( GTK_TREE_STORE (sd->myListStore[0]),
                            iter, NULL);
        gtk_tree_store_set ( GTK_TREE_STORE (sd->myListStore[0]),
                            iter,
                            0, chipfeature->name,
                            1, "0.0",        /* chipfeature->formatted_value */
                            2, False,        /* chipfeature->show */
                            3, "#000000",    /* chipfeature->color */
                            3, "#000000",    /* chipfeature->color */
                            4, 0.0,            /* chipfeature->min_value */
                            5, 0.0,            /* chipfeature->max_value */
                            -1);
    }
    TRACE ("leaves init_widgets");
}
