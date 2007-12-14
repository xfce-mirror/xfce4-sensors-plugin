
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


int sensors_get_feature_wrapper (const sensors_chip_name *name, int number, double *value)
{
    #if SENSORS_API_VERSION < 0x400 /* libsensors3 */
        return sensors_get_feature (*name, number, value);
    #else
        return sensors_get_value (name, number, value);
    #endif
}


t_chip *setup_chip (GPtrArray *chips, const sensors_chip_name *name, int num_sensorchips)
{
    t_chip* chip;

    TRACE ("enters setup_chip");

    chip = g_new0 (t_chip, 1);

    g_ptr_array_add (chips, chip);

    chip->chip_name = (sensors_chip_name *) g_malloc (sizeof(sensors_chip_name));
    memcpy ( (void *) (chip->chip_name), (void *) name, sizeof(sensors_chip_name) );

    #if SENSORS_API_VERSION < 0x400 /* libsensor 3 code */
        chip->sensorId = g_strdup_printf ("%s-%x-%x", name->prefix, name->bus,
                                            name->addr);
    #else
        switch (name->bus.type) {
            case SENSORS_BUS_TYPE_I2C:
            case SENSORS_BUS_TYPE_SPI:
                chip->sensorId = g_strdup_printf ("%s-%x-%x", name->prefix,
                                                    name->bus.nr, name->addr);
                break;
            default:
                chip->sensorId = g_strdup_printf ("%s-%x", name->prefix,
                                                    name->addr);
        }
    #endif

    chip->num_features=0;
    chip->name = _("LM Sensors");
    chip->chip_features = g_ptr_array_new();

    #if SENSORS_API_VERSION < 0x400 /* libsensors3 */
        chip->description = g_strdup (
                                sensors_get_adapter_name (num_sensorchips-1));
    #else
        chip->description = g_strdup (sensors_get_adapter_name (&name->bus));
    #endif

    TRACE ("leaves setup_chip");

    return chip;
}


#if SENSORS_API_VERSION >= 0x400 /* libsensors4 */
void
categorize_sensor_type_libsensors4 (t_chipfeature *chipfeature,
                                    const sensors_feature *feature,
                                    const sensors_chip_name *name,
                                    int number)
{
    const sensors_subfeature *sub_feature = NULL;
    double sensorFeature;

    switch (feature->type) {
        case SENSORS_FEATURE_IN:
            chipfeature->class = VOLTAGE;
            chipfeature->min_value = 1.0;
            chipfeature->max_value = 12.2;

            if ((sub_feature = sensors_get_subfeature (name, feature,
                    SENSORS_SUBFEATURE_IN_MIN)) &&
                    !sensors_get_value (name, number, &sensorFeature))
                chipfeature->min_value = sensorFeature;

            if ((sub_feature = sensors_get_subfeature (name, feature,
                    SENSORS_SUBFEATURE_IN_MAX)) &&
                    !sensors_get_value (name, number, &sensorFeature))
                chipfeature->max_value = sensorFeature;

            break;

        case SENSORS_FEATURE_FAN:
            chipfeature->class = SPEED;
            chipfeature->min_value = 1000.0;
            chipfeature->max_value = 3500.0;

            if ((sub_feature = sensors_get_subfeature (name, feature,
                    SENSORS_SUBFEATURE_FAN_MIN)) &&
                    !sensors_get_value (name, number, &sensorFeature))
                chipfeature->min_value = sensorFeature;

            break;

        case SENSORS_FEATURE_TEMP:
            chipfeature->class = TEMPERATURE;
            chipfeature->min_value = 0.0;
            chipfeature->max_value = 80.0;

            if ((sub_feature = sensors_get_subfeature (name, feature,
                    SENSORS_SUBFEATURE_TEMP_MIN)) &&
                    !sensors_get_value (name, number, &sensorFeature))
                chipfeature->min_value = sensorFeature;

            if (((sub_feature = sensors_get_subfeature (name, feature,
                    SENSORS_SUBFEATURE_TEMP_MAX)) ||
                    (sub_feature = sensors_get_subfeature (name, feature,
                    SENSORS_SUBFEATURE_TEMP_CRIT))) &&
                    !sensors_get_value (name, number, &sensorFeature))
                chipfeature->max_value = sensorFeature;
            break;

        case SENSORS_FEATURE_VID:
            chipfeature->class = VOLTAGE;
            chipfeature->min_value = 1.0;
            chipfeature->max_value = 3.5;
            break;

        case SENSORS_FEATURE_BEEP_ENABLE:
            chipfeature->class = STATE;
            chipfeature->min_value = 1.0;
            chipfeature->max_value = 3.5;
            break;

        default: /* UNKNOWN */
            chipfeature->class = OTHER;
            chipfeature->min_value = 0.0;
            chipfeature->max_value = 7000.0;
    }
}
#endif


void setup_chipfeature_common (t_chipfeature *chipfeature, int number,
                               double sensorFeature)
{
    chipfeature->color = "#00B000";
    chipfeature->valid = TRUE;

    chipfeature->raw_value = sensorFeature;
    chipfeature->address = number;
    chipfeature->show = FALSE;
}


#if SENSORS_API_VERSION < 0x400 /* libsensors3 */
void setup_chipfeature (t_chipfeature *chipfeature, int number,
                        double sensorFeature)
{
    TRACE ("enters setup_chipfeature");

    setup_chipfeature_common (chipfeature, number, sensorFeature);

    /* g_free (chipfeature->formatted_value);
    chipfeature->formatted_value = g_strdup_printf ("%+5.1f", sensorFeature); */

    categorize_sensor_type (chipfeature);

    TRACE ("leaves setup_chipfeature");
}
#else
void setup_chipfeature_libsensors4 (t_chipfeature *chipfeature,
                                    const sensors_feature *feature, int number,
                                    double sensorFeature,
                                    const sensors_chip_name *name)
{

    setup_chipfeature_common (chipfeature, number, sensorFeature);

    categorize_sensor_type_libsensors4 (chipfeature, feature, name, number);
}
#endif


#if SENSORS_API_VERSION < 0x400 /* libsensors3 */
t_chipfeature *find_chipfeature (const sensors_chip_name *name, t_chip *chip,
                                 int number)
{
    int res;
    double sensorFeature;
    t_chipfeature *chipfeature;

    TRACE ("enters find_chipfeature");

    chipfeature = g_new0 (t_chipfeature, 1);

    if (sensors_get_ignored (*(name), number)==1) {
        g_free (chipfeature->name); /*  ?  */
        res = sensors_get_label (*(name), number, &(chipfeature->name));

        if (res==0) {
            res = sensors_get_feature (*(name), number, &sensorFeature);

            if (res==0) {
                setup_chipfeature (chipfeature, number, sensorFeature);
                chip->num_features++;
                TRACE("leaves find_chipfeature");
                return chipfeature;
            }
        }
    }

    g_free (chipfeature);
    TRACE ("leaves find_chipfeature with null");
    return NULL;
}
#else
t_chipfeature *find_chipfeature (const sensors_chip_name *name, t_chip *chip,
                                 const sensors_feature *feature)
{
    const sensors_subfeature *sub_feature = NULL;
    int res, number = -1;
    double sensorFeature;
    t_chipfeature *chipfeature;

    TRACE ("enters find_chipfeature");

    switch (feature->type) {
        case SENSORS_FEATURE_IN:
            sub_feature = sensors_get_subfeature (name, feature,
                                                  SENSORS_SUBFEATURE_IN_INPUT);
            break;
        case SENSORS_FEATURE_FAN:
            sub_feature = sensors_get_subfeature (name, feature,
                                                 SENSORS_SUBFEATURE_FAN_INPUT);
            break;
        case SENSORS_FEATURE_TEMP:
            sub_feature = sensors_get_subfeature (name, feature,
                                                 SENSORS_SUBFEATURE_TEMP_INPUT);
            break;
        case SENSORS_FEATURE_VID:
            sub_feature = sensors_get_subfeature (name, feature,
                                                 SENSORS_SUBFEATURE_VID);
            break;
        case SENSORS_FEATURE_BEEP_ENABLE:
            sub_feature = sensors_get_subfeature (name, feature,
                                                 SENSORS_SUBFEATURE_BEEP_ENABLE);
            break;
        default:
            sub_feature = sensors_get_subfeature (name, feature,
                                                  SENSORS_SUBFEATURE_UNKNOWN);
    }
    if (sub_feature)
        number = sub_feature->number;

    if (number==-1)
        return NULL;

    chipfeature = g_new0 (t_chipfeature, 1);

    chipfeature->name = sensors_get_label (name, feature);

    if (!chipfeature->name)
        chipfeature->name = feature->name;

    if (chipfeature->name)
    {
        res = sensors_get_value (name, number, &sensorFeature);
        if (res==0)
        {
            setup_chipfeature_libsensors4 (chipfeature, feature, number,
                                           sensorFeature, name);
            chip->num_features++;
            TRACE("leaves find_chipfeature");
            return chipfeature;
        }
    }

    g_free(chipfeature);

    TRACE("leaves find_chipfeature with null");
    return NULL;
}
#endif


int initialize_libsensors (GPtrArray *chips)
{
    int sensorsInit, nr1, nr2, num_sensorchips; /*    , numchips;  */
    t_chip *chip;
    t_chipfeature *chipfeature; /* , *furtherchipfeature; */
    const sensors_chip_name *detected_chip;
#if SENSORS_API_VERSION < 0x400 /* libsensors3 */
    FILE *file;
    const sensors_feature_data *sfd;

    TRACE("enters initialize_libsensors");

    errno = 0;
    file = fopen("/etc/sensors.conf", "r");

    if (errno != ENOENT) /* the file actually exists */
    {
        sensorsInit = sensors_init (file);
        if (sensorsInit != 0)
        {
            g_printf(_("Error: Could not connect to sensors!"));
            /* FIXME: better popup window? write to special logfile? */
            fclose (file);
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
#else
    const sensors_feature *sfd;
    TRACE("enters initialize_libsensors");

    sensorsInit = sensors_init (NULL);
    if (sensorsInit != 0)
    {
        g_printf(_("Error: Could not connect to sensors!"));
        /* FIXME: better popup window? write to special logfile? */
        return -2;
    }

    num_sensorchips = 0;
    detected_chip = sensors_get_detected_chips (NULL, &num_sensorchips);
    /* iterate over chips on mainboard */
    while (detected_chip!=NULL)
    {
        chip = setup_chip (chips, detected_chip, num_sensorchips);

        nr1 = 0;
        nr2 = 0;
        /* iterate over chip features, i.e. id, cpu temp, mb temp... */
        /* numchips = get_number_chip_features (detected_chip); */
        sfd = sensors_get_features (detected_chip, &nr1);
        while (sfd != NULL)
        {
            chipfeature = find_chipfeature (detected_chip, chip, sfd);
            if (chipfeature!=NULL) {
                g_ptr_array_add (chip->chip_features, chipfeature);
            }
            sfd = sensors_get_features (detected_chip, &nr1);
        }

        detected_chip = sensors_get_detected_chips (NULL, &num_sensorchips);
    } /* end while sensor chipNames */

    TRACE ("leaves initialize_libsensors with 1");
    return 1;
#endif
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

void
free_lmsensors_chip (gpointer chip)
{
    t_chip *c;

    c = (t_chip *) chip;

    g_free (c->name);
    g_free (c->chip_name->prefix);

    #if SENSORS_API_VERSION < 0x400
        g_free (c->chip_name->busname);
    #endif

}
