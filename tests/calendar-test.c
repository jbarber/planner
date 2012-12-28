#include <config.h>
#include <locale.h>
#include <string.h>
#include <stdlib.h>
#include "libplanner/mrp-project.h"
#include "libplanner/mrp-calendar.h"
#include "self-check.h"

gint
main (gint argc, gchar **argv)
{
	MrpApplication *app;
	MrpProject     *project;
        MrpCalendar    *base, *derive, *copy;
        MrpInterval    *interval;
        mrptime         time_tue, time_sat, time_sun, time_27nov, time_28nov;
        MrpDay         *day_a, *day_b, *day_c, *def_1_id;
        GList          *l = NULL;

        g_type_init ();

	app = mrp_application_new ();

	project = mrp_project_new (NULL);

        base = mrp_calendar_new ("Base", project);

        interval = mrp_interval_new (0, 10);
        l = g_list_prepend (l, interval);
        interval = mrp_interval_new (40, 120);
        l = g_list_prepend (l, interval);
        interval = mrp_interval_new (60, 40);
        l = g_list_prepend (l, interval);

        mrp_calendar_day_set_intervals (base, mrp_day_get_work (), l);

        /* Setup default week, we might want do this in libmrproject itself? */
        mrp_calendar_set_default_days (base,
                                       MRP_CALENDAR_DAY_MON, mrp_day_get_work (),
                                       MRP_CALENDAR_DAY_TUE, mrp_day_get_work (),
                                       MRP_CALENDAR_DAY_WED, mrp_day_get_work (),
                                       MRP_CALENDAR_DAY_THU, mrp_day_get_work (),
                                       MRP_CALENDAR_DAY_FRI, mrp_day_get_work (),
                                       MRP_CALENDAR_DAY_SAT, mrp_day_get_nonwork (),
                                       MRP_CALENDAR_DAY_SUN, mrp_day_get_nonwork (),
                                       -1);

        /* Derive two calendars and copy one */
        derive = mrp_calendar_derive ("Derive", base);
        copy   = mrp_calendar_copy ("Copy", base);

        /* Get some test dates */
        time_tue   = mrp_time_from_string ("20021112", NULL);
        time_sat   = mrp_time_from_string ("20021116", NULL);
        time_sun   = mrp_time_from_string ("20021117", NULL);
        time_27nov = mrp_time_from_string ("20021127", NULL);
        time_28nov = mrp_time_from_string ("20021128", NULL);

        /****************************************************/
        /** Check one: Base calendars default week         **/
        /****************************************************/
        day_a = mrp_calendar_get_day (base, time_tue, TRUE);
        day_b = mrp_calendar_get_day (base, time_sat, TRUE);
        CHECK_INTEGER_RESULT (mrp_day_get_id (day_a),
                              mrp_day_get_id (mrp_day_get_work ()));
        CHECK_INTEGER_RESULT (mrp_day_get_id (day_b),
                              mrp_day_get_id (mrp_day_get_nonwork ()));

        /****************************************************/
        /** Check two: Copied calendars default week       **/
        /****************************************************/
        day_a = mrp_calendar_get_day (copy, time_tue, TRUE);
        day_b = mrp_calendar_get_day (copy, time_sat, TRUE);
        day_c = mrp_calendar_get_day (copy, time_sun, TRUE);
        CHECK_INTEGER_RESULT (mrp_day_get_id (day_a),
                              mrp_day_get_id (mrp_day_get_work ()));
        CHECK_INTEGER_RESULT (mrp_day_get_id (day_b),
                              mrp_day_get_id (mrp_day_get_nonwork ()));
        CHECK_INTEGER_RESULT (mrp_day_get_id (day_c),
                              mrp_day_get_id (mrp_day_get_nonwork ()));

        /****************************************************/
        /** Check three: Own defined day type              **/
        /****************************************************/
        def_1_id = mrp_day_add (NULL, "Define 1", "Own defined day");

        /* Reset the values in the copied calendar */
        mrp_calendar_set_default_days (copy,
                                       MRP_CALENDAR_DAY_TUE, def_1_id,
                                       MRP_CALENDAR_DAY_FRI, def_1_id,
                                       MRP_CALENDAR_DAY_SUN, def_1_id,
                                       -1);

        /* Check that it worked fine with our own defined value */
        day_a = mrp_calendar_get_day (copy, time_sat, TRUE);
        day_b = mrp_calendar_get_day (copy, time_sun, TRUE);
        CHECK_INTEGER_RESULT (mrp_day_get_id (day_a),
                              mrp_day_get_id (mrp_day_get_nonwork ()));
        CHECK_INTEGER_RESULT (mrp_day_get_id (day_b),
                              mrp_day_get_id (def_1_id));

        /****************************************************/
        /** Check four: Derived and overridden value       **/
        /****************************************************/
        mrp_calendar_set_default_days (derive,
                                       MRP_CALENDAR_DAY_TUE, def_1_id,
                                       -1);
        day_a = mrp_calendar_get_day (base, time_tue, TRUE);
        day_b = mrp_calendar_get_day (derive, time_tue, TRUE);
        CHECK_INTEGER_RESULT (mrp_day_get_id (day_a),
                              mrp_day_get_id (mrp_day_get_work ()));
        CHECK_INTEGER_RESULT (mrp_day_get_id (day_b),
                              mrp_day_get_id (def_1_id));

        /****************************************************/
        /** Check five: Using base calendars values        **/
        /****************************************************/

        /* Set the base calendar day type on Tuesdays to be nonwork
           and set the derived calendars type to be USE_BASE */
        mrp_calendar_set_default_days (base,
                                       MRP_CALENDAR_DAY_TUE, mrp_day_get_nonwork (),
                                       -1);
        mrp_calendar_set_default_days (derive,
                                       MRP_CALENDAR_DAY_TUE, mrp_day_get_use_base (),
                                       -1);

        /* Make sure that the returned values match */
        day_a = mrp_calendar_get_day (base, time_tue, TRUE);
        day_b = mrp_calendar_get_day (derive, time_tue, TRUE);
        CHECK_INTEGER_RESULT (mrp_day_get_id (day_b), mrp_day_get_id (day_a));

        /****************************************************/
        /** Check six: Check calendar names                **/
        /****************************************************/
        CHECK_STRING_RESULT ((char *)mrp_calendar_get_name (base), "Base");
        CHECK_STRING_RESULT ((char *)mrp_calendar_get_name (derive), "Derive");
        CHECK_STRING_RESULT ((char *)mrp_calendar_get_name (copy), "Copy");

        /******************************************************/
        /** Check seven: Overriding days in derived calendar **/
        /******************************************************/
        mrp_calendar_set_days (base,
                               time_27nov, def_1_id,
                               (mrptime) -1);
        mrp_calendar_set_days (derive,
                               time_28nov, mrp_day_get_nonwork (),
                               (mrptime) -1);

        day_a = mrp_calendar_get_day (base,   time_27nov, TRUE);
        day_b = mrp_calendar_get_day (derive, time_27nov, TRUE);
        CHECK_INTEGER_RESULT (mrp_day_get_id (day_a),
                              mrp_day_get_id (def_1_id));
        CHECK_INTEGER_RESULT (mrp_day_get_id (day_b),
                              mrp_day_get_id (def_1_id));

        mrp_calendar_set_days (derive,
                               time_27nov, mrp_day_get_nonwork (),
                               (mrptime) -1);
        day_b = mrp_calendar_get_day (derive, time_27nov, TRUE);
        CHECK_INTEGER_RESULT (mrp_day_get_id (day_b),
                              mrp_day_get_id (mrp_day_get_nonwork ()));

        mrp_calendar_set_days (derive,
                               time_27nov, mrp_day_get_use_base (),
                               (mrptime) -1);
        day_b = mrp_calendar_get_day (derive, time_27nov, TRUE);
        CHECK_INTEGER_RESULT (mrp_day_get_id (day_a),
                              mrp_day_get_id (day_b));

	g_object_unref (app);
	return EXIT_SUCCESS;
}

