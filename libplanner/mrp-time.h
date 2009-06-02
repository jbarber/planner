/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2005 Imendio AB
 * Copyright (C) 2002 CodeFactory AB
 * Copyright (C) 2002 Richard Hult <richard@imendio.com>
 * Copyright (C) 2002 Mikael Hallendal <micke@imendio.com>
 * Copyright (C) 2002 Alvaro del Castillo <acs@barrapunto.com>
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

#ifndef __MRP_TIME_H__
#define __MRP_TIME_H__

#include <time.h>
#include <glib.h>
#include <glib-object.h>

typedef long mrptime;

#define MRP_TIME_INVALID 0
#define MRP_TIME_MIN 0
#define MRP_TIME_MAX 2147483647

typedef enum {
	MRP_TIME_UNIT_NONE,
	MRP_TIME_UNIT_YEAR,
	MRP_TIME_UNIT_HALFYEAR,
	MRP_TIME_UNIT_QUARTER,
	MRP_TIME_UNIT_MONTH,
	MRP_TIME_UNIT_WEEK,
	MRP_TIME_UNIT_DAY,
	MRP_TIME_UNIT_HALFDAY,
	MRP_TIME_UNIT_TWO_HOURS,
	MRP_TIME_UNIT_HOUR
} MrpTimeUnit;

mrptime      mrp_time_current_time       (void);
mrptime      mrp_time_compose            (gint          year,
					  gint          month,
					  gint          day,
					  gint          hour,
					  gint          minute,
					  gint          second);
gboolean     mrp_time_decompose          (mrptime       t,
					  gint         *year,
					  gint         *month,
					  gint         *day,
					  gint         *hour,
					  gint         *minute,
					  gint         *second);
mrptime      mrp_time_from_string        (const gchar  *str,
					  GError      **err);
gchar *      mrp_time_to_string          (mrptime       t);
mrptime      mrp_time_from_msdate_string (const gchar  *str);
mrptime      mrp_time_align_day          (mrptime       t);
mrptime      mrp_time_align_prev         (mrptime       t,
					  MrpTimeUnit   unit);
mrptime      mrp_time_align_next         (mrptime       t,
					  MrpTimeUnit   unit);
gint         mrp_time_day_of_week        (mrptime       t);
gint         mrp_time_week_number        (mrptime       t);
const gchar *mrp_time_day_name           (mrptime       t);
const gchar *mrp_time_month_name         (mrptime       t);
const gchar *mrp_time_month_name_initial (mrptime       t);
gchar *      mrp_time_format             (const gchar  *format,
					  mrptime       t);
gchar *      mrp_time_format_locale      (mrptime       t);
void         mrp_time_debug_print        (mrptime       t);
GParamSpec * mrp_param_spec_time         (const gchar  *name,
					  const gchar  *nick,
					  const gchar  *blurb,
					  GParamFlags   flags);
mrptime      mrp_time_from_tm 		 (struct tm *tm);


/*
 * New API here.
 */

typedef struct _MrpTime MrpTime;


MrpTime *    mrp_time2_new               (void);
void         mrp_time2_free              (MrpTime     *t);
void         mrp_time2_set_date          (MrpTime     *t,
					  gint         year,
					  gint         month,
					  gint         day);
void         mrp_time2_set_time          (MrpTime     *t,
					  gint         hour,
					  gint         min,
					  gint         sec);
void         mrp_time2_get_date          (MrpTime     *t,
					  gint        *year,
					  gint        *month,
					  gint        *day);
void         mrp_time2_get_time          (MrpTime     *t,
					  gint        *hour,
					  gint        *min,
					  gint        *sec);
void         mrp_time2_add_years         (MrpTime     *t,
					  gint         years);
void         mrp_time2_add_months        (MrpTime     *t,
					  gint         months);
void         mrp_time2_add_days          (MrpTime     *t,
					  gint         days);
void         mrp_time2_add_seconds       (MrpTime     *t,
					  gint64       secs);
void         mrp_time2_add_minutes       (MrpTime     *t,
					  gint64       mins);
void         mrp_time2_add_hours         (MrpTime     *t,
					  gint64       hours);
void         mrp_time2_subtract_years    (MrpTime     *t,
					  gint         years);
void         mrp_time2_subtract_months   (MrpTime     *t,
					  gint         months);
void         mrp_time2_subtract_days     (MrpTime     *t,
					  gint         days);
void         mrp_time2_subtract_hours    (MrpTime     *t,
					  gint64       hours);
void         mrp_time2_subtract_minutes  (MrpTime     *t,
					  gint64       mins);
void         mrp_time2_subtract_seconds  (MrpTime     *t,
					  gint64       secs);
void         mrp_time2_debug_print       (MrpTime     *t);
gboolean     mrp_time2_set_from_string   (MrpTime     *t,
					  const gchar *str);
gchar *      mrp_time2_to_string         (MrpTime     *t);
void         mrp_time2_set_epoch         (MrpTime     *t,
					  time_t       epoch);
time_t       mrp_time2_get_epoch         (MrpTime     *t);
const gchar *mrp_time2_get_day_name      (MrpTime     *t);
const gchar *mrp_time2_get_month_name    (MrpTime     *t);
const gchar *mrp_time2_get_month_initial (MrpTime     *t);
gint         mrp_time2_get_week_number   (MrpTime     *t,
					  gint        *year);
void         mrp_time2_align_prev        (MrpTime     *t,
					  MrpTimeUnit  unit);
void         mrp_time2_align_next        (MrpTime     *t,
					  MrpTimeUnit  unit);
void         mrp_time2_copy              (MrpTime     *dst,
					  MrpTime     *src);
void         mrp_time2_clear             (MrpTime     *t);
gint         mrp_time2_compare           (MrpTime     *t1,
					  MrpTime     *t2);

#endif /* __MRP_TIME_H__ */
