/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
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

mrptime      mrp_time_current_time       (void);

struct tm *  mrp_time_to_tm              (mrptime       t);

mrptime      mrp_time_from_tm            (struct tm    *tm);

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

gint         mrp_time_day_of_week        (mrptime       t);

gint         mrp_time_week_number        (mrptime       t);

const gchar *mrp_time_day_name           (mrptime       t);

const gchar *mrp_time_month_name         (mrptime       t);

const gchar *mrp_time_month_name_initial (mrptime       t);

gchar *      mrp_time_format             (const gchar  *format,
					  mrptime       t);

void         mrp_time_debug_print        (mrptime       t);

GParamSpec * mrp_param_spec_time         (const gchar  *name,
					  const gchar  *nick,
					  const gchar  *blurb,
					  GParamFlags   flags);


#endif /* __MRP_TIME_H__ */
