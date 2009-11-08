/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * GTK Calendar Widget
 * Copyright (C) 1998 Cesar Miquel and Shawn T. Amundson
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
 * Modified by the GTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/.
 */

#ifndef __PLANNER_CALENDAR_H__
#define __PLANNER_CALENDAR_H__

#include <gdk/gdk.h>
#include <gtk/gtk.h>

#define PLANNER_TYPE_CALENDAR                  (planner_calendar_get_type ())
#define PLANNER_CALENDAR(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), PLANNER_TYPE_CALENDAR, PlannerCalendar))
#define PLANNER_CALENDAR_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), PLANNER_TYPE_CALENDAR, PlannerCalendarClass))
#define PLANNER_IS_CALENDAR(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PLANNER_TYPE_CALENDAR))
#define PLANNER_IS_CALENDAR_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), PLANNER_TYPE_CALENDAR))
#define PLANNER_CALENDAR_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), PLANNER_TYPE_CALENDAR, PlannerCalendarClass))


typedef struct _PlannerCalendar	      PlannerCalendar;
typedef struct _PlannerCalendarClass       PlannerCalendarClass;

typedef enum
{
	PLANNER_CALENDAR_SHOW_HEADING	= 1 << 0,
	PLANNER_CALENDAR_SHOW_DAY_NAMES	= 1 << 1,
	PLANNER_CALENDAR_NO_MONTH_CHANGE	= 1 << 2,
	PLANNER_CALENDAR_SHOW_WEEK_NUMBERS	= 1 << 3,
	PLANNER_CALENDAR_WEEK_START_MONDAY	= 1 << 4} PlannerCalendarDisplayOptions;

typedef enum
{
	PLANNER_CALENDAR_MARK_NONE,
	PLANNER_CALENDAR_MARK_BOLD,
	PLANNER_CALENDAR_MARK_UNDERLINE,
	PLANNER_CALENDAR_MARK_SHADE,
	PLANNER_CALENDAR_MARK_STRIPE
} PlannerCalendarMarkType;

struct _PlannerCalendar
{
	GtkWidget widget;

	GtkStyle  *header_style;
	GtkStyle  *label_style;

	gint month;
	gint year;
	gint selected_day;

	gint day_month[6][7];
	gint day[6][7];

	gint num_marked_dates;
	gint marked_date[31];
	PlannerCalendarDisplayOptions  display_flags;
	GdkColor marked_date_color[31];

	GdkGC *gc;
	GdkGC *xor_gc;

	gint focus_row;
	gint focus_col;

	gint highlight_row;
	gint highlight_col;

	gpointer private_data;
	gchar grow_space [32];

	/* Padding for future expansion */
	void (*_gtk_reserved1) (void);
	void (*_gtk_reserved2) (void);
	void (*_gtk_reserved3) (void);
	void (*_gtk_reserved4) (void);
};

struct _PlannerCalendarClass
{
	GtkWidgetClass parent_class;

	/* Signal handlers */
	void (* month_changed)		(PlannerCalendar *calendar);
	void (* day_selected)			(PlannerCalendar *calendar);
	void (* day_selected_double_click)	(PlannerCalendar *calendar);
	void (* prev_month)			(PlannerCalendar *calendar);
	void (* next_month)			(PlannerCalendar *calendar);
	void (* prev_year)			(PlannerCalendar *calendar);
	void (* next_year)			(PlannerCalendar *calendar);

};


GType	   planner_calendar_get_type	(void) G_GNUC_CONST;
GtkWidget* planner_calendar_new		(void);

gboolean   planner_calendar_select_month	(PlannerCalendar *calendar,
					 guint	      month,
					 guint	      year);
void	   planner_calendar_select_day	(PlannerCalendar *calendar,
					 guint	      day);
gboolean   planner_calendar_unmark_day	(PlannerCalendar *calendar,
					 guint	      day);
void	   planner_calendar_clear_marks	(PlannerCalendar *calendar);


void	   planner_calendar_display_options (PlannerCalendar		  *calendar,
					 PlannerCalendarDisplayOptions flags);

void	   planner_calendar_get_date	(PlannerCalendar *calendar,
				 guint	     *year,
				 guint	     *month,
				 guint	     *day);
void	   planner_calendar_freeze		(PlannerCalendar *calendar);
void	   planner_calendar_thaw		(PlannerCalendar *calendar);



void       planner_calendar_mark_day (PlannerCalendar         *calendar,
				 guint               day,
				 PlannerCalendarMarkType  type);


#endif /* __PLANNER_CALENDAR_H__ */
