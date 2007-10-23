
#include "lmsensors.h"
#include "middlelayer.h"
#include "types.h"

#include <errno.h>

#include <glib/garray.h>
#include <glib/gmessages.h>
#include <glib/gmem.h>
#include <glib/gprintf.h>
#include <glib/gstrfuncs.h>

#include <stdio.h>
#include <string.h>

/* Unused
int get_number_chip_features (const sensors_chip_name *name)
{
    int nr1 = 0, nr2 = 0, numer = 0;
    const sensors_feature_data *sfd;

    TRACE("enters get_number_chip_features");

    do {
        sfd = sensors_get_all_features (*name, &nr1, &nr2);
        if (sfd!=NULL)
            number++;
    } while (sfd!=NULL);

    TRACE("leaves get_number_chip_features");

    return number-1;
} */


t_chip *setup_chip (GPtrArray *chips, const sensors_chip_name *name, int num_sensorchips)
{
    t_chip* chip;

    TRACE("enters setup_chip");

    chip = g_new0 (t_chip, 1);

    g_ptr_array_add (chips, chip);

    chip->chip_name = (sensors_chip_name *) g_malloc (sizeof(sensors_chip_name));
    memcpy ( (void *) (chip->chip_name), (void *) name, sizeof(sensors_chip_name) );

    chip->sensorId = g_strdup_printf ("%s-%x-%x", name->prefix, name->bus, name->addr);
    chip->num_features=0;
    chip->name = g_strdup (_("LM Sensors"));
    chip->chip_features = g_ptr_array_new();

    chip->description = g_strdup (sensors_get_adapter_name (num_sensorchips-1));

    TRACE("leaves setup_chip");

    return chip;
}


void setup_chipfeature (t_chipfeature *chipfeature, int number, double sensorFeature)
{
    TRACE("enters setup_chipfeature");

    chipfeature->color = "#00B000";
    chipfeature->valid = TRUE;
    chipfeature->formatted_value = g_strdup_printf("%+5.1f", sensorFeature);
    chipfeature->raw_value = sensorFeature;
    chipfeature->address = number;
    chipfeature->show = FALSE;

    categorize_sensor_type (chipfeature);

    TRACE("leaves setup_chipfeature");
}


t_chipfeature *find_chipfeature    (const sensors_chip_name *name, t_chip *chip, int number)
{
    int res;
    double sensorFeature;
    t_chipfeature *chipfeature;

    TRACE("enters find_chipfeature");

    chipfeature = g_new0 (t_chipfeature, 1);

    if (sensors_get_ignored (*(name), number)==1) {
        res = sensors_get_label(*(name), number,
                                &(chipfeature->name));

        if (res==0) {
            res = sensors_get_feature (*(name), number,
                                        &sensorFeature);

            if (res==0) {
                setup_chipfeature (chipfeature, number, sensorFeature);
                chip->num_features++;
                TRACE("leaves find_chipfeature");
                return chipfeature;
            }
        }
    }

    TRACE("leaves find_chipfeature with null");
    return NULL;
}


int initialize_libsensors (GPtrArray *chips)
{
    int sensorsInit, nr1, nr2, num_sensorchips; /*    , numchips;  */
    t_chip *chip;
    t_chipfeature *chipfeature; /* , *furtherchipfeature; */
    FILE *file;
    const sensors_chip_name *detected_chip;
    const sensors_feature_data *sfd;

    TRACE("enters initialize_libsensors");

    errno = 0;
    file = fopen("/etc/sensors.conf", "r");

    if (errno != ENOENT) /* the file actually exists */
    {
        sensorsInit = sensors_init(file);
        if (sensorsInit != 0)
        {
            g_printf(_("Error: Could not connect to sensors!"));
            /* FIXME: better popup window? write to special logfile? */
            return -2;
        }

        num_sensorchips = 0;
        detected_chip = sensors_get_detected_chips ( &num_sensorchips);

        /* iterate over chips on mainboard */
        while (detected_chip!=NULL)
        {
            chip = setup_chip (chips, detected_chip, num_sensorchips);

            nr1 = 0;
            nr2 = 0;
            /* iterate over chip features, i.e. id, cpu temp, mb temp... */
            /* numchips = get_number_chip_features (detected_chip); */
            sfd = sensors_get_all_features (*detected_chip, &nr1, &nr2);
            while (sfd != NULL)
            {
                chipfeature = find_chipfeature (detected_chip, chip, sfd->number);
                if (chipfeature!=NULL) {
                    g_ptr_array_add (chip->chip_features, chipfeature);
                }
                sfd = sensors_get_all_features (*detected_chip, &nr1, &nr2);
            }

            detected_chip = sensors_get_detected_chips (&num_sensorchips);
        } /* end while sensor chipNames */

        fclose (file);
        TRACE ("leaves initialize_libsensors with 1");
        return 1;
    }
    else {
        fclose (file);
        TRACE ("leaves initialize_libsensors with -1");
        return -1;
    }
}


void
refresh_lmsensors (gpointer chip_feature, gpointer data)
{
    t_chipfeature *cf;

    TRACE ("leaves refresh_lmsensors");

    g_assert(chip_feature!=NULL);

    cf = (t_chipfeature *) chip_feature;

    TRACE ("leaves refresh_lmsensors");
}
