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

#ifndef __MG_CALENDAR_H__
#define __MG_CALENDAR_H__

#include <gdk/gdk.h>
#include <gtk/gtkwidget.h>

#define MG_TYPE_CALENDAR                  (planner_calendar_get_type ())
#define MG_CALENDAR(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MG_TYPE_CALENDAR, MgCalendar))
#define MG_CALENDAR_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), MG_TYPE_CALENDAR, MgCalendarClass))
#define MG_IS_CALENDAR(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MG_TYPE_CALENDAR))
#define MG_IS_CALENDAR_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), MG_TYPE_CALENDAR))
#define MG_CALENDAR_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), MG_TYPE_CALENDAR, MgCalendarClass))


typedef struct _MgCalendar	      MgCalendar;
typedef struct _MgCalendarClass       MgCalendarClass;

typedef enum
{
	MG_CALENDAR_SHOW_HEADING	= 1 << 0,
	MG_CALENDAR_SHOW_DAY_NAMES	= 1 << 1,
	MG_CALENDAR_NO_MONTH_CHANGE	= 1 << 2,
	MG_CALENDAR_SHOW_WEEK_NUMBERS	= 1 << 3,
	MG_CALENDAR_WEEK_START_MONDAY	= 1 << 4} MgCalendarDisplayOptions;

typedef enum
{
	MG_CALENDAR_MARK_NONE,
	MG_CALENDAR_MARK_BOLD,
	MG_CALENDAR_MARK_UNDERLINE,
	MG_CALENDAR_MARK_SHADE,
	MG_CALENDAR_MARK_STRIPE
} MgCalendarMarkType;

struct _MgCalendar
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
	MgCalendarDisplayOptions  display_flags;
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

struct _MgCalendarClass
{
	GtkWidgetClass parent_class;
	
	/* Signal handlers */
	void (* month_changed)		(MgCalendar *calendar);
	void (* day_selected)			(MgCalendar *calendar);
	void (* day_selected_double_click)	(MgCalendar *calendar);
	void (* prev_month)			(MgCalendar *calendar);
	void (* next_month)			(MgCalendar *calendar);
	void (* prev_year)			(MgCalendar *calendar);
	void (* next_year)			(MgCalendar *calendar);
	
};


GType	   planner_calendar_get_type	(void) G_GNUC_CONST;
GtkWidget* planner_calendar_new		(void);

gboolean   planner_calendar_select_month	(MgCalendar *calendar, 
					 guint	      month,
					 guint	      year);
void	   planner_calendar_select_day	(MgCalendar *calendar,
					 guint	      day);
gboolean   planner_calendar_unmark_day	(MgCalendar *calendar,
					 guint	      day);
void	   planner_calendar_clear_marks	(MgCalendar *calendar);


void	   planner_calendar_display_options (MgCalendar		  *calendar,
					 MgCalendarDisplayOptions flags);

void	   planner_calendar_get_date	(MgCalendar *calendar, 
				 guint	     *year,
				 guint	     *month,
				 guint	     *day);
void	   planner_calendar_freeze		(MgCalendar *calendar);
void	   planner_calendar_thaw		(MgCalendar *calendar);



void       planner_calendar_mark_day (MgCalendar         *calendar,
				 guint               day,
				 MgCalendarMarkType  type);


#endif /* __MG_CALENDAR_H__ */
