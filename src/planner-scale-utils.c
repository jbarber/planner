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
#include <math.h>
#include <libgnome/gnome-i18n.h>
#include "planner-scale-utils.h"

#define WEEK (60*60*24*7)
#define DAY (60*60*24)
#define HALFDAY (60*60*4)
#define HOUR (60*60)

static const PlannerScaleConf scale_conf[] = {
	/* Major unit             Major format              Minor unit                Minor format */

	{ PLANNER_SCALE_UNIT_YEAR,     PLANNER_SCALE_FORMAT_MEDIUM,   PLANNER_SCALE_UNIT_HALFYEAR,   PLANNER_SCALE_FORMAT_SHORT,  WEEK },

	{ PLANNER_SCALE_UNIT_YEAR,     PLANNER_SCALE_FORMAT_MEDIUM,   PLANNER_SCALE_UNIT_HALFYEAR,   PLANNER_SCALE_FORMAT_SHORT,  WEEK },
	{ PLANNER_SCALE_UNIT_YEAR,     PLANNER_SCALE_FORMAT_MEDIUM,   PLANNER_SCALE_UNIT_QUARTER,    PLANNER_SCALE_FORMAT_SHORT,  WEEK },
	{ PLANNER_SCALE_UNIT_YEAR,     PLANNER_SCALE_FORMAT_MEDIUM,   PLANNER_SCALE_UNIT_QUARTER,    PLANNER_SCALE_FORMAT_MEDIUM, WEEK },

	{ PLANNER_SCALE_UNIT_HALFYEAR, PLANNER_SCALE_FORMAT_LONG,     PLANNER_SCALE_UNIT_MONTH,      PLANNER_SCALE_FORMAT_LONG,   WEEK },

	{ PLANNER_SCALE_UNIT_QUARTER,  PLANNER_SCALE_FORMAT_LONG,     PLANNER_SCALE_UNIT_MONTH,      PLANNER_SCALE_FORMAT_MEDIUM, DAY },

	{ PLANNER_SCALE_UNIT_MONTH,    PLANNER_SCALE_FORMAT_LONG,     PLANNER_SCALE_UNIT_WEEK,       PLANNER_SCALE_FORMAT_MEDIUM, DAY },

	{ PLANNER_SCALE_UNIT_WEEK,     PLANNER_SCALE_FORMAT_LONG,     PLANNER_SCALE_UNIT_DAY,        PLANNER_SCALE_FORMAT_SHORT,  DAY },
	{ PLANNER_SCALE_UNIT_WEEK,     PLANNER_SCALE_FORMAT_LONG,     PLANNER_SCALE_UNIT_DAY,        PLANNER_SCALE_FORMAT_MEDIUM, HALFDAY },

	{ PLANNER_SCALE_UNIT_DAY,      PLANNER_SCALE_FORMAT_LONG,     PLANNER_SCALE_UNIT_HALFDAY,    PLANNER_SCALE_FORMAT_MEDIUM, HALFDAY },
	{ PLANNER_SCALE_UNIT_DAY,      PLANNER_SCALE_FORMAT_LONG,     PLANNER_SCALE_UNIT_HALFDAY,    PLANNER_SCALE_FORMAT_MEDIUM, HOUR },

	{ PLANNER_SCALE_UNIT_DAY,      PLANNER_SCALE_FORMAT_LONG,     PLANNER_SCALE_UNIT_TWO_HOURS,  PLANNER_SCALE_FORMAT_MEDIUM, HOUR },

	{ PLANNER_SCALE_UNIT_DAY,      PLANNER_SCALE_FORMAT_LONG,     PLANNER_SCALE_UNIT_HOUR,       PLANNER_SCALE_FORMAT_MEDIUM, HOUR },
	{ PLANNER_SCALE_UNIT_DAY,      PLANNER_SCALE_FORMAT_LONG,     PLANNER_SCALE_UNIT_HOUR,       PLANNER_SCALE_FORMAT_MEDIUM, HOUR }
};

const PlannerScaleConf *planner_scale_conf = scale_conf;

mrptime
planner_scale_time_prev (mrptime     t,
		    PlannerScaleUnit unit)
{
	struct tm *tm;

	tm = mrp_time_to_tm (t);
	
	switch (unit) {
	case PLANNER_SCALE_UNIT_HOUR:
		tm->tm_min = 0;
		tm->tm_sec = 0;
		break;

	case PLANNER_SCALE_UNIT_TWO_HOURS:
		tm->tm_min = 0;
		tm->tm_sec = 0;
		tm->tm_hour -= 2 - tm->tm_hour % 2;
		break;

	case PLANNER_SCALE_UNIT_HALFDAY:
		if (tm->tm_hour < 12) {
			tm->tm_hour = 0;
		} else {
			tm->tm_hour = 12;
		}
		tm->tm_min = 0;
		tm->tm_sec = 0;
		break;
		
	case PLANNER_SCALE_UNIT_DAY:
		tm->tm_hour = 0;
		tm->tm_min = 0;
		tm->tm_sec = 0;
		break;
		
	case PLANNER_SCALE_UNIT_WEEK:
		tm->tm_mday -= tm->tm_wday - START_OF_WEEK; 
		tm->tm_hour = 0;
		tm->tm_min = 0;
		tm->tm_sec = 0;
		break;

	case PLANNER_SCALE_UNIT_MONTH:
		tm->tm_mday = 1;
		tm->tm_hour = 0;
		tm->tm_min = 0;
		tm->tm_sec = 0;
		break;

	case PLANNER_SCALE_UNIT_QUARTER:
		tm->tm_mday = 1;
		tm->tm_hour = 0;
		tm->tm_min = 0;
		tm->tm_sec = 0;
		if (tm->tm_mon >= 0 && tm->tm_mon <= 2) {
			tm->tm_mon = 0;
		}
		else if (tm->tm_mon >= 3 && tm->tm_mon <= 5) {
			tm->tm_mon = 3;
		}
		else if (tm->tm_mon >= 6 && tm->tm_mon <= 8) {
			tm->tm_mon = 6;
		}
		else if (tm->tm_mon >= 9 && tm->tm_mon <= 11) {
			tm->tm_mon = 9;
		}
		break;
		
	case PLANNER_SCALE_UNIT_HALFYEAR:
		if (tm->tm_mon <= 5) {
			tm->tm_mon = 0;
		} else {
			tm->tm_mon = 6;
		}
		tm->tm_mday = 1;
		tm->tm_hour = 0;
		tm->tm_min = 0;
		tm->tm_sec = 0;
		break;

	case PLANNER_SCALE_UNIT_YEAR:
		tm->tm_mday = 1;
		tm->tm_mon = 0;
		tm->tm_hour = 0;
		tm->tm_min = 0;
		tm->tm_sec = 0;
		break;

	case PLANNER_SCALE_UNIT_NONE:
		break;

	default:
		g_assert_not_reached ();
	}

	return mrp_time_from_tm (tm);
}

mrptime
planner_scale_time_next (mrptime     t,
		    PlannerScaleUnit unit)
{
	struct tm *tm;
	
	tm = mrp_time_to_tm (t);
	
	switch (unit) {
	case PLANNER_SCALE_UNIT_HOUR:
		tm->tm_min = 0;
		tm->tm_sec = 0;
		tm->tm_hour++;
		break;

	case PLANNER_SCALE_UNIT_TWO_HOURS:
		tm->tm_min = 0;
		tm->tm_sec = 0;
		tm->tm_hour += 2 - tm->tm_hour % 2;
		break;

	case PLANNER_SCALE_UNIT_HALFDAY:
		if (tm->tm_hour < 12) {
			tm->tm_hour = 12;
		} else {
			tm->tm_hour = 0;
			tm->tm_mday++;
		}
		tm->tm_min = 0;
		tm->tm_sec = 0;
		break;
		
	case PLANNER_SCALE_UNIT_DAY:
		tm->tm_hour = 0;
		tm->tm_min = 0;
		tm->tm_sec = 0;
		tm->tm_mday++;
		break;
		
	case PLANNER_SCALE_UNIT_WEEK:
		tm->tm_hour = 0;
		tm->tm_min = 0;
		tm->tm_sec = 0;
		tm->tm_mday += 6 - tm->tm_wday + START_OF_WEEK + 1;
		break;

	case PLANNER_SCALE_UNIT_MONTH:
		tm->tm_mday = 1;
		tm->tm_hour = 0;
		tm->tm_min = 0;
		tm->tm_sec = 0;
		tm->tm_mon++;
		break;

	case PLANNER_SCALE_UNIT_QUARTER:
		tm->tm_mday = 1;
		tm->tm_hour = 0;
		tm->tm_min = 0;
		tm->tm_sec = 0;
		if (tm->tm_mon >= 0 && tm->tm_mon <= 2) {
			tm->tm_mon = 3;
		}
		else if (tm->tm_mon >= 3 && tm->tm_mon <= 5) {
			tm->tm_mon = 6;
		}
		else if (tm->tm_mon >= 6 && tm->tm_mon <= 8) {
			tm->tm_mon = 9;
		}
		else if (tm->tm_mon >= 9 && tm->tm_mon <= 11) {
			tm->tm_mon = 12;
		}
		break;
		
	case PLANNER_SCALE_UNIT_HALFYEAR:
		if (tm->tm_mon <= 5) {
			tm->tm_mon = 6;
		} else {
			tm->tm_mon = 0;
			tm->tm_year++;
		}
		tm->tm_mday = 1;
		tm->tm_hour = 0;
		tm->tm_min = 0;
		tm->tm_sec = 0;
		break;
		
	case PLANNER_SCALE_UNIT_YEAR:
		tm->tm_mon = 0;
		tm->tm_mday = 1;
		tm->tm_hour = 0;
		tm->tm_min = 0;
		tm->tm_sec = 0;
		tm->tm_year++;
		break;

	case PLANNER_SCALE_UNIT_NONE:
		break;
		
	default:
		g_assert_not_reached ();
	}

	return mrp_time_from_tm (tm);
}

gchar *
planner_scale_format_time (mrptime       t,
		      PlannerScaleUnit   unit,
		      PlannerScaleFormat format)
{
	struct tm *tm;
	gchar     *str = NULL;
	gint       num;

	tm = mrp_time_to_tm (t);
	
	switch (unit) {
	case PLANNER_SCALE_UNIT_HOUR:
		switch (format) {
		default:
			str = g_strdup_printf ("%d", tm->tm_hour);
			break;
		}
		break;

	case PLANNER_SCALE_UNIT_TWO_HOURS:
		switch (format) {
		default:
			str = g_strdup_printf ("%d", tm->tm_hour);
			break;
		}
		break;
		
	case PLANNER_SCALE_UNIT_HALFDAY:
		switch (format) {
		default:
			str = g_strdup_printf ("%d", tm->tm_hour);
			break;
		}
		break;
		
	case PLANNER_SCALE_UNIT_DAY:
		switch (format) {
		case PLANNER_SCALE_FORMAT_SHORT:
			str = g_strdup_printf ("%d", tm->tm_mday);
			break;
		case PLANNER_SCALE_FORMAT_MEDIUM:
			str = g_strdup_printf ("%s %d",
					       mrp_time_day_name (t),
					       tm->tm_mday);
			break;
		case PLANNER_SCALE_FORMAT_LONG:
			str = g_strdup_printf ("%s, %s %d",
					       mrp_time_day_name (t),
					       mrp_time_month_name (t),
					       tm->tm_mday);
			break;
		}
		break;

	case PLANNER_SCALE_UNIT_WEEK:
		switch (format) {
		case PLANNER_SCALE_FORMAT_SHORT:
			/* i18n: Short "Week", preferably 2 letters. */
			str = g_strdup_printf (_("Wk %d"),
					       mrp_time_week_number (t));
			break;
		case PLANNER_SCALE_FORMAT_MEDIUM:
			str = g_strdup_printf (_("Week %d"),
					       mrp_time_week_number (t));
			break; 
		case PLANNER_SCALE_FORMAT_LONG:
			/* i18n: Week, year. */
			str = g_strdup_printf (_("Week %d, %d"),
					       mrp_time_week_number (t),
					       tm->tm_year + 1900);
			break;
		}
		break;

	case PLANNER_SCALE_UNIT_MONTH:
		switch (format) {
		case PLANNER_SCALE_FORMAT_SHORT:
			str = g_strdup_printf ("%s",
					       mrp_time_month_name_initial (t));
			break;
		case PLANNER_SCALE_FORMAT_MEDIUM:
			str = g_strdup_printf ("%s",
					       mrp_time_month_name (t));
			break;
		case PLANNER_SCALE_FORMAT_LONG:
			str = g_strdup_printf ("%s %d",
					       mrp_time_month_name (t),
					       tm->tm_year + 1900);
			break;
		}
		break;
		
	case PLANNER_SCALE_UNIT_QUARTER:
		num = 1 + floor (tm->tm_mon / 3);
		
		switch (format) {
		case PLANNER_SCALE_FORMAT_SHORT:
			/* i18n: Short "Quarter", preferrably 1 letter. */
			str = g_strdup_printf (_("Q%d"), num);
			break;
		case PLANNER_SCALE_FORMAT_MEDIUM:
			/* i18n: Short "Quarter", preferrably 2-3 letters. */
			str = g_strdup_printf (_("Qtr %d"), num);
			break; 
		case PLANNER_SCALE_FORMAT_LONG:
			/* i18n: Year, short "Quarter", preferrably 2-3 letters. */
			str = g_strdup_printf (_("%d, Qtr %d"),
					       tm->tm_year + 1900,
					       num);
			break;
		}
		break;
	
	case PLANNER_SCALE_UNIT_HALFYEAR:
		num = 1 + floor (tm->tm_mon / 6);

		switch (format) {
		case PLANNER_SCALE_FORMAT_SHORT:
			/* i18n: Short "Half year", prefferably 1 letter. */
			str = g_strdup_printf (_("H%d"), num);
			break;
		case PLANNER_SCALE_FORMAT_LONG:
		case PLANNER_SCALE_FORMAT_MEDIUM:
			/* i18n: Year, short "Half year", prefferably 1 letter. */
			str = g_strdup_printf (_("%04d, H%d"),
					       tm->tm_year + 1900,
					       num);
			break;
		}
		break;

	case PLANNER_SCALE_UNIT_YEAR:
		switch (format) {
		default:
			str = g_strdup_printf ("%d", tm->tm_year + 1900);
			break;
		}
		break;

	case PLANNER_SCALE_UNIT_NONE:
		str = NULL;
		break;
		
	default:
		g_assert_not_reached ();
		break;
	}

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

