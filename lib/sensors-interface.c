/* File sensors-interface.c
 *
 *   Copyright 2008-2017 Fabian Nowak (timystery@arcor.de)
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
#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

/* Global includes */
#if defined(HAVE_LIBNOTIFY4) || defined(HAVE_LIBNOTIFY7)
#include <libnotify/notify.h>
#endif
/* #include <stdlib.h> */

/* Glib/Gtk includes */
#include <glib.h>
#include <gtk/gtk.h>

/* Xfce includes */
#include <libxfce4ui/libxfce4ui.h>

/* Package includes */
#include <sensors-interface-common.h>
#include <sensors-interface.h>
#include <middlelayer.h>


#define gtk_hbox_new(homogeneous, spacing) \
        gtk_box_new(GTK_ORIENTATION_HORIZONTAL, spacing)

#define gtk_vbox_new(homogeneous, spacing) \
        gtk_box_new(GTK_ORIENTATION_VERTICAL, spacing)


/* forward declaration to not make gcc 4.3 -Wall complain */
void produce_min_max_values (t_chipfeature *chipfeature, t_tempscale scale, float *minval, float *maxval);


/* -------------------------------------------------------------------------- */
void
produce_min_max_values (t_chipfeature *chipfeature, t_tempscale scale, float *minval, float *maxval)
{
  /* assume that min and max values are read from the hddtemp/lmsensors/acpi as
   * degree celsius per default -- very sorry for the non-metric peoples */
   if (chipfeature->class==TEMPERATURE && scale == FAHRENHEIT) {
      *minval = chipfeature->min_value * 9/5 + 32;
      *maxval = chipfeature->max_value * 9/5 + 32;
   } else {
      *minval = chipfeature->min_value;
      *maxval = chipfeature->max_value;
   }
}


/* -------------------------------------------------------------------------- */
void
fill_gtkTreeStore (GtkTreeStore *model, t_chip *chip, t_tempscale scale, t_sensors_dialog *sd)
{
    int featureindex, res;
    double sensorFeature;
    t_chipfeature *chipfeature;
    gboolean *suppress;
    GtkTreeIter iter;
#if defined(HAVE_LIBNOTIFY4) || defined(HAVE_LIBNOTIFY7)
    NotifyNotification *nn;
    GError *error = NULL;
    gchar *icon;
#endif
    gchar *summary, *body;
    float minval, maxval;


    summary = _("Sensors Plugin Failure");
    body = _("Seems like there was a problem reading a sensor "
                    "feature value.\nProper proceeding cannot be "
                    "guaranteed.");
#if defined(HAVE_LIBNOTIFY4) || defined(HAVE_LIBNOTIFY7)
    icon = "xfce-sensors";
#endif

    TRACE ("enters fill_gtkTreeStore");

    suppress = &(sd->sensors->suppressmessage);

    for (featureindex=0; featureindex < chip->num_features; featureindex++)
    {
        chipfeature = (t_chipfeature *) g_ptr_array_index (chip->chip_features, featureindex);
        g_assert (chipfeature!=NULL);

        if ( chipfeature->valid == TRUE ) {
            res = sensor_get_value
                    (chip, chipfeature->address, &sensorFeature, suppress);
            if ( res!=0 && !suppress) {

#if defined(HAVE_LIBNOTIFY4) || defined(HAVE_LIBNOTIFY7)
                if (!notify_is_initted())
                    notify_init(PACKAGE); /* NOTIFY_APPNAME */

#ifdef HAVE_LIBNOTIFY7
                nn = notify_notification_new (summary, body, icon);
#elif HAVE_LIBNOTIFY4
                nn = notify_notification_new (summary, body, icon, NULL);
#endif
                notify_notification_show(nn, &error);
#else
                DBG("%s\n%s", summary, body);
#endif

                /* FIXME: Better popup a window or DBG message or quit plugin. */
                break;
            }
            if (chipfeature->formatted_value != NULL)
                g_free (chipfeature->formatted_value);

            chipfeature->formatted_value = g_new (gchar, 0);
            format_sensor_value (scale, chipfeature, sensorFeature,
                                 &(chipfeature->formatted_value));

            produce_min_max_values (chipfeature, scale, &minval, &maxval);

            chipfeature->raw_value = sensorFeature;
            gtk_tree_store_append (model, &iter, NULL);
            //if (sd->plugin_dialog)
                gtk_tree_store_set ( model, &iter,
                                 0, chipfeature->name,
                                1, chipfeature->formatted_value,
                                2, chipfeature->show,
                                3, chipfeature->color,
                                4, minval,
                                5, maxval,
                                 -1);
            //else
                //gtk_tree_store_set ( model, iter,
                                 //0, chipfeature->name,
                                //1, chipfeature->formatted_value,
                                //2, minval,
                                //3, maxval,
                                 //-1);
        } /* end if sensors-valid */
    }

    TRACE ("leaves fill_gtkTreeStore");
}


/* -------------------------------------------------------------------------- */
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
    gtk_widget_set_valign(label, GTK_ALIGN_CENTER);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

    gtk_widget_show (sd->myComboBox);
    gtk_box_pack_start (GTK_BOX (hbox), sd->myComboBox, FALSE, FALSE, 0);

    gtk_label_set_mnemonic_widget(GTK_LABEL(label), sd->myComboBox);

    gtk_combo_box_active =
        gtk_combo_box_get_active(GTK_COMBO_BOX(sd->myComboBox));

    chip = g_ptr_array_index (sd->sensors->chips, gtk_combo_box_active);
        DBG("index: %d, chip: %p\n", gtk_combo_box_active, chip);

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
    gtk_widget_set_valign(label, GTK_ALIGN_CENTER);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

    sd->mySensorLabel = gtk_label_new (chip->description);

    gtk_widget_show (sd->mySensorLabel);
    gtk_box_pack_start (GTK_BOX (hbox), sd->mySensorLabel, FALSE, FALSE, 0);

    g_signal_connect (G_OBJECT (sd->myComboBox), "changed",
                      G_CALLBACK (sensor_entry_changed), sd );

    TRACE ("leaves add_type_box");
}


/* -------------------------------------------------------------------------- */
void
add_update_time_box (GtkWidget * vbox, t_sensors_dialog * sd)
{
    GtkWidget *myLabel, *myBox;
    GtkAdjustment *spinner_adj;

    TRACE ("enters add_update_time_box");

    spinner_adj = (GtkAdjustment *) gtk_adjustment_new (
/* TODO: restore original */
        sd->sensors->sensors_refresh_time, 1.0, 990.0, 1.0, 60.0, 0.0);

    /* creates the spinner, with no decimal places */
    sd->spin_button_update_time = gtk_spin_button_new (spinner_adj, 10.0, 0);

    myLabel = gtk_label_new_with_mnemonic ( _("U_pdate interval (seconds):"));
    gtk_label_set_mnemonic_widget (GTK_LABEL(myLabel), sd->spin_button_update_time);

    myBox = gtk_hbox_new(FALSE, BORDER);

    gtk_box_pack_start (GTK_BOX (myBox), myLabel, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (myBox), sd->spin_button_update_time, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), myBox, FALSE, FALSE, 0);

    gtk_widget_show (myLabel);
    gtk_widget_show (sd->spin_button_update_time);
    gtk_widget_show (myBox);

    g_signal_connect   (G_OBJECT (spinner_adj), "value_changed",
                        G_CALLBACK (adjustment_value_changed), sd );

    TRACE ("leaves add_update_time_box");
}


/* -------------------------------------------------------------------------- */
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
    //if (sd->plugin_dialog)
        g_object_set ( (gpointer*) myCellRendererText, "editable", TRUE, NULL );

    aTreeViewColumn = gtk_tree_view_column_new_with_attributes (_("Name"),
                        myCellRendererText, "text", 0, NULL);

    //if (sd->plugin_dialog)
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

    //if (sd->plugin_dialog)
    //{
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
    //}

    myCellRendererText = gtk_cell_renderer_text_new ();
    //if (sd->plugin_dialog)
    //{
        g_object_set ( (gpointer*) myCellRendererText, "editable", TRUE, NULL );
        aTreeViewColumn = gtk_tree_view_column_new_with_attributes
                    (_("Min"), myCellRendererText, "text", 4, NULL);
        g_signal_connect(G_OBJECT(myCellRendererText), "edited",
                        G_CALLBACK(minimum_changed), sd);
    //}
    //else
        //aTreeViewColumn = gtk_tree_view_column_new_with_attributes
                    //(_("Min"), myCellRendererText, "text", 2, NULL);


    gtk_tree_view_append_column(GTK_TREE_VIEW(sd->myTreeView),
                        GTK_TREE_VIEW_COLUMN(aTreeViewColumn));

    myCellRendererText = gtk_cell_renderer_text_new ();
    //if (sd->plugin_dialog)
    //{
        g_object_set ( (gpointer*) myCellRendererText, "editable", TRUE, NULL );
        aTreeViewColumn = gtk_tree_view_column_new_with_attributes
                    (_("Max"), myCellRendererText, "text", 5, NULL);
        g_signal_connect(G_OBJECT(myCellRendererText), "edited",
                        G_CALLBACK(maximum_changed), sd);
    //}
    //else
        //aTreeViewColumn = gtk_tree_view_column_new_with_attributes
                    //(_("Max"), myCellRendererText, "text", 3, NULL);

    gtk_tree_view_append_column(GTK_TREE_VIEW(sd->myTreeView),
                        GTK_TREE_VIEW_COLUMN(aTreeViewColumn));

    myScrolledWindow = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (
        GTK_SCROLLED_WINDOW (myScrolledWindow), GTK_POLICY_AUTOMATIC,
        GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (myScrolledWindow),
                                       GTK_SHADOW_ETCHED_IN);
    gtk_container_set_border_width (GTK_CONTAINER (myScrolledWindow), 0);
    /* gtk_scrolled_window_add_with_viewport (
        GTK_SCROLLED_WINDOW (myScrolledWindow), sd->myTreeView); */
    gtk_container_add (GTK_CONTAINER (myScrolledWindow), sd->myTreeView);

    gtk_box_pack_start (GTK_BOX (vbox), myScrolledWindow, TRUE, TRUE, BORDER);

    gtk_widget_show (sd->myTreeView);
    gtk_widget_show (myScrolledWindow);

    TRACE ("leaves add_sensor_settings_box");
}


/* -------------------------------------------------------------------------- */
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


/* -------------------------------------------------------------------------- */
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


/* -------------------------------------------------------------------------- */
void
free_widgets (t_sensors_dialog *ptr_sensors_dialog)
{
    int idx_chip;
    TRACE ("enters free_widgets");

    // produces gtk warning messages when closing the program:
    //(xfce4-sensors:1471): Gtk-CRITICAL **: IA__gtk_widget_get_realized: assertion 'GTK_IS_WIDGET (widget)' failed
    //(xfce4-sensors:1471): GLib-GObject-WARNING **: instance with invalid (NULL) class pointer
    //(xfce4-sensors:1471): GLib-GObject-CRITICAL **: g_signal_emit_valist: assertion 'G_TYPE_CHECK_INSTANCE (instance)' failed
    //g_object_unref (ptr_sensors_dialog->spin_button_update_time);

    // TODO: unref or free: g_free(gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(ptr_sensors_dialog->spin_button_update_time)));

    for (idx_chip=0; idx_chip < ptr_sensors_dialog->sensors->num_sensorchips; idx_chip++)
    {
        GtkTreeIter iter_list_store;
        if ( gtk_tree_model_get_iter_first(GTK_TREE_MODEL (ptr_sensors_dialog->myListStore[idx_chip]), &iter_list_store) )
        {
            while ( gtk_tree_store_remove ( GTK_TREE_STORE (ptr_sensors_dialog->myListStore[idx_chip]), &iter_list_store) )
                            ;;
        }
        gtk_tree_store_clear(ptr_sensors_dialog->myListStore[idx_chip]);
        g_object_unref(ptr_sensors_dialog->myListStore[idx_chip]);
    }

    // makes plugin/program crash
    //g_ptr_array_free(ptr_sensors_dialog->sensors->chips, TRUE); /* TODO: Use middlelayer function */

    g_return_if_fail (ptr_sensors_dialog != NULL);

    g_return_if_fail (ptr_sensors_dialog->sensors != NULL);

    /* remove timeout functions */
    //remove_gsource (ptr_sensors_dialog->sensors->timeout_id);

    /* double-click improvement */
    //remove_gsource (ptr_sensors_dialog->sensors->doubleclick_id);

    /* free structures and arrays */
    g_ptr_array_foreach (ptr_sensors_dialog->sensors->chips, free_chip, NULL);

    /* stop association to libsensors and others*/
    cleanup_interfaces();

    g_ptr_array_free (ptr_sensors_dialog->sensors->chips, TRUE);

    g_free (ptr_sensors_dialog->sensors->plugin_config_file);
    ptr_sensors_dialog->sensors->plugin_config_file = NULL;
    g_free (ptr_sensors_dialog->sensors->command_name);
    ptr_sensors_dialog->sensors->command_name = NULL;

    g_free (ptr_sensors_dialog->sensors->str_fontsize);
    ptr_sensors_dialog->sensors->str_fontsize = NULL;
    //g_object_unref(ptr_sensors_dialog->sensors->widget_sensors);
    //gtk_widget_destroy (ptr_sensors_dialog->sensors->widget_sensors);
    //ptr_sensors_dialog->sensors->widget_sensors = NULL;
    //if (ptr_sensors_dialog->sensors->widget_sensors)
        //g_free(ptr_sensors_dialog->sensors->widget_sensors);

    //g_free (ptr_sensors_dialog->sensors);
    //ptr_sensors_dialog->sensors = NULL;

    TRACE ("leaves free_widgets");
}


/* -------------------------------------------------------------------------- */
void
init_widgets (t_sensors_dialog *sd)
{
    int idx_chip;
    t_chip *ptr_chip_structure;
    t_chipfeature *chipfeature;
    GtkTreeIter iter;
    t_sensors *sensors;
    GtkTreeStore *model;

    TRACE ("enters init_widgets");

    g_return_if_fail(sd != NULL);

    sensors = sd->sensors;

    for (idx_chip=0; idx_chip < sensors->num_sensorchips; idx_chip++) {
        sd->myListStore[idx_chip] = gtk_tree_store_new (6, G_TYPE_STRING,
                        G_TYPE_STRING, G_TYPE_BOOLEAN, G_TYPE_STRING,
                        G_TYPE_FLOAT, G_TYPE_FLOAT);

        ptr_chip_structure = (t_chip *) g_ptr_array_index (sensors->chips, idx_chip);
        gtk_combo_box_text_append_text ( GTK_COMBO_BOX_TEXT(sd->myComboBox),
                                    ptr_chip_structure->sensorId );
        model = GTK_TREE_STORE (sd->myListStore[idx_chip]);

        fill_gtkTreeStore (model, ptr_chip_structure, sensors->scale,  sd);
    }

    if(sd->sensors->num_sensorchips == 0) {
        ptr_chip_structure = (t_chip *) g_ptr_array_index(sensors->chips, 0);
        g_assert (ptr_chip_structure!=NULL);
        gtk_combo_box_text_append_text ( GTK_COMBO_BOX_TEXT(sd->myComboBox),
                                ptr_chip_structure->sensorId );

        sd->myListStore[0] = gtk_tree_store_new (6, G_TYPE_STRING,
                                                G_TYPE_STRING, G_TYPE_BOOLEAN,
                                                G_TYPE_STRING, G_TYPE_DOUBLE,
                                                G_TYPE_DOUBLE);
        chipfeature = (t_chipfeature *) g_ptr_array_index (ptr_chip_structure->chip_features, 0);
        g_assert (chipfeature!=NULL);

        g_free(chipfeature->formatted_value);
        chipfeature->formatted_value = g_strdup ("0.0");
        chipfeature->raw_value = 0.0;

        gtk_tree_store_append ( GTK_TREE_STORE (sd->myListStore[0]),
                            &iter, NULL);
        //if (sd->plugin_dialog)
        gtk_tree_store_set ( GTK_TREE_STORE (sd->myListStore[0]),
                            &iter,
                            0, chipfeature->name,
                            1, "0.0",        /* chipfeature->formatted_value */
                            2, FALSE,        /* chipfeature->show */
                            3, "#000000",    /* chipfeature->color */
                            4, 0.0,            /* chipfeature->min_value */
                            5, 0.0,            /* chipfeature->max_value */
                            -1);
        //else
            //gtk_tree_store_set ( GTK_TREE_STORE (sd->myListStore[0]),
                            //iter,
                            //0, chipfeature->name,
                            //1, "0.0",        /* chipfeature->formatted_value */
                            //2, 0.0,            /* chipfeature->min_value */
                            //3, 0.0,            /* chipfeature->max_value */
                            //-1);
    }
    TRACE ("leaves init_widgets");
}


/* -------------------------------------------------------------------------- */
void
reload_listbox (t_sensors_dialog *sd)
{
    int idx_chip;
    t_chip *ptr_chip_structure;
    GtkTreeStore *ptr_tree_store;
    t_sensors *ptr_sensors_structure;

    TRACE ("enters reload_listbox");

    g_return_if_fail(sd != NULL);

    ptr_sensors_structure = sd->sensors;

    for (idx_chip=0; idx_chip < ptr_sensors_structure->num_sensorchips; idx_chip++) {
        ptr_chip_structure = (t_chip *) g_ptr_array_index (ptr_sensors_structure->chips, idx_chip);

        ptr_tree_store = sd->myListStore[idx_chip];
        g_assert(ptr_tree_store != NULL);
        gtk_tree_store_clear (ptr_tree_store);

        fill_gtkTreeStore (ptr_tree_store, ptr_chip_structure, ptr_sensors_structure->scale, sd);

    }
    TRACE ("leaves reload_listbox");
}
