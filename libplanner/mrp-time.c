/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2005 Imendio AB
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
#ifndef WIN32
#include <langinfo.h>
#else
#include <windows.h>
#endif
#include "mrp-time.h"
#include "mrp-types.h"
#include <glib/gi18n.h>
#include "mrp-private.h"

struct _MrpTime {
	GDate date;
	gint  hour;
	gint  min;
	gint  sec;
};

static const gchar *short_month_names[12];
static const gchar *month_names[12];


static const gchar *month_names_initial[12];
static const gchar *short_day_names[7];
static const gchar *day_names[7];

static struct tm *mrp_time_to_tm   (mrptime    t);



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
	MrpTime t;

	mrp_time2_set_date (&t, year, month, day);
	mrp_time2_set_time (&t, hour, minute, second);

	return mrp_time2_get_epoch (&t);
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
	MrpTime t2;
	gint    tmp;

	mrp_time2_set_epoch (&t2, t);

	if (!year) {
		year = &tmp;
	}
	if (!month) {
		month = &tmp;
	}
	if (!day) {
		day = &tmp;
	}
	if (!hour) {
		hour = &tmp;
	}
	if (!minute) {
		minute = &tmp;
	}
	if (!second) {
		second = &tmp;
	}

	mrp_time2_get_date (&t2, year, month, day);
	mrp_time2_get_time (&t2, hour, minute, second);

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
	MrpTime t2;

	mrp_time2_set_epoch (&t2, t);

	mrp_time2_debug_print (&t2);
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
	g_setenv ("TZ", "UTC", TRUE);
	
	t = mktime (tm);

	/* And set it back. */
	if (old_tz != NULL && old_tz[0] != '\0') {
		tmp = g_strconcat ("TZ=", old_tz, NULL);
		putenv (tmp);
		g_free (tmp);
	} else {
		g_unsetenv ("TZ");
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
	time_t     t;
	struct tm *tm;

	t = time (NULL);
	tm = localtime (&t);
	return mrp_time_from_tm (tm);
}

/**
 * mrp_time_to_tm:
 * @t: an #mrptime value
 * 
 * Converts @t to a struct tm value.
 * 
 * Return value: struct tm time, which is static data and should not be
 * modified or freed.
 **/
static struct tm *
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
		if (sscanf (str, "%04d%02d%02d", &year, &month, &day) != 3) {
			return 0;
		}
	} else {
		gchar tsep;
		
		if (sscanf (str,"%04d%02d%02d%c%02d%02d%02d",
			    &year, &month, &day,
			    &tsep,
			    &hour, &minute, &second) != 7) {
			return 0;
		}
		    
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
	MrpTime t2;

	mrp_time2_set_epoch (&t2, t);

	return mrp_time2_to_string (&t2);
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
	MrpTime t2;

	mrp_time2_set_epoch (&t2, t);
	mrp_time2_align_prev (&t2, MRP_TIME_UNIT_DAY);

	return mrp_time2_get_epoch (&t2);
}

mrptime
mrp_time_align_prev (mrptime t, MrpTimeUnit unit)
{
	MrpTime t2;

	mrp_time2_set_epoch (&t2, t);
	mrp_time2_align_prev (&t2, unit);

	return mrp_time2_get_epoch (&t2);
}

mrptime
mrp_time_align_next (mrptime t, MrpTimeUnit unit)
{
	MrpTime t2;

	mrp_time2_set_epoch (&t2, t);
	mrp_time2_align_next (&t2, unit);

	return mrp_time2_get_epoch (&t2);
}

/**
 * mrp_time_day_of_week:
 * @t: an #mrptime value
 * 
 * Retrieves the day of week of the specified time.
 * 
 * Return value: The day of week, in the range 0 to 6, where Sunday is 0.
 **/
gint
mrp_time_day_of_week (mrptime t)
{
	MrpTime t2;
	gint    weekday;

	mrp_time2_set_epoch (&t2, t);

	weekday = g_date_get_weekday (&t2.date);
	if (weekday == 7) {
		weekday = 0;
	}
	
	return weekday;
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
	MrpTime t2;
	
	mrp_time2_set_epoch (&t2, t);
	
	return mrp_time2_get_week_number (&t2, NULL);
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

	g_warning ("day name");
	
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
	MrpTime t2;

	g_return_val_if_fail (t > 0, NULL);

	mrp_time2_set_epoch (&t2, t);

	return short_month_names[g_date_get_month (&t2.date)-1];
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
	MrpTime t2;

	g_return_val_if_fail (t > 0, NULL);

	mrp_time2_set_epoch (&t2, t);

	return month_names_initial[g_date_get_month (&t2.date)-1];
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
	
#ifndef WIN32
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
#else
	for (i = 0; i < 12; i++) {
		gunichar c;
		int len;
		gchar *buffer;

		len = GetLocaleInfo(LOCALE_USER_DEFAULT,
				    LOCALE_SABBREVMONTHNAME1+i,
				    NULL,
				    0);
		buffer = g_malloc(len);

		GetLocaleInfo(LOCALE_USER_DEFAULT,
			      LOCALE_SABBREVMONTHNAME1+i,
			      buffer,
			      len);
		short_month_names[i] = g_locale_to_utf8(buffer, -1, NULL, NULL,
							NULL);
		
		len = GetLocaleInfo(LOCALE_USER_DEFAULT,
				    LOCALE_SMONTHNAME1+i,
				    NULL,
				    0);
		buffer = g_realloc(buffer, len);
		
		GetLocaleInfo(LOCALE_USER_DEFAULT,
			      LOCALE_SMONTHNAME1+i,
			      buffer,
			      len);
		month_names[i] = g_locale_to_utf8(buffer, -1, NULL, NULL,
						  NULL);
		g_free(buffer);
		
		c = g_utf8_get_char (month_names[i]);
		month_names_initial[i] = g_malloc0 (7);
		g_unichar_to_utf8 (c, (char *)month_names_initial[i]);
		
	}

	for (i = 0; i < 7; i++) {
		int len;
		gchar *buffer;
		
		len = GetLocaleInfo(LOCALE_USER_DEFAULT,
				    LOCALE_SABBREVDAYNAME1+i,
				    NULL,
				    0);
		buffer = g_malloc(len);

		GetLocaleInfo(LOCALE_USER_DEFAULT,
			      LOCALE_SABBREVDAYNAME1+i,
			      buffer,
			      len);
		// days in MS start with Monday like in GLIB, so we
		// we need to offset Mon-Sat and fix Sunday
		short_day_names[i < 6 ? i + 1 : 0] = g_locale_to_utf8(buffer, -1, NULL, NULL,
								      NULL);
		
		len = GetLocaleInfo(LOCALE_USER_DEFAULT,
				    LOCALE_SDAYNAME1+i,
				    NULL,
				    0);
		buffer = g_realloc(buffer, len);

		GetLocaleInfo(LOCALE_USER_DEFAULT,
			      LOCALE_SABBREVDAYNAME1+i,
			      buffer,
			      len);
		// days in MS start with Monday like in GLIB, so we
		// we need to offset Mon-Sat and fix Sunday
		day_names[i < 6 ? i + 1 : 0] = g_locale_to_utf8(buffer, -1, NULL, NULL,
								NULL);
		g_free(buffer);
	}
#endif
}

static gint
time_format_helper (const gchar *format,
		    MrpTime     *t,
		    gchar       *buffer)
{
	gint  len = 0;
	gchar str[5];
	gint  year, month, day;
	gint  hour, min, sec;
	gint  weekday;
	
	if (!format) {
		return 1;
	}

	year = g_date_get_year (&t->date);
	month = g_date_get_month (&t->date);
	day = g_date_get_day (&t->date);

	hour = t->hour;
	min = t->min;
	sec = t->sec;
	
	weekday = g_date_get_weekday (&t->date);
	if (weekday == 7) {
		weekday = 0;
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
				strcpy (buffer + len, short_day_names[weekday]);
			}
			len += strlen (short_day_names[weekday]);
			break;
		case 'A':
			/* The full weekday name (Monday, Tuesday, ...). */
			if (buffer) {
				strcpy (buffer + len, day_names[weekday]);
			}
			len += strlen (day_names[weekday]);
			break;
		case 'b':
			/* The abbreviated month name (Jan, Feb, ...). */
			if (buffer) {
				strcpy (buffer + len, short_month_names[month-1]);
			}
			len += strlen (short_month_names[month-1]);
			break;
		case 'B':
			/* The full month name (January, February, ...). */
			if (buffer) {
				strcpy (buffer + len, month_names[month-1]);
			}
			len += strlen (month_names[month-1]);
			break;
		case 'd':
			/* The day of the month (01 - 31). */
			if (buffer) {
				buffer[len] = day / 10 + '0';
				buffer[len+1] = day - 10 * (day / 10) + '0';
			}
			len += 2;
			break;
		case 'e':
			/* The day of the month (1 - 31). */
			if (buffer) {
				if (day > 9) {
					buffer[len] = day / 10 + '0';
					buffer[len+1] = day - 10 * (day / 10) + '0';
				} else {
					buffer[len] = day + '0';
				}
			}
			len += day > 9 ? 2 : 1;
			break;
		case 'H':
			/* The hour using a 24-hour clock (00 - 23). */
			if (buffer) {
				buffer[len] = hour / 10 + '0';
				buffer[len+1] = hour - 10 * (hour / 10) + '0';
			}
			len += 2;			
			break;
		case 'I':
			/* The hour using a 12-hour clock (01 - 12). */
			if (buffer) {
				tmp = hour % 12;

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
			if (buffer) {
				if (hour > 9) {
					buffer[len] = hour / 10 + '0';
					buffer[len+1] = hour - 10 * (hour / 10) + '0';
				} else {
					buffer[len] = hour + '0';
				}
			}
			len += hour > 9 ? 2 : 1;
			break;
		case 'l':
			/* The hour using a 12-hour clock (1 - 12). */
			tmp = hour % 12;
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
				buffer[len] = month / 10 + '0';
				buffer[len+1] = month - 10 * (month / 10) + '0';
			}
			len += 2;	
			break;
		case 'M':
			/* The minute (00 - 59). */
			if (buffer) {
				buffer[len] = min / 10 + '0';
				buffer[len+1] = min - 10 * (min / 10) + '0';
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
				buffer[len] = hour / 10 + '0';
				buffer[len+1] = hour - 10 * (hour / 10) + '0';
			}
			len += 2;

			if (buffer) {
				buffer[len] = ':';
			}
			len++;
			
			if (buffer) {
				buffer[len] = min / 10 + '0';
				buffer[len+1] = min - 10 * (min / 10) + '0';
			}
			len += 2;
			break;
		case 'S':
			/* The second (00 - 61). */
			if (buffer) {
				buffer[len] = sec / 10 + '0';
				buffer[len+1] = sec - 10 * (sec / 10) + '0';
			}
			len += 2;
			break;
		case 'U':
			/* The week number, (1 - 53), starting with the first
			 * Sunday as the first day of week 1.
			 */
			g_warning ("%%W not implemented");
			break;
		case 'W':
			/* The week number, (1 - 53), starting with the first
			 *  Monday as the first day of week 1.
			 */
			snprintf (str, sizeof (str), "%d", mrp_time2_get_week_number (t, NULL));
			if (buffer) {
				strcpy (buffer + len, str);
			}
			len += strlen (str);
			break;
		case 'y':
			/* The year without a century (range 00 to 99). */
			if (buffer) {
				tmp = year % 100;
				buffer[len] = tmp / 10 + '0';
				tmp -= 10 * (tmp / 10);
				buffer[len+1] = tmp + '0';
			}
			len += 2;
			break;
		case 'Y':
			/* The year including the century. */
			if (buffer) {
				tmp = year;
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
	MrpTime  t2;
	gint     len;
	gchar   *buffer;

	mrp_time2_set_epoch (&t2, t);

	len = time_format_helper (format, &t2, NULL);
	buffer = g_malloc (len);
	time_format_helper (format, &t2, buffer);

	return buffer;
}

/**
 * mrp_time_format_locale:
 * @t: an #mrptime value
 * 
 * Formats a string with time values. For format is the preferred for the
 * current locale.
 *
 * Return value: Newly created string that needs to be freed.
 **/
gchar *
mrp_time_format_locale (mrptime t)
{
	struct tm   *tm;
	gchar        buffer[256];
	const gchar *format = "%x"; /* Keep in variable get rid of warning. */

	tm = mrp_time_to_tm (t);

	if (!strftime (buffer, sizeof (buffer), format, tm)) {
		return g_strup ("");
	}

	return g_strdup (buffer);
}


/*
 * New API, the long-term plan is to move completely to this.
 */

#define SECS_IN_MIN  60
#define SECS_IN_HOUR (60*60)
#define SECS_IN_DAY  (60*60*24)

MrpTime *
mrp_time2_new (void)
{
	MrpTime *t;

	t = g_new0 (MrpTime, 1);

	g_date_clear (&t->date, 1);
	
	return t;
}

void
mrp_time2_free (MrpTime *t)
{
	g_free (t);
}

void
mrp_time2_set_date (MrpTime *t,
		    gint     year,
		    gint     month,
		    gint     day)
{
	g_return_if_fail (t != NULL);
	g_return_if_fail (year >= 1 && year <= 9999);
	g_return_if_fail (month >= 1 && month <= 12);
	g_return_if_fail (day >= 1 && day <= 31);
	
	g_date_set_dmy (&t->date, day, month, year);
}

void
mrp_time2_set_time (MrpTime *t,
		    gint     hour,
		    gint     min,
		    gint     sec)
{
	g_return_if_fail (t != NULL);
	g_return_if_fail (hour >= 0 && hour < 24);
	g_return_if_fail (min >= 0 && min < 60);
	g_return_if_fail (sec >= 0 && sec < 60);
	
	t->hour = hour;
	t->min = min;
	t->sec = sec;
}

void
mrp_time2_get_date (MrpTime *t,
		    gint    *year,
		    gint    *month,
		    gint    *day)
{
	g_return_if_fail (t != NULL);
	g_return_if_fail (year != NULL);
	g_return_if_fail (month != NULL);
	g_return_if_fail (day != NULL);
	
	*year = g_date_get_year (&t->date);
	*month = g_date_get_month (&t->date);
	*day = g_date_get_day (&t->date);
}

void
mrp_time2_get_time (MrpTime *t,
		    gint    *hour,
		    gint    *min,
		    gint    *sec)
{
	g_return_if_fail (t != NULL);
	g_return_if_fail (hour != NULL);
	g_return_if_fail (min != NULL);
	g_return_if_fail (sec != NULL);
	
	*hour = t->hour;
	*min = t->min;
	*sec = t->sec;
}

void
mrp_time2_add_years (MrpTime *t, gint years)
{
	g_return_if_fail (t != NULL);
	g_return_if_fail (years >= 0);

	g_date_add_years (&t->date, years);
}

void
mrp_time2_add_months (MrpTime *t, gint months)
{
	g_return_if_fail (t != NULL);
	g_return_if_fail (months >= 0);

	g_date_add_months (&t->date, months);
}

void
mrp_time2_add_days (MrpTime *t, gint days)
{
	g_return_if_fail (t != NULL);
	g_return_if_fail (days >= 0);

	g_date_add_days (&t->date, days);
}

void
mrp_time2_add_seconds (MrpTime *t, gint64 secs)
{
	gint days;
	
	g_return_if_fail (t != NULL);
	g_return_if_fail (secs >= 0);

	secs += t->sec + SECS_IN_MIN * t->min + SECS_IN_HOUR * t->hour;
	
	/* Add whole days first. */
	days = secs / SECS_IN_DAY;
	secs = secs % SECS_IN_DAY;

	g_date_add_days (&t->date, days);

	/* Handle hours/minutes/seconds. */
	t->hour = secs / SECS_IN_HOUR;
	secs = secs % SECS_IN_HOUR;

	t->min = secs / SECS_IN_MIN;
	t->sec = secs % SECS_IN_MIN;
}

void
mrp_time2_add_minutes (MrpTime *t, gint64 mins)
{
	g_return_if_fail (t != NULL);
	g_return_if_fail (mins >= 0);

	mrp_time2_add_seconds (t, mins * SECS_IN_MIN);
}

void
mrp_time2_add_hours (MrpTime *t, gint64 hours)
{
	g_return_if_fail (t != NULL);
	g_return_if_fail (hours >= 0);

	mrp_time2_add_seconds (t, hours * SECS_IN_HOUR);
}

void
mrp_time2_subtract_seconds (MrpTime *t, gint64 secs)
{
	gint days;
	
	g_return_if_fail (t != NULL);
	g_return_if_fail (secs >= 0);

	/* Remove whole days first. */
	days = secs / SECS_IN_DAY;
	secs = secs % SECS_IN_DAY;
	g_date_subtract_days (&t->date, days);

	secs = t->sec + SECS_IN_MIN * t->min + SECS_IN_HOUR * t->hour - secs;

	t->hour = secs / SECS_IN_HOUR;
	secs = secs % SECS_IN_HOUR;

	t->min = secs / SECS_IN_MIN;
	t->sec = secs % SECS_IN_MIN;
}

void
mrp_time2_subtract_minutes (MrpTime *t, gint64 mins)
{
	g_return_if_fail (t != NULL);
	g_return_if_fail (mins >= 0);

	mrp_time2_subtract_seconds (t, mins * SECS_IN_MIN);
}

void
mrp_time2_subtract_hours (MrpTime *t, gint64 hours)
{
	g_return_if_fail (t != NULL);
	g_return_if_fail (hours >= 0);

	mrp_time2_subtract_seconds (t, hours * SECS_IN_HOUR);
}

void
mrp_time2_subtract_days (MrpTime *t, gint days)
{
	g_return_if_fail (t != NULL);
	g_return_if_fail (days >= 0);

	g_date_subtract_days (&t->date, days);
}

void
mrp_time2_subtract_months (MrpTime *t, gint months)
{
	g_return_if_fail (t != NULL);
	g_return_if_fail (months >= 0);

	g_date_subtract_months (&t->date, months);
}

void
mrp_time2_subtract_years (MrpTime *t, gint years)
{
	g_return_if_fail (t != NULL);
	g_return_if_fail (years >= 0);

	g_date_subtract_years (&t->date, years);
}

void
mrp_time2_debug_print (MrpTime *t)
{
	gchar str[128];

	g_date_strftime (str, 128, "%04Y-%02m-%02d",
			 &t->date);

	g_print ("%s %02d:%02d:%02d\n", str, 
		 t->hour,
		 t->min,
		 t->sec);
}

gboolean
mrp_time2_set_from_string (MrpTime      *t,
			   const gchar  *str)
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
			return FALSE;
		}
	}
	else if (len == 8) { /* A date. */
		is_utc = TRUE;
		is_date = TRUE;
	} else {
		return FALSE;
	}
	
	if (is_date) {
		if (sscanf (str, "%04d%02d%02d", &year, &month, &day) != 3) {
			return FALSE;
		}
	} else {
		gchar tsep;
		
		if (sscanf (str,"%04d%02d%02d%c%02d%02d%02d",
			    &year, &month, &day,
			    &tsep,
			    &hour, &minute, &second) != 7) {
			return 0;
		}
		    
		if (tsep != 'T') {
			return FALSE;
		}
		
	}

	mrp_time2_set_date (t, year, month, day);
	if (!is_date) {
		mrp_time2_set_time (t, hour, minute, second);
	}
	
	return TRUE;
}

gchar *
mrp_time2_to_string (MrpTime *t)
{
	gint year, month, day;

	day = g_date_get_day (&t->date);
	month = g_date_get_month (&t->date);
	year = g_date_get_year (&t->date);
	
	return g_strdup_printf ("%04d%02d%02dT%02d%02d%02dZ",
				year, month, day,
				t->hour, t->min, t->sec);
}

void
mrp_time2_set_epoch (MrpTime *t, time_t epoch)
{
	memset (t, 0, sizeof (MrpTime));

	mrp_time2_set_date (t, 1970, 1, 1);
	mrp_time2_add_seconds (t, epoch);
}

time_t
mrp_time2_get_epoch (MrpTime *t)
{
	GDate  date;
	time_t epoch;

	g_date_set_dmy (&date, 1, 1, 1970);
	
	epoch = SECS_IN_DAY * g_date_days_between (&date, &t->date);

	epoch += t->hour * SECS_IN_HOUR + t->min * SECS_IN_MIN + t->sec;

	return epoch;
}

const gchar *
mrp_time2_get_day_name (MrpTime *t)
{
	GDateWeekday day;
	
	g_return_val_if_fail (t != NULL, NULL);

	day = g_date_get_weekday (&t->date);

	/* Weekday is mon-sun 1-7, while the array is sun-sat 0-6. */
	if (day == 7) {
		day = 0;
	}	
	
	return short_day_names[day];
}

const gchar *
mrp_time2_get_month_name (MrpTime *t)
{
	g_return_val_if_fail (t != NULL, NULL);

	return short_month_names[g_date_get_month (&t->date)-1];
}

const gchar *
mrp_time2_get_month_initial (MrpTime *t)
{
	g_return_val_if_fail (t != NULL, NULL);

	return month_names_initial[g_date_get_month (&t->date)-1];
}

/* From GLlib 2.6. */
static guint
stolen_g_date_get_iso8601_week_of_year (const GDate *d)
{
  guint j, d4, L, d1, w;

  /* Formula taken from the Calendar FAQ; the formula was for the
   * Julian Period which starts on 1 January 4713 BC, so we add
   * 1,721,425 to the number of days before doing the formula. 
   */
  j  = g_date_get_julian (d) + 1721425;
  d4 = (j + 31741 - (j % 7)) % 146097 % 36524 % 1461;
  L  = d4 / 1460;
  d1 = ((d4 - L) % 365) + L;
  w  = d1 / 7 + 1;

  return w;
}

gint
mrp_time2_get_week_number (MrpTime *t, gint *y)
{
	gint week;
	gint year;

	g_return_val_if_fail (t != NULL, 0);

	week = stolen_g_date_get_iso8601_week_of_year (&t->date);

	/* Calculate the year this week belongs to as it can be different than 
	 * the year of the date (e.g. December 31 2002 is in week 1 of 2003).
	 */
	if(y != NULL) {
		year = g_date_get_year (&t->date);
  
		switch(g_date_get_month (&t->date)) {
		case G_DATE_JANUARY:
			if(week > 50) {
				year--;
			}
			break;
		case G_DATE_DECEMBER:
			if(week == 1) {
				year++;
			}
			break;
		default:
			break;
		}

		*y = year;
	}
	return week;
}

void
mrp_time2_align_prev (MrpTime *t, MrpTimeUnit unit)
{
	GDateWeekday weekday;
	GDateMonth   month;
	
	g_return_if_fail (t != NULL);
	
	switch (unit) {
	case MRP_TIME_UNIT_HOUR:
		t->min = 0;
		t->sec = 0;
		break;

	case MRP_TIME_UNIT_TWO_HOURS:
		t->min = 0;
		t->sec = 0;
		if (t->hour >= 2) {
			mrp_time2_subtract_hours (t, 2 - t->hour % 2);
		} else {
			t->hour = 0;
		}
		break;
		
	case MRP_TIME_UNIT_HALFDAY:
		if (t->hour < 12) {
			t->hour = 0;
		} else {
			t->hour = 12;
		}
		t->min = 0;
		t->sec = 0;
		break;
		
	case MRP_TIME_UNIT_DAY:
		t->hour = 0;
		t->min = 0;
		t->sec = 0;
		break;

	case MRP_TIME_UNIT_WEEK:
		/* FIXME: We currently hardcode monday as week start .*/
		weekday = g_date_get_weekday (&t->date);
		g_date_subtract_days (&t->date, weekday - 1);
		t->hour = 0;
		t->min = 0;
		t->sec = 0;
		break;

	case MRP_TIME_UNIT_MONTH:
		g_date_set_day (&t->date, 1);
		t->hour = 0;
		t->min = 0;
		t->sec = 0;
		break;

	case MRP_TIME_UNIT_QUARTER:
		g_date_set_day (&t->date, 1);
		t->hour = 0;
		t->min = 0;
		t->sec = 0;
		month = g_date_get_month (&t->date);
		if (month > 1 && month <= 3) {
			g_date_set_month (&t->date, 1);
		}
		else if (month > 4 && month <= 6) {
			g_date_set_month (&t->date, 4);
		}
		else if (month > 7 && month <= 9) {
			g_date_set_month (&t->date, 7);
		}
		else if (month > 10 && month <= 12) {
			g_date_set_month (&t->date, 10);
		}
		break;
		
	case MRP_TIME_UNIT_HALFYEAR:
		g_date_set_day (&t->date, 1);
		t->hour = 0;
		t->min = 0;
		t->sec = 0;
		month = g_date_get_month (&t->date);
		if (month > 1 && month <= 6) {
			g_date_set_month (&t->date, 1);
		} else if (month > 7 && month <= 12) {
			g_date_set_month (&t->date, 7);
		}
		break;

	case MRP_TIME_UNIT_YEAR:
		g_date_set_month (&t->date, 1);
		g_date_set_day (&t->date, 1);
		t->hour = 0;
		t->min = 0;
		t->sec = 0;
		break;

	case MRP_TIME_UNIT_NONE:
		g_assert_not_reached ();
	}
}

void
mrp_time2_align_next (MrpTime *t, MrpTimeUnit unit)
{
	GDateWeekday weekday;
	GDateMonth   month;
	
	g_return_if_fail (t != NULL);

	switch (unit) {
	case MRP_TIME_UNIT_HOUR:
		t->min = 0;
		t->sec = 0;
		mrp_time2_add_hours (t, 1);
		break;

	case MRP_TIME_UNIT_TWO_HOURS:
		t->min = 0;
		t->sec = 0;
		mrp_time2_add_hours (t, 2 - t->hour % 2);
		break;

	case MRP_TIME_UNIT_HALFDAY:
		t->min = 0;
		t->sec = 0;
		if (t->hour < 12) {
			t->hour = 12;
		} else {
			t->hour = 0;
			mrp_time2_add_days (t, 1);
		}
		break;
		
	case MRP_TIME_UNIT_DAY:
		t->hour = 0;
		t->min = 0;
		t->sec = 0;
		mrp_time2_add_days (t, 1);
		break;
		
	case MRP_TIME_UNIT_WEEK:
		/* FIXME: We currently hardcode monday as week start .*/
		weekday = g_date_get_weekday (&t->date);
		t->hour = 0;
		t->min = 0;
		t->sec = 0;
		mrp_time2_add_days (t, 8 - weekday);
		break;

	case MRP_TIME_UNIT_MONTH:
		t->hour = 0;
		t->min = 0;
		t->sec = 0;
		g_date_set_day (&t->date, 1);
		g_date_add_months (&t->date, 1);
		break;

	case MRP_TIME_UNIT_QUARTER:
		t->hour = 0;
		t->min = 0;
		t->sec = 0;
		g_date_set_day (&t->date, 1);
		month = g_date_get_month (&t->date);
		if (month >= 1 && month <= 3) {
			g_date_set_month (&t->date, 4);
		}
		else if (month >= 4 && month <= 6) {
			g_date_set_month (&t->date, 7);
		}
		else if (month >= 7 && month <= 9) {
			g_date_set_month (&t->date, 10);
		}
		else if (month >= 10 && month <= 12) {
			g_date_set_month (&t->date, 1);
			g_date_add_years (&t->date, 1);
		}
		break;
		
	case MRP_TIME_UNIT_HALFYEAR:
		g_date_set_day (&t->date, 1);
		t->hour = 0;
		t->min = 0;
		t->sec = 0;
		month = g_date_get_month (&t->date);
		if (month >= 1 && month <= 6) {
			g_date_set_month (&t->date, 7);
		} else if (month >= 7 && month <= 12) {
			g_date_set_month (&t->date, 1);
			g_date_add_years (&t->date, 1);
		}
		break;
		
	case MRP_TIME_UNIT_YEAR:
		t->hour = 0;
		t->min = 0;
		t->sec = 0;
		g_date_set_day (&t->date, 1);
		g_date_set_month (&t->date, 1);
		g_date_add_years (&t->date, 1);
		break;

	case MRP_TIME_UNIT_NONE:
	default:
		g_assert_not_reached ();
	}
}

void
mrp_time2_copy (MrpTime *dst, MrpTime *src)
{
	g_return_if_fail (dst != NULL);
	g_return_if_fail (src != NULL);

	memcpy (src, dst, sizeof (MrpTime));
}

void
mrp_time2_clear (MrpTime *t)
{
	memset (t, 0, sizeof (MrpTime));
}

gint
mrp_time2_compare (MrpTime *t1, MrpTime *t2)
{
	gint ret;
	gint s1, s2;

	ret = g_date_compare (&t1->date, &t2->date);
	if (ret != 0) {
		return ret;
	}

	s1 = t1->hour * SECS_IN_HOUR + t1->min * SECS_IN_MIN + t1->sec;
	s2 = t2->hour * SECS_IN_HOUR + t2->min * SECS_IN_MIN + t2->sec;

	if (s1 < s2) {
		return -1;
	}
	else if (s1 > s2) {
		return 1;
	}

	return 0;
}


