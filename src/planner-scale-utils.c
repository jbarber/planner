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
#include <math.h>
#include <glib/gi18n.h>
#include "planner-scale-utils.h"

#define WEEK    (60*60*24*7)
#define DAY     (60*60*24)
#define HALFDAY (60*60*4)
#define HOUR    (60*60)

static const PlannerScaleConf scale_conf[] = {
	/* Major unit                  Major format                   Minor unit                     Minor format */

	{ MRP_TIME_UNIT_YEAR,     PLANNER_SCALE_FORMAT_MEDIUM,   MRP_TIME_UNIT_HALFYEAR,   PLANNER_SCALE_FORMAT_SHORT,  WEEK },

	{ MRP_TIME_UNIT_YEAR,     PLANNER_SCALE_FORMAT_MEDIUM,   MRP_TIME_UNIT_HALFYEAR,   PLANNER_SCALE_FORMAT_SHORT,  WEEK },
	{ MRP_TIME_UNIT_YEAR,     PLANNER_SCALE_FORMAT_MEDIUM,   MRP_TIME_UNIT_QUARTER,    PLANNER_SCALE_FORMAT_SHORT,  WEEK },
	{ MRP_TIME_UNIT_YEAR,     PLANNER_SCALE_FORMAT_MEDIUM,   MRP_TIME_UNIT_QUARTER,    PLANNER_SCALE_FORMAT_MEDIUM, WEEK },

	{ MRP_TIME_UNIT_HALFYEAR, PLANNER_SCALE_FORMAT_LONG,     MRP_TIME_UNIT_MONTH,      PLANNER_SCALE_FORMAT_LONG,   WEEK },

	{ MRP_TIME_UNIT_QUARTER,  PLANNER_SCALE_FORMAT_LONG,     MRP_TIME_UNIT_MONTH,      PLANNER_SCALE_FORMAT_MEDIUM, DAY },

	{ MRP_TIME_UNIT_MONTH,    PLANNER_SCALE_FORMAT_LONG,     MRP_TIME_UNIT_WEEK,       PLANNER_SCALE_FORMAT_MEDIUM, DAY },

	{ MRP_TIME_UNIT_WEEK,     PLANNER_SCALE_FORMAT_LONG,     MRP_TIME_UNIT_DAY,        PLANNER_SCALE_FORMAT_SHORT,  DAY },
	{ MRP_TIME_UNIT_WEEK,     PLANNER_SCALE_FORMAT_LONG,     MRP_TIME_UNIT_DAY,        PLANNER_SCALE_FORMAT_MEDIUM, HALFDAY },

	{ MRP_TIME_UNIT_DAY,      PLANNER_SCALE_FORMAT_LONG,     MRP_TIME_UNIT_HALFDAY,    PLANNER_SCALE_FORMAT_MEDIUM, HALFDAY },
	{ MRP_TIME_UNIT_DAY,      PLANNER_SCALE_FORMAT_LONG,     MRP_TIME_UNIT_HALFDAY,    PLANNER_SCALE_FORMAT_MEDIUM, HOUR },

	{ MRP_TIME_UNIT_DAY,      PLANNER_SCALE_FORMAT_LONG,     MRP_TIME_UNIT_TWO_HOURS,  PLANNER_SCALE_FORMAT_MEDIUM, HOUR },

	{ MRP_TIME_UNIT_DAY,      PLANNER_SCALE_FORMAT_LONG,     MRP_TIME_UNIT_HOUR,       PLANNER_SCALE_FORMAT_MEDIUM, HOUR },
	{ MRP_TIME_UNIT_DAY,      PLANNER_SCALE_FORMAT_LONG,     MRP_TIME_UNIT_HOUR,       PLANNER_SCALE_FORMAT_MEDIUM, HOUR }
};

const PlannerScaleConf *planner_scale_conf = scale_conf;


gchar *
planner_scale_format_time (mrptime            t,
			   MrpTimeUnit        unit,
			   PlannerScaleFormat format)
{
	MrpTime *t2;
	gchar   *str = NULL;
	gint     num;
	gint     year, month, week, day;
	gint     hour, min, sec;

	t2 = mrp_time2_new ();
	mrp_time2_set_epoch (t2, t);

	mrp_time2_get_date (t2, &year, &month, &day);
	mrp_time2_get_time (t2, &hour, &min, &sec);

	switch (unit) {
	case MRP_TIME_UNIT_HOUR:
		switch (format) {
		default:
			str = g_strdup_printf ("%d", hour);
			break;
		}
		break;

	case MRP_TIME_UNIT_TWO_HOURS:
		switch (format) {
		default:
			str = g_strdup_printf ("%d", hour);
			break;
		}
		break;

	case MRP_TIME_UNIT_HALFDAY:
		switch (format) {
		default:
			str = g_strdup_printf ("%d", hour);
			break;
		}
		break;

	case MRP_TIME_UNIT_DAY:
		switch (format) {
		case PLANNER_SCALE_FORMAT_SHORT:
			str = g_strdup_printf ("%d", day);
			break;
		case PLANNER_SCALE_FORMAT_MEDIUM:
			str = g_strdup_printf ("%s %d",
					       mrp_time2_get_day_name (t2),
					       day);
			break;
		case PLANNER_SCALE_FORMAT_LONG:
			str = g_strdup_printf ("%s, %s %d",
					       mrp_time2_get_day_name (t2),
					       mrp_time2_get_month_name (t2),
					       day);
			break;
		}
		break;

	case MRP_TIME_UNIT_WEEK:
		switch (format) {
		case PLANNER_SCALE_FORMAT_SHORT:
			/* i18n: Short "Week", preferably 2 letters. */
			str = g_strdup_printf (_("Wk %d"),
					       mrp_time2_get_week_number (t2, NULL));
			break;
		case PLANNER_SCALE_FORMAT_MEDIUM:
			str = g_strdup_printf (_("Week %d"),
					       mrp_time2_get_week_number (t2, NULL));
			break;
		case PLANNER_SCALE_FORMAT_LONG:
			/* i18n: Week, year. */
			week = mrp_time2_get_week_number (t2, &year),
			str = g_strdup_printf (_("Week %d, %d"),
					       week,
					       year);
			break;
		}
		break;

	case MRP_TIME_UNIT_MONTH:
		switch (format) {
		case PLANNER_SCALE_FORMAT_SHORT:
			str = g_strdup_printf ("%s",
					       mrp_time2_get_month_initial (t2));
			break;
		case PLANNER_SCALE_FORMAT_MEDIUM:
			str = g_strdup_printf ("%s",
					       mrp_time2_get_month_name (t2));
			break;
		case PLANNER_SCALE_FORMAT_LONG:
			str = g_strdup_printf ("%s %d",
					       mrp_time2_get_month_name (t2),
					       year);
			break;
		}
		break;

	case MRP_TIME_UNIT_QUARTER:
		num = 1 + floor (month / 3);

		switch (format) {
		case PLANNER_SCALE_FORMAT_SHORT:
			/* i18n: Short "Quarter", preferably 1 letter. */
			str = g_strdup_printf (_("Q%d"), num);
			break;
		case PLANNER_SCALE_FORMAT_MEDIUM:
			/* i18n: Short "Quarter", preferably 2-3 letters. */
			str = g_strdup_printf (_("Qtr %d"), num);
			break;
		case PLANNER_SCALE_FORMAT_LONG:
			/* i18n: Year, short "Quarter", preferably 2-3 letters. */
			str = g_strdup_printf (_("%d, Qtr %d"),
					       year,
					       num);
			break;
		}
		break;

	case MRP_TIME_UNIT_HALFYEAR:
		num = 1 + floor (month / 6);

		switch (format) {
		case PLANNER_SCALE_FORMAT_SHORT:
			/* i18n: Short "Half year", preferably 1 letter. */
			str = g_strdup_printf (_("H%d"), num);
			break;
		case PLANNER_SCALE_FORMAT_MEDIUM:
		case PLANNER_SCALE_FORMAT_LONG:
			/* i18n: Year, short "Half year", preferably 1 letter. */
			str = g_strdup_printf (_("%04d, H%d"),
					       year,
					       num);
			break;
		}
		break;

	case MRP_TIME_UNIT_YEAR:
		switch (format) {
		default:
			str = g_strdup_printf ("%d", year);
			break;
		}
		break;

	case MRP_TIME_UNIT_NONE:
		str = NULL;
		break;

	default:
		g_assert_not_reached ();
		break;
	}

	mrp_time2_free (t2);

	return str;
}

gint
planner_scale_clamp_zoom (gdouble zoom)
{
	gint level;

	level = floor (zoom + 0.5);
	level = CLAMP (level, 0, 12);

	return level;
}

