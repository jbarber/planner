/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2002 CodeFactory AB
 * Copyright (C) 2002 Richard Hult <richard@imendio.com>
 * Copyright (C) 2002 Mikael Hallendal <micke@imendio.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <config.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <math.h>
#include <glib.h>
#include <libgnome/gnome-i18n.h>
#include "planner-format.h"

gchar *
planner_format_int (gint number)
{
	GList        *iterator, *list = NULL, *l;
	struct lconv *locality;
	gint          char_length = 0;
	gint          group_count = 0;
	guchar       *grouping;
	int           last_count = 3;
	int           divider;
	char         *value;
	char         *value_iterator;

	locality = localeconv ();
	grouping = locality->grouping;
	while (number) {
		char *group;
		switch (*grouping) {
		default:
			last_count = *grouping;
			grouping++;
		case 0:
			divider = pow (10, last_count);
			if (number >= divider) {
				group = g_strdup_printf ("%0*d", last_count, number % divider);
			} else {
				group = g_strdup_printf ("%d", number % divider);
			}
			number /= divider;
			break;
		case CHAR_MAX:
			group = g_strdup_printf ("%d", number);
			number = 0;
			break;
		}
		char_length += strlen (group);
		list = g_list_prepend (list, group);
		group_count ++;
	}

	if (list) {
		value = g_new (gchar, 1 + char_length + (group_count - 1) * strlen (locality->thousands_sep));

		iterator = list;
		value_iterator = value;

		strcpy (value_iterator, iterator->data);
		value_iterator += strlen (iterator->data);
		for (iterator = iterator->next; iterator; iterator = iterator->next) {
			strcpy (value_iterator, locality->thousands_sep);
			value_iterator += strlen (locality->thousands_sep);

			strcpy (value_iterator, iterator->data);
			value_iterator += strlen (iterator->data);
		}

		for (l = list; l; l = l->next) {
			g_free (l->data);
		}
		g_list_free (list);

		return value;
	} else {
		return g_strdup ("0");
	}
}

static gchar*
format_get_n_chars (gint n, gchar c)
{
	gchar *str;

	str = g_new0 (gchar, n);

	memset (str, c, n);

	return str;
}

static void
format_strip_trailing_zeroes (gchar *str)
{
	gint len;
	gint i;

	len = strlen (str);

	i = len - 1;
	while (str[i] == '0' && i > 0) {
		i--;
	}

	if (i < len - 1) {
	    str[i+1] = 0;
	}
}

gchar *
planner_format_duration (gint duration,
		    gint day_length)
{
	gint   days;
	gint   hours;

	days = duration / (60*60*day_length);
	duration -= days * 60*60*day_length;
	hours = duration / (60*60);

	if (days > 0 && hours > 0) {
		return g_strdup_printf (_("%dd %dh"), days, hours);
	}
	else if (days > 0) {
		return g_strdup_printf (_("%dd"), days);
	}
	else if (hours > 0) {
		return g_strdup_printf (_("%dh"), hours);
	} else {
		return g_strdup ("");
	}
}

#if 0
gchar *
planner_format_duration (gint duration,
		    gint day_length) 
{
	gchar  *str, *svalue;
	gfloat  days;
	gfloat  rounded;

	days = duration / (60.0*60.0 * day_length);

	/* Take rounding in consideration when deciding on whether to output
	 * "day" or "days".
	 */
	rounded = floor (days * 10 + 0.5);
	
	if (rounded > 10.0) {
		str = planner_format_float (days, 1, FALSE);
		svalue = g_strdup_printf (_("%s days"), str);
		g_free (str);
	} else if (rounded == 0.0) {
		svalue = g_strdup (_("0 days"));
	} else {
		str = planner_format_float (days, 1, FALSE);
		svalue = g_strdup_printf (_("%s day"), str);
		g_free (str);
	}

	return svalue;	
}
#endif

gchar * 
planner_format_date (mrptime date)
{
	gchar *svalue;

	if (date == MRP_TIME_INVALID) {
		svalue = g_strdup ("");	
	} else {
		/* i18n: this string is the date nr and month name, displayed
		 * e.g. in the date cells in the task tree. See
		 * libmrproject/docs/DateFormat.
		 */
		svalue = mrp_time_format (_("%b %e"), date);
	}
	
	return svalue;	
}

gchar *
planner_format_float (gfloat   number,
		 guint    precision,
		 gboolean fill_with_zeroes)
{
	gint          int_part;
	gint          fraction;
	gint          pow10;
	struct lconv *locality;
	gchar        *str_sign;
	gchar        *str_intpart;
	gchar        *decimal_point;
	gchar        *str_fraction;
	gchar        *value;

	locality = localeconv ();
	
	int_part = abs (number);
	pow10 = pow (10, precision);
	fraction = floor (0.5 + (number - (int) number) * pow10);

	fraction = abs (fraction);
	
	if (fraction >= pow10) {
		int_part++;

		fraction -= pow10;
	}
	
	str_intpart = planner_format_int (int_part);

	if (!strcmp (locality->mon_decimal_point, "")) {
		decimal_point = ".";
	}
	else {
		decimal_point = locality->mon_decimal_point;
	}

	if (number < 0) {
		str_sign = "-";
	} else {
		str_sign = "";
	}
	
	str_fraction = NULL;
	
	if (fraction == 0) {
		if (fill_with_zeroes) {
			str_fraction = format_get_n_chars (precision, '0');
		}
	}
	else {
		str_fraction = g_strdup_printf ("%0*d", precision, fraction);
		if (!fill_with_zeroes) {
			format_strip_trailing_zeroes (str_fraction);
		}
	}

	if (str_fraction) {
		value = g_strconcat (str_sign, str_intpart, decimal_point, str_fraction, NULL);
		g_free (str_intpart);
		g_free (str_fraction);
	} else {
		value = g_strconcat (str_sign, str_intpart, NULL);
		g_free (str_intpart);
	}

	return value;
}
