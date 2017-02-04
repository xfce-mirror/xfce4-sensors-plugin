/* $Id$ */
/*  Copyright 2004-2010 Fabian Nowak (timystery@arcor.de)
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

void quick_message_notify (gchar *message);
void quick_message (gchar *message);
void quick_message_dialog (gchar *message);
gboolean quick_message_with_checkbox (gchar *message, gchar *checkboxtext);

void read_disks_netcat (t_chip *chip);
void read_disks_linux26 (t_chip *chip);
int get_hddtemp_d_str (char *buffer, size_t bufsize);
void read_disks_fallback (t_chip *chip);
void remove_unmonitored_drives (t_chip *chip, gboolean *suppressmessage);
void populate_detected_drives (t_chip *chip);

#if defined(HAVE_LIBNOTIFY4) || defined(HAVE_LIBNOTIFY7)
void
notification_suppress_messages (NotifyNotification *n, gchar *action, gpointer *data)
{
    if (strcmp(action, "confirmed")!=0)
        return;

    /* FIXME: Use channels or propagate private object or use static global variable */
}

void
quick_message_notify (gchar *message)
{
    NotifyNotification *nn;
    gchar *summary, *body, *icon;
    GError *error = NULL;

    summary = "Hddtemp Information";
    body = message;
    icon = "xfce-sensors";

    if (!notify_is_initted())
        notify_init(PACKAGE); /* NOTIFY_APPNAME */

#ifdef HAVE_LIBNOTIFY7
    nn = notify_notification_new (summary, body, icon);
#elif HAVE_LIBNOTIFY4
    nn = notify_notification_new (summary, body, icon, NULL);
#endif
    /* FIXME: Use channels or propagate private object or use static global variable */
    //notify_notification_add_action (nn,
                            //"confirmed",
                            //_("Don't show this message again"),
                            //(NotifyActionCallback) notification_suppress_messages,
                            //NULL);
    notify_notification_show(nn, &error);
}
#else
void
quick_message_dialog (gchar *message)
{

    GtkWidget *dialog;  /*, *label; */

    TRACE ("enters quick_message");

    dialog = gtk_message_dialog_new (NULL,
                                  GTK_DIALOG_DESTROY_WITH_PARENT,
                                  GTK_MESSAGE_INFO,
                                  GTK_BUTTONS_CLOSE,
                                  message, NULL);

    g_signal_connect_swapped (dialog, "response",
                             G_CALLBACK (gtk_widget_destroy), dialog);

    gtk_dialog_run(GTK_DIALOG(dialog));

    TRACE ("leaves quick_message");
}


gboolean
quick_message_with_checkbox (gchar *message, gchar *checkboxtext) {

    GtkWidget *dialog, *checkbox;  /*, *label; */
    gboolean is_active;

    TRACE ("enters quick_message");

    dialog = gtk_message_dialog_new (NULL,
                                  0, /* GTK_DIALOG_DESTROY_WITH_PARENT */
                                  GTK_MESSAGE_INFO,
                                  GTK_BUTTONS_CLOSE,
                                  message, NULL);

    gtk_window_set_title(GTK_WINDOW(dialog), _("Sensors Plugin"));

    checkbox = gtk_check_button_new_with_mnemonic (checkboxtext);

    gtk_box_pack_start (GTK_BOX(GTK_DIALOG(dialog)->vbox), checkbox, FALSE, FALSE, 0);
    gtk_widget_show(checkbox);

    gtk_dialog_run(GTK_DIALOG(dialog));

    is_active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(checkbox));

    gtk_widget_destroy (dialog);

    TRACE ("leaves quick_message");

    return is_active;
}
#endif


void
quick_message (gchar *message)
{
#if defined(HAVE_LIBNOTIFY4) || defined(HAVE_LIBNOTIFY7)
    quick_message_notify (message);
#else
    quick_message_dialog (message);
#endif
}


#ifdef HAVE_NETCAT
void
read_disks_netcat (t_chip *chip)
{
    char reply[REPLY_MAX_SIZE], *tmp, *tmp2, *tmp3;
    int result;

    t_chipfeature *cf;

    bzero(&reply, REPLY_MAX_SIZE);
    result = get_hddtemp_d_str(reply, REPLY_MAX_SIZE);
    DBG ("reply=%s with result=%d\n", reply, (int) result);
    if (result==-1)
    {
      return;
    }

    tmp = str_split (reply, DOUBLE_DELIMITER);
    do {
        //g_printf ("Found token: %s\n", tmp);
        cf = g_new0(t_chipfeature, 1);

        tmp2 = g_strdup (tmp);
        tmp3 = strtok (tmp2, SINGLE_DELIMITER);
        cf->devicename = g_strdup(tmp3);
        tmp3 = strtok (NULL, SINGLE_DELIMITER);
        cf->name = g_strdup(tmp3);

        g_ptr_array_add(chip->chip_features, cf);
        chip->num_features++;

        g_free (tmp2);
    }
    while ( (tmp = str_split(NULL, DOUBLE_DELIMITER)) );
}
#else
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
#endif


void
remove_unmonitored_drives (t_chip *chip, gboolean *suppressmessage)
{
    int i, val_disk_temperature;
    t_chipfeature *ptr_chipfeature;

    TRACE ("enters remove_unmonitored_drives");

    for (i=0; i<chip->num_features; i++)
    {
        ptr_chipfeature = g_ptr_array_index (chip->chip_features, i);
        val_disk_temperature = get_hddtemp_value (ptr_chipfeature->devicename, suppressmessage);
        if (val_disk_temperature == NO_VALID_HDDTEMP_PROGRAM)
        {
            DBG ("removing single disk");
            free_chipfeature ( (gpointer) ptr_chipfeature, NULL);
            g_ptr_array_remove_index (chip->chip_features, i);
            i--;
            chip->num_features--;
        }
        else if (val_disk_temperature == NO_VALID_TEMPERATURE_VALUE)
        {
            for (i=0; i < chip->num_features; i++) {
                DBG ("remove %d\n", i);
                ptr_chipfeature = g_ptr_array_index (chip->chip_features, i);
                free_chipfeature ( (gpointer) ptr_chipfeature, NULL);
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
    int idx_disks;
    /* double value; */
    t_chipfeature *ptr_chipfeature;

    TRACE ("enters populate_detected_drives");

    //chip->sensorId = g_strdup(_("Hard disks"));

    for (idx_disks=0; idx_disks < chip->num_features; idx_disks++)
    {
       ptr_chipfeature = g_ptr_array_index (chip->chip_features, idx_disks);
       g_assert (ptr_chipfeature!=NULL);

       ptr_chipfeature->address = idx_disks;

       ptr_chipfeature->color = g_strdup("#B000B0");
       ptr_chipfeature->valid = TRUE;
       //ptr_chipfeature->formatted_value = g_strdup ("0.0"); /* _printf("%+5.1f", 0.0); */
       ptr_chipfeature->raw_value = 0.0;

       ptr_chipfeature->class = TEMPERATURE;
       ptr_chipfeature->min_value = 10.0;
       ptr_chipfeature->max_value = 50.0;

       ptr_chipfeature->show = FALSE;
    }

    TRACE ("leaves populate_detected_drives");
}


int
initialize_hddtemp (GPtrArray *chips, gboolean *suppressmessage)
{
#ifndef HAVE_NETCAT
    int generation, major, result;
    struct utsname *p_uname = NULL;
#endif
    int retval;
    t_chip *chip;

    g_assert (chips!=NULL);

    TRACE ("enters initialize_hddtemp");

    chip = g_new0 (t_chip, 1);

/*    chip->chip_name = (const sensors_chip_name *)
            ( _("Hard disks"), 0, 0, _("Hard disks") ); */

    chip->chip_features = g_ptr_array_new ();
    chip->num_features = 0;
    chip->description = g_strdup(_("S.M.A.R.T. harddisk temperatures"));
    chip->name = g_strdup(_("Hard disks"));
    chip->sensorId = g_strdup("Hard disks");
    chip->type = HDD;
#ifdef HAVE_NETCAT
    read_disks_netcat (chip);
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
        read_disks_linux26 (chip);
    else
        read_disks_fallback (chip); /* hopefully, that's a safe variant */

    g_free(p_uname);
#endif


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



int
get_hddtemp_d_str (char *buffer, size_t bufsize)
{
    int sock;
    struct sockaddr_in servername;
    struct hostent *hostinfo;
    int nbytes = 0, nchunk = 0;

    /* Create the socket. */
    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
      return HDDTEMP_CONNECTION_FAILED;
    }

    /* Connect to the server. */
    servername.sin_family = AF_INET;
    servername.sin_port = htons(HDDTEMP_PORT);
    hostinfo = gethostbyname("localhost");
    if (hostinfo == NULL) {
/*  fprintf (stderr, "Unknown host %s.\n", hostname);*/
      return HDDTEMP_CONNECTION_FAILED;
    }
    servername.sin_addr = *(struct in_addr *) hostinfo->h_addr;

    if (connect (sock, (struct sockaddr *) &servername, sizeof (servername)) < 0) {
/*  perror ("connect (client)");*/
      return HDDTEMP_CONNECTION_FAILED;
    }

    /* Read data from server. */
    for (;;) {
      nchunk = read(sock, buffer+nbytes, bufsize-nbytes-1);
      if (nchunk < 0) {
          /* Read error. */
    /*      perror ("read");*/
          close (sock);
          return HDDTEMP_CONNECTION_FAILED;
      } else if (nchunk == 0) {
          /* End-of-file. */
          break;
      } else {
          /* Data read. */
          nbytes += nchunk;
      }
    }

    buffer[nbytes] = 0;
    close (sock);
    return nbytes;
}


double
get_hddtemp_value (char* disk, gboolean *suppressmessage)
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
    gchar *tmp, *tmp2, *tmp3;
    char reply[REPLY_MAX_SIZE];
    int val_hddtemp_result;
#endif

    if (disk==NULL)
      return NO_VALID_TEMPERATURE_VALUE;

    if (suppressmessage!=NULL)
        f_nevershowagain = *suppressmessage;
    else
        f_nevershowagain = FALSE;

    TRACE ("enters get_hddtemp_value for %s with suppress=%d", disk, f_nevershowagain); /* *suppressmessage); */

#ifdef HAVE_NETCAT

    bzero(&reply, REPLY_MAX_SIZE);
    val_hddtemp_result = get_hddtemp_d_str(reply, REPLY_MAX_SIZE);
    if (val_hddtemp_result==HDDTEMP_CONNECTION_FAILED)
    {
      return NO_VALID_HDDTEMP_PROGRAM;
    }

    tmp3 = "-255";
    tmp = str_split (reply, DOUBLE_DELIMITER);
    do {
        //g_printf ("Found token: %s for disk %s\n", tmp, disk);
        tmp2 = g_strdup (tmp);
        tmp3 = strtok (tmp2, SINGLE_DELIMITER); // device name
        if (strcmp(tmp3, disk)==0)
        {
            strtok(NULL, SINGLE_DELIMITER); // name
            tmp3 = strdup(strtok(NULL, SINGLE_DELIMITER)); // value
            exit_status = 0;
            ptr_f_error = NULL;
            g_free (tmp2);
            break;
        }
        g_free (tmp2);
    }
    while ( (tmp = str_split(NULL, DOUBLE_DELIMITER)) );

    ptr_str_stdout = tmp3;

#else
    ptr_str_hddtemp_call = g_strdup_printf ( "%s -n -q %s", PATH_HDDTEMP, disk);
    f_result = g_spawn_command_line_sync ( (const gchar*) ptr_str_hddtemp_call,
            &ptr_str_stdout, &ptr_str_stderr, &exit_status, &ptr_f_error);
#endif

    DBG ("Exit code %d on %s with stdout of %s.\n", exit_status, disk, ptr_str_stdout);

    /* filter those with no sensors out */
    if (exit_status==0 && strncmp(disk, "/dev/fd", 6)==0) { /* is returned for floppy disks */
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
            //ptr_str_message = g_strconcat(ptr_str_message, _("\nYou can disable these notifications in the settings dialog.\n");
            quick_message_notify (ptr_str_message);
            //f_nevershowagain = FALSE;
#else
            ptr_str_checkbutton = g_strdup(_("Suppress this message in future"));
            f_nevershowagain = quick_message_with_checkbox(ptr_str_message, ptr_str_checkbutton);
#endif

            if (suppressmessage!=NULL)
                *suppressmessage = f_nevershowagain;
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
            //f_nevershowagain = FALSE;
#else
            ptr_str_checkbutton = g_strdup(_("Suppress this message in future"));
            f_nevershowagain = quick_message_with_checkbox (ptr_str_message, ptr_str_checkbutton);
#endif

            if (suppressmessage!=NULL)
                *suppressmessage = f_nevershowagain;
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
refresh_hddtemp (gpointer chip_feature, gpointer data)
{
    t_chipfeature *ptr_chipfeature;
    double val_drive_temperature;
    t_sensors *ptr_sensors_plugin_data;
    gboolean *ptr_f_suppress = NULL;

    g_assert (chip_feature!=NULL);

    TRACE ("enters refresh_hddtemp");

    if (data != NULL)
    {
        ptr_sensors_plugin_data = (t_sensors *) data;
        ptr_f_suppress = &(ptr_sensors_plugin_data->suppressmessage);
    }

    ptr_chipfeature = (t_chipfeature *) chip_feature;

    val_drive_temperature = get_hddtemp_value (ptr_chipfeature->devicename, ptr_f_suppress);

    //g_free (ptr_chipfeature->formatted_value);
    //ptr_chipfeature->formatted_value = g_strdup_printf(_("%.1f °C"), val_drive_temperature);
    ptr_chipfeature->raw_value = val_drive_temperature;

    TRACE ("leaves refresh_hddtemp");
}


//void
//free_hddtemp_chip (gpointer chip)
//{
    //t_chip *ptr_chip;

    //ptr_chip = (t_chip *) chip;

    ////if (ptr_chip->sensorId)
        ////g_free (ptr_chip->sensorId);

    ////if (ptr_chip->chip_name->)
        ////g_free (ptr_chip->chip_name->);
//}
