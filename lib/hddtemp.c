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

/* Note for programmers and editors: Try to use 4 spaces instead of Tab! */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

/* Package includes */
#include <hddtemp.h>
#include <middlelayer.h>
#include <types.h>

/* Gtk/Glib includes */
#include <glib.h>
/* #include <glib/garray.h>
#include <glib/gdir.h>
#include <glib/gerror.h>
#include <glib/gmem.h>
#include <glib/gmessages.h>
#include <glib/gprintf.h>
#include <glib/gspawn.h>
#include <glib/gstrfuncs.h> */
#include <gtk/gtk.h>
/* #include <gtk/gtkcheckbutton.h>
#include <gtk/gtkmessagedialog.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkstock.h> */

/* Global includes */
#include <libnotify/notify.h>
/* #include <stdio.h> */
#include <stdlib.h>
#include <string.h>

#include <sys/utsname.h>

#include <unistd.h>

#ifdef HAVE_LIBNOTIFY
void quick_message_notify (gchar *message)
{
    NotifyNotification *nn;
    gchar *summary, *body, *icon;
    GError *error = NULL;

    summary = "Hddtemp Information";
    body = message;
    icon = "xfce-sensors";

    if (!notify_is_initted())
        notify_init(PACKAGE); /* NOTIFY_APPNAME */

    nn = notify_notification_new(summary, body, icon, NULL);
    notify_notification_show(nn, &error);
}
#else
void quick_message_dialog (gchar *message)
{

    GtkWidget *dialog;  /*, *label; */

    TRACE ("enters quick_message");

    dialog = gtk_message_dialog_new (NULL,
                                  GTK_DIALOG_DESTROY_WITH_PARENT,
                                  GTK_MESSAGE_INFO,
                                  GTK_BUTTONS_CLOSE,
                                  message);

    /* dialog = gtk_dialog_new_with_buttons (_("Could not run \"hddtemp\""),
                                         NULL, 0, // GTK DIALOG NO MODAL ;-)
                                         GTK_STOCK_CLOSE, GTK_RESPONSE_NONE,
                                         NULL);
    label = gtk_label_new (message);
    gtk_label_set_line_wrap (GTK_LABEL(label), TRUE);
    gtk_label_set_width_chars (GTK_LABEL(label), 60); */

    g_signal_connect_swapped (dialog, "response",
                             G_CALLBACK (gtk_widget_destroy), dialog);

    /*
    gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox), label);
    gtk_widget_show_all (dialog); */
    gtk_dialog_run(GTK_DIALOG(dialog));

    TRACE ("leaves quick_message");
}
#endif


void quick_message (gchar *message)
{
#ifdef HAVE_LIBNOTIFY
    quick_message_notify (message);
#else
    quick_message_dialog (message);
#endif
}



gboolean quick_message_with_checkbox (gchar *message, gchar *checkboxtext) {

    GtkWidget *dialog, *checkbox;  /*, *label; */
    gboolean is_active;

    TRACE ("enters quick_message");

    dialog = gtk_message_dialog_new (NULL,
                                  0, /* GTK_DIALOG_DESTROY_WITH_PARENT */
                                  GTK_MESSAGE_INFO,
                                  GTK_BUTTONS_CLOSE,
                                  message);

    gtk_window_set_title(GTK_WINDOW(dialog), _("Xfce 4 Sensors Plugin"));

    checkbox = gtk_check_button_new_with_mnemonic (checkboxtext);

    gtk_box_pack_start (GTK_BOX(GTK_DIALOG(dialog)->vbox), checkbox, FALSE, FALSE, 0);
    gtk_widget_show(checkbox);

    gtk_dialog_run(GTK_DIALOG(dialog));

    is_active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(checkbox));

    gtk_widget_destroy (dialog);

    TRACE ("leaves quick_message");

    return is_active;
}


void
read_disks_fallback (t_chip *chip)
{
    GError *error;
    GDir *gdir;
    t_chipfeature *chipfeature;
    const gchar* dirname;

    TRACE ("enters read_disks_fallback");

    /* read from /proc/ide */
    error = NULL;
    gdir = g_dir_open ("/proc/ide/", 0, &error);

    while ( (dirname = g_dir_read_name (gdir))!=NULL ) {
        if ( strncmp (dirname, "hd", 2)==0 || strncmp (dirname, "sd", 2)==0) {
            /* TODO: look, if /dev/dirname exists? */
            chipfeature = g_new0 (t_chipfeature, 1);
            chipfeature->devicename = g_strconcat ("/dev/", dirname, NULL);
            chipfeature->name = g_strdup(chipfeature->devicename);
            g_ptr_array_add (chip->chip_features, chipfeature);
            chip->num_features++;
        }
    }

    g_dir_close (gdir);

    /* FIXME: read SCSI info from where? SATA?  */

    TRACE ("leaves read_disks_fallback");
}


void
read_disks_linux26 (t_chip *chip)
{
    GDir *gdir;
    t_chipfeature *chipfeature;
    const gchar* dirname;

    TRACE ("enters read_disks_linux26");

    /* read from /sys/block */
    gdir = g_dir_open ("/sys/block/", 0, NULL);
    while ( (dirname = g_dir_read_name (gdir))!=NULL ) {
        /* if ( strncmp (dirname, "ram", 3)!=0 &&
             strncmp (dirname, "loop", 4)!=0 &&
             strncmp (dirname, "md", 2)!=0 &&
             strncmp (dirname, "fd", 2)!=0 &&
             strncmp (dirname, "mmc", 3)!=0 &&
             strncmp (dirname, "dm-", 3)!=0 ) { */
            if ( strncmp (dirname, "hd", 2)==0 ||
                            strncmp (dirname, "sd", 2)==0 ) {
            /* TODO: look, if /dev/dirname exists? */
            chipfeature = g_new0 (t_chipfeature, 1);
            chipfeature->devicename = g_strconcat ("/dev/", dirname, NULL); /* /proc/ide/hda/model ?? */
            chipfeature->name = g_strdup(chipfeature->devicename);
            g_ptr_array_add (chip->chip_features, chipfeature);
            chip->num_features++;
        }
    }

    g_dir_close (gdir);

    TRACE ("leaves read_disks_linux26");
}


void
remove_unmonitored_drives (t_chip *chip, gboolean *suppressmessage)
{
    int i, result;
    t_chipfeature *chipfeature;

    TRACE ("enters remove_unmonitored_drives");

    for (i=0; i<chip->num_features; i++)
    {
        chipfeature = g_ptr_array_index (chip->chip_features, i);
        result = get_hddtemp_value (chipfeature->devicename, suppressmessage);
        if (result == 0.0)
        {
            DBG ("removing single disk");
            free_chipfeature ( (gpointer) chipfeature, NULL);
            g_ptr_array_remove_index (chip->chip_features, i);
            i--;
            chip->num_features--;
        }
        else if (result == ZERO_KELVIN)
        {
            for (i=0; i < chip->num_features; i++) {
                DBG ("remove %d\n", i);
                chipfeature = g_ptr_array_index (chip->chip_features, i);
                free_chipfeature ( (gpointer) chipfeature, NULL);
            }
            g_ptr_array_free (chip->chip_features, TRUE);
            // chip->chip_features = g_ptr_array_new();
            chip->num_features=0;
            DBG ("Returning because of bad hddtemp.\n");
            return;
        }
    }

    TRACE ("leaves remove_unmonitored_drives");
}


void
populate_detected_drives (t_chip *chip)
{
    int diskIndex;
    /* double value; */
    t_chipfeature *chipfeature;

    TRACE ("enters populate_detected_drives");

    chip->sensorId = g_strdup(_("Hard disks"));

    for (diskIndex=0; diskIndex < chip->num_features; diskIndex++)
    {
       chipfeature = g_ptr_array_index (chip->chip_features, diskIndex);
       g_assert (chipfeature!=NULL);

       chipfeature->address = diskIndex;

       /* chipfeature->name = g_strdup(chipfeature->devicename); */

       chipfeature->color = g_strdup("#B000B0");
       chipfeature->valid = TRUE;
       chipfeature->formatted_value = g_strdup ("0.0"); /* _printf("%+5.1f", 0.0); */
       chipfeature->raw_value = 0.0;

       chipfeature->class = TEMPERATURE;
       chipfeature->min_value = 10.0;
       chipfeature->max_value = 50.0;

       chipfeature->show = FALSE;
    }

    TRACE ("leaves populate_detected_drives");
}


int
initialize_hddtemp (GPtrArray *chips, gboolean *suppressmessage)
{
    int generation, major, result, retval;
    struct utsname *p_uname;
    t_chip *chip;

    g_assert (chips!=NULL);

    TRACE ("enters initialize_hddtemp");

    chip = g_new (t_chip, 1);

    chip->chip_name = (const sensors_chip_name *)
            ( _("Hard disks"), 0, 0, _("Hard disks") );

    chip->chip_features = g_ptr_array_new ();
    chip->num_features = 0;
    chip->description = g_strdup(_("S.M.A.R.T. harddisk temperatures"));
    chip->name = g_strdup(_("Hard disks"));
    chip->sensorId = g_strdup("Hard disks");
    chip->type = HDD;

    p_uname = (struct utsname *) malloc (sizeof(struct utsname));
    result =  uname (p_uname);
    if (result!=0) {
        g_free(p_uname);
        return -1;
    }

    generation = atoi ( p_uname->release ); /* this might cause trouble on */
    major = atoi ( p_uname->release+2 );      /* other systems than Linux! */
                /* actually, wanted to use build time configuration therefore */

    /* Note: This is actually supposed to be carried out by ifdef HAVE_LINUX
     and major/minor number stuff from compile time*/
    if (strcmp(p_uname->sysname, "Linux")==0 && major>=5)
        read_disks_linux26 (chip);
    else
        read_disks_fallback (chip); /* hopefully, that's a safe variant */

    g_free(p_uname);

    remove_unmonitored_drives (chip, suppressmessage);
    DBG  ("numfeatures=%d\n", chip->num_features);
    if ( chip->num_features>0 ) {  /* if (1) */

        populate_detected_drives (chip);
        g_ptr_array_add (chips, chip);
        retval = 2;
    }
    else {
        retval = 0;
    }

    TRACE ("leaves initialize_hddtemp");

    return retval;
}


double
get_hddtemp_value (char* disk, gboolean *suppressmessage)
{
    gchar *standard_output, *standard_error;
    gchar *cmd_line, *msg_text, *checktext;
    gint exit_status=0;
    double value;
    gboolean result, nevershowagain;
    GError *error;

    if (suppressmessage!=NULL)
        nevershowagain = *suppressmessage;
    else
        nevershowagain = FALSE;

    TRACE ("enters get_hddtemp_value for %s with suppress=%d", disk, nevershowagain); /* *suppressmessage); */

    cmd_line = g_strdup_printf ( "%s -n -q %s", PATH_HDDTEMP, disk);

    msg_text = NULL;

    error = NULL;
    result = g_spawn_command_line_sync ( (const gchar*) cmd_line,
            &standard_output, &standard_error, &exit_status, &error);

    DBG ("Exit code %d on %s with stdout of %s.\n", exit_status, disk, standard_output);

    /* filter those with no sensors out */
    if (exit_status==0 && strncmp(disk, "/dev/fd", 6)==0) { /* is returned for floppy disks */
        DBG("exit_status==0 && strncmp(disk, \"/dev/fd\", 6)==0");
        value = 0.0;
    }
    else if ((exit_status==256 || (standard_error && strlen(standard_error)>0))
            && access (PATH_HDDTEMP, X_OK)==0) /* || strlen(standard_error)>0) */
    {
        /* note that this check does only work for some versions of hddtemp. */
        if (!nevershowagain) {
            msg_text = g_strdup_printf(_("\"hddtemp\" was not executed correctly, "
                            "although it is executable. This is most probably due "
                            "to the disks requiring root privileges to read their "
                            "temperatures, and \"hddtemp\" not being setuid root."
                            "\n\n"
                            "An easy but dirty solution is to run \"chmod u+s %s"
                            "\" as root user and restart this plugin "
                            "or its panel.\n\n"
                            "Calling \"%s\" gave the following error:\n%s\nwith a return value of %d.\n"),
                            PATH_HDDTEMP, cmd_line, standard_error, exit_status);
            checktext = g_strdup(_("Suppress this message in future"));
            quick_message (msg_text);
            nevershowagain = FALSE; //quick_message_with_checkbox(msg_text, checktext);

            if (suppressmessage!=NULL)
                *suppressmessage = nevershowagain;
        }
        else {
            DBG  ("Suppressing dialog with exit_code=256 or output on standard_error");
        }

        value = ZERO_KELVIN;
    }
    /* else if (strlen(standard_error)>0) {
        msg_text = g_strdup_printf (_("An error occurred when executing"
                                      " \"%s\":\n%s"), cmd_line, standard_error);
        quick_message (msg_text);
        value = ZERO_KELVIN;
    } */

    else if (error && (!result || exit_status!=0))
    {
         DBG  ("error %s\n", error->message);
        if (!nevershowagain) {
            msg_text = g_strdup_printf (_("An error occurred when executing"
                                      " \"%s\":\n%s"), cmd_line, error->message);
            checktext = g_strdup(_("Suppress this message in future"));
            nevershowagain = quick_message_with_checkbox (msg_text, checktext);

             if (suppressmessage!=NULL)
                *suppressmessage = nevershowagain;
        }
        else {
            DBG  ("Suppressing dialog because of error in g_spawn_cl");
        }
        value = 0.0;
    }
    else if (standard_output && strlen(standard_output) > 0)
    {
        DBG("got the only useful return value of 0 and value of %s.\n", standard_output);
        /* hddtemp does not return floating values, but only integer ones.
          So have an easier life with atoi.
          FIXME: Use strtod() instead?*/
        value = (double) (atoi ( (const char*) standard_output) );
    }
    else {
        DBG("No condition applied.");
        value = 0.0;
    }

    g_free (cmd_line);
    g_free (standard_output);
    g_free (standard_error);
    g_free (msg_text);

    TRACE ("leaves get_hddtemp_value");

    return value;
}


void
refresh_hddtemp (gpointer chip_feature, gpointer data)
{
    t_chipfeature *cf;
    double value;

    g_assert (chip_feature!=NULL);

    TRACE ("enters refresh_hddtemp");

    cf = (t_chipfeature *) chip_feature;

    value = get_hddtemp_value (cf->devicename, NULL);

    /* actually, that's done in the gui part */
    g_free (cf->formatted_value);
    /*  if (scale == FAHRENHEIT) {
        cf->formatted_value = g_strdup_printf(_("%5.1f °F"), (float) (value * 9/5 + 32) );
    } else { // Celsius  */
        cf->formatted_value = g_strdup_printf(_("%5.1f °C"), value);
    /* } */
    cf->raw_value = value;

    TRACE ("leaves refresh_hddtemp");
}

