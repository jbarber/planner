/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2005 Imendio AB
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
#include <glib/gi18n.h>
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
planner_format_duration_with_day_length (gint duration,
					 gint day_length)
{
	gint days;
	gint hours;
	gint minutes;

	day_length = day_length;

	if (day_length == 0) {
		return g_strdup ("");
	}

	days = duration / day_length;
	duration -= days * day_length;
	hours = duration / (60*60);
	duration -= hours * (60*60);
	minutes = duration / 60;

	if (days > 0) {
		if (hours > 0) {
			return g_strdup_printf (_("%dd %dh"), days, hours);
		}
		else {
			return g_strdup_printf (_("%dd"), days);
		}
	}
	else if (hours > 0) {
		if (minutes > 0) {
			return g_strdup_printf (_("%dh %dmin"), hours, minutes);
		}
		else {
			return g_strdup_printf (_("%dh"), hours);
		}
	}
	else if (minutes > 0) {
		return g_strdup_printf (_("%dmin"), minutes);
	} else {
		return g_strdup ("");
	}
}

gchar *
planner_format_duration (MrpProject *project,
			 gint        duration)
{
	MrpCalendar *calendar;
	gint         day_length;

	calendar = mrp_project_get_calendar (project);
	day_length = mrp_calendar_day_get_total_work (calendar, mrp_day_get_work ());

	if (day_length == 0) {
		day_length = 8*60*60;
	}

	return planner_format_duration_with_day_length (duration, day_length);
}

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

/* Strips out all occurances of thousands separators. */
gint
planner_parse_int (const gchar *str)
{
	GString      *tmp;
	const gchar  *p;
	gint          value;
	struct lconv *locality;
	gint          sep_len;

	if (!str || !str[0]) {
		return 0;
	}

	locality = localeconv ();
	sep_len = strlen (locality->thousands_sep);

	if (sep_len == 0) {
		return atoi (str);
	}

	tmp = g_string_new (NULL);

	p = str;
	while (*p) {
		if (strncmp (p, locality->thousands_sep, sep_len) == 0) {
			p = g_utf8_offset_to_pointer (p, sep_len);
		} else {
			g_string_append_unichar (tmp, g_utf8_get_char (p));
			p = g_utf8_next_char (p);
		}
	}

	value = atoi (tmp->str);
	g_string_free (tmp, TRUE);

	return value;
}

/* Strips out all occurances of thousands separators. */
gfloat
planner_parse_float (const gchar *str)
{
	const gchar  *p;
	gint          int_part, dec_part;
	gint          dec_factor;
	struct lconv *locality;
	gchar        *dec_point;
	gint          dec_len;
	gint          i;

	if (!str || !str[0]) {
		return 0.0;
	}

	/* First get the integer part. */
	int_part = planner_parse_int (str);

	locality = localeconv ();

	dec_len = strlen (locality->mon_decimal_point);
	if (dec_len == 0) {
		dec_point = ".";
		dec_len = 1;
	}
	else {
		dec_point = locality->mon_decimal_point;
	}

	p = strstr (str, dec_point);
	if (!p) {
		return int_part;
	}

	p = p + dec_len;

	dec_part = planner_parse_int (p);

	i = dec_part;
	dec_factor = 0;
	while (i > 0) {
		i = i / 10;
		dec_factor++;
	}

	if (dec_factor == 0) {
		dec_factor = 1;
	}

	return int_part + dec_part / pow (10, dec_factor);
}

typedef enum {
	UNIT_NONE,
	UNIT_MONTH,
	UNIT_WEEK,
	UNIT_DAY,
	UNIT_HOUR,
	UNIT_MINUTE
} Unit;

typedef struct {
	gchar *name;
	Unit   unit;
} Units;

/* The comments here are for i18n, they get extracted to the po files. */
static Units units[] = {
	{ N_("mon"),     UNIT_MONTH },  /* month unit variant accepted in input */
	{ N_("month"),   UNIT_MONTH },  /* month unit variant accepted in input */
	{ N_("months"),  UNIT_MONTH },  /* month unit variant accepted in input */
	{ N_("w"),       UNIT_WEEK },   /* week unit variant accepted in input */
	{ N_("week"),    UNIT_WEEK },   /* week unit variant accepted in input */
	{ N_("weeks"),   UNIT_WEEK },   /* week unit variant accepted in input */
	{ N_("d"),       UNIT_DAY },    /* day unit variant accepted in input */
	{ N_("day"),     UNIT_DAY },    /* day unit variant accepted in input */
	{ N_("days"),    UNIT_DAY },    /* day unit variant accepted in input */
	{ N_("h"),       UNIT_HOUR },   /* hour unit variant accepted in input */
	{ N_("hour"),    UNIT_HOUR },   /* hour unit variant accepted in input */
	{ N_("hours"),   UNIT_HOUR },   /* hour unit variant accepted in input */
	{ N_("min"),     UNIT_MINUTE }, /* minute unit variant accepted in input */
	{ N_("minute"),  UNIT_MINUTE }, /* minute unit variant accepted in input */
	{ N_("minutes"), UNIT_MINUTE }  /* minute unit variant accepted in input */
};

static gint num_units = G_N_ELEMENTS (units);

static Unit
format_get_unit_from_string (const gchar *str)
{
	static Units    *translated_units;
	static gboolean  inited = FALSE;
	Unit             unit = UNIT_NONE;
	gint             i;
	gchar           *tmp;

	if (!inited) {
		translated_units = g_new0 (Units, num_units);

		for (i = 0; i < num_units; i++) {
			tmp = g_utf8_casefold (_(units[i].name), -1);

			translated_units[i].name = tmp;
			translated_units[i].unit = units[i].unit;
		}

		inited = TRUE;
	}

	for (i = 0; i < num_units; i++) {
		if (!strncmp (str, translated_units[i].name,
			      strlen (translated_units[i].name))) {
			unit = translated_units[i].unit;
		}
	}

	if (unit != UNIT_NONE) {
		return unit;
	}

	/* Try untranslated names as a fallback. */
	for (i = 0; i < num_units; i++) {
		if (!strncmp (str, units[i].name, strlen (units[i].name))) {
			unit = units[i].unit;
		}
	}

	return unit;
}

static gint
format_multiply_with_unit (gdouble value,
			   Unit    unit,
			   gint    seconds_per_month,
			   gint    seconds_per_week,
			   gint    seconds_per_day)
{
	switch (unit) {
	case UNIT_MONTH:
		value *= seconds_per_month;
		break;
	case UNIT_WEEK:
		value *= seconds_per_week;
		break;
	case UNIT_DAY:
		value *= seconds_per_day;
		break;
	case UNIT_HOUR:
		value *= 60*60;
		break;
	case UNIT_MINUTE:
		value *= 60;
		break;
	case UNIT_NONE:
		return 0;
	}

	return floor (value + 0.5);
}

gint
planner_parse_duration_with_day_length (const gchar *input,
					gint         day_length)
{
	gchar   *str;
	gchar   *p;
	gchar   *end_ptr;
	gdouble  dbl;
	Unit     unit;
	gint     total;
	gint     seconds_per_month;
	gint     seconds_per_week;

	/* Hardcode these for now. */
	seconds_per_week = day_length * 5;
	seconds_per_month = day_length * 30;

	str = g_utf8_casefold (input, -1);
	if (!str) {
		return 0;
	}

	total = 0;
	p = str;
	while (*p) {
		while (*p && g_unichar_isalpha (g_utf8_get_char (p))) {
			p = g_utf8_next_char (p);
		}

		if (*p == 0) {
			break;
		}

		dbl = g_strtod (p, &end_ptr);
		if (end_ptr == p) {
			break;
		}

		if (end_ptr) {
			unit = format_get_unit_from_string (end_ptr);

			/* If no unit was specified and it was the first number
			 * in the input, treat it as "day".
			 */
			if (unit == UNIT_NONE && p == str) {
				unit = UNIT_DAY;
			}

			total += format_multiply_with_unit (dbl,
							    unit,
							    seconds_per_month,
							    seconds_per_week,
							    day_length);
		}

		if (end_ptr && *end_ptr == 0) {
			break;
		}

		p = end_ptr + 1;
	}

	g_free (str);

	return total;
}

gint
planner_parse_duration (MrpProject  *project,
			const gchar *input)
{
	gint day_length;

	day_length = mrp_calendar_day_get_total_work (
		mrp_project_get_calendar (project),
		mrp_day_get_work ());

	return planner_parse_duration_with_day_length (input, day_length);
}
