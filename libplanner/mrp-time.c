/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2002-2003 CodeFactory AB
 * Copyright (C) 2002-2003 Richard Hult <richard@imendio.com>
 * Copyright (C) 2002 Mikael Hallendal <micke@imendio.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <config.h>
#include <glib-object.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <langinfo.h>
#include "mrp-time.h"
#include "mrp-types.h"
#include <glib/gi18n.h>
#include "mrp-private.h"

static const gchar *short_month_names[12];
static const gchar *month_names[12];

static const gchar *month_names_initial[12];
static const gchar *short_day_names[7];
static const gchar *day_names[7];

/**
 * mrp_time_compose:
 * @year: the year
 * @month: the month
 * @day: the day
 * @hour: the hour
 * @minute: the minute
 * @second: the second
 * 
 * Composes an #mrptime value from the separate components.
 * 
 * Return value: An #mrptime value.
 **/
mrptime
mrp_time_compose (gint year,
		  gint month,
		  gint day,
		  gint hour,
		  gint minute,
		  gint second)
{
	struct tm tm;
	
	memset (&tm, 0, sizeof (struct tm));
	
	tm.tm_year  = year - 1900;
	tm.tm_mon   = month - 1;
	tm.tm_mday  = day;
	tm.tm_hour  = hour;
	tm.tm_min   = minute;
	tm.tm_sec   = second;
	tm.tm_isdst = -1;

	return mrp_time_from_tm (&tm);
}

/**
 * mrp_time_decompose:
 * @t: an #mrptime value to decompose
 * @year: location to store year, or %NULL
 * @month: location to store month, or %NULL
 * @day: location to store day, or %NULL
 * @hour: location to store hour, or %NULL
 * @minute: location to store minute, or %NULL
 * @second: location to store second, or %NULL
 * 
 * Splits up an #mrptime value into its components.
 * 
 * Return value: %TRUE on success.
 **/
gboolean
mrp_time_decompose (mrptime  t,
		    gint    *year,
		    gint    *month,
		    gint    *day,
		    gint    *hour,
		    gint    *minute,
		    gint    *second)
{
	struct tm *tm;
	time_t     tt;

	tt = t;
	tm = gmtime (&tt);

	if (tm == NULL) {
		return FALSE;
	}
	
	if (year) {
		*year = tm->tm_year + 1900;
	}
	if (month) {
		*month = tm->tm_mon + 1;
	}
	if (day) {
		*day = tm->tm_mday;
	}
	if (hour) {
		*hour = tm->tm_hour;
	}
	if (minute) {
		*minute = tm->tm_min;
	}
	if (second) {
		*second = tm->tm_sec;
	}

	return TRUE;
}	    

/**
 * mrp_time_debug_print:
 * @t: an  #mrptime
 * 
 * Prints the time on stdout, for debugging purposes.
 **/
void
mrp_time_debug_print (mrptime t)
{
	struct tm *tm;
	time_t     tt;

	tt = t;

	tm = gmtime (&t);

	if (tm == NULL) {
		g_print ("<Invalid time>\n");
		return;
	}

	g_print ("%04d-%02d-%02d %s %02d:%02d:%02d\n",
		 tm->tm_year + 1900,
		 tm->tm_mon + 1,
		 tm->tm_mday,
		 short_day_names[tm->tm_wday],
		 tm->tm_hour,
		 tm->tm_min,
		 tm->tm_sec);
}

/**
 * mrp_time_from_tm:
 * @tm: pointer to a struct tm time value
 * 
 * Converts a struct tm value to an #mrptime value.
 * 
 * Return value: #mrptime value.
 **/
mrptime
mrp_time_from_tm (struct tm *tm)
{
	gchar   *old_tz;
	gchar   *tmp;
	mrptime  t;

	/* This is a hack. Set the timezone to UTC temporarily. */
	old_tz = g_strdup (g_getenv ("TZ"));
	putenv ("TZ=UTC");
	
	t = mktime (tm);

	/* And set it back. */
	if (old_tz != NULL && old_tz[0] != '\0') {
		tmp = g_strconcat ("TZ=", old_tz, NULL);
		putenv (tmp);
		g_free (tmp);
	} else {
		unsetenv ("TZ");
	}

	g_free (old_tz);
	
	return t;
}

/**
 * mrp_time_current_time:
 * 
 * Retrieves the current time as an #mrptime value.
 *
 * Return value: Current time.
 **/
mrptime
mrp_time_current_time (void)
{
	mrptime    t;
	time_t     tt;
	struct tm *tm;

	tt = time (NULL);
	tm = localtime (&tt);
	t = mrp_time_from_tm (tm);

	return t;
}

/**
 * mrp_time_to_tm:
 * @t: an #mrptime value
 * 
 * Converts @t to a struct tm value.
 * 
 * Return value: struct tm time, which is static data and should not be
 * modified or freed..
 **/
struct tm *
mrp_time_to_tm (mrptime t)
{
	time_t tt;

	tt = t;
	
	return gmtime (&tt);
}

/**
 * mrp_time_from_string:
 * @str: a string with a time, ISO8601 format
 * @err: Location to store error, or %NULL
 * 
 * Parses an ISO8601 time string and converts it to an #mrptime.
 * 
 * Return value: Converted time value.
 **/
mrptime
mrp_time_from_string (const gchar  *str,
		      GError      **err)
{
	gint     len;
	gboolean is_utc;
	gboolean is_date;
	gint     year;
	gint     month;
	gint     day;
	gint     hour = 0;
	gint     minute = 0;
	gint     second = 0;

	len = strlen (str);

	if (len == 15) { /* floating time */
		is_utc = FALSE;
		is_date = FALSE;
	} else if (len == 16) { /* UTC time, ends in 'Z' */
		is_utc = TRUE;
		is_date = FALSE;
		
		if (str[15] != 'Z') {
			/*g_set_error (err,
				     GQuark domain,
				     gint code,
				     const gchar *format,
				     ...);*/

			return 0;
		}
	}
	else if (len == 8) { /* A date. */
		is_utc = TRUE;
		is_date = TRUE;
	} else {
		/*g_set_error (err,
		  GQuark domain,
		  gint code,
		  const gchar *format,
		  ...);*/

		return 0;
	}
	
	if (is_date) {
		sscanf (str, "%04d%02d%02d", &year, &month, &day);
	} else {
		gchar tsep;
		
		sscanf (str,"%04d%02d%02d%c%02d%02d%02d",
			&year, &month, &day,
			&tsep,
			&hour, &minute, &second);
		
		if (tsep != 'T') {
			/*g_set_error (err,
			  GQuark domain,
			  gint code,
			  const gchar *format,
			  ...);*/
			return 0;
		}
		
	}

	/* FIXME: If we want to support reading times other than in UTC,
	 * implement that here.
	 */
	
	return mrp_time_compose (year,
				 month,
				 day,
				 hour,
				 minute,
				 second);
}

/**
 * mrp_time_to_string:
 * @t: an #mrptime time
 * 
 * Converts a time value to an ISO8601 string.
 * 
 * Return value: Allocated string that needs to be freed.
 **/
gchar *
mrp_time_to_string (mrptime t)
{
	struct tm *tm;

	tm = mrp_time_to_tm (t);
	
	return g_strdup_printf ("%04d%02d%02dT%02d%02d%02dZ",
				tm->tm_year + 1900,
				tm->tm_mon + 1,
				tm->tm_mday,
				tm->tm_hour,
				tm->tm_min,
				tm->tm_sec);
}

/**
 * mrp_time_align_day:
 * @t: an #mrptime value
 * 
 * Aligns a time value to the start of the day.
 * 
 * Return value: Aligned value.
 **/
mrptime
mrp_time_align_day (mrptime t)
{
	struct tm *tm;

	tm = mrp_time_to_tm (t);
	tm->tm_hour = 0;
	tm->tm_min = 0;
	tm->tm_sec = 0;

	return mrp_time_from_tm (tm);
}

/**
 * mrp_time_day_of_week:
 * @t: an #mrptime value
 * 
 * Retrieves the day of week of the specified time.
 * 
 * Return value: The day of week, in the range 0 to6, where Sunday is 0.
 **/
gint
mrp_time_day_of_week (mrptime t)
{
	struct tm *tm;

	tm = mrp_time_to_tm (t);
	
	return tm->tm_wday;
}

/**
 * mrp_time_week_number:
 * @t: an #mrptime value
 * 
 * Retrieves the week number of the specified time.
 * 
 * Return value: ISO standard week number.
 **/
gint
mrp_time_week_number (mrptime t)
{
	struct tm *tm;
	gchar      str[5];
	
	tm = mrp_time_to_tm (t);

	strftime (str, sizeof (str), "%V", tm);

	return atoi (str);
}

/**
 * mrp_param_spec_time:
 * @name: name of the property
 * @nick: nick for the propery
 * @blurb: blurb for the property
 * @flags: flags
 * 
 * Convenience function for creating a #GParamSpec carrying an #mrptime value.
 * 
 * Return value: Newly created #GparamSpec.
 **/
GParamSpec *
mrp_param_spec_time (const gchar *name,
		     const gchar *nick,
		     const gchar *blurb,
		     GParamFlags flags)
{
	return g_param_spec_long (name,
				  nick,
				  blurb,
				  MRP_TIME_MIN, MRP_TIME_MAX, MRP_TIME_MIN,
				  flags);
}


/*
 * Pass in 4/16/97 and get 19970416 out.
 * Lets hope the ms dates are y2k compliant.
 */
static char *
time_convert_slashed_us_date_to_iso (const char *date)
{
	char  scratch[9]; /* yyyymmdd */
	int   i;

	i = 0; 

	g_assert (date [i] != '\0');
	g_assert (date [i + 1] != '\0');

	/* Month */
	if (date [i + 1] == '/') {
		scratch [4] = '0';
		scratch [5] = date [i];
		i+=2;
	} else {
		g_assert (date [i + 2] == '/');
		scratch [4] = date [i];
		scratch [5] = date [i + 1];
		i+=3;
	}

	g_assert (date [i] != '\0');
	g_assert (date [i + 1] != '\0');

	/* Day */
	if (date [i + 1] == '/') {
		scratch [6] = '0';
		scratch [7] = date [i];
		i+=2;
	} else {
		g_assert (date [i + 2] == '/');
		scratch [6] = date [i];
		scratch [7] = date [i + 1];
		i+=3;
	}

	g_assert (date [i] != '\0');
	g_assert (date [i + 1] != '\0');

	/* Year */
	if (date [i + 2] == '\0') {
		/* And here we have the ugly [ like my butt ] Y2K hack
		   God bless all those who live to see 2090 */
		if (date [i] >= '9') {
			scratch [0] = '1';
			scratch [1] = '9';
		} else {
			scratch [0] = '2';
			scratch [1] = '0';
		}
		scratch [2] = date [i];
		scratch [3] = date [i + 1];
	} else { /* assume 4 digit */
		g_assert (date [i + 3] != '\0');
		scratch [0] = date [i];
		scratch [1] = date [i + 1];
		scratch [2] = date [i + 2];
		scratch [3] = date [i + 3];
	}

	scratch [8] = '\0';

	return g_strdup (scratch);
}

static const gchar *ms_day_names[] = {
	"Mon",
	"Tue",
	"Wed",
	"Thu",
	"Fri",
	"Sat",
	"Sun"
};

static const gchar *ms_month_names[] = {
	"Jan",
	"Feb",
	"Mar",
	"Apr",
	"May",
	"Jun",
	"Jul",
	"Aug",
	"Sep",
	"Oct",
	"Nov",
	"Dec"
};

/**
 * mrp_time_from_msdate_string:
 * @str: Date/time string formatted as in MS Project
 * 
 * Converts an MS date string to an #mrptime value.
 * 
 * Return value: Converted time value.
 **/
mrptime
mrp_time_from_msdate_string (const gchar *str)
{
	/* FIXME: horrible hacks lurk here */
	mrptime  ret;	
	gboolean contains_slash;
	gboolean has_day_prefix;
	gint     i;

	has_day_prefix = FALSE;
	for (i = 0; i < 7; i++) {
		if (!strncmp (str, ms_day_names[i], 3)) {
			has_day_prefix = TRUE;
			break;
		}
	}
       
	contains_slash = (strstr (str, "/") != NULL);

	if (contains_slash && has_day_prefix) {
		gchar *date;

		g_assert (str[3] == ' ');

		date = time_convert_slashed_us_date_to_iso (&str[4]);

		ret = mrp_time_from_string (date, NULL);

		g_free (date);

		return ret;
	} else {
		gboolean has_month_prefix = FALSE;
		char scratch[9]; /* yyyymmdd */
		const char *ptr = str;
		
		/* Try format of type "Nov 15 '97" */
		for (i = 0; i < 12; i++) {
			if (!strncmp (str, ms_month_names[i], 3)) {
				has_month_prefix = TRUE;
				i++; /* Vector starts from 0, month numbers don't */
				break;
			}
		}

		if (has_month_prefix) {
			scratch[8] = '\0';
			scratch[4] = i > 9 ? '1' : '0';
			scratch[5] = (i % 10) + '0';

			/* Take care of the month */
			ptr += 3;
			
			while (ptr[0] == ' ') 
				ptr++;

			
			/* Now for the day */
			if ((ptr[0] >= '0' && ptr[0] <= '9')) {
				if ((ptr[1] >= '0' && ptr[1] <= '9')) {
					/* We have a two-number day */
					scratch[6] = ptr[0];
					scratch[7] = ptr[1];
					ptr += 2;
				} else {
					scratch[6] = '0';
					scratch[7] = ptr[0];
					ptr += 1;
				}
			}

			/* And now the year */
			
			while (ptr[0] == ' ') 
				ptr++;

			if (ptr[0] == '\'') {
				ptr++;

				/* Y2K hack */
				if (ptr[0] >= '9') {
					scratch [0] = '1';
					scratch [1] = '9';
				} else {
					scratch [0] = '2';
					scratch [1] = '0';
				}
			} else {
				scratch[0] = ptr[0];
				scratch[1] = ptr[1];
				ptr += 2;
			}

			scratch[2] = ptr[0];
			scratch[3] = ptr[1];
			
			ret = mrp_time_from_string (scratch, NULL);

			return ret;
		}
	}

	g_warning ("Unknown MS date format '%s'", str);
	return MRP_TIME_INVALID;
}

/**
 * mrp_time_day_name:
 * @t: an #mrptime value
 * 
 * Retrieves the name of the day of the specified time. 
 * 
 * Return value: The day name, which is static data.
 **/
const gchar *
mrp_time_day_name (mrptime t)
{
	gint dow;

	g_return_val_if_fail (t > 0, NULL);

	dow = mrp_time_day_of_week (t);
	
	return short_day_names[dow];
}

/**
 * mrp_time_month_name:
 * @t: an #mrptime value
 * 
 * Retrieves the name of the month of the specified time.
 * 
 * Return value: The month name, which is static data.
 **/
const gchar *
mrp_time_month_name (mrptime t)
{
	struct tm *tm;

	g_return_val_if_fail (t > 0, NULL);

	tm = mrp_time_to_tm (t);
	
	return short_month_names[tm->tm_mon];
}

/**
 * mrp_time_month_name_initial:
 * @t: an #mrptime value
 * 
 * Retrieves the initial letter for the month of the specified time.
 * 
 * Return value: The initial, which is static data.
 **/
const gchar *
mrp_time_month_name_initial (mrptime t)
{
	struct tm *tm;

	g_return_val_if_fail (t > 0, NULL);

	tm = mrp_time_to_tm (t);
	
	return month_names_initial[tm->tm_mon];
}

/**
 * imrp_time_init:
 * 
 * Initializes the time functions. Must be called before using any mrp_
 * functions.
 **/
void
imrp_time_init (void)
{
	gint i;
	
	/* Get month and day names. */
	
	for (i = 0; i < 12; i++) {
		gunichar c;
		
		short_month_names[i] = g_locale_to_utf8 (nl_langinfo (ABMON_1 + i),
							 -1, NULL, NULL, NULL);
		month_names[i] = g_locale_to_utf8 (nl_langinfo (MON_1 + i),
						   -1, NULL, NULL, NULL);
		
		c = g_utf8_get_char (month_names[i]);
		month_names_initial[i] = g_malloc0 (7);
		g_unichar_to_utf8 (c, (char *)month_names_initial[i]);
		
	}

	for (i = 0; i < 7; i++) {
		short_day_names[i] = g_locale_to_utf8 (nl_langinfo (ABDAY_1 + i),
						       -1, NULL, NULL, NULL);
		
		day_names[i] = g_locale_to_utf8 (nl_langinfo (DAY_1 + i),
						 -1, NULL, NULL, NULL);
	}
}

static gint
time_format_helper (const gchar *format,
		    struct tm   *tm,
		    gchar       *buffer)
{
	gint  len = 0;
	gchar str[5];
  
	if (!format) {
		return 1;
	}
  
	while (*format) {
		register gchar c = *format++;
		register gint tmp;

		if (c != '%') {
			if (buffer) {
				buffer[len] = c;
			}
			
			len++;
			continue;
		}
	
		c = *format++;
		switch (c) {
		case 'a':
			/* The abbreviated weekday name (Mon, Tue, ...). */
			if (buffer) {
				strcpy (buffer + len, short_day_names[tm->tm_wday]);
			}
			len += strlen (short_day_names[tm->tm_wday]);
			break;
		case 'A':
			/* The full weekday name (Monday, Tuesday, ...). */
			tmp = tm->tm_wday;
			if (buffer) {
				strcpy (buffer + len, day_names[tmp]);
			}
			len += strlen (day_names[tmp]);
			break;
		case 'b':
			/* The abbreviated month name (Jan, Feb, ...). */
			tmp = tm->tm_mon;
			if (buffer) {
				strcpy (buffer + len, short_month_names[tmp]);
			}
			len += strlen (short_month_names[tmp]);
			break;
		case 'B':
			/* The full month name (January, February, ...). */
			tmp = tm->tm_mon;
			if (buffer) {
				strcpy (buffer + len, month_names[tmp]);
			}
			len += strlen (month_names[tmp]);
			break;
		case 'd':
			/* The day of the month (01 - 31). */
			if (buffer) {
				tmp = tm->tm_mday;

				buffer[len] = tmp / 10 + '0';
				buffer[len+1] = tmp - 10 * (tmp / 10) + '0';
			}
			len += 2;
			break;
		case 'e':
			/* The day of the month (1 - 31). */
			tmp = tm->tm_mday;
			if (buffer) {
				if (tmp > 9) {
					buffer[len] = tmp / 10 + '0';
					buffer[len+1] = tmp - 10 * (tmp / 10) + '0';
				} else {
					buffer[len] = tmp + '0';
				}
			}
			len += tmp > 9 ? 2 : 1;
			break;
		case 'H':
			/* The hour using a 24-hour clock (00 - 23). */
			if (buffer) {
				tmp = tm->tm_hour;
				
				buffer[len] = tmp / 10 + '0';
				buffer[len+1] = tmp - 10 * (tmp / 10) + '0';
			}
			len += 2;			
			break;
		case 'I':
			/* The hour using a 12-hour clock (01 - 12). */
			if (buffer) {
				tmp = tm->tm_hour % 12;

				if (tmp == 0) {
					tmp = 12;
				}
				
				buffer[len] = tmp / 10 + '0';
				buffer[len+1] = tmp - 10 * (tmp / 10) + '0';
			}
			len += 2;	
			break;
		case 'j':
			/* The day of the year (001 - 366). */
			g_warning ("%%j not implemented.");
			if (buffer) {
				buffer[len] = ' ';
				buffer[len+1] = ' ';
				buffer[len+2] = ' ';
			}
			len += 3;
			break;
		case 'k':
			/* The hour using a 24-hour clock (0 to 23). */
			tmp = tm->tm_hour;
			if (buffer) {
				if (tmp > 9) {
					buffer[len] = tmp / 10 + '0';
					buffer[len+1] = tmp - 10 * (tmp / 10) + '0';
				} else {
					buffer[len] = tmp + '0';
				}
			}
			len += tmp > 9 ? 2 : 1;
			break;
		case 'l':
			/* The hour using a 12-hour clock (1 - 12). */
			tmp = tm->tm_hour % 12;
			if (tmp == 0) {
				tmp = 12;
			}
			
			if (buffer) {
				if (tmp > 9) {
					buffer[len] = tmp / 10 + '0';
					buffer[len+1] = tmp - 10 * (tmp / 10) + '0';
				} else {
					buffer[len] = tmp + '0';
				}
			}
			len += tmp > 9 ? 2 : 1;
			break;
		case 'm':
			/* The month number (01 to 12). */
			if (buffer) {
				tmp = tm->tm_mon + 1;

				buffer[len] = tmp / 10 + '0';
				buffer[len+1] = tmp - 10 * (tmp / 10) + '0';
			}
			len += 2;	
			break;
		case 'M':
			/* The minute (00 - 59). */
			if (buffer) {
				tmp = tm->tm_min;
				
				buffer[len] = tmp / 10 + '0';
				buffer[len+1] = tmp - 10 * (tmp / 10) + '0';
			}
			len += 2;	
			break;
		case 'p':
			/* Either 'AM' or 'PM' according  to the given time value. */
			g_warning ("%%p not yet implemented.");
			if (buffer) {
				buffer[len] = ' ';
				buffer[len+1] = ' ';
			}
			len += 2;
			break;
		case 'P':
			/* Like %p but in lowercase. */
			g_warning ("%%P not yet implemented.");
			if (buffer) {
				buffer[len] = ' ';
				buffer[len+1] = ' ';
			}
			len += 2;
			break;
		case 'R':
			/* The time in 24 hour notation (%H:%M). FIXME: use locale. */
			if (buffer) {
				tmp = tm->tm_hour;
				
				buffer[len] = tmp / 10 + '0';
				buffer[len+1] = tmp - 10 * (tmp / 10) + '0';
			}
			len += 2;

			if (buffer) {
				buffer[len] = ':';
			}
			len++;
			
			if (buffer) {
				tmp = tm->tm_min;
				
				buffer[len] = tmp / 10 + '0';
				buffer[len+1] = tmp - 10 * (tmp / 10) + '0';
			}
			len += 2;
			break;
		case 'S':
			/* The second (00 - 61). */
			if (buffer) {
				tmp = tm->tm_sec;
				
				buffer[len] = tmp / 10 + '0';
				buffer[len+1] = tmp - 10 * (tmp / 10) + '0';
			}
			len += 2;
			break;
		case 'U':
			/* The week number, (1 - 53), starting with the first
			 * Sunday as the first day of week 1.
			 */
			strftime (str, sizeof (str), "%U", tm);
			if (buffer) {
				strcpy (buffer + len, str);
			}
			len += strlen (str);
			break;
		case 'W':
			/* The week number, (1 - 53), starting with the first
			 *  Monday as the first day of week 1.
			 */
			strftime (str, sizeof (str), "%W", tm);
			if (buffer) {
				strcpy (buffer + len, str);
			}
			len += strlen (str);
			break;
		case 'y':
			/* The year without a century (range 00 to 99). */
			if (buffer) {
				tmp = tm->tm_year % 100;
				buffer[len] = tmp / 10 + '0';
				tmp -= 10 * (tmp / 10);
				buffer[len+1] = tmp + '0';
			}
			len += 2;
			break;
		case 'Y':
			/* The year including the century. */
			if (buffer) {
				tmp = tm->tm_year + 1900;
				
				buffer[len] = tmp / 1000 + '0';
				tmp -= 1000 * (tmp / 1000);
				buffer[len+1] = tmp / 100 + '0';
				tmp -= 100 * (tmp / 100);
				buffer[len+2] = tmp / 10 + '0';
				tmp -= 10 * (tmp / 10);
				buffer[len+3] = tmp + '0';
			}
			len += 4;
			break;
		default:
			g_warning ("Failed to parse format string.");
			break;
		}
	}

	if (buffer) {
		buffer[len] = 0;
	}
	
	/* Include the terminating zero. */
	return len + 1;
}

/**
 * mrp_time_format:
 * @format: format string 
 * @t: an #mrptime value
 * 
 * Formats a string with time values. The following format codes are allowed:
 * <informalexample><programlisting>
 * %a     The abbreviated weekday name (Mon, Tue, ...)
 * %A     The full weekday name (Monday, Tuesday, ...)
 * %b     The abbreviated month name (Jan, Feb, ...)
 * %B     The full month name (January, February, ...)
 * %d     The day of the month (01 - 31).
 * %e     The day of the month (1 - 31).
 * %H     The hour using a 24-hour clock (00 - 23).
 * %I     The hour using a 12-hour clock (01 - 12).
 * %j     The day of the year (001 - 366).
 * %k     The hour using a 24-hour clock (0 to 23).
 * %l     The hour using a 12-hour clock (1 - 12).
 * %m     The month number (01 to 12).
 * %M     The minute (00 - 59).
 * %p     Either 'AM' or 'PM' according  to the given time value.
 * %P     Like %p but in lowercase.
 * %R     The time in 24 hour notation (%H:%M).
 * %S     The second (00 - 61).
 * %U     The week number, (1 - 53), starting with the first Sunday as the first day of week 1.
 * %W     The week number, (1 - 53), starting with the first Monday as the first day of week 1.
 * %y     The year without a century (range 00 to 99).
 * %Y     The year including the century.
 * </programlisting></informalexample>
 *
 * Return value: Newly created string that needs to be freed.
 **/
gchar *
mrp_time_format (const gchar *format, mrptime t)
{
	struct tm *tm;
	gint       len;
	gchar     *buffer;

	tm = mrp_time_to_tm (t);

	len = time_format_helper (format, tm, NULL);

	buffer = g_malloc (len);

	time_format_helper (format, tm, buffer);

	return buffer;
}

/*
       %a     The abbreviated weekday name (Mon, Tue, ...)
       %A     The full weekday name (Monday, Tuesday, ...)
       %b     The abbreviated month name (Jan, Feb, ...)
       %B     The full month name (January, February, ...)
       %d     The day of the month (01 - 31).
       %e     The day of the month (1 - 31).
       %H     The hour using a 24-hour clock (00 - 23).
       %I     The hour using a 12-hour clock (01 - 12).
       %j     The day of the year (001 - 366).
       %k     The hour using a 24-hour clock (0 to 23).
       %l     The hour using a 12-hour clock (1 - 12).
       %m     The month number (01 to 12).
       %M     The minute (00 - 59).
       %p     Either 'AM' or 'PM' according  to the given time value.
       %P     Like %p but in lowercase.
       %R     The time in 24 hour notation (%H:%M).
       %S     The second (00 - 61).
       %U     The week number, (1 - 53), starting with the first
              Sunday as the first day of week 1.
       %W     The week number, (1 - 53), starting with the first
              Monday as the first day of week 1.
       %y     The year without a century (range 00 to 99).
       %Y     The year including the century.

*/


