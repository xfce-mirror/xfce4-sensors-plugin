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

#include "hddtemp.h"
#include "types.h"

#include <glib/garray.h>
#include <glib/gdir.h>
#include <glib/gerror.h>
#include <glib/gmem.h>
#include <glib/gmessages.h>
#include <glib/gprintf.h>
#include <glib/gspawn.h>
#include <glib/gstrfuncs.h>

/* #include <stdio.h> */
#include <stdlib.h>
#include <string.h>

#include <sys/utsname.h>

void
read_disks_linux24 (t_chip *chip)
{
    GError *error;
    GDir *gdir;
    gchar * disk;
    t_chipfeature *chipfeature;
    const gchar* dirname;

    TRACE ("enters read_disks_linux24");

    /* read from /proc/ide */
    error = NULL;
    gdir = g_dir_open ("/proc/ide/", 0, &error);

    while ( (dirname = g_dir_read_name (gdir))!=NULL ) {
        if ( strncmp (dirname, "hd", 2)==0 || strncmp (dirname, "sd", 2)==0) {
            disk = g_strconcat ("/dev/", dirname, NULL);
            /* TODO: look, if /dev/dirname exists? */
            chipfeature = g_new0 (t_chipfeature, 1);
            chipfeature->name = disk;
            g_ptr_array_add (chip->chip_features, chipfeature);
            chip->num_features++;
        }
    }
    /* FIXME: read SCSI info from where? SATA?  */

    TRACE ("leaves read_disks_linux24");
}


void
read_disks_linux26 (t_chip *chip)
{
    GError *error;
    GDir *gdir;
    gchar * disk;
    t_chipfeature *chipfeature;
    const gchar* dirname;

    TRACE ("enters read_disks_linux26");

    /* read from /sys/block */
    error = NULL;
    gdir = g_dir_open ("/sys/block/", 0, &error);
    while ( (dirname = g_dir_read_name (gdir))!=NULL ) {
        if ( strncmp (dirname, "ram", 3)!=0 &&
             strncmp (dirname, "loop", 4)!=0 ) {
            disk = g_strconcat ("/dev/", dirname, NULL);
            /* TODO: look, if /dev/dirname exists? */
            chipfeature = g_new0 (t_chipfeature, 1);
            chipfeature->name = disk; /* /proc/ide/hda/model ?? */
            g_ptr_array_add (chip->chip_features, chipfeature);
            chip->num_features++;
        }
    }

    TRACE ("leaves read_disks_linux26");
}


void
remove_unmonitored_drives (t_chip *chip)
{
    int i;
    t_chipfeature *chipfeature;

    TRACE ("enters remove_unmonitored_drives");

    for (i=0; i<chip->num_features; i++) {
        chipfeature = g_ptr_array_index (chip->chip_features, i);
        if ( get_hddtemp_value (chipfeature->name)==0.0) {
            g_ptr_array_remove_index (chip->chip_features, i);
            i--;
            chip->num_features--;
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
       g_assert  (chipfeature!=NULL);

       chipfeature->address = diskIndex;

       chipfeature->color = "#B000B0";
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
initialize_hddtemp (GPtrArray *chips)
{
    int generation, major, result, retval;
    struct utsname *p_uname;
    t_chip *chip;

    g_assert (chips!=NULL);

    TRACE ("enters initialize_hddtemp");

    chip = g_new (t_chip, 1);

    chip->chip_name = (const sensors_chip_name *)
            ( g_strdup(_("Hard disks")), 0, 0, g_strdup(_("Hard disks")) );

    chip->chip_features = g_ptr_array_new ();
    chip->num_features = 0;
    chip->description = g_strdup (_("S.M.A.R.T. harddisk temperatures"));
    chip->name = g_strdup (_("Hard disks"));
    chip->type = HDD;

    p_uname = (struct utsname *) malloc (sizeof(struct utsname));
    result =  uname (p_uname);
    if (result!=0)
        return -1;

    generation = atoi ( p_uname->release ); /* this might cause trouble on */
    major = atoi ( p_uname->release+2 );      /* other systems than Linux! */
                /* actually, wanted to use build time configuration therefore /*

    /* Note: This is actually supposed to be carried out by ifdef HAVE_LINUX
     and major/minor number stuff from compile time*/
    if (strcmp(p_uname->sysname, "Linux")==0 && major>=5)
        read_disks_linux26 (chip);
    else
        read_disks_linux24 (chip); /* hopefully, that's a safe variant */

    remove_unmonitored_drives (chip);

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
get_hddtemp_value (char* disk)
{
    gchar *standard_output, *standard_error, *cmd_line;
    gint exit_status=0;
    double value;
    gboolean result;

    TRACE ("enters get_hddtemp_value for %s", disk);

    /*    FIXME: On self-installed systems, this is /usr/local/sbin!    */
    cmd_line = g_strdup_printf ( "/usr/sbin/hddtemp -n -q %s", disk);
    result = g_spawn_command_line_sync (cmd_line,
            &standard_output, &standard_error, &exit_status, NULL);

    /* filter those with no sensors out */
    if (!result || exit_status!=0 /* || error!=NULL */ ) {
        return 0.0;
    }

    /* hddtemp does not return floating values, but only integer ones.
      So have an easier life with atoi. */
    value = (double) (atoi ( (const char*) standard_output) );

    g_free (cmd_line);
    g_free (standard_output);
    g_free (standard_error);

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

    value = get_hddtemp_value (cf->name);

    cf->formatted_value = g_strdup_printf("%+5.2f", value);
    cf->raw_value = value;

    TRACE ("leaves refresh_hddtemp");
}
