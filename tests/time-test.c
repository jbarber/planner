#include <config.h>
#include <locale.h>
#include <string.h>
#include <stdlib.h>
#include "libplanner/mrp-time.h"
#include "self-check.h"

/* Helper function to make the output look nicer */
static gchar *
STRING (mrptime time)
{
 	return mrp_time_to_string (time);
}

gint
main (gint argc, gchar **argv)
{
	mrptime t;

	setlocale (LC_ALL, "");

	t = mrp_time_compose (2002, 3, 31, 0, 0, 0);

	/* Test mrp_time_new_from_string */
	CHECK_STRING_RESULT (STRING (mrp_time_from_string ("20020329", NULL)), "20020329T000000Z");
	CHECK_STRING_RESULT (STRING (mrp_time_from_string ("19991231", NULL)), "19991231T000000Z");
	CHECK_STRING_RESULT (STRING (mrp_time_from_string ("invalid", NULL)), "19700101T000000Z");

	/* Test mrp_time_new_from_msdate_string */
	CHECK_STRING_RESULT (STRING (mrp_time_from_msdate_string ("Wed 1/1/97")), "19970101T000000Z");
	CHECK_STRING_RESULT (STRING (mrp_time_from_msdate_string ("Thu 2/5/98")), "19980205T000000Z");
	CHECK_STRING_RESULT (STRING (mrp_time_from_msdate_string ("Fri 3/29/02")), "20020329T000000Z");
	CHECK_STRING_RESULT (STRING (mrp_time_from_msdate_string ("Nov 15 '97")), "19971115T000000Z");
	CHECK_STRING_RESULT (STRING (mrp_time_from_msdate_string ("Nov 15 '02")), "20021115T000000Z");
	CHECK_STRING_RESULT (STRING (mrp_time_from_msdate_string ("Feb 5 '02")), "20020205T000000Z");
	CHECK_STRING_RESULT (STRING (mrp_time_from_msdate_string ("Feb 5 1997")), "19970205T000000Z");

	return EXIT_SUCCESS;
}

