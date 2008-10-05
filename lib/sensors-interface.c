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
#include <glib/gtree.h>
#include <gtk/gtk.h>

/* Xfce includes */
#include <libxfcegui4/libxfcegui4.h>

/* Package includes */
#include <sensors-interface-common.h>
#include <sensors-interface.h>


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
