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


/* forward declarations */
void produce_min_max_values (t_chipfeature *ptr_chipfeature, t_tempscale scale, float *minval, float *maxval);


/* -------------------------------------------------------------------------- */
void
produce_min_max_values (t_chipfeature *ptr_chipfeature, t_tempscale scale, float *minval, float *maxval)
{
  /* assume that min and max values are read from the hddtemp/lmsensors/acpi as
   * degree celsius per default -- very sorry for the non-metric peoples */
   if (ptr_chipfeature->class==TEMPERATURE && scale == FAHRENHEIT) {
      *minval = ptr_chipfeature->min_value * 9/5 + 32;
      *maxval = ptr_chipfeature->max_value * 9/5 + 32;
   } else {
      *minval = ptr_chipfeature->min_value;
      *maxval = ptr_chipfeature->max_value;
   }
}


/* -------------------------------------------------------------------------- */
void
fill_gtkTreeStore (GtkTreeStore *ptr_treestore, t_chip *ptr_chip, t_tempscale tempscale, t_sensors_dialog *ptr_sensorsdialog)
{
    int idx_chipfeature;
    double val_sensorfeature;
    t_chipfeature *ptr_chipfeature;
    gboolean *ptr_suppressnotifications;
    GtkTreeIter iter_list_store;
#if defined(HAVE_LIBNOTIFY4) || defined(HAVE_LIBNOTIFY7)
    NotifyNotification *ptr_notifynotification;
    GError *ptr_error = NULL;
    gchar *ptr_iconpath;
#endif
    gchar *str_summary, *str_body;
    float val_minimum, val_maximum;


    str_summary = _("Sensors Plugin Failure");
    str_body = _("Seems like there was a problem reading a sensor "
                    "feature value.\nProper proceeding cannot be "
                    "guaranteed.");
#if defined(HAVE_LIBNOTIFY4) || defined(HAVE_LIBNOTIFY7)
    ptr_iconpath = "xfce-sensors";
#endif

    ptr_suppressnotifications = &(ptr_sensorsdialog->sensors->suppressmessage);

    for (idx_chipfeature=0; idx_chipfeature < ptr_chip->num_features; idx_chipfeature++)
    {
        ptr_chipfeature = (t_chipfeature *) g_ptr_array_index (ptr_chip->chip_features, idx_chipfeature);
        g_assert (ptr_chipfeature!=NULL);

        if ( ptr_chipfeature->valid == TRUE ) {
            int result;
            result = sensor_get_value
                    (ptr_chip, ptr_chipfeature->address, &val_sensorfeature, ptr_suppressnotifications);
            if (result!=0 && !*ptr_suppressnotifications) {

#if defined(HAVE_LIBNOTIFY4) || defined(HAVE_LIBNOTIFY7)
                if (!notify_is_initted())
                    notify_init(PACKAGE); /* NOTIFY_APPNAME */

#ifdef HAVE_LIBNOTIFY7
                ptr_notifynotification = notify_notification_new (str_summary, str_body, ptr_iconpath);
#elif HAVE_LIBNOTIFY4
                ptr_notifynotification = notify_notification_new (str_summary, str_body, ptr_iconpath, NULL);
#endif
                notify_notification_show(ptr_notifynotification, &ptr_error);
#else
                DBG("%s\n%s", str_summary, str_body);
#endif

                /* FIXME: Better popup a window or DBG message or quit plugin. */
                break;
            }
            if (ptr_chipfeature->formatted_value != NULL)
                g_free (ptr_chipfeature->formatted_value);

            ptr_chipfeature->formatted_value = g_new (gchar, 0);
            format_sensor_value (tempscale, ptr_chipfeature, val_sensorfeature,
                                 &(ptr_chipfeature->formatted_value));

            produce_min_max_values (ptr_chipfeature, tempscale, &val_minimum, &val_maximum);

            ptr_chipfeature->raw_value = val_sensorfeature;
            gtk_tree_store_append (ptr_treestore, &iter_list_store, NULL);
            gtk_tree_store_set (ptr_treestore, &iter_list_store,
                                eTreeColumn_Name, ptr_chipfeature->name,
                                eTreeColumn_Value, ptr_chipfeature->formatted_value,
                                eTreeColumn_Show, ptr_chipfeature->show,
                                eTreeColumn_Color, ptr_chipfeature->color,
                                eTreeColumn_Min, val_minimum,
                                eTreeColumn_Max, val_maximum,
                                -1);
        } /* end if sensors-valid */
    }
}


/* -------------------------------------------------------------------------- */
void
add_type_box (GtkWidget *ptr_widget_vbox, t_sensors_dialog *ptr_sensorsdialog)
{
    GtkWidget *ptr_hbox, *ptr_labelwidget;
    t_chip *ptr_chip;
    gint idx_activecomboboxentry;

    ptr_hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_widget_show (ptr_hbox);
    gtk_box_pack_start (GTK_BOX (ptr_widget_vbox), ptr_hbox, FALSE, FALSE, 0);

    ptr_labelwidget = gtk_label_new_with_mnemonic (_("Sensors t_ype:"));
    gtk_widget_show (ptr_labelwidget);
    gtk_widget_set_valign(ptr_labelwidget, GTK_ALIGN_CENTER);
    gtk_box_pack_start (GTK_BOX (ptr_hbox), ptr_labelwidget, FALSE, FALSE, 0);

    gtk_widget_show (ptr_sensorsdialog->myComboBox);
    gtk_box_pack_start (GTK_BOX (ptr_hbox), ptr_sensorsdialog->myComboBox, FALSE, FALSE, 0);

    gtk_label_set_mnemonic_widget(GTK_LABEL(ptr_labelwidget), ptr_sensorsdialog->myComboBox);

    idx_activecomboboxentry =
        gtk_combo_box_get_active(GTK_COMBO_BOX(ptr_sensorsdialog->myComboBox));

    ptr_chip = g_ptr_array_index (ptr_sensorsdialog->sensors->chips, idx_activecomboboxentry);
    DBG("index: %d, chip: %p\n", idx_activecomboboxentry, ptr_chip);

    ptr_hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_widget_show (ptr_hbox);
    gtk_box_pack_start (GTK_BOX (ptr_widget_vbox), ptr_hbox, FALSE, FALSE, 0);

    ptr_labelwidget = gtk_label_new_with_mnemonic (_("Description:"));
    gtk_widget_show (ptr_labelwidget);
    gtk_widget_set_valign(ptr_labelwidget, GTK_ALIGN_CENTER);
    gtk_box_pack_start (GTK_BOX (ptr_hbox), ptr_labelwidget, FALSE, FALSE, 0);

    ptr_sensorsdialog->mySensorLabel = gtk_label_new (ptr_chip->description);

    gtk_widget_show (ptr_sensorsdialog->mySensorLabel);
    gtk_box_pack_start (GTK_BOX (ptr_hbox), ptr_sensorsdialog->mySensorLabel, FALSE, FALSE, 0);

    g_signal_connect (G_OBJECT (ptr_sensorsdialog->myComboBox), "changed",
                      G_CALLBACK (sensor_entry_changed), ptr_sensorsdialog );
}


/* -------------------------------------------------------------------------- */
void
add_update_time_box (GtkWidget *ptr_widget_vbox, t_sensors_dialog *ptr_sensorsdialog)
{
    GtkWidget *ptr_labelwidget, *ptr_hbox;
    GtkAdjustment *ptr_spinneradjustment;

    ptr_spinneradjustment = (GtkAdjustment *) gtk_adjustment_new (
        /* TODO: restore original */
        ptr_sensorsdialog->sensors->sensors_refresh_time, 1.0, 990.0, 1.0, 60.0, 0.0);

    /* creates the spinner, with no decimal places */
    ptr_sensorsdialog->spin_button_update_time = gtk_spin_button_new (ptr_spinneradjustment, 10.0, 0);

    ptr_labelwidget = gtk_label_new_with_mnemonic ( _("U_pdate interval (seconds):"));
    gtk_label_set_mnemonic_widget (GTK_LABEL(ptr_labelwidget), ptr_sensorsdialog->spin_button_update_time);

    ptr_hbox = gtk_hbox_new(FALSE, BORDER);

    gtk_box_pack_start (GTK_BOX (ptr_hbox), ptr_labelwidget, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (ptr_hbox), ptr_sensorsdialog->spin_button_update_time, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (ptr_widget_vbox), ptr_hbox, FALSE, FALSE, 0);

    gtk_widget_show (ptr_labelwidget);
    gtk_widget_show (ptr_sensorsdialog->spin_button_update_time);
    gtk_widget_show (ptr_hbox);

    g_signal_connect   (G_OBJECT (ptr_spinneradjustment), "value_changed",
                        G_CALLBACK (adjustment_value_changed), ptr_sensorsdialog );
}


/* -------------------------------------------------------------------------- */
void
add_sensor_settings_box (GtkWidget *ptr_widget_vbox, t_sensors_dialog *ptr_sensorsdialog)
{
    GtkTreeViewColumn *ptr_treeviewcolumn;
    GtkCellRenderer *ptr_textcellrenderer, *ptr_togglecellrenderer;
    GtkWidget *ptr_scrolledwindow;
    gint idx_activecomboboxentry;

    idx_activecomboboxentry =
        gtk_combo_box_get_active(GTK_COMBO_BOX(ptr_sensorsdialog->myComboBox));

    ptr_sensorsdialog->myTreeView = gtk_tree_view_new_with_model
        ( GTK_TREE_MODEL ( ptr_sensorsdialog->myListStore[ idx_activecomboboxentry ] ) );

    ptr_textcellrenderer = gtk_cell_renderer_text_new ();
    g_object_set ( (gpointer*) ptr_textcellrenderer, "editable", TRUE, NULL );

    ptr_treeviewcolumn = gtk_tree_view_column_new_with_attributes (_("Name"),
                        ptr_textcellrenderer, "text", eTreeColumn_Name, NULL);

    g_signal_connect    (G_OBJECT (ptr_textcellrenderer), "edited",
                        G_CALLBACK (list_cell_text_edited), ptr_sensorsdialog);

    gtk_tree_view_column_set_expand (ptr_treeviewcolumn, TRUE);
    gtk_tree_view_append_column (GTK_TREE_VIEW (ptr_sensorsdialog->myTreeView),
                        GTK_TREE_VIEW_COLUMN (ptr_treeviewcolumn));

    ptr_textcellrenderer = gtk_cell_renderer_text_new ();
    ptr_treeviewcolumn = gtk_tree_view_column_new_with_attributes (_("Value"),
                        ptr_textcellrenderer, "text", eTreeColumn_Value, NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (ptr_sensorsdialog->myTreeView),
                        GTK_TREE_VIEW_COLUMN (ptr_treeviewcolumn));

    ptr_togglecellrenderer = gtk_cell_renderer_toggle_new();
    ptr_treeviewcolumn = gtk_tree_view_column_new_with_attributes (_("Show"),
                        ptr_togglecellrenderer, "active", eTreeColumn_Show, NULL);
    g_signal_connect    (G_OBJECT (ptr_togglecellrenderer), "toggled",
                        G_CALLBACK (list_cell_toggle), ptr_sensorsdialog );
    gtk_tree_view_append_column (GTK_TREE_VIEW (ptr_sensorsdialog->myTreeView),
                        GTK_TREE_VIEW_COLUMN (ptr_treeviewcolumn));

    ptr_textcellrenderer = gtk_cell_renderer_text_new ();
    g_object_set ( (gpointer*) ptr_textcellrenderer, "editable", TRUE, NULL );
    ptr_treeviewcolumn = gtk_tree_view_column_new_with_attributes (_("Color"),
                        ptr_textcellrenderer, "text", eTreeColumn_Color, NULL);
    g_signal_connect    (G_OBJECT (ptr_textcellrenderer), "edited",
                        G_CALLBACK (list_cell_color_edited), ptr_sensorsdialog);
    gtk_tree_view_append_column (GTK_TREE_VIEW (ptr_sensorsdialog->myTreeView),
                        GTK_TREE_VIEW_COLUMN (ptr_treeviewcolumn));

    ptr_textcellrenderer = gtk_cell_renderer_text_new ();

    g_object_set ( (gpointer*) ptr_textcellrenderer, "editable", TRUE, NULL );
    ptr_treeviewcolumn = gtk_tree_view_column_new_with_attributes
                (_("Min"), ptr_textcellrenderer, "text", eTreeColumn_Min, NULL);
    g_signal_connect(G_OBJECT(ptr_textcellrenderer), "edited",
                    G_CALLBACK(minimum_changed), ptr_sensorsdialog);

    gtk_tree_view_append_column(GTK_TREE_VIEW(ptr_sensorsdialog->myTreeView),
                        GTK_TREE_VIEW_COLUMN(ptr_treeviewcolumn));

    ptr_textcellrenderer = gtk_cell_renderer_text_new ();

    g_object_set ( (gpointer*) ptr_textcellrenderer, "editable", TRUE, NULL );
    ptr_treeviewcolumn = gtk_tree_view_column_new_with_attributes
                (_("Max"), ptr_textcellrenderer, "text", eTreeColumn_Max, NULL);
    g_signal_connect(G_OBJECT(ptr_textcellrenderer), "edited",
                    G_CALLBACK(maximum_changed), ptr_sensorsdialog);

    gtk_tree_view_append_column(GTK_TREE_VIEW(ptr_sensorsdialog->myTreeView),
                        GTK_TREE_VIEW_COLUMN(ptr_treeviewcolumn));

    ptr_scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (
        GTK_SCROLLED_WINDOW (ptr_scrolledwindow), GTK_POLICY_AUTOMATIC,
        GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (ptr_scrolledwindow),
                                       GTK_SHADOW_ETCHED_IN);
    gtk_container_set_border_width (GTK_CONTAINER (ptr_scrolledwindow), 0);
    gtk_container_add (GTK_CONTAINER (ptr_scrolledwindow), ptr_sensorsdialog->myTreeView);

    gtk_box_pack_start (GTK_BOX (ptr_widget_vbox), ptr_scrolledwindow, TRUE, TRUE, BORDER);

    gtk_widget_show (ptr_sensorsdialog->myTreeView);
    gtk_widget_show (ptr_scrolledwindow);
}


/* -------------------------------------------------------------------------- */
void
add_temperature_unit_box (GtkWidget *ptr_widget_vbox, t_sensors_dialog *ptr_sensorsdialog)
{
    GtkWidget *ptr_hbox, *ptr_labelwidget, *ptr_radiobutton_celsius, *ptr_radiobutton_fahrenheit;

    ptr_hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_widget_show (ptr_hbox);

    ptr_labelwidget = gtk_label_new ( _("Temperature scale:"));
    ptr_radiobutton_celsius = gtk_radio_button_new_with_mnemonic (NULL,
                                                              _("_Celsius"));
    ptr_radiobutton_fahrenheit = gtk_radio_button_new_with_mnemonic(
      gtk_radio_button_get_group(GTK_RADIO_BUTTON(ptr_radiobutton_celsius)), _("_Fahrenheit"));

    gtk_widget_show(ptr_radiobutton_celsius);
    gtk_widget_show(ptr_radiobutton_fahrenheit);
    gtk_widget_show(ptr_labelwidget);

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ptr_radiobutton_celsius),
                    ptr_sensorsdialog->sensors->scale == CELSIUS);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ptr_radiobutton_fahrenheit),
                    ptr_sensorsdialog->sensors->scale == FAHRENHEIT);

    gtk_box_pack_start(GTK_BOX (ptr_hbox), ptr_labelwidget, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX (ptr_hbox), ptr_radiobutton_celsius, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX (ptr_hbox), ptr_radiobutton_fahrenheit, FALSE, FALSE, 0);

    gtk_box_pack_start (GTK_BOX (ptr_widget_vbox), ptr_hbox, FALSE, TRUE, 0);

    g_signal_connect (G_OBJECT (ptr_radiobutton_celsius), "toggled",
                      G_CALLBACK (temperature_unit_change), ptr_sensorsdialog );
}


/* -------------------------------------------------------------------------- */
void
add_sensors_frame (GtkWidget *ptr_widget_notebook, t_sensors_dialog * ptr_sensorsdialog)
{
    GtkWidget *ptr_vbox, *ptr_label;

    ptr_vbox = gtk_vbox_new (FALSE, OUTER_BORDER);
    gtk_container_set_border_width (GTK_CONTAINER(ptr_vbox), OUTER_BORDER);
    gtk_widget_show (ptr_vbox);

    ptr_label = gtk_label_new_with_mnemonic(_("_Sensors"));
    gtk_widget_show (ptr_label);

    gtk_container_set_border_width (GTK_CONTAINER (ptr_vbox), BORDER<<1);

    gtk_notebook_append_page (GTK_NOTEBOOK(ptr_widget_notebook), ptr_vbox, ptr_label);

    add_type_box (ptr_vbox, ptr_sensorsdialog);

    add_sensor_settings_box (ptr_vbox, ptr_sensorsdialog);

    add_temperature_unit_box (ptr_vbox, ptr_sensorsdialog);
}


/* -------------------------------------------------------------------------- */
void
free_widgets (t_sensors_dialog *ptr_sensors_dialog)
{
    int idx_chip;

    g_return_if_fail(ptr_sensors_dialog != NULL);

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

    g_return_if_fail (ptr_sensors_dialog != NULL);

    g_return_if_fail (ptr_sensors_dialog->sensors != NULL);

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
}


/* -------------------------------------------------------------------------- */
void
init_widgets (t_sensors_dialog *ptr_sensorsdialog)
{
    int idx_chip;
    t_chip *ptr_chip_structure;
    t_chipfeature *ptr_chipfeature;
    GtkTreeIter iter;
    t_sensors *ptr_sensors;
    GtkTreeStore *ptr_treemodel;

    g_return_if_fail(ptr_sensorsdialog != NULL);

    ptr_sensors = ptr_sensorsdialog->sensors;

    for (idx_chip=0; idx_chip < ptr_sensors->num_sensorchips; idx_chip++) {
        ptr_sensorsdialog->myListStore[idx_chip] = gtk_tree_store_new (6, G_TYPE_STRING,
                        G_TYPE_STRING, G_TYPE_BOOLEAN, G_TYPE_STRING,
                        G_TYPE_FLOAT, G_TYPE_FLOAT);

        ptr_chip_structure = (t_chip *) g_ptr_array_index (ptr_sensors->chips, idx_chip);
        gtk_combo_box_text_append_text ( GTK_COMBO_BOX_TEXT(ptr_sensorsdialog->myComboBox),
                                    ptr_chip_structure->sensorId );
        ptr_treemodel = GTK_TREE_STORE (ptr_sensorsdialog->myListStore[idx_chip]);

        fill_gtkTreeStore (ptr_treemodel, ptr_chip_structure, ptr_sensors->scale,  ptr_sensorsdialog);
    }

    if (ptr_sensors->num_sensorchips == 0) {
        ptr_chip_structure = (t_chip *) g_ptr_array_index(ptr_sensors->chips, 0);
        g_assert (ptr_chip_structure!=NULL);
        gtk_combo_box_text_append_text ( GTK_COMBO_BOX_TEXT(ptr_sensorsdialog->myComboBox),
                                ptr_chip_structure->sensorId );

        ptr_sensorsdialog->myListStore[0] = gtk_tree_store_new (6, G_TYPE_STRING,
                                                G_TYPE_STRING, G_TYPE_BOOLEAN,
                                                G_TYPE_STRING, G_TYPE_DOUBLE,
                                                G_TYPE_DOUBLE);
        ptr_chipfeature = (t_chipfeature *) g_ptr_array_index (ptr_chip_structure->chip_features, 0);
        g_assert (ptr_chipfeature!=NULL);

        ptr_chipfeature->formatted_value = g_strdup ("0.0");
        ptr_chipfeature->raw_value = 0.0;

        gtk_tree_store_append ( GTK_TREE_STORE (ptr_sensorsdialog->myListStore[0]),
                            &iter, NULL);
        gtk_tree_store_set ( GTK_TREE_STORE (ptr_sensorsdialog->myListStore[0]),
                            &iter,
                            eTreeColumn_Name, ptr_chipfeature->name,
                            eTreeColumn_Value, "0.0",        /* chipfeature->formatted_value */
                            eTreeColumn_Show, FALSE,         /* chipfeature->show */
                            eTreeColumn_Color, "#000000",    /* chipfeature->color */
                            eTreeColumn_Min, 0.0,            /* chipfeature->min_value */
                            eTreeColumn_Max, 0.0,            /* chipfeature->max_value */
                            -1);
    }
}


/* -------------------------------------------------------------------------- */
void
reload_listbox (t_sensors_dialog *ptr_sensorsdialog)
{
    int idx_chip;
    t_chip *ptr_chip_structure;
    GtkTreeStore *ptr_tree_store;
    t_sensors *ptr_sensors_structure;

    g_return_if_fail(ptr_sensorsdialog != NULL);

    ptr_sensors_structure = ptr_sensorsdialog->sensors;

    for (idx_chip=0; idx_chip < ptr_sensors_structure->num_sensorchips; idx_chip++) {
        ptr_chip_structure = (t_chip *) g_ptr_array_index (ptr_sensors_structure->chips, idx_chip);

        ptr_tree_store = ptr_sensorsdialog->myListStore[idx_chip];
        g_assert(ptr_tree_store != NULL);
        gtk_tree_store_clear (ptr_tree_store);

        fill_gtkTreeStore (ptr_tree_store, ptr_chip_structure, ptr_sensors_structure->scale, ptr_sensorsdialog);

    }
}
