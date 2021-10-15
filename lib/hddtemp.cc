/* hddtemp.cc
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>
#include <gtk/gtk.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <unistd.h>

/* Package includes */
#include <hddtemp.h>
#include <middlelayer.h>
#include <sensors-interface-common.h>
#include <types.h>

#ifdef HAVE_LIBNOTIFY
#include <libnotify/notify.h>
#endif

#ifdef HAVE_NETCAT
# ifndef NETCAT_PATH
#  define NETCAT_PATH "/bin/netcat"
# endif
# define DOUBLE_DELIMITER "||"
# define SINGLE_DELIMITER "|"
#endif

#ifndef HDDTEMP_PORT
# define HDDTEMP_PORT 7634
#endif


#define REPLY_MAX_SIZE 512

#define HDDTEMP_CONNECTION_FAILED -1 /* Connection problems to hddtemp */


void quick_message_notify (gchar *message);
void quick_message (gchar *message);
void quick_message_dialog (gchar *message);
gboolean quick_message_with_checkbox (gchar *message, gchar *checkbox_text);

void read_disks_netcat (t_chip *chip);
void read_disks_linux26 (t_chip *chip);
int get_hddtemp_d_str (char *buffer, size_t bufsize);
void read_disks_fallback (t_chip *chip);
void remove_unmonitored_drives (t_chip *chip, gboolean *suppress_message);
void populate_detected_drives (t_chip *chip);


/* -------------------------------------------------------------------------- */
#ifdef HAVE_LIBNOTIFY
static void
notification_suppress_messages (NotifyNotification *notification, const gchar *action, gpointer *data)
{
    if (strcmp (action, "confirmed") != 0)
        return;

    /* FIXME: Use channels or propagate private object or use static global variable */
}


/* -------------------------------------------------------------------------- */
void
quick_message_notify (gchar *message)
{
    const gchar *summary = "Hddtemp Information";
    const gchar *icon = "xfce-sensors";

    if (!notify_is_initted())
        notify_init(PACKAGE); /* NOTIFY_APPNAME */

    NotifyNotification *notification = notify_notification_new (summary, message, icon);

    /* FIXME: Use channels or propagate private object or use static global variable */
    if (0)
    {
        notify_notification_add_action (notification,
                                        "confirmed",
                                        _("Don't show this message again"),
                                        (NotifyActionCallback) notification_suppress_messages,
                                        NULL, NULL);
    }

    GError *error = NULL;
    notify_notification_show (notification, &error);
}
#else
/* -------------------------------------------------------------------------- */
void
quick_message_dialog (gchar *message)
{
    GtkWidget *dialog;

    dialog = gtk_message_dialog_new (NULL,
                                     GTK_DIALOG_DESTROY_WITH_PARENT,
                                     GTK_MESSAGE_INFO,
                                     GTK_BUTTONS_CLOSE,
                                     message, NULL);

    g_signal_connect_swapped (dialog, "response",
                              G_CALLBACK (gtk_widget_destroy), dialog);

    // gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_show_all(dialog);
}


/* -------------------------------------------------------------------------- */
gboolean
quick_message_with_checkbox (gchar *message, gchar *checkbox_text)
{
    GtkWidget *dialog, *checkbox, *content_area;

    dialog = gtk_message_dialog_new (NULL,
                                     0, /* GTK_DIALOG_DESTROY_WITH_PARENT */
                                     GTK_MESSAGE_INFO,
                                     GTK_BUTTONS_CLOSE,
                                     message, NULL);

    gtk_window_set_title(GTK_WINDOW(dialog), _("Sensors Plugin"));

    checkbox = gtk_check_button_new_with_mnemonic (checkbox_text);

    content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
    gtk_container_add (GTK_CONTAINER (content_area), checkbox);
    gtk_widget_show (checkbox);

    gtk_dialog_run (GTK_DIALOG (dialog));

    gboolean is_active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (checkbox));

    gtk_widget_destroy (dialog);

    return is_active;
}
#endif


/* -------------------------------------------------------------------------- */
void
quick_message (gchar *message)
{
#ifdef HAVE_LIBNOTIFY
    quick_message_notify (message);
#else
    quick_message_dialog (message);
#endif
}


#ifdef HAVE_NETCAT
/* Global variable storing last position in splitted string used for str_split(s, d) */
static char *str_split_position;

/* -------------------------------------------------------------------------- */
/**
 * Returns tokens of the string one after the other, split by the string delim.
 * Just like strtok, initialize with a valid pointer and continue with passing NULL
 * as string argument.
 * @param string String to split
 * @param delim String of the complete delimiting string, order and content are important.
 * @return pointer onto next token, or NULL on end, or NULL on bad delimiter.
 */
static char*
str_split (char *string, const char *delim)
{
    char *retval;

    if (string != NULL)
        str_split_position = string;

    if (str_split_position == NULL)
        return NULL;

    if (delim == NULL)
        return NULL;

    char *p = strstr (str_split_position, delim);
    if (p != NULL)
    {
        size_t strlen_delim = strlen (delim);
        memset (p, '\0', strlen_delim );
        retval = str_split_position;
        str_split_position = p + strlen_delim;
    }
    else
    {
        retval = str_split_position;
        str_split_position = NULL;
    }

    return retval;
}

/* -------------------------------------------------------------------------- */
void
read_disks_netcat (t_chip *chip)
{
    char reply[REPLY_MAX_SIZE] = {0};
    int result;

    result = get_hddtemp_d_str(reply, REPLY_MAX_SIZE);
    DBG ("reply=%s with result=%d\n", reply, (int) result);
    if (result == -1)
      return;

    char *tmp = str_split (reply, DOUBLE_DELIMITER);
    do {
        t_chipfeature *feature = g_new0(t_chipfeature, 1);

        char *tmp2 = g_strdup (tmp);
        char *tmp3 = strtok (tmp2, SINGLE_DELIMITER);
        feature->devicename = g_strdup(tmp3);
        tmp3 = strtok (NULL, SINGLE_DELIMITER);
        feature->name = g_strdup(tmp3);

        g_ptr_array_add(chip->chip_features, feature);
        chip->num_features++;

        g_free (tmp2);
    }
    while ( (tmp = str_split(NULL, DOUBLE_DELIMITER)) );
}
#else
/* -------------------------------------------------------------------------- */
void
read_disks_fallback (t_chip *chip)
{
    /* read from /proc/ide */
    GError *error = NULL;
    GDir *dir = g_dir_open ("/proc/ide/", 0, &error);

    const gchar *device_name;
    while ((device_name = g_dir_read_name (dir)) != NULL) {
        if (strncmp (device_name, "hd", 2)==0 || strncmp (device_name, "sd", 2)==0) {
            /* TODO: look whether /dev/str_devicename exists? */
            t_chipfeature *feature;
            feature = g_new0 (t_chipfeature, 1);
            feature->devicename = g_strconcat ("/dev/", device_name, NULL);
            feature->name = g_strdup(feature->devicename);
            g_ptr_array_add (chip->chip_features, feature);
            chip->num_features++;
        }
    }

    g_dir_close (dir);

    /* FIXME: read SCSI info from where? SATA?  */
}


/* -------------------------------------------------------------------------- */
void
read_disks_linux26 (t_chip *chip)
{
    const gchar *device_name;

    /* read from /sys/block */
    GDir *dir = g_dir_open ("/sys/block/", 0, NULL);
    while ((device_name = g_dir_read_name (dir)) != NULL) {
        /* if ( strncmp (str_devicename, "ram", 3)!=0 &&
             strncmp (str_devicename, "loop", 4)!=0 &&
             strncmp (str_devicename, "md", 2)!=0 &&
             strncmp (str_devicename, "fd", 2)!=0 &&
             strncmp (str_devicename, "mmc", 3)!=0 &&
             strncmp (str_devicename, "dm-", 3)!=0 ) { */
        if (strncmp (device_name, "hd", 2) == 0 || strncmp (device_name, "sd", 2) == 0)
        {
            /* TODO: look whether /dev/str_devicename exists? */
            t_chipfeature *feature;
            feature = g_new0 (t_chipfeature, 1);
            feature->devicename = g_strconcat ("/dev/", device_name, NULL); /* /proc/ide/hda/model ?? */
            feature->name = g_strdup(feature->devicename);
            g_ptr_array_add (chip->chip_features, feature);
            chip->num_features++;
        }
    }

    g_dir_close (dir);
}
#endif


/* -------------------------------------------------------------------------- */
void
remove_unmonitored_drives (t_chip *chip, gboolean *suppress_message)
{
    int idx_feature;

    for (idx_feature=0; idx_feature < chip->num_features; idx_feature++)
    {
        auto feature = (t_chipfeature*) g_ptr_array_index (chip->chip_features, idx_feature);
        int temperature = get_hddtemp_value (feature->devicename, suppress_message);
        if (temperature == NO_VALID_HDDTEMP_PROGRAM)
        {
            DBG ("removing single disk");
            free_chipfeature ((gpointer) feature, NULL);
            g_ptr_array_remove_index (chip->chip_features, idx_feature);
            idx_feature--;
            chip->num_features--;
        }
        else if (temperature == NO_VALID_TEMPERATURE_VALUE)
        {
            for (idx_feature=0; idx_feature < chip->num_features; idx_feature++) {
                DBG ("remove %d\n", idx_feature);
                feature = (t_chipfeature*) g_ptr_array_index (chip->chip_features, idx_feature);
                free_chipfeature ( (gpointer) feature, NULL);
            }
            g_ptr_array_free (chip->chip_features, TRUE);
            chip->num_features=0;
            DBG ("Returning because of bad hddtemp.\n");
            return;
        }
    }
}


/* -------------------------------------------------------------------------- */
void
populate_detected_drives (t_chip *chip)
{
    for (gint idx_disk=0; idx_disk < chip->num_features; idx_disk++)
    {
       auto feature = (t_chipfeature*) g_ptr_array_index (chip->chip_features, idx_disk);
       g_assert (feature!=NULL);

       feature->address = idx_disk;

       feature->color_orNull = g_strdup ("#B000B0");
       feature->valid = TRUE;
       feature->raw_value = 0.0;

       feature->cls = TEMPERATURE;
       feature->min_value = 10.0;
       feature->max_value = 50.0;

       feature->show = FALSE;
    }
}


/* -------------------------------------------------------------------------- */
int
initialize_hddtemp (GPtrArray *chips, gboolean *suppress_message)
{
#ifndef HAVE_NETCAT
    int generation_linuxkernel, majorversion_linuxkernel;
    struct utsname *unixname = NULL;
#endif
    int result;

    g_assert (chips!=NULL);

    t_chip *chip = g_new0 (t_chip, 1);

    chip->chip_features = g_ptr_array_new ();
    chip->num_features = 0;
    chip->description = g_strdup (_("S.M.A.R.T. harddisk temperatures"));
    chip->name = g_strdup (_("Hard disks"));
    chip->sensorId = g_strdup ("Hard disks");
    chip->type = HDD;
#ifdef HAVE_NETCAT
    read_disks_netcat (chip);
#else
    unixname = (struct utsname *) malloc (sizeof(struct utsname));
    result = uname (unixname);
    if (result!=0) {
        g_free(unixname);
        return -1;
    }

    generation_linuxkernel = atoi ( unixname->release ); /* this might cause trouble on */
    majorversion_linuxkernel = atoi ( unixname->release+2 );      /* other systems than Linux! */
                /* actually, wanted to use build time configuration therefore */

    /* Note: This is actually supposed to be carried out by ifdef HAVE_LINUX
     and major/minor number stuff from compile time*/

    if (strcmp (unixname->sysname, "Linux")==0 && (generation_linuxkernel>=3 || (generation_linuxkernel==2 && majorversion_linuxkernel>=5)))
        read_disks_linux26 (chip);
    else
        read_disks_fallback (chip); /* hopefully, that's a safe variant */

    g_free (unixname);
#endif

    remove_unmonitored_drives (chip, suppress_message);
    DBG ("numfeatures=%d\n", chip->num_features);
    if ( chip->num_features>0 ) {  /* if (1) */

        populate_detected_drives (chip);
        g_ptr_array_add (chips, chip);
        result = 2;
    }
    else {
        free_chip(chip, NULL);
        result = 0;
    }

    return result;
}


#ifdef HAVE_NETCAT
/* -------------------------------------------------------------------------- */
int
get_hddtemp_d_str (char *buffer, size_t bufsize)
{
    struct sockaddr_in sockaddr;
    struct hostent *hostinfo;
    ssize_t num_read_bytes_total = 0;

    /* Create the socket. */
    int fd = socket (PF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
      return HDDTEMP_CONNECTION_FAILED;
    }

    /* Connect to the server. */
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port = htons(HDDTEMP_PORT);
    hostinfo = gethostbyname ("localhost");
    if (hostinfo == NULL || !hostinfo->h_addr_list[0]) {
      close (fd);
      return HDDTEMP_CONNECTION_FAILED;
    }

    memcpy (&sockaddr.sin_addr, hostinfo->h_addr_list[0], sizeof(sockaddr.sin_addr));

    if (connect (fd, (struct sockaddr*) &sockaddr, sizeof (sockaddr)) < 0) {
      close (fd);
      return HDDTEMP_CONNECTION_FAILED;
    }

    /* Read data from server. */
    for (;;) {
      ssize_t num_read = read (fd, buffer + num_read_bytes_total, bufsize - num_read_bytes_total - 1);
      if (num_read < 0) {
          /* Read error. */
          close (fd);
          return HDDTEMP_CONNECTION_FAILED;
      } else if (num_read == 0) {
          /* End-of-file. */
          break;
      } else {
          /* Data read. */
          num_read_bytes_total += num_read;
      }
    }

    buffer[num_read_bytes_total] = 0;
    close (fd);

    return num_read_bytes_total;
}
#endif


/* -------------------------------------------------------------------------- */
double
get_hddtemp_value (char *disk, gboolean *suppress_message)
{
    gchar *str_stdout = NULL, *str_stderr = NULL;
    gchar *hddtemp_call = NULL, *message = NULL;
    gchar *check_button = NULL;
    gint exit_status = 0;
    double temperature;
    gboolean f_result = FALSE, f_nevershowagain = FALSE;
    GError *f_error = NULL;

#ifdef HAVE_NETCAT
    gchar *tmp, *tmp2, *tmp3;
    char reply[REPLY_MAX_SIZE] = {0};
#endif

    if (disk == NULL)
      return NO_VALID_TEMPERATURE_VALUE;

    if (suppress_message != NULL)
        f_nevershowagain = *suppress_message;

#ifdef HAVE_NETCAT

    if (HDDTEMP_CONNECTION_FAILED == get_hddtemp_d_str (reply, REPLY_MAX_SIZE))
        return NO_VALID_HDDTEMP_PROGRAM;

    tmp3 = g_strdup_printf ("%d", NO_VALID_TEMPERATURE_VALUE);
    if ( (tmp = str_split (reply, DOUBLE_DELIMITER)) ) {
        do {
            if ((tmp2 = strtok (tmp, SINGLE_DELIMITER)) // device name
                && (strcmp (tmp2, disk) == 0)) {
                if ( strtok (NULL, SINGLE_DELIMITER) // name
                    && (tmp2 = strtok (NULL, SINGLE_DELIMITER)) ) { // value
                    g_free (tmp3);
                    tmp3 = strdup (tmp2);
                }

                exit_status = 0;
                f_error = NULL;
                break;
            }
        }
        while ( (tmp = str_split (NULL, DOUBLE_DELIMITER)) );
    }

    str_stdout = tmp3;

#else
    hddtemp_call = g_strdup_printf ( "%s -n -q %s", PATH_HDDTEMP, disk);
    f_result = g_spawn_command_line_sync ( (const gchar*) hddtemp_call,
            &str_stdout, &str_stderr, &exit_status, &f_error);
#endif

    DBG ("Exit code %d on %s with stdout of %s.\n", exit_status, disk, str_stdout);

    /* filter those with no sensors out */
    if (exit_status==0 && strncmp(disk, "/dev/fd", 6)==0) { /* is returned for floppy disks */
        DBG("exit_status==0 && strncmp(disk, \"/dev/fd\", 6)==0");
        temperature = NO_VALID_TEMPERATURE_VALUE;
    }
    else if ((exit_status==256 || (str_stderr && strlen(str_stderr)>0))
            && access (PATH_HDDTEMP, X_OK)==0) /* || strlen(ptr_str_stderr)>0) */
    {
        /* note that this check does only work for some versions of hddtemp. */
        if (!f_nevershowagain) {
            message = g_strdup_printf(_("\"hddtemp\" was not executed correctly, "
                            "although it is executable. This is most probably due "
                            "to the disks requiring root privileges to read their "
                            "temperatures, and \"hddtemp\" not being setuid root."
                            "\n\n"
                            "An easy but dirty solution is to run \"chmod u+s %s"
                            "\" as root user and restart this plugin "
                            "or its panel.\n\n"
                            "Calling \"%s\" gave the following error:\n%s\nwith a return value of %d.\n"),
                            PATH_HDDTEMP, hddtemp_call, str_stderr, exit_status);

#ifdef HAVE_LIBNOTIFY
            quick_message_notify (message);
#else
            check_button = g_strdup(_("Suppress this message in future"));
            f_nevershowagain = quick_message_with_checkbox(message, check_button);
#endif

            if (suppress_message!=NULL)
                *suppress_message = f_nevershowagain;
        }
        else {
            DBG  ("Suppressing dialog with exit_code=256 or output on ptr_str_stderr");
        }

        temperature = NO_VALID_HDDTEMP_PROGRAM;
    }

    else if (f_error && (!f_result || exit_status!=0))
    {
        DBG  ("error %s\n", f_error->message);
        if (!f_nevershowagain) {
            message = g_strdup_printf (_("An error occurred when executing"
                                      " \"%s\":\n%s"), hddtemp_call, f_error->message);
#ifdef HAVE_LIBNOTIFY
            quick_message_notify (message);
#else
            check_button = g_strdup(_("Suppress this message in future"));
            f_nevershowagain = quick_message_with_checkbox (message, check_button);
#endif

            if (suppress_message!=NULL)
                *suppress_message = f_nevershowagain;
        }
        else {
            DBG  ("Suppressing dialog because of error in g_spawn_cl");
        }
        temperature = NO_VALID_HDDTEMP_PROGRAM;
    }
    else if (str_stdout && strlen(str_stdout) > 0)
    {
        DBG("got the only useful return value of 0 and value of %s.\n", str_stdout);
        /* hddtemp does not return floating values, but only integer ones.
          So have an easier life with atoi.
          FIXME: Use strtod() instead?*/
        if ( 0 == strcmp (str_stdout, "drive is sleeping")
          || 0 == strcmp (str_stdout, "SLP") )
            temperature = HDDTEMP_DISK_SLEEPING;
        else if (g_ascii_isalpha (str_stdout[0]) == TRUE) // UNK or NA etc.
            temperature = NO_VALID_TEMPERATURE_VALUE;
        else
            temperature = (double) (atoi ( (const char*) str_stdout) );
    }
    else {
        DBG("No condition applied.");
        temperature = NO_VALID_HDDTEMP_PROGRAM;
    }

    g_free (hddtemp_call);
    g_free (str_stdout);
    g_free (str_stderr);
    g_free (message);
    g_free (check_button);

    if (f_error)
      g_error_free(f_error);

    return temperature;
}


/* -------------------------------------------------------------------------- */
void
refresh_hddtemp (gpointer ptr_chip_feature, gpointer sensors)
{
    gboolean *suppress_message = NULL;

    g_assert (ptr_chip_feature != NULL);

    if (sensors != NULL)
    {
        auto plugin_data = (t_sensors*) sensors;
        suppress_message = &plugin_data->suppressmessage;
    }

    auto feature = (t_chipfeature *) ptr_chip_feature;
    double temperature = get_hddtemp_value (feature->devicename, suppress_message);
    feature->raw_value = temperature;
}
