/* File: hddtemp.c
 *
 *  Copyright 2004-2017 Fabian Nowak (timystery@arcor.de)
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
#include <sensors-interface-common.h>

/* Gtk/Glib includes */
#include <glib.h>
#include <gtk/gtk.h>

/* Global includes */
#if defined(HAVE_LIBNOTIFY4) || defined(HAVE_LIBNOTIFY7)
#include <libnotify/notify.h>
#endif
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include <sys/utsname.h>

#include <unistd.h>

#ifdef HAVE_NETCAT
# include "helpers.c"
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


/* forward declaration for GCC 4.3 -Wall */
#if defined(HAVE_LIBNOTIFY4) || defined(HAVE_LIBNOTIFY7)
void notification_suppress_messages (NotifyNotification *n, gchar *action, gpointer *data);
#endif

void quick_message_notify (gchar *str_message);
void quick_message (gchar *str_message);
void quick_message_dialog (gchar *str_message);
gboolean quick_message_with_checkbox (gchar *str_message, gchar *str_checkboxtext);

void read_disks_netcat (t_chip *ptr_chip);
void read_disks_linux26 (t_chip *ptr_chip);
int get_hddtemp_d_str (char *str_buffer, size_t siz_buffer);
void read_disks_fallback (t_chip *ptr_chip);
void remove_unmonitored_drives (t_chip *ptr_chip, gboolean *ptr_suppressmessage);
void populate_detected_drives (t_chip *ptr_chip);

#if defined(HAVE_LIBNOTIFY4) || defined(HAVE_LIBNOTIFY7)
void
notification_suppress_messages (NotifyNotification *ptr_notification, gchar *str_action, gpointer *ptr_data)
{
    if (strcmp(str_action, "confirmed")!=0)
        return;

    /* FIXME: Use channels or propagate private object or use static global variable */
}

void
quick_message_notify (gchar *str_message)
{
    NotifyNotification *ptr_notification;
    const gchar *str_summary, *str_icon;
    GError *ptr_error = NULL;

    str_summary = "Hddtemp Information";
    str_icon = "xfce-sensors";

    if (!notify_is_initted())
        notify_init(PACKAGE); /* NOTIFY_APPNAME */

#ifdef HAVE_LIBNOTIFY7
    ptr_notification = notify_notification_new (str_summary, str_message, str_icon);
#elif HAVE_LIBNOTIFY4
    ptr_notification = notify_notification_new (str_summary, str_message, str_icon, NULL);
#endif
    /* FIXME: Use channels or propagate private object or use static global variable */
    //notify_notification_add_action (ptr_notification,
                            //"confirmed",
                            //_("Don't show this message again"),
                            //(NotifyActionCallback) notification_suppress_messages,
                            //NULL);
    notify_notification_show(ptr_notification, &ptr_error);
}
#else
void
quick_message_dialog (gchar *str_message)
{
    GtkWidget *ptr_dialog;  /*, *label; */

    TRACE ("enters quick_message");

    ptr_dialog = gtk_message_dialog_new (NULL,
                                  GTK_DIALOG_DESTROY_WITH_PARENT,
                                  GTK_MESSAGE_INFO,
                                  GTK_BUTTONS_CLOSE,
                                  message, NULL);

    g_signal_connect_swapped (dialog, "response",
                             G_CALLBACK (gtk_widget_destroy), ptr_dialog);

    gtk_dialog_run(GTK_DIALOG(ptr_dialog));

    TRACE ("leaves quick_message");
}


gboolean
quick_message_with_checkbox (gchar *str_message, gchar *str_checkboxtext)
{
    GtkWidget *ptr_dialog, *ptr_checkbox;  /*, *label; */
    gboolean is_active;

    TRACE ("enters quick_message");

    ptr_dialog = gtk_message_dialog_new (NULL,
                                  0, /* GTK_DIALOG_DESTROY_WITH_PARENT */
                                  GTK_MESSAGE_INFO,
                                  GTK_BUTTONS_CLOSE,
                                  str_message, NULL);

    gtk_window_set_title(GTK_WINDOW(ptr_dialog), _("Sensors Plugin"));

    ptr_checkbox = gtk_check_button_new_with_mnemonic (str_checkboxtext);

    gtk_box_pack_start (GTK_BOX(GTK_DIALOG(ptr_dialog)->vbox), ptr_checkbox, FALSE, FALSE, 0);
    gtk_widget_show(ptr_checkbox);

    gtk_dialog_run(GTK_DIALOG(ptr_dialog));

    is_active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(ptr_checkbox));

    gtk_widget_destroy (ptr_dialog);

    TRACE ("leaves quick_message");

    return is_active;
}
#endif


void
quick_message (gchar *str_message)
{
#if defined(HAVE_LIBNOTIFY4) || defined(HAVE_LIBNOTIFY7)
    quick_message_notify (str_message);
#else
    quick_message_dialog (str_message);
#endif
}


#ifdef HAVE_NETCAT
void
read_disks_netcat (t_chip *ptr_chip)
{
    char str_reply[REPLY_MAX_SIZE], *str_tmp, *str_tmp2, *str_tmp3;
    int result;

    t_chipfeature *ptr_chipfeature;

    bzero(&str_reply, REPLY_MAX_SIZE);
    result = get_hddtemp_d_str(str_reply, REPLY_MAX_SIZE);
    DBG ("reply=%s with result=%d\n", str_reply, (int) result);
    if (result==-1)
    {
      return;
    }

    str_tmp = str_split (str_reply, DOUBLE_DELIMITER);
    do {
        ptr_chipfeature = g_new0(t_chipfeature, 1);

        str_tmp2 = g_strdup (str_tmp);
        str_tmp3 = strtok (str_tmp2, SINGLE_DELIMITER);
        ptr_chipfeature->devicename = g_strdup(str_tmp3);
        str_tmp3 = strtok (NULL, SINGLE_DELIMITER);
        ptr_chipfeature->name = g_strdup(str_tmp3);

        g_ptr_array_add(ptr_chip->chip_features, ptr_chipfeature);
        ptr_chip->num_features++;

        g_free (str_tmp2);
    }
    while ( (str_tmp = str_split(NULL, DOUBLE_DELIMITER)) );
}
#else
void
read_disks_fallback (t_chip *ptr_chip)
{
    GError *ptr_error;
    GDir *ptr_dir;
    t_chipfeature *ptr_chipfeature;
    const gchar* str_devicename;

    TRACE ("enters read_disks_fallback");

    /* read from /proc/ide */
    ptr_error = NULL;
    ptr_dir = g_dir_open ("/proc/ide/", 0, &ptr_error);

    while ( (str_devicename = g_dir_read_name (ptr_dir))!=NULL ) {
        if ( strncmp (str_devicename, "hd", 2)==0 || strncmp (str_devicename, "sd", 2)==0) {
            /* TODO: look, if /dev/str_devicename exists? */
            ptr_chipfeature = g_new0 (t_chipfeature, 1);
            ptr_chipfeature->devicename = g_strconcat ("/dev/", str_devicename, NULL);
            ptr_chipfeature->name = g_strdup(ptr_chipfeature->devicename);
            g_ptr_array_add (ptr_chip->chip_features, ptr_chipfeature);
            ptr_chip->num_features++;
        }
    }

    g_dir_close (ptr_dir);

    /* FIXME: read SCSI info from where? SATA?  */

    TRACE ("leaves read_disks_fallback");
}


void
read_disks_linux26 (t_chip *ptr_chip)
{
    GDir *ptr_dir;
    t_chipfeature *ptr_chipfeature;
    const gchar* str_devicename;

    TRACE ("enters read_disks_linux26");

    /* read from /sys/block */
    ptr_dir = g_dir_open ("/sys/block/", 0, NULL);
    while ( (str_devicename = g_dir_read_name (ptr_dir))!=NULL ) {
        /* if ( strncmp (str_devicename, "ram", 3)!=0 &&
             strncmp (str_devicename, "loop", 4)!=0 &&
             strncmp (str_devicename, "md", 2)!=0 &&
             strncmp (str_devicename, "fd", 2)!=0 &&
             strncmp (str_devicename, "mmc", 3)!=0 &&
             strncmp (str_devicename, "dm-", 3)!=0 ) { */
        if ( strncmp (str_devicename, "hd", 2)==0 ||
              strncmp (str_devicename, "sd", 2)==0 ) {
            /* TODO: look, if /dev/str_devicename exists? */
            ptr_chipfeature = g_new0 (t_chipfeature, 1);
            ptr_chipfeature->devicename = g_strconcat ("/dev/", str_devicename, NULL); /* /proc/ide/hda/model ?? */
            ptr_chipfeature->name = g_strdup(ptr_chipfeature->devicename);
            g_ptr_array_add (ptr_chip->chip_features, ptr_chipfeature);
            ptr_chip->num_features++;
        }
    }

    g_dir_close (ptr_dir);

    TRACE ("leaves read_disks_linux26");
}
#endif


void
remove_unmonitored_drives (t_chip *ptr_chip, gboolean *suppressmessage)
{
    int idx_features, val_disk_temperature;
    t_chipfeature *ptr_chipfeature;

    TRACE ("enters remove_unmonitored_drives");

    for (idx_features=0; idx_features<ptr_chip->num_features; idx_features++)
    {
        ptr_chipfeature = g_ptr_array_index (ptr_chip->chip_features, idx_features);
        val_disk_temperature = get_hddtemp_value (ptr_chipfeature->devicename, suppressmessage);
        if (val_disk_temperature == NO_VALID_HDDTEMP_PROGRAM)
        {
            DBG ("removing single disk");
            free_chipfeature ( (gpointer) ptr_chipfeature, NULL);
            g_ptr_array_remove_index (ptr_chip->chip_features, idx_features);
            idx_features--;
            ptr_chip->num_features--;
        }
        else if (val_disk_temperature == NO_VALID_TEMPERATURE_VALUE)
        {
            for (idx_features=0; idx_features < ptr_chip->num_features; idx_features++) {
                DBG ("remove %d\n", idx_features);
                ptr_chipfeature = g_ptr_array_index (ptr_chip->chip_features, idx_features);
                free_chipfeature ( (gpointer) ptr_chipfeature, NULL);
            }
            g_ptr_array_free (ptr_chip->chip_features, TRUE);
            ptr_chip->num_features=0;
            DBG ("Returning because of bad hddtemp.\n");
            return;
        }
    }

    TRACE ("leaves remove_unmonitored_drives");
}


void
populate_detected_drives (t_chip *ptr_chip)
{
    int idx_disks;
    t_chipfeature *ptr_chipfeature;

    TRACE ("enters populate_detected_drives");

    for (idx_disks=0; idx_disks < ptr_chip->num_features; idx_disks++)
    {
       ptr_chipfeature = g_ptr_array_index (ptr_chip->chip_features, idx_disks);
       g_assert (ptr_chipfeature!=NULL);

       ptr_chipfeature->address = idx_disks;

       ptr_chipfeature->color = g_strdup("#B000B0");
       ptr_chipfeature->valid = TRUE;
       ptr_chipfeature->raw_value = 0.0;

       ptr_chipfeature->class = TEMPERATURE;
       ptr_chipfeature->min_value = 10.0;
       ptr_chipfeature->max_value = 50.0;

       ptr_chipfeature->show = FALSE;
    }

    TRACE ("leaves populate_detected_drives");
}


int
initialize_hddtemp (GPtrArray *arr_ptr_chips, gboolean *suppressmessage)
{
#ifndef HAVE_NETCAT
    int generation, major, result;
    struct utsname *p_uname = NULL;
#endif
    int retval;
    t_chip *ptr_chip;

    g_assert (arr_ptr_chips!=NULL);

    TRACE ("enters initialize_hddtemp");

    ptr_chip = g_new0 (t_chip, 1);

    ptr_chip->chip_features = g_ptr_array_new ();
    ptr_chip->num_features = 0;
    ptr_chip->description = g_strdup(_("S.M.A.R.T. harddisk temperatures"));
    ptr_chip->name = g_strdup(_("Hard disks"));
    ptr_chip->sensorId = g_strdup("Hard disks");
    ptr_chip->type = HDD;
#ifdef HAVE_NETCAT
    read_disks_netcat (ptr_chip);
#else
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

    if (strcmp(p_uname->sysname, "Linux")==0 && (generation>=3 || (generation==2 && major>=5)))
        read_disks_linux26 (ptr_chip);
    else
        read_disks_fallback (ptr_chip); /* hopefully, that's a safe variant */

    g_free(p_uname);
#endif


    remove_unmonitored_drives (ptr_chip, suppressmessage);
    DBG  ("numfeatures=%d\n", ptr_chip->num_features);
    if ( ptr_chip->num_features>0 ) {  /* if (1) */

        populate_detected_drives (ptr_chip);
        g_ptr_array_add (arr_ptr_chips, ptr_chip);
        retval = 2;
    }
    else {
        retval = 0;
    }

    TRACE ("leaves initialize_hddtemp");

    return retval;
}



int
get_hddtemp_d_str (char *buffer, size_t bufsize)
{
    int socket_number;
    struct sockaddr_in sockaddr_hddtemplocalhost;
    struct hostent *ptr_hostinfo;
    int num_read_bytes_total = 0, num_read = 0;

    /* Create the socket. */
    socket_number = socket(PF_INET, SOCK_STREAM, 0);
    if (socket_number < 0) {
      return HDDTEMP_CONNECTION_FAILED;
    }

    /* Connect to the server. */
    sockaddr_hddtemplocalhost.sin_family = AF_INET;
    sockaddr_hddtemplocalhost.sin_port = htons(HDDTEMP_PORT);
    ptr_hostinfo = gethostbyname("localhost");
    if (ptr_hostinfo == NULL) {
/*  fprintf (stderr, "Unknown host %s.\n", hostname);*/
      return HDDTEMP_CONNECTION_FAILED;
    }
    sockaddr_hddtemplocalhost.sin_addr = *(struct in_addr *) ptr_hostinfo->h_addr;

    if (connect (socket_number, (struct sockaddr *) &sockaddr_hddtemplocalhost, sizeof (sockaddr_hddtemplocalhost)) < 0) {
/*  perror ("connect (client)");*/
      return HDDTEMP_CONNECTION_FAILED;
    }

    /* Read data from server. */
    for (;;) {
      num_read = read(socket_number, buffer+num_read_bytes_total, bufsize-num_read_bytes_total-1);
      if (num_read < 0) {
          /* Read error. */
    /*      perror ("read");*/
          close (socket_number);
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
    close (socket_number);

    return num_read_bytes_total;
}


double
get_hddtemp_value (char* str_disk, gboolean *ptr_suppressmessage)
{
    gchar *ptr_str_stdout=NULL, *ptr_str_stderr=NULL;
    gchar *ptr_str_hddtemp_call=NULL, *ptr_str_message=NULL;

#if !defined(HAVE_LIBNOTIFY4) && !defined(HAVE_LIBNOTIFY7)
    gchar *ptr_str_checkbutton = NULL;
#endif
    gint exit_status=0;
    double val_drive_temperature;
    gboolean f_result=FALSE, f_nevershowagain;
    GError *ptr_f_error=NULL;

#ifdef HAVE_NETCAT
    gchar *str_tmp, *str_tmp2, *str_tmp3;
    char reply[REPLY_MAX_SIZE];
    int val_hddtemp_result;
#endif

    if (str_disk==NULL)
      return NO_VALID_TEMPERATURE_VALUE;

    if (ptr_suppressmessage!=NULL)
        f_nevershowagain = *ptr_suppressmessage;
    else
        f_nevershowagain = FALSE;

    TRACE ("enters get_hddtemp_value for %s with suppress=%d", str_disk, f_nevershowagain); /* *ptr_suppressmessage); */

#ifdef HAVE_NETCAT

    bzero(&reply, REPLY_MAX_SIZE);
    val_hddtemp_result = get_hddtemp_d_str(reply, REPLY_MAX_SIZE);
    if (val_hddtemp_result==HDDTEMP_CONNECTION_FAILED)
    {
      return NO_VALID_HDDTEMP_PROGRAM;
    }

    str_tmp3 = "-255";
    str_tmp = str_split (reply, DOUBLE_DELIMITER);
    do {
        str_tmp2 = g_strdup (str_tmp);
        str_tmp3 = strtok (str_tmp2, SINGLE_DELIMITER); // device name
        if (strcmp(str_tmp3, str_disk)==0)
        {
            strtok(NULL, SINGLE_DELIMITER); // name
            str_tmp3 = strdup(strtok(NULL, SINGLE_DELIMITER)); // value
            exit_status = 0;
            ptr_f_error = NULL;
            g_free (str_tmp2);
            break;
        }
        g_free (str_tmp2);
    }
    while ( (str_tmp = str_split(NULL, DOUBLE_DELIMITER)) );

    ptr_str_stdout = str_tmp3;

#else
    ptr_str_hddtemp_call = g_strdup_printf ( "%s -n -q %s", PATH_HDDTEMP, str_disk);
    f_result = g_spawn_command_line_sync ( (const gchar*) ptr_str_hddtemp_call,
            &ptr_str_stdout, &ptr_str_stderr, &exit_status, &ptr_f_error);
#endif

    DBG ("Exit code %d on %s with stdout of %s.\n", exit_status, str_disk, ptr_str_stdout);

    /* filter those with no sensors out */
    if (exit_status==0 && strncmp(str_disk, "/dev/fd", 6)==0) { /* is returned for floppy disks */
        DBG("exit_status==0 && strncmp(disk, \"/dev/fd\", 6)==0");
        val_drive_temperature = NO_VALID_TEMPERATURE_VALUE;
    }
    else if ((exit_status==256 || (ptr_str_stderr && strlen(ptr_str_stderr)>0))
            && access (PATH_HDDTEMP, X_OK)==0) /* || strlen(ptr_str_stderr)>0) */
    {
        /* note that this check does only work for some versions of hddtemp. */
        if (!f_nevershowagain) {
            ptr_str_message = g_strdup_printf(_("\"hddtemp\" was not executed correctly, "
                            "although it is executable. This is most probably due "
                            "to the disks requiring root privileges to read their "
                            "temperatures, and \"hddtemp\" not being setuid root."
                            "\n\n"
                            "An easy but dirty solution is to run \"chmod u+s %s"
                            "\" as root user and restart this plugin "
                            "or its panel.\n\n"
                            "Calling \"%s\" gave the following error:\n%s\nwith a return value of %d.\n"),
                            PATH_HDDTEMP, ptr_str_hddtemp_call, ptr_str_stderr, exit_status);

#if defined(HAVE_LIBNOTIFY4) || defined(HAVE_LIBNOTIFY7)
            quick_message_notify (ptr_str_message);
#else
            ptr_str_checkbutton = g_strdup(_("Suppress this message in future"));
            f_nevershowagain = quick_message_with_checkbox(ptr_str_message, ptr_str_checkbutton);
#endif

            if (ptr_suppressmessage!=NULL)
                *ptr_suppressmessage = f_nevershowagain;
        }
        else {
            DBG  ("Suppressing dialog with exit_code=256 or output on ptr_str_stderr");
        }

        val_drive_temperature = NO_VALID_HDDTEMP_PROGRAM;
    }

    else if (ptr_f_error && (!f_result || exit_status!=0))
    {
        DBG  ("error %s\n", ptr_f_error->message);
        if (!f_nevershowagain) {
            ptr_str_message = g_strdup_printf (_("An error occurred when executing"
                                      " \"%s\":\n%s"), ptr_str_hddtemp_call, ptr_f_error->message);
#if defined(HAVE_LIBNOTIFY4) || defined(HAVE_LIBNOTIFY7)
            quick_message_notify (ptr_str_message);
#else
            ptr_str_checkbutton = g_strdup(_("Suppress this message in future"));
            f_nevershowagain = quick_message_with_checkbox (ptr_str_message, ptr_str_checkbutton);
#endif

            if (ptr_suppressmessage!=NULL)
                *ptr_suppressmessage = f_nevershowagain;
        }
        else {
            DBG  ("Suppressing dialog because of error in g_spawn_cl");
        }
        val_drive_temperature = NO_VALID_HDDTEMP_PROGRAM;
    }
    else if (ptr_str_stdout && strlen(ptr_str_stdout) > 0)
    {
        DBG("got the only useful return value of 0 and value of %s.\n", ptr_str_stdout);
        /* hddtemp does not return floating values, but only integer ones.
          So have an easier life with atoi.
          FIXME: Use strtod() instead?*/
        if (0 == strcmp(ptr_str_stdout, "drive is sleeping"))
            val_drive_temperature = HDDTEMP_DISK_SLEEPING;
        else
            val_drive_temperature = (double) (atoi ( (const char*) ptr_str_stdout) );
    }
    else {
        DBG("No condition applied.");
        val_drive_temperature = NO_VALID_HDDTEMP_PROGRAM;
    }

    g_free (ptr_str_hddtemp_call);
    g_free (ptr_str_stdout);
    g_free (ptr_str_stderr);
    g_free (ptr_str_message);
#if !defined(HAVE_LIBNOTIFY4) && !defined(HAVE_LIBNOTIFY7)
    g_free (ptr_str_checkbutton);
#endif

    if (ptr_f_error)
      g_error_free(ptr_f_error);

    TRACE ("leaves get_hddtemp_value");

    return val_drive_temperature;
}


void
refresh_hddtemp (gpointer chip_feature, gpointer ptr_sensors)
{
    t_chipfeature *ptr_chipfeature;
    double val_drive_temperature;
    t_sensors *ptr_sensors_plugin_data;
    gboolean *ptr_f_suppress = NULL;

    g_assert (chip_feature!=NULL);

    TRACE ("enters refresh_hddtemp");

    if (ptr_sensors != NULL)
    {
        ptr_sensors_plugin_data = (t_sensors *) ptr_sensors;
        ptr_f_suppress = &(ptr_sensors_plugin_data->suppressmessage);
    }

    ptr_chipfeature = (t_chipfeature *) chip_feature;

    val_drive_temperature = get_hddtemp_value (ptr_chipfeature->devicename, ptr_f_suppress);

    ptr_chipfeature->raw_value = val_drive_temperature;

    TRACE ("leaves refresh_hddtemp");
}
