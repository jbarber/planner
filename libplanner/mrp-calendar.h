/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2001-2002 CodeFactory AB
 * Copyright (C) 2001-2002 Richard Hult <richard@imendio.com>
 * Copyright (C) 2001-2002 Mikael Hallendal <micke@imendio.com>
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

#ifndef __MRP_CALENDAR_H__
#define __MRP_CALENDAR_H__

#include <glib-object.h>
#include <time.h>

#include <libplanner/mrp-object.h>
#include <libplanner/mrp-types.h>
#include <libplanner/mrp-time.h>

#define MRP_TYPE_CALENDAR		(mrp_calendar_get_type ())
#define MRP_CALENDAR(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), MRP_TYPE_CALENDAR, MrpCalendar))
#define MRP_CALENDAR_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), MRP_TYPE_CALENDAR, MrpCalendarClass))
#define MRP_IS_CALENDAR(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), MRP_TYPE_CALENDAR))
#define MRP_IS_CALENDAR_CLASS(klass)	(G_TYPE_CHECK_TYPE ((obj), MRP_TYPE_CALENDAR))
#define MRP_CALENDAR_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), MRP_TYPE_CALENDAR, MrpCalendarClass))

#define MRP_TYPE_INTERVAL               (mrp_interval_get_type ())

typedef struct _MrpCalendar         MrpCalendar;
typedef struct _MrpCalendarClass    MrpCalendarClass;
typedef struct _MrpCalendarPriv     MrpCalendarPriv;
typedef struct _MrpInterval         MrpInterval;

#include <libplanner/mrp-day.h>

/* Used for saving calendar data. */
typedef struct {
	MrpDay *day;
	GList *intervals;
} MrpDayWithIntervals;

typedef struct {
	mrptime date;
	MrpDay *day;
} MrpDateWithDay;

struct _MrpCalendar {
	MrpObject        parent;

	MrpCalendarPriv *priv;
};

struct _MrpCalendarClass {
	MrpObjectClass   parent_class;
};

enum {
	MRP_CALENDAR_DAY_SUN,
	MRP_CALENDAR_DAY_MON,
	MRP_CALENDAR_DAY_TUE,
	MRP_CALENDAR_DAY_WED,
	MRP_CALENDAR_DAY_THU,
	MRP_CALENDAR_DAY_FRI,
	MRP_CALENDAR_DAY_SAT
};

GType        mrp_calendar_get_type                 (void) G_GNUC_CONST;
MrpCalendar *mrp_calendar_new                      (const gchar *name,
						    MrpProject  *project);
void         mrp_calendar_add                      (MrpCalendar *calendar,
						    MrpCalendar *parent);
MrpCalendar *mrp_calendar_copy                     (const gchar *name,
						    MrpCalendar *calendar);
MrpCalendar *mrp_calendar_derive                   (const gchar *name,
						    MrpCalendar *parent);
void         mrp_calendar_reparent                 (MrpCalendar *new_parent,
						    MrpCalendar *child);
void         mrp_calendar_remove                   (MrpCalendar *calendar);
const gchar *mrp_calendar_get_name                 (MrpCalendar *calendar);
void         mrp_calendar_set_name                 (MrpCalendar *calendar,
						    const gchar *name);
void         mrp_calendar_day_set_intervals        (MrpCalendar *calendar,
						    MrpDay      *day,
						    GList       *intervals);
GList *      mrp_calendar_day_get_intervals        (MrpCalendar *calendar,
						    MrpDay      *day,
						    gboolean     check_ancestors);
gint         mrp_calendar_day_get_total_work       (MrpCalendar *calendar,
						    MrpDay      *day);
MrpDay *     mrp_calendar_get_day                  (MrpCalendar *calendar,
						    mrptime      date,
						    gboolean     check_ancestors);
MrpDay *     mrp_calendar_get_default_day          (MrpCalendar *calendar,
						    gint         week_day);
void         mrp_calendar_set_default_days         (MrpCalendar *calendar,
						    gint         week_day,
						    ...);
void         mrp_calendar_set_days                 (MrpCalendar *calendar,
						    mrptime      date,
						    ...);
MrpCalendar *mrp_calendar_get_parent               (MrpCalendar *calendar);
GList *      mrp_calendar_get_children             (MrpCalendar *calendar);
GList *      mrp_calendar_get_overridden_days      (MrpCalendar *calendar);
GList *      mrp_calendar_get_all_overridden_dates (MrpCalendar *calendar);

/* Interval */
GType        mrp_interval_get_type                 (void);
MrpInterval *mrp_interval_new                      (mrptime      start,
						    mrptime      end);
MrpInterval *mrp_interval_copy                     (MrpInterval *interval);
MrpInterval *mrp_interval_ref                      (MrpInterval *interval);
void         mrp_interval_unref                    (MrpInterval *interval);
void         mrp_interval_get_absolute             (MrpInterval *interval,
						    mrptime      offset,
						    mrptime     *start,
						    mrptime     *end);


#endif /* __MRP_CALENDAR_H__ */
