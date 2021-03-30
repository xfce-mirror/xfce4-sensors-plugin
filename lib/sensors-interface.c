/* File: sensors-interface.c
 *
 * Copyright 2008-2017 Fabian Nowak (timystery@arcor.de)
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#if defined(HAVE_LIBNOTIFY4) || defined(HAVE_LIBNOTIFY7)
#include <libnotify/notify.h>
#endif

#include <glib.h>
#include <gtk/gtk.h>
#include <libxfce4ui/libxfce4ui.h>

/* Package includes */
#include <middlelayer.h>
#include <sensors-interface.h>
#include <sensors-interface-common.h>


#define gtk_hbox_new(homogeneous, spacing) \
        gtk_box_new(GTK_ORIENTATION_HORIZONTAL, spacing)

#define gtk_vbox_new(homogeneous, spacing) \
        gtk_box_new(GTK_ORIENTATION_VERTICAL, spacing)


/* -------------------------------------------------------------------------- */
static void
produce_min_max_values (t_chipfeature *feature, t_tempscale scale, float *minval, float *maxval)
{
  /* assume that min and max values are read from the hddtemp/lmsensors/acpi as
   * degree celsius per default -- very sorry for the non-metric peoples */
   if (feature->class==TEMPERATURE && scale == FAHRENHEIT) {
      *minval = feature->min_value * 9/5 + 32;
      *maxval = feature->max_value * 9/5 + 32;
   } else {
      *minval = feature->min_value;
      *maxval = feature->max_value;
   }
}


/* -------------------------------------------------------------------------- */
void
fill_gtkTreeStore (GtkTreeStore *treestore, t_chip *chip, t_tempscale tempscale, t_sensors_dialog *dialog)
{
    gint idx_chipfeature;
    double feature_value;
    t_chipfeature *feature;
    gboolean *suppressnotifications;
    GtkTreeIter iter_list_store;
#if defined(HAVE_LIBNOTIFY4) || defined(HAVE_LIBNOTIFY7)
    NotifyNotification *notification;
    GError *error = NULL;
    gchar *iconpath;
#endif
    gchar *summary, *body;
    float minval, maxval;


    summary = _("Sensors Plugin Failure");
    body = _("Seems like there was a problem reading a sensor "
                    "feature value.\nProper proceeding cannot be "
                    "guaranteed.");
#if defined(HAVE_LIBNOTIFY4) || defined(HAVE_LIBNOTIFY7)
    iconpath = "xfce-sensors";
#endif

    suppressnotifications = &dialog->sensors->suppressmessage;

    for (idx_chipfeature=0; idx_chipfeature < chip->num_features; idx_chipfeature++)
    {
        feature = (t_chipfeature*) g_ptr_array_index (chip->chip_features, idx_chipfeature);
        g_assert (feature!=NULL);

        if (feature->valid) {
            int result;
            result = sensor_get_value (chip, feature->address, &feature_value, suppressnotifications);
            if (result!=0 && !*suppressnotifications) {

#if defined(HAVE_LIBNOTIFY4) || defined(HAVE_LIBNOTIFY7)
                if (!notify_is_initted())
                    notify_init(PACKAGE); /* NOTIFY_APPNAME */

#ifdef HAVE_LIBNOTIFY7
                notification = notify_notification_new (summary, body, iconpath);
#elif HAVE_LIBNOTIFY4
                notification = notify_notification_new (summary, body, iconpath, NULL);
#endif
                notify_notification_show(notification, &error);
#else
                DBG("%s\n%s", summary, body);
#endif

                /* FIXME: Better popup a window or DBG message or quit plugin. */
                break;
            }
            if (feature->formatted_value != NULL)
                g_free (feature->formatted_value);

            feature->formatted_value = g_new (gchar, 0);
            format_sensor_value (tempscale, feature, feature_value,
                                 &feature->formatted_value);

            produce_min_max_values (feature, tempscale, &minval, &maxval);

            feature->raw_value = feature_value;
            gtk_tree_store_append (treestore, &iter_list_store, NULL);
            gtk_tree_store_set (treestore, &iter_list_store,
                                eTreeColumn_Name, feature->name,
                                eTreeColumn_Value, feature->formatted_value,
                                eTreeColumn_Show, feature->show,
                                eTreeColumn_Color, feature->color,
                                eTreeColumn_Min, minval,
                                eTreeColumn_Max, maxval,
                                -1);
        }
    }
}


/* -------------------------------------------------------------------------- */
void
add_type_box (GtkWidget *vbox, t_sensors_dialog *dialog)
{
    GtkWidget *hbox, *ptr_labelwidget;
    t_chip *chip;
    gint active_entry;

    hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_widget_show (hbox);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

    ptr_labelwidget = gtk_label_new_with_mnemonic (_("Sensors t_ype:"));
    gtk_widget_show (ptr_labelwidget);
    gtk_widget_set_valign(ptr_labelwidget, GTK_ALIGN_CENTER);
    gtk_box_pack_start (GTK_BOX (hbox), ptr_labelwidget, FALSE, FALSE, 0);

    gtk_widget_show (dialog->myComboBox);
    gtk_box_pack_start (GTK_BOX (hbox), dialog->myComboBox, FALSE, FALSE, 0);

    gtk_label_set_mnemonic_widget(GTK_LABEL(ptr_labelwidget), dialog->myComboBox);

    active_entry = gtk_combo_box_get_active(GTK_COMBO_BOX(dialog->myComboBox));

    chip = g_ptr_array_index (dialog->sensors->chips, active_entry);
    DBG("index: %d, chip: %p\n", active_entry, chip);

    hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_widget_show (hbox);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

    ptr_labelwidget = gtk_label_new_with_mnemonic (_("Description:"));
    gtk_widget_show (ptr_labelwidget);
    gtk_widget_set_valign(ptr_labelwidget, GTK_ALIGN_CENTER);
    gtk_box_pack_start (GTK_BOX (hbox), ptr_labelwidget, FALSE, FALSE, 0);

    dialog->mySensorLabel = gtk_label_new (chip->description);

    gtk_widget_show (dialog->mySensorLabel);
    gtk_box_pack_start (GTK_BOX (hbox), dialog->mySensorLabel, FALSE, FALSE, 0);

    g_signal_connect (G_OBJECT (dialog->myComboBox), "changed",
                      G_CALLBACK (sensor_entry_changed), dialog );
}


/* -------------------------------------------------------------------------- */
void
add_update_time_box (GtkWidget *vbox, t_sensors_dialog *dialog)
{
    GtkWidget *label, *hbox;
    GtkAdjustment *spinner_adjustment;

    spinner_adjustment = (GtkAdjustment*) gtk_adjustment_new (
        /* TODO: restore original */
        dialog->sensors->sensors_refresh_time, 1.0, 990.0, 1.0, 60.0, 0.0);

    /* creates the spinner, with no decimal places */
    dialog->spin_button_update_time = gtk_spin_button_new (spinner_adjustment, 10.0, 0);

    label = gtk_label_new_with_mnemonic ( _("U_pdate interval (seconds):"));
    gtk_label_set_mnemonic_widget (GTK_LABEL(label), dialog->spin_button_update_time);

    hbox = gtk_hbox_new(FALSE, BORDER);

    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (hbox), dialog->spin_button_update_time, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

    gtk_widget_show (label);
    gtk_widget_show (dialog->spin_button_update_time);
    gtk_widget_show (hbox);

    g_signal_connect (G_OBJECT (spinner_adjustment), "value_changed",
                      G_CALLBACK (adjustment_value_changed), dialog );
}


/* -------------------------------------------------------------------------- */
void
add_sensor_settings_box (GtkWidget *vbox, t_sensors_dialog *dialog)
{
    GtkTreeViewColumn *tree_view_column;
    GtkCellRenderer *text_cell_renderer, *toggle_cell_renderer;
    GtkWidget *scrolled_window;
    gint active_entry;

    active_entry = gtk_combo_box_get_active (GTK_COMBO_BOX(dialog->myComboBox));

    dialog->myTreeView = gtk_tree_view_new_with_model
        (GTK_TREE_MODEL (dialog->myListStore[active_entry]));

    text_cell_renderer = gtk_cell_renderer_text_new ();
    g_object_set ((gpointer*) text_cell_renderer, "editable", TRUE, NULL);

    tree_view_column = gtk_tree_view_column_new_with_attributes (_("Name"),
                        text_cell_renderer, "text", eTreeColumn_Name, NULL);

    g_signal_connect (G_OBJECT (text_cell_renderer), "edited",
                      G_CALLBACK (list_cell_text_edited), dialog);

    gtk_tree_view_column_set_expand (tree_view_column, TRUE);
    gtk_tree_view_append_column (GTK_TREE_VIEW (dialog->myTreeView),
                                 GTK_TREE_VIEW_COLUMN (tree_view_column));

    text_cell_renderer = gtk_cell_renderer_text_new ();
    tree_view_column = gtk_tree_view_column_new_with_attributes (_("Value"),
                        text_cell_renderer, "text", eTreeColumn_Value, NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (dialog->myTreeView),
                                 GTK_TREE_VIEW_COLUMN (tree_view_column));

    toggle_cell_renderer = gtk_cell_renderer_toggle_new();
    tree_view_column = gtk_tree_view_column_new_with_attributes (_("Show"),
                        toggle_cell_renderer, "active", eTreeColumn_Show, NULL);
    g_signal_connect (G_OBJECT (toggle_cell_renderer), "toggled",
                      G_CALLBACK (list_cell_toggle), dialog );
    gtk_tree_view_append_column (GTK_TREE_VIEW (dialog->myTreeView),
                                 GTK_TREE_VIEW_COLUMN (tree_view_column));

    text_cell_renderer = gtk_cell_renderer_text_new ();
    g_object_set ((gpointer*) text_cell_renderer, "editable", TRUE, NULL);
    tree_view_column = gtk_tree_view_column_new_with_attributes (_("Color"),
                        text_cell_renderer, "text", eTreeColumn_Color, NULL);
    g_signal_connect (G_OBJECT (text_cell_renderer), "edited",
                      G_CALLBACK (list_cell_color_edited), dialog);
    gtk_tree_view_append_column (GTK_TREE_VIEW (dialog->myTreeView),
                                 GTK_TREE_VIEW_COLUMN (tree_view_column));

    text_cell_renderer = gtk_cell_renderer_text_new ();

    g_object_set ((gpointer*) text_cell_renderer, "editable", TRUE, NULL);
    tree_view_column = gtk_tree_view_column_new_with_attributes
                (_("Min"), text_cell_renderer, "text", eTreeColumn_Min, NULL);
    g_signal_connect (G_OBJECT (text_cell_renderer), "edited",
                      G_CALLBACK (minimum_changed), dialog);

    gtk_tree_view_append_column (GTK_TREE_VIEW (dialog->myTreeView),
                                 GTK_TREE_VIEW_COLUMN (tree_view_column));

    text_cell_renderer = gtk_cell_renderer_text_new ();

    g_object_set ((gpointer*) text_cell_renderer, "editable", TRUE, NULL);
    tree_view_column = gtk_tree_view_column_new_with_attributes
                (_("Max"), text_cell_renderer, "text", eTreeColumn_Max, NULL);
    g_signal_connect (G_OBJECT (text_cell_renderer), "edited",
                      G_CALLBACK (maximum_changed), dialog);

    gtk_tree_view_append_column (GTK_TREE_VIEW (dialog->myTreeView),
                                 GTK_TREE_VIEW_COLUMN (tree_view_column));

    scrolled_window = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (
        GTK_SCROLLED_WINDOW (scrolled_window), GTK_POLICY_AUTOMATIC,
        GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window),
                                         GTK_SHADOW_ETCHED_IN);
    gtk_container_set_border_width (GTK_CONTAINER (scrolled_window), 0);
    gtk_container_add (GTK_CONTAINER (scrolled_window), dialog->myTreeView);

    gtk_box_pack_start (GTK_BOX (vbox), scrolled_window, TRUE, TRUE, BORDER);

    gtk_widget_show (dialog->myTreeView);
    gtk_widget_show (scrolled_window);
}


/* -------------------------------------------------------------------------- */
void
add_temperature_unit_box (GtkWidget *vbox, t_sensors_dialog *dialog)
{
    GtkWidget *hbox, *label, *button_celsius, *button_fahrenheit;

    hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_widget_show (hbox);

    label = gtk_label_new ( _("Temperature scale:"));
    button_celsius = gtk_radio_button_new_with_mnemonic (NULL, _("_Celsius"));
    button_fahrenheit = gtk_radio_button_new_with_mnemonic(
      gtk_radio_button_get_group (GTK_RADIO_BUTTON (button_celsius)), _("_Fahrenheit"));

    gtk_widget_show (button_celsius);
    gtk_widget_show (button_fahrenheit);
    gtk_widget_show (label);

    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(button_celsius),
                                  dialog->sensors->scale == CELSIUS);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(button_fahrenheit),
                                  dialog->sensors->scale == FAHRENHEIT);

    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (hbox), button_celsius, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (hbox), button_fahrenheit, FALSE, FALSE, 0);

    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);

    g_signal_connect (G_OBJECT (button_celsius), "toggled",
                      G_CALLBACK (temperature_unit_change), dialog );
}


/* -------------------------------------------------------------------------- */
void
add_sensors_frame (GtkWidget *notebook, t_sensors_dialog * dialog)
{
    GtkWidget *vbox, *label;

    vbox = gtk_vbox_new (FALSE, OUTER_BORDER);
    gtk_container_set_border_width (GTK_CONTAINER(vbox), OUTER_BORDER);
    gtk_widget_show (vbox);

    label = gtk_label_new_with_mnemonic (_("_Sensors"));
    gtk_widget_show (label);

    gtk_container_set_border_width (GTK_CONTAINER (vbox), 2*BORDER);

    gtk_notebook_append_page (GTK_NOTEBOOK (notebook), vbox, label);

    add_type_box (vbox, dialog);

    add_sensor_settings_box (vbox, dialog);

    add_temperature_unit_box (vbox, dialog);
}


/* -------------------------------------------------------------------------- */
void
free_widgets (t_sensors_dialog *dialog)
{
    gint idx_chip;

    g_return_if_fail (dialog != NULL);

    for (idx_chip=0; idx_chip < dialog->sensors->num_sensorchips; idx_chip++)
    {
        GtkTreeIter iter_list_store;
        if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (dialog->myListStore[idx_chip]), &iter_list_store))
        {
            while (gtk_tree_store_remove ( GTK_TREE_STORE (dialog->myListStore[idx_chip]), &iter_list_store))
                            ;;
        }
        gtk_tree_store_clear (dialog->myListStore[idx_chip]);
        g_object_unref (dialog->myListStore[idx_chip]);
    }

    g_return_if_fail (dialog != NULL);
    g_return_if_fail (dialog->sensors != NULL);

    /* free structures and arrays */
    g_ptr_array_foreach (dialog->sensors->chips, free_chip, NULL);

    /* stop association to libsensors and others*/
    cleanup_interfaces ();

    g_ptr_array_free (dialog->sensors->chips, TRUE);

    g_free (dialog->sensors->plugin_config_file);
    dialog->sensors->plugin_config_file = NULL;
    g_free (dialog->sensors->command_name);
    dialog->sensors->command_name = NULL;

    g_free (dialog->sensors->str_fontsize);
    dialog->sensors->str_fontsize = NULL;
}


/* -------------------------------------------------------------------------- */
void
init_widgets (t_sensors_dialog *dialog)
{
    gint idx_chip;
    t_chip *chip;
    t_chipfeature *feature;
    GtkTreeIter iter;
    t_sensors *sensors;
    GtkTreeStore *treemodel;

    g_return_if_fail (dialog != NULL);

    sensors = dialog->sensors;

    for (idx_chip=0; idx_chip < sensors->num_sensorchips; idx_chip++) {
        dialog->myListStore[idx_chip] = gtk_tree_store_new (6, G_TYPE_STRING,
                        G_TYPE_STRING, G_TYPE_BOOLEAN, G_TYPE_STRING,
                        G_TYPE_FLOAT, G_TYPE_FLOAT);

        chip = (t_chip*) g_ptr_array_index (sensors->chips, idx_chip);
        gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (dialog->myComboBox),
                                        chip->sensorId );
        treemodel = GTK_TREE_STORE (dialog->myListStore[idx_chip]);

        fill_gtkTreeStore (treemodel, chip, sensors->scale,  dialog);
    }

    if (sensors->num_sensorchips == 0) {
        chip = (t_chip*) g_ptr_array_index (sensors->chips, 0);
        g_assert (chip!=NULL);
        gtk_combo_box_text_append_text ( GTK_COMBO_BOX_TEXT(dialog->myComboBox),
                                chip->sensorId );

        dialog->myListStore[0] = gtk_tree_store_new (6, G_TYPE_STRING,
                                                     G_TYPE_STRING, G_TYPE_BOOLEAN,
                                                     G_TYPE_STRING, G_TYPE_DOUBLE,
                                                     G_TYPE_DOUBLE);
        feature = (t_chipfeature*) g_ptr_array_index (chip->chip_features, 0);
        g_assert (feature!=NULL);

        feature->formatted_value = g_strdup ("0.0");
        feature->raw_value = 0.0;

        gtk_tree_store_append (GTK_TREE_STORE (dialog->myListStore[0]), &iter, NULL);
        gtk_tree_store_set (GTK_TREE_STORE (dialog->myListStore[0]),
                            &iter,
                            eTreeColumn_Name, feature->name,
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
reload_listbox (t_sensors_dialog *dialog)
{
    gint idx_chip;
    t_chip *chip;
    GtkTreeStore *tree_store;
    t_sensors *sensors;

    g_return_if_fail (dialog != NULL);

    sensors = dialog->sensors;

    for (idx_chip=0; idx_chip < sensors->num_sensorchips; idx_chip++) {
        chip = (t_chip*) g_ptr_array_index (sensors->chips, idx_chip);

        tree_store = dialog->myListStore[idx_chip];
        g_assert (tree_store != NULL);
        gtk_tree_store_clear (tree_store);

        fill_gtkTreeStore (tree_store, chip, sensors->scale, dialog);
    }
}
