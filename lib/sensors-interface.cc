/* sensors-interface.cc
 * Part of xfce4-sensors-plugin
 *
 * Copyright (c) 2008-2017 Fabian Nowak <timystery@arcor.de>
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

/* The fixes file has to be included before any other #include directives */
#include "xfce4++/util/fixes.h"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_LIBNOTIFY
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
produce_min_max_values (const Ptr<t_chipfeature> &feature, t_tempscale scale, float *minval, float *maxval)
{
  /* assume that min and max values are read from the hddtemp/lmsensors/acpi as
   * degree celsius per default -- very sorry for the non-metric peoples */
   if (feature->cls == TEMPERATURE && scale == FAHRENHEIT) {
      *minval = feature->min_value * 9/5 + 32;
      *maxval = feature->max_value * 9/5 + 32;
   } else {
      *minval = feature->min_value;
      *maxval = feature->max_value;
   }
}


/* -------------------------------------------------------------------------- */
void
fill_gtkTreeStore (GtkTreeStore *treestore, const Ptr<t_chip> &chip, t_tempscale tempscale, const Ptr<t_sensors_dialog> &dialog)
{
    for (auto feature : chip->chip_features)
    {
        if (feature->valid)
        {
            bool *suppressnotifications = &dialog->sensors->suppressmessage;
            Optional<double> feature_value = sensor_get_value (chip, feature->address, suppressnotifications);
            if (feature_value.has_value())
            {
                feature->formatted_value = format_sensor_value (tempscale, feature, feature_value.value());

                float minval, maxval;
                produce_min_max_values (feature, tempscale, &minval, &maxval);

                feature->raw_value = feature_value.value();

                GtkTreeIter iter_list_store;
                gtk_tree_store_append (treestore, &iter_list_store, NULL);
                gtk_tree_store_set (treestore, &iter_list_store,
                                    eTreeColumn_Name, feature->name.c_str(),
                                    eTreeColumn_Value, feature->formatted_value.c_str(),
                                    eTreeColumn_Show, feature->show,
                                    eTreeColumn_Color, !feature->color_orEmpty.empty() ? feature->color_orEmpty.c_str() : "",
                                    eTreeColumn_Min, minval,
                                    eTreeColumn_Max, maxval,
                                    -1);
            }
            else
            {
                if (!*suppressnotifications)
                {
                    const gchar *summary = _("Sensors Plugin Failure");
                    const gchar *body = _("Seems like there was a problem reading a sensor "
                                          "feature value.\nProper proceeding cannot be "
                                          "guaranteed.");

    #ifdef HAVE_LIBNOTIFY
                    if (!notify_is_initted())
                        notify_init(PACKAGE); /* NOTIFY_APPNAME */

                    const gchar *iconpath = "xfce-sensors";
                    NotifyNotification *notification = notify_notification_new (summary, body, iconpath);
                    GError *error = NULL;
                    notify_notification_show(notification, &error);
    #else
                    g_warning ("%s\n%s", summary, body);
    #endif

                    /* FIXME: Better popup a window or DBG message or quit plugin. */
                }
                break;
            }
        }
    }
}


/* -------------------------------------------------------------------------- */
void
add_type_box (GtkWidget *vbox, const Ptr<t_sensors_dialog> &dialog)
{
    GtkWidget *hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_widget_show (hbox);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

    GtkWidget *ptr_labelwidget = gtk_label_new_with_mnemonic (_("Sensors t_ype:"));
    gtk_widget_show (ptr_labelwidget);
    gtk_widget_set_valign(ptr_labelwidget, GTK_ALIGN_CENTER);
    gtk_box_pack_start (GTK_BOX (hbox), ptr_labelwidget, FALSE, FALSE, 0);

    gtk_widget_show (dialog->myComboBox);
    gtk_box_pack_start (GTK_BOX (hbox), dialog->myComboBox, FALSE, FALSE, 0);

    gtk_label_set_mnemonic_widget(GTK_LABEL(ptr_labelwidget), dialog->myComboBox);

    gint active_entry = gtk_combo_box_get_active(GTK_COMBO_BOX(dialog->myComboBox));

    auto chip = dialog->sensors->chips[active_entry];

    hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_widget_show (hbox);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

    ptr_labelwidget = gtk_label_new_with_mnemonic (_("Description:"));
    gtk_widget_show (ptr_labelwidget);
    gtk_widget_set_valign(ptr_labelwidget, GTK_ALIGN_CENTER);
    gtk_box_pack_start (GTK_BOX (hbox), ptr_labelwidget, FALSE, FALSE, 0);

    dialog->mySensorLabel = gtk_label_new (chip->description.c_str());

    gtk_widget_show (dialog->mySensorLabel);
    gtk_box_pack_start (GTK_BOX (hbox), dialog->mySensorLabel, FALSE, FALSE, 0);

    xfce4::connect_changed (GTK_COMBO_BOX (dialog->myComboBox), [dialog](GtkComboBox *widget) {
        sensor_entry_changed (GTK_WIDGET (widget), dialog);
    });
}


/* -------------------------------------------------------------------------- */
void
add_update_time_box (GtkWidget *vbox, const Ptr<t_sensors_dialog> &dialog)
{
    GtkAdjustment *spinner_adjustment = gtk_adjustment_new (
        /* TODO: restore original */
        dialog->sensors->sensors_refresh_time, 1.0, 990.0, 1.0, 60.0, 0.0);

    /* creates the spinner, with no decimal places */
    dialog->spin_button_update_time = gtk_spin_button_new (spinner_adjustment, 10.0, 0);

    GtkWidget *label = gtk_label_new_with_mnemonic ( _("U_pdate interval (seconds):"));
    gtk_label_set_mnemonic_widget (GTK_LABEL(label), dialog->spin_button_update_time);

    GtkWidget *hbox = gtk_hbox_new(FALSE, BORDER);

    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (hbox), dialog->spin_button_update_time, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

    gtk_widget_show (label);
    gtk_widget_show (dialog->spin_button_update_time);
    gtk_widget_show (hbox);

    xfce4::connect_value_changed (spinner_adjustment, [dialog](GtkAdjustment *adj) {
        adjustment_value_changed (adj, dialog);
    });
}


/* -------------------------------------------------------------------------- */
void
add_sensor_settings_box (GtkWidget *vbox, const Ptr<t_sensors_dialog> &dialog)
{
    GtkTreeViewColumn *tree_view_column;
    GtkCellRenderer *text_cell_renderer, *toggle_cell_renderer;

    gint active_entry = gtk_combo_box_get_active (GTK_COMBO_BOX(dialog->myComboBox));

    dialog->myTreeView = gtk_tree_view_new_with_model
        (GTK_TREE_MODEL (dialog->myListStore[active_entry]));

    text_cell_renderer = gtk_cell_renderer_text_new ();
    g_object_set ((gpointer*) text_cell_renderer, "editable", TRUE, NULL);

    tree_view_column = gtk_tree_view_column_new_with_attributes (_("Name"),
                        text_cell_renderer, "text", eTreeColumn_Name, NULL);

    xfce4::connect_edited (GTK_CELL_RENDERER_TEXT (text_cell_renderer),
        [dialog](GtkCellRendererText *r, gchar *path, gchar *new_text) {
            list_cell_text_edited (r, path, new_text, dialog);
        }
    );

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
    xfce4::connect_toggled (GTK_CELL_RENDERER_TOGGLE (toggle_cell_renderer),
        [dialog](GtkCellRendererToggle *r, gchar *path) {
            list_cell_toggle (r, path, dialog);
        }
    );
    gtk_tree_view_append_column (GTK_TREE_VIEW (dialog->myTreeView),
                                 GTK_TREE_VIEW_COLUMN (tree_view_column));

    text_cell_renderer = gtk_cell_renderer_text_new ();
    g_object_set ((gpointer*) text_cell_renderer, "editable", TRUE, NULL);
    tree_view_column = gtk_tree_view_column_new_with_attributes (_("Color"),
                        text_cell_renderer, "text", eTreeColumn_Color, NULL);
    xfce4::connect_edited (GTK_CELL_RENDERER_TEXT (text_cell_renderer),
        [dialog](GtkCellRendererText *r, gchar *path, gchar *new_text) {
            list_cell_color_edited (r, path, new_text, dialog);
        }
    );
    gtk_tree_view_append_column (GTK_TREE_VIEW (dialog->myTreeView),
                                 GTK_TREE_VIEW_COLUMN (tree_view_column));

    text_cell_renderer = gtk_cell_renderer_text_new ();

    g_object_set ((gpointer*) text_cell_renderer, "editable", TRUE, NULL);
    tree_view_column = gtk_tree_view_column_new_with_attributes
                (_("Min"), text_cell_renderer, "text", eTreeColumn_Min, NULL);
    xfce4::connect_edited (GTK_CELL_RENDERER_TEXT (text_cell_renderer),
        [dialog](GtkCellRendererText *r, gchar *path, gchar *new_text) {
            minimum_changed (r, path, new_text, dialog);
        }
    );

    gtk_tree_view_append_column (GTK_TREE_VIEW (dialog->myTreeView),
                                 GTK_TREE_VIEW_COLUMN (tree_view_column));

    text_cell_renderer = gtk_cell_renderer_text_new ();

    g_object_set ((gpointer*) text_cell_renderer, "editable", TRUE, NULL);
    tree_view_column = gtk_tree_view_column_new_with_attributes
                (_("Max"), text_cell_renderer, "text", eTreeColumn_Max, NULL);
    xfce4::connect_edited (GTK_CELL_RENDERER_TEXT (text_cell_renderer),
        [dialog](GtkCellRendererText *r, gchar *path, gchar *new_text) {
            maximum_changed (r, path, new_text, dialog);
        }
    );

    gtk_tree_view_append_column (GTK_TREE_VIEW (dialog->myTreeView),
                                 GTK_TREE_VIEW_COLUMN (tree_view_column));

    GtkWidget *scrolled_window = gtk_scrolled_window_new (NULL, NULL);
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
add_temperature_unit_box (GtkWidget *vbox, const Ptr<t_sensors_dialog> &dialog)
{
    GtkWidget *hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_widget_show (hbox);

    GtkWidget *label = gtk_label_new ( _("Temperature scale:"));
    GtkWidget *button_celsius = gtk_radio_button_new_with_mnemonic (NULL, _("_Celsius"));
    GtkWidget *button_fahrenheit = gtk_radio_button_new_with_mnemonic(
      gtk_radio_button_get_group (GTK_RADIO_BUTTON (button_celsius)), _("_Fahrenheit"));

    gtk_widget_show (button_celsius);
    gtk_widget_show (button_fahrenheit);
    gtk_widget_show (label);

    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(button_celsius), dialog->sensors->scale == CELSIUS);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(button_fahrenheit), dialog->sensors->scale == FAHRENHEIT);

    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (hbox), button_celsius, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (hbox), button_fahrenheit, FALSE, FALSE, 0);

    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);

    xfce4::connect_toggled (GTK_TOGGLE_BUTTON (button_celsius), [dialog](GtkToggleButton *button) {
        temperature_unit_change (button, dialog);
    });
}


/* -------------------------------------------------------------------------- */
void
add_sensors_frame (GtkWidget *notebook, const Ptr<t_sensors_dialog> &dialog)
{
    GtkWidget *vbox = gtk_vbox_new (FALSE, OUTER_BORDER);
    gtk_container_set_border_width (GTK_CONTAINER(vbox), OUTER_BORDER);
    gtk_widget_show (vbox);

    GtkWidget *label = gtk_label_new_with_mnemonic (_("_Sensors"));
    gtk_widget_show (label);

    gtk_container_set_border_width (GTK_CONTAINER (vbox), 2*BORDER);

    gtk_notebook_append_page (GTK_NOTEBOOK (notebook), vbox, label);

    add_type_box (vbox, dialog);

    add_sensor_settings_box (vbox, dialog);

    add_temperature_unit_box (vbox, dialog);
}


/* -------------------------------------------------------------------------- */
void
free_widgets (const Ptr<t_sensors_dialog> &dialog)
{
    for (size_t idx_chip = 0; idx_chip < dialog->sensors->chips.size(); idx_chip++)
    {
        GtkTreeIter iter_list_store;
        if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (dialog->myListStore[idx_chip]), &iter_list_store))
        {
            while (gtk_tree_store_remove (GTK_TREE_STORE (dialog->myListStore[idx_chip]), &iter_list_store)) {}
        }
        gtk_tree_store_clear (dialog->myListStore[idx_chip]);
        g_object_unref (dialog->myListStore[idx_chip]);
    }

    /* stop association to libsensors and others*/
    cleanup_interfaces ();

    dialog->sensors->chips.clear();

    dialog->sensors->command_name = "";
    dialog->sensors->plugin_config_file = "";
    dialog->sensors->str_fontsize = "";
}


/* -------------------------------------------------------------------------- */
void
init_widgets (const Ptr<t_sensors_dialog> &dialog)
{
    auto sensors = dialog->sensors;

    for (size_t idx_chip = 0; idx_chip < sensors->chips.size(); idx_chip++) {
        auto tree_store = gtk_tree_store_new (6, G_TYPE_STRING,
                                              G_TYPE_STRING, G_TYPE_BOOLEAN,
                                              G_TYPE_STRING, G_TYPE_FLOAT,
                                              G_TYPE_FLOAT);

        dialog->myListStore.push_back(tree_store);

        auto chip = sensors->chips[idx_chip];
        gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (dialog->myComboBox), chip->sensorId.c_str());

        fill_gtkTreeStore (tree_store, chip, sensors->scale,  dialog);
    }

    if (sensors->chips.empty()) {
        auto chip = xfce4::make<t_chip>();

        gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (dialog->myComboBox), chip->sensorId.c_str());

        auto list_store = gtk_tree_store_new (6, G_TYPE_STRING,
                                              G_TYPE_STRING, G_TYPE_BOOLEAN,
                                              G_TYPE_STRING, G_TYPE_FLOAT,
                                              G_TYPE_FLOAT);

        dialog->myListStore.push_back(list_store);

        auto feature = xfce4::make<t_chipfeature>();
        feature->formatted_value = "0.0";
        feature->raw_value = 0.0;

        GtkTreeIter iter;
        gtk_tree_store_append (list_store, &iter, NULL);
        gtk_tree_store_set (list_store,
                            &iter,
                            eTreeColumn_Name, feature->name.c_str(),
                            eTreeColumn_Value, "0.0",        /* chipfeature->formatted_value */
                            eTreeColumn_Show, FALSE,         /* chipfeature->show */
                            eTreeColumn_Color, "#000000",    /* chipfeature->color */
                            eTreeColumn_Min, 0.0f,           /* chipfeature->min_value */
                            eTreeColumn_Max, 0.0f,           /* chipfeature->max_value */
                            -1);
    }
}


/* -------------------------------------------------------------------------- */
void
reload_listbox (const Ptr<t_sensors_dialog> &dialog)
{
    auto sensors = dialog->sensors;

    for (size_t idx_chip = 0; idx_chip < sensors->chips.size(); idx_chip++) {
        auto chip = sensors->chips[idx_chip];

        GtkTreeStore *tree_store = dialog->myListStore[idx_chip];
        g_assert (tree_store != NULL);
        gtk_tree_store_clear (tree_store);

        fill_gtkTreeStore (tree_store, chip, sensors->scale, dialog);
    }
}


/* -------------------------------------------------------------------------- */
t_labelledlevelbar::~t_labelledlevelbar()
{
    if (databox)
        gtk_widget_destroy (databox);
    if (label)
        gtk_widget_destroy (label);
    if (progressbar)
        gtk_widget_destroy (progressbar);

    if (css_provider)
        g_object_unref (css_provider);
    if (databox)
        g_object_unref (databox);
    if (label)
        g_object_unref (label);
    if (progressbar)
        g_object_unref (progressbar);
}
