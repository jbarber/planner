/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2004 Imendio HB
 * Copyright (C) 2001-2003 CodeFactory AB
 * Copyright (C) 2001-2003 Richard Hult <richard@imendio.com>
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

#include <config.h>
#include <string.h>

#include "mrp-marshal.h"
#include "mrp-intl.h"
#include "mrp-private.h"
#include "mrp-time.h"
#include "mrp-calendar.h"

/* Properties */
enum {
	PROP_0,
	PROP_NAME,
	PROP_PROJECT,
	PROP_CALENDAR
};

/* Signals, might use MrpObject::changed instead. */
enum {
	CALENDAR_CHANGED,
	LAST_SIGNAL
};

struct _MrpCalendarPriv {
	MrpProject  *project;
	gchar       *name;

	/* This can override the default calendar */
	MrpDay      *default_days[7];
	
	/* Tree structure */
	MrpCalendar *parent;
	GList      *children;
	
	/* Working time intervals set for day types in this calendar */
	GHashTable  *day_intervals;

	/* This can override single days and is hashed on the date */
	GHashTable  *days;
};

struct _MrpInterval 
{
        mrptime         start;
        mrptime         end;

        /* Private */
        guint           ref_count;
};

static void        calendar_class_init           (MrpCalendarClass *class);
static void        calendar_init                 (MrpCalendar      *module);
static void        calendar_finalize             (GObject          *object);
static void        calendar_set_property         (GObject          *object,
						  guint             prop_id,
						  const GValue     *value,
						  GParamSpec       *pspec);
static void        calendar_get_property         (GObject          *object,
						  guint             prop_id,
						  GValue           *value,
						  GParamSpec       *pspec);
static MrpDay *    calendar_get_default_day      (MrpCalendar      *calendar, 
						  mrptime           date,
						  gboolean          derive);
static MrpDay *    calendar_get_day              (MrpCalendar      *calendar,
						  mrptime           date,
						  gboolean          derive);
static MrpCalendar *  calendar_new               (const gchar      *name,
						  MrpCalendar      *parent);
static void        calendar_add_child            (MrpCalendar      *parent,
						  MrpCalendar      *child);
static void        calendar_reparent             (MrpCalendar      *new_parent,
						  MrpCalendar      *child);
static void        calendar_emit_changed         (MrpCalendar      *calendar);
static GList *    calendar_clean_intervals      (GList           *list);


static MrpObjectClass *parent_class;
static guint           signals[LAST_SIGNAL];

GType
mrp_calendar_get_type (void)
{
	static GType object_type = 0;

	if (!object_type) {
		static const GTypeInfo object_info = {
			sizeof (MrpCalendarClass),
			NULL,		/* base_init */
			NULL,		/* base_finalize */
			(GClassInitFunc) calendar_class_init,
			NULL,		/* class_finalize */
			NULL,		/* class_data */
			sizeof (MrpCalendar),
			0,              /* n_preallocs */
			(GInstanceInitFunc) calendar_init,
		};

		object_type = g_type_register_static (MRP_TYPE_OBJECT, 
						      "MrpCalendar", 
						      &object_info, 0);
	}

	return object_type;
}

static void
calendar_class_init (MrpCalendarClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	parent_class = MRP_OBJECT_CLASS (g_type_class_peek_parent (klass));

	object_class->finalize     = calendar_finalize;
	object_class->get_property = calendar_get_property;
	object_class->set_property = calendar_set_property;

	signals[CALENDAR_CHANGED] =
		g_signal_new ("calendar-changed",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      0,
			      NULL, NULL,
			      mrp_marshal_VOID__VOID,
			      G_TYPE_NONE, 
			      0);
	
	g_object_class_install_property (object_class,
					 PROP_NAME,
					 g_param_spec_string ("name",
							      "Name",
							      "The name of the calendar",
							      "empty",
							      G_PARAM_READWRITE));

	g_object_class_install_property (object_class,
					 PROP_PROJECT,
					 g_param_spec_pointer ("project",
							       "Project",
							       "The project this calendar belongs to",
							       G_PARAM_READWRITE));

	imrp_day_setup_defaults ();
}

static void
calendar_init (MrpCalendar *calendar)
{
	MrpCalendarPriv *priv;
	
	priv = g_new0 (MrpCalendarPriv, 1);
	
	priv->name   = NULL;
	priv->parent = NULL;
	priv->project = NULL;
	priv->days   = g_hash_table_new_full (NULL, NULL,
					      NULL, 
					      (GDestroyNotify) mrp_day_unref);
	priv->children = NULL;

	priv->day_intervals = g_hash_table_new (NULL, NULL);

	calendar->priv = priv;
}

static void
calendar_finalize (GObject *object)
{
	MrpCalendar     *calendar;
	MrpCalendarPriv *priv;

	calendar = MRP_CALENDAR (object);
	priv     = calendar->priv;

	/* FIXME: free the hash tables, etc... */

	g_free (calendar->priv);
	calendar->priv = NULL;

	if (G_OBJECT_CLASS (parent_class)->finalize) {
		(* G_OBJECT_CLASS (parent_class)->finalize) (object);
	}
}

static void
calendar_get_property (GObject    *object,
		       guint       prop_id,
		       GValue     *value,
		       GParamSpec *pspec)
{
	MrpCalendar     *calendar;
	MrpCalendarPriv *priv;
	
	calendar = MRP_CALENDAR (object);
	priv     = calendar->priv;
	
	switch (prop_id) {
	case PROP_NAME:
		g_value_set_string (value, priv->name);
		break;
	case PROP_PROJECT:
		g_value_set_object (value, priv->project);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
calendar_set_property (GObject         *object,
		       guint            prop_id,
		       const GValue    *value,
		       GParamSpec      *pspec)
{
	MrpCalendar     *calendar;
	MrpCalendarPriv *priv;
	
	calendar = MRP_CALENDAR (object);
	priv     = calendar->priv;
	
	switch (prop_id) {
	case PROP_NAME:
		mrp_calendar_set_name (calendar, g_value_get_string (value));
		break;
	case PROP_PROJECT:
		priv->project = MRP_PROJECT (g_value_get_pointer (value));
		break;
	default:
		break;
	}
}

static MrpDay *
calendar_get_default_day (MrpCalendar *calendar, mrptime date, gboolean derive)
{
	MrpCalendarPriv *priv;
	gint             week_day;
	
	g_return_val_if_fail (MRP_IS_CALENDAR (calendar), 0);

	priv     = calendar->priv;
	week_day = mrp_time_day_of_week (date);
	
	if (priv->default_days[week_day] == mrp_day_get_use_base ()) {
		if (!derive) {
			return mrp_day_get_use_base ();
		}
		
		/* Shouldn't be possible to set MRP_DAY_TYPE_USE_BASE when
		   priv->parent == NULL so no need to check here */
		return mrp_calendar_get_day (priv->parent, date, TRUE);
	}

	return priv->default_days[week_day];
}

static MrpDay *
calendar_get_day (MrpCalendar *calendar, mrptime date, gboolean derive)
{
	MrpCalendarPriv *priv;
	MrpDay          *day;
	
	g_return_val_if_fail (MRP_IS_CALENDAR (calendar), 0);

	priv = calendar->priv;

	day = (MrpDay *) g_hash_table_lookup (priv->days, 
					      GINT_TO_POINTER ((int)date));
	if (!day) {
		if (derive && priv->parent) {
			return calendar_get_day (priv->parent, date, derive);
		} else {
			return NULL;
		}
	}

	return day;
}

static MrpCalendar *
calendar_new (const gchar *name, MrpCalendar *parent)
{
	MrpCalendar *calendar;
	
	calendar = g_object_new (MRP_TYPE_CALENDAR,
				 "name", name, 
				 "project", parent->priv->project,
				 NULL);
	
	calendar_add_child (parent, calendar);

	return calendar;
}

static void
calendar_add_child (MrpCalendar *parent, MrpCalendar *child)
{
	if (child->priv->project != parent->priv->project) {
		g_warning ("Trying to add child calendar from different project than the parent calendar");
		return;
	}

	parent->priv->children = g_list_prepend (parent->priv->children, 
						  g_object_ref (child));
	
	child->priv->parent = parent;
}

static void
calendar_reparent (MrpCalendar *new_parent, MrpCalendar *child)
{
	if (child->priv->parent) {
		MrpCalendar *parent;

		parent = child->priv->parent;
		parent->priv->children = g_list_remove (parent->priv->children,
							 child);
		child->priv->parent = NULL;
	}

	calendar_add_child (new_parent, child);
	g_object_unref (child);
}

/**
 * mrp_calendar_new:
 * @name: name of the calendar
 * @project: the #MrpProject that the new calendar will belong to
 * 
 * Creates a new #MrpCalendar. The calendar will be empty so you need to set the
 * default week and/or override days, see mrp_calendar_set_default_days() and
 * mrp_calendar_set_days().
 *
 * Return value: A new #MrpCalendar.
 **/
MrpCalendar *
mrp_calendar_new (const gchar *name, MrpProject *project)
{
	MrpCalendar *calendar;

	calendar = calendar_new (name, mrp_project_get_root_calendar (project));
	
	imrp_project_signal_calendar_tree_changed (project);
	imrp_project_set_needs_saving (project, TRUE);

	return calendar;
}

static void
foreach_copy_day_intervals (gpointer     key,
			    gpointer     value,
			    MrpCalendar *copy)
{
	MrpDay *day = key;
	GList *list;

	list = g_list_copy (value);
	g_list_foreach (list, (GFunc) mrp_interval_ref, NULL);

	g_hash_table_insert (copy->priv->day_intervals, day, list);
}

static void
foreach_copy_days (gpointer     key,
		   gpointer     value,
		   MrpCalendar *copy)
{
	MrpDay *day = value;
	
	g_hash_table_insert (copy->priv->days, key, mrp_day_ref (day));
}

/**
 * mrp_calendar_add:
 * @calendar: a #MrpCalendar to add
 * @parent: a #MrpCalendar to inherit from 
 * 
 * Add @calendar to the project 
 * 
 * Return value:
 **/
void
mrp_calendar_add (MrpCalendar *calendar, MrpCalendar *parent)
{
	calendar_add_child (parent, calendar);

	imrp_project_signal_calendar_tree_changed (calendar->priv->project);
	imrp_project_set_needs_saving (calendar->priv->project, TRUE);
}

/**
 * mrp_calendar_copy:
 * @name: the name of the new calendar
 * @calendar: a #MrpCalendar to copy
 * 
 * Copies @calendar, making the new calendar a base calendar, that does not have
 * a parent.
 * 
 * Return value: a new #MrpCalendar that is a copy of @calendar.
 **/
MrpCalendar *
mrp_calendar_copy (const gchar *name, MrpCalendar *calendar)
{
	MrpCalendar *parent, *ret_val;

	parent = mrp_project_get_root_calendar (calendar->priv->project);
	
	ret_val = calendar_new (name, parent);

 	memcpy (ret_val->priv->default_days,
 		calendar->priv->default_days,
 		7 * sizeof (gint));

	g_hash_table_foreach (calendar->priv->day_intervals,
			      (GHFunc) foreach_copy_day_intervals,
			      ret_val);  

	g_hash_table_foreach (calendar->priv->days,
			      (GHFunc) foreach_copy_days,
			      ret_val);  

	imrp_project_signal_calendar_tree_changed (calendar->priv->project);
	imrp_project_set_needs_saving (calendar->priv->project, TRUE);

	return ret_val;
}

/**
 * mrp_calendar_derive:
 * @name: the name of the new calendar
 * @parent: the #MrpCalendar to derive
 * 
 * Derives a new calendar from @parent. The new calendar will inherit all
 * properties from @parent, so if no days are overridden, the calendars will be
 * identical.
 * 
 * Return value: a new #MrpCalendar that is derived from @parent.
 **/
MrpCalendar *
mrp_calendar_derive (const gchar *name, MrpCalendar *parent)
{
	MrpCalendar *ret_val;
	int          i;
	
	g_return_val_if_fail (MRP_IS_CALENDAR (parent), NULL);

	ret_val = calendar_new (name, parent);

	for (i = 0; i < 7; ++i) {
		ret_val->priv->default_days[i] = mrp_day_get_use_base ();
	}

	imrp_project_signal_calendar_tree_changed (ret_val->priv->project);
	imrp_project_set_needs_saving (ret_val->priv->project, TRUE);

	return ret_val;
}

/**
 * mrp_calendar_reparent:
 * @new_parent: the new parent
 * @child: an #MrpCalendar
 *
 * Changes the parent of @calendar so that it inherits @new_parent, instead of
 * its old parent.
 * 
 **/
void
mrp_calendar_reparent (MrpCalendar *new_parent, MrpCalendar *child)
{
	g_return_if_fail (MRP_IS_CALENDAR (new_parent));
	g_return_if_fail (MRP_IS_CALENDAR (child));
	
	calendar_reparent (new_parent, child);
	
	imrp_project_signal_calendar_tree_changed (new_parent->priv->project);
	imrp_project_set_needs_saving (new_parent->priv->project, TRUE);
}

/**
 * mrp_calendar_remove:
 * @calendar: an #MrpCalendar
 * 
 * Removes @calendar from the project. If the calendar is used by the project, a
 * new calendar is set for the project. If the calendar has a parent, the parent
 * is used, otherwise the first child of the root is used. For resources, the
 * calendar is exchanged for the parent if one exists, otherwise the resource
 * calendar is unset, so that the project default will be used.
 * 
 **/
void
mrp_calendar_remove (MrpCalendar *calendar)
{
	MrpCalendarPriv *priv;
	MrpCalendar     *parent;
	MrpCalendar     *root;
	GList          *list, *l;
	GList           *resources, *r;
	MrpCalendar     *tmp_cal, *new_cal = NULL;

	g_return_if_fail (MRP_IS_CALENDAR (calendar));

	priv   = calendar->priv;
	parent = priv->parent;

	root = mrp_project_get_root_calendar (priv->project);
	
	/* See if this calendar is used anywhere, if so we need to use another
	 * calendar.
	 *
	 */

	/* Project. Here we try to use the parent calendar, or if that's the
	 * root, use the first calendar under the root.
	 */
	if (parent != root) {
		new_cal = parent;
	} else {
		list = mrp_calendar_get_children (root);
		if (list) {
			new_cal = list->data;
		}	
	}
	
	if (!new_cal) {
		g_warning ("Couldn't find fallback calendar.");
	}
	
	tmp_cal = mrp_project_get_calendar (priv->project);
	
	if (tmp_cal == calendar) {
		g_object_set (priv->project, "calendar", new_cal, NULL);
	}
	
	/* Resources. Here we try to use the parent or if that fails,
	 * unset calendar so we get the project default.
	 */
	if (parent != root) {
		new_cal = parent;
	} else {
		new_cal = NULL;
	}
	
	resources = mrp_project_get_resources (priv->project);
	for (r = resources; r; r = r->next) {
		MrpResource *resource = r->data;
		
		tmp_cal = mrp_resource_get_calendar (resource);
		if (tmp_cal == calendar) {
			mrp_resource_set_calendar (resource, new_cal);
		}
	}
	
	/* FIXME: Need to check tasks when/if they get calendar
	 * support. Do it like for the resources.
	 */

	
	/* Remove it. We need to work on a copy, since the real list will be
	 * changed in calendar_reparent. Prevents corrupt list with infinite
	 * loops etc.
	 */
	list = g_list_copy (priv->children);
	
	for (l = list; l; l = l->next) {
		MrpCalendar *child = l->data;
		
		if (parent) {
			calendar_reparent (parent, child);
		} else {
			/* FIXME: Should never happen, right? */
			g_warning ("No new parent.");
			child->priv->parent = NULL;
		}
	}

	g_list_free (list);
	
	if (parent) {
		parent->priv->children = g_list_remove (parent->priv->children,
							 calendar);
		priv->parent = NULL;
	}

	imrp_project_signal_calendar_tree_changed (priv->project);
	imrp_project_set_needs_saving (priv->project, TRUE);
	
	g_object_unref (calendar);
}

/**
 * mrp_calendar_get_name:
 * @calendar: an #MrpCalendar
 * 
 * Retrieves the name of the calendar.
 * 
 * Return value: the calendar name.
 **/
const gchar *
mrp_calendar_get_name (MrpCalendar *calendar)
{
	g_return_val_if_fail (MRP_IS_CALENDAR (calendar), "");
	
	return calendar->priv->name;
}

/**
 * mrp_calendar_set_name:
 * @calendar: an #MrpCalendar
 * @name: the new name
 *
 * Sets the name of the calendar.
 * 
 **/
void
mrp_calendar_set_name (MrpCalendar *calendar, const gchar *name)
{
	MrpCalendarPriv *priv;
	
	g_return_if_fail (MRP_IS_CALENDAR (calendar));
	g_return_if_fail (name != NULL);

	priv = calendar->priv;
	
	g_free (priv->name);
	priv->name = g_strdup (name);
}

/**
 * mrp_calendar_day_set_intervals:
 * @calendar: an #MrpCalendar
 * @day: an #MrpDay
 * @intervals: list of #MrpInterval to set for the specified day
 *
 * Overrides the working time for the day type @day when used in @calendar.
 * 
 **/
void
mrp_calendar_day_set_intervals (MrpCalendar *calendar,
				MrpDay      *day,
				GList      *intervals)
{
	MrpCalendarPriv *priv;
	GList          *list;

	g_return_if_fail (MRP_IS_CALENDAR (calendar));

	priv = calendar->priv;

	list = g_hash_table_lookup (priv->day_intervals, day);
	if (list) {
		g_list_foreach (list, (GFunc) mrp_interval_unref, NULL);
		g_list_free (list);

		g_hash_table_remove (priv->day_intervals, day);
	}
	
	list = calendar_clean_intervals (intervals);

	g_hash_table_insert (priv->day_intervals, day, list);

	calendar_emit_changed (calendar);
	imrp_project_set_needs_saving (priv->project, TRUE);
}

/**
 * mrp_calendar_day_get_intervals:
 * @calendar: an #MrpCalendar
 * @day: an #MrpDay
 * @check_ancestors: specifies if the whole calendar hierarchy should be checked
 * 
 * Retrieves the working time for the given day/calendar combination. If
 * @check_ancestors is %TRUE, the calendar hierarchy is searched until a
 * calendar that has set the working time for this day type is found. If %FALSE,
 * the returned list will be empty if there is no explicit working time set for
 * @calendar.
 * 
 * Return value: List of #MrpInterval, specifying the working time for @day.
 **/
GList *
mrp_calendar_day_get_intervals (MrpCalendar *calendar,
				MrpDay      *day,
				gboolean     check_ancestors)
{
	MrpCalendarPriv *priv;
	GList          *list = NULL;
	
	g_return_val_if_fail (MRP_IS_CALENDAR (calendar), NULL);

	priv = calendar->priv;
	
	/* Look upwards in the tree structure until we find a calendar that has
	 * defined the working time intervals for this day type.
	 */
	list = g_hash_table_lookup (calendar->priv->day_intervals, day);

	if (!list && check_ancestors && priv->parent) {
		return mrp_calendar_day_get_intervals (priv->parent, day, TRUE);
	}

	return list;
}

/**
 * mrp_calendar_day_get_total_work:
 * @calendar: an #MrpCalendar
 * @day: an #MrpDay
 * 
 * Calculates the total amount of work for @day in @calendar.
 * 
 * Return value: the amount of work in seconds.
 **/
gint
mrp_calendar_day_get_total_work (MrpCalendar *calendar,
				 MrpDay      *day)
{
	MrpCalendarPriv *priv;
	GList          *list, *l;
	MrpInterval     *ival;
	gint             total = 0;
	mrptime          start, end;
	
	g_return_val_if_fail (MRP_IS_CALENDAR (calendar), 0);

	priv = calendar->priv;

	list = mrp_calendar_day_get_intervals (calendar, day, TRUE);

	for (l = list; l; l = l->next) {
		ival = l->data;

		mrp_interval_get_absolute (ival, 0, &start, &end);
		
		total += end - start;
	}

	return total;
}

/**
 * mrp_calendar_get_default_day:
 * @calendar: an #MrpCalendar
 * @week_day: integer in the range 0 - 6, where 0 is Sunday
 * 
 * Retrieves the default day for @calendar.
 * 
 * Return value: default #MrpDay.
 **/
MrpDay *
mrp_calendar_get_default_day (MrpCalendar *calendar,
			      gint         week_day)
{
	MrpCalendarPriv *priv;
	
	g_return_val_if_fail (MRP_IS_CALENDAR (calendar), NULL);

	priv = calendar->priv;

	return priv->default_days[week_day];
}

/**
 * mrp_calendar_set_default_days:
 * @calendar: an #MrpCalendar
 * @week_day: integer in the range 0 - 6, where 0 is Sunday
 * @...: #MrpDay followed by more week day/#MrpDay pairs, terminated by -1
 * 
 * Sets days in the default week for @calendar. Those are the days that are used
 * as fallback is a date is not overridden.
 *
 **/
void
mrp_calendar_set_default_days (MrpCalendar *calendar,
			       gint         week_day,
			       ...)
{
	MrpCalendarPriv *priv;
	va_list          args;
	
	g_return_if_fail (MRP_IS_CALENDAR (calendar));
	
	priv = calendar->priv;

	va_start (args, week_day);
	
	/* Loop the args */
	for (; week_day != -1; week_day = va_arg (args, gint)) {
		MrpDay *day = (MrpDay *) va_arg (args, gpointer);
		
		if (day == mrp_day_get_use_base () && !priv->parent) {
			g_warning ("Trying to set day type to use base calendar on a base calendar");
			continue;
		}
		
		priv->default_days[week_day] = day;
	}
	
	va_end (args);

	calendar_emit_changed (calendar);
	imrp_project_set_needs_saving (priv->project, TRUE);
}

/**
 * mrp_calendar_set_days:
 * @calendar: an #MrpCalendar
 * @date: an #mrptime
 * @...: #MrpDay followed by more #mrptime/#MrpDay pairs, terminated by -1
 *
 * Overrides specific dates in @calendar, setting the type of day to use for
 * those dates.
 * 
 **/
void
mrp_calendar_set_days (MrpCalendar *calendar,
		       mrptime      date,
		       ...)
{
	MrpCalendarPriv *priv;
	mrptime          time;
	gint             key;
	va_list          args;

	g_return_if_fail (MRP_IS_CALENDAR (calendar));
	
	priv = calendar->priv;
	va_start (args, date);
	
	for (time = date; time != -1; time = va_arg (args, mrptime)) {
		MrpDay *day;
		
		key = (int) mrp_time_align_day (time);
		
		day = (MrpDay *) va_arg (args, gpointer);
		if (day == mrp_day_get_use_base ()) {
			if (!priv->parent) {
				g_warning ("Trying to set USE_BASE in a base calendar");
				continue;
			}
			g_hash_table_remove (priv->days,
					     GINT_TO_POINTER (key));
			continue;
		}

		g_hash_table_insert (priv->days, GINT_TO_POINTER (key),
				     mrp_day_ref (day));
	}

	calendar_emit_changed (calendar);
	imrp_project_set_needs_saving (priv->project, TRUE);
}

/**
 * mrp_calendar_get_parent:
 * @calendar: an #MrpCalendar
 * 
 * Retrieves the parent calendar of @calendar. The parent is the calendar that a
 * calendar falls back to if a date or day type is not overridden.
 * 
 * Return value: The parent calendar.
 **/
MrpCalendar *
mrp_calendar_get_parent (MrpCalendar *calendar)
{
	g_return_val_if_fail (MRP_IS_CALENDAR (calendar), NULL);
	
	return calendar->priv->parent;
}

/**
 * mrp_calendar_get_children:
 * @calendar: an #MrpCalendar
 * 
 * Retreives a list of the children, i.e. the calenderas that are immediately
 * derived from @calendar.
 * 
 * Return value: List of @calendar's children.
 **/
GList *
mrp_calendar_get_children (MrpCalendar *calendar)
{
	g_return_val_if_fail (MRP_IS_CALENDAR (calendar), NULL);
	
	return calendar->priv->children;
}

/**
 * mrp_calendar_get_day:
 * @calendar: an #MrpCalendar
 * @date: an #mrptime
 * @check_ancestors: specifies if the whole calendar hierarchy should be checked
 * 
 * Retrieves the day type for the given date and calender. If @check_ancestors
 * is %TRUE, the parent and grandparent, and so on, is searched if @calendar
 * does not have an overridden day type for the specified date.
 * 
 * Return value: An #MrpDay.
 **/
MrpDay *
mrp_calendar_get_day (MrpCalendar *calendar, 
		      mrptime      date, 
		      gboolean     check_ancestors)
{
	MrpCalendarPriv *priv;
	mrptime          aligned_date;
	MrpDay          *day;

	g_return_val_if_fail (MRP_IS_CALENDAR (calendar), NULL);
	
	priv         = calendar->priv;
	aligned_date = mrp_time_align_day (date);
	day          = calendar_get_day (calendar, aligned_date, check_ancestors);
	
	if (!day) {
		return calendar_get_default_day (calendar,
						 aligned_date,
						 check_ancestors);
	}

	return day;
}

/* Interval */
static void     interval_free (MrpInterval *interval);

static void
interval_free (MrpInterval *interval)
{
        g_return_if_fail (interval != NULL);
        
        g_free (interval);
}

/**
 * mrp_interval_new:
 * @start: an #mrptime specifying the start of the interval
 * @end: an #mrptime specifying the end of the interval
 * 
 * Creates a new #MrpInterval ranging from @start to @end. 
 * 
 * Return value: The newly created interval.
 **/
MrpInterval *
mrp_interval_new (mrptime         start,
                  mrptime         end)
{
        MrpInterval *ret_val;
        
        ret_val = g_new0 (MrpInterval, 1);
        
        ret_val->start     = start;
        ret_val->end       = end;
        ret_val->ref_count = 1;

        return ret_val;
}

/**
 * mrp_interval_copy:
 * @interval: an #MrpInterval
 * 
 * Copies @interval.
 * 
 * Return value: The copied interval.
 **/
MrpInterval *
mrp_interval_copy (MrpInterval *interval)
{
        MrpInterval *ret_val;

        g_return_val_if_fail (interval != NULL, NULL);
        
        ret_val = g_new0 (MrpInterval, 1);
        
        memcpy (ret_val, interval, sizeof (MrpInterval));

	ret_val->ref_count = 1;
	
        return ret_val;
}

/**
 * mrp_interval_ref:
 * @interval: an #MrpInterval
 * 
 * Increases the reference count on @interval.
 * 
 * Return value: The interval.
 **/
MrpInterval *
mrp_interval_ref (MrpInterval *interval)
{
        g_return_val_if_fail (interval != NULL, NULL);
        
        interval->ref_count++;

        return interval;
}

/**
 * mrp_interval_unref:
 * @interval: an #MrpInterval
 *
 * Decreases the reference count on @interval. When the count goes to 0, the
 * interval is freed.
 * 
 **/
void
mrp_interval_unref (MrpInterval *interval)
{
        g_return_if_fail (interval != NULL);

        interval->ref_count--;
        
        if (interval->ref_count <= 0) {
                interval_free (interval);
        }
}

/**
 * mrp_interval_get_absolute:
 * @interval: an #MrpInterval
 * @offset: the offset to add to start and end
 * @start: location to store start time, or %NULL
 * @end: location to store end time, or %NULL
 *
 * Retrieves the start and end time of #interval, with an optional @offset.
 * 
 **/
void
mrp_interval_get_absolute (MrpInterval *interval,
			   mrptime      offset,
			   mrptime     *start,
			   mrptime     *end)
{
	g_return_if_fail (interval != NULL);
	
	if (start) {
		*start = interval->start + offset;
	}
	if (end) {
		*end   = interval->end + offset;
	}
}

static void
foreach_day_interval_add_to_list (MrpDay  *day,
				  GList  *intervals, 
				  GList **list)
{
	MrpDayWithIntervals *di = g_new0 (MrpDayWithIntervals, 1);
	di->day = day;
	di->intervals = intervals;
	
	*list = g_list_prepend (*list, di);
}

/**
 * mrp_calendar_get_overridden_days:
 * @calendar: an #MrpCalendar
 * 
 * Retrieves the days that are overridden in this calendar, and the intervals
 * that they are overriden with. This is mainly used when saving calendar data.
 * 
 * Return value: A list of #MrpDayWithIntervals structs, that must be freed
 * (both the list and the data).
 **/
GList *
mrp_calendar_get_overridden_days (MrpCalendar *calendar)
{
	MrpCalendarPriv *priv;
	GList           *ret_val = NULL;

	g_return_val_if_fail (MRP_IS_CALENDAR (calendar), NULL);
	
	priv = calendar->priv;
	
	g_hash_table_foreach (priv->day_intervals, 
			      (GHFunc) foreach_day_interval_add_to_list,
			      &ret_val);

	return ret_val;
}

static void
foreach_day_add_to_list (gpointer key, MrpDay *day, GList **list)
{
	MrpDateWithDay *dd = g_new0 (MrpDateWithDay, 1);
	dd->date = GPOINTER_TO_INT (key);
	dd->day  = day;
	
	*list = g_list_prepend (*list, dd);
}

/**
 * mrp_calendar_get_all_overridden_dates:
 * @calendar: an #MrpCalendar
 * 
 * Retrieves the overridden dates of @calendar, i.e. the specific dates that
 * differ from the parent calendar.
 *
 * Return value: A list of #MrpDateWithDay structs, that must be freed (both the
 * list and the data).
 **/
GList *
mrp_calendar_get_all_overridden_dates (MrpCalendar *calendar)
{
	MrpCalendarPriv *priv;
	GList           *ret_val = NULL;
	
	g_return_val_if_fail (MRP_IS_CALENDAR (calendar), NULL);
	
	priv = calendar->priv;
	
	g_hash_table_foreach (priv->days,
			      (GHFunc) foreach_day_add_to_list,
			      &ret_val);

	return ret_val;
}

typedef struct {
	MrpDay *day;
	GList *list;
} MatchingDayData;

static void
foreach_matching_day_add_to_list (gpointer         key,
				  MrpDay          *day,
				  MatchingDayData *data)
{
	if (day == data->day) {
		data->list = g_list_prepend (data->list, key);
	}
}

void
imrp_calendar_replace_day (MrpCalendar *calendar,
			   MrpDay      *orig_day,
			   MrpDay      *new_day)
{
	MrpCalendarPriv *priv;
	MatchingDayData  data;
	GList          *l;
	gint             i;

	g_return_if_fail (MRP_IS_CALENDAR (calendar));
	g_return_if_fail (orig_day != NULL);
	g_return_if_fail (new_day != NULL);

	priv = calendar->priv;
	
	/* Default week. */
	for (i = 0; i < 7; i++) {
		if (priv->default_days[i] == orig_day) {
			priv->default_days[i] = new_day;
		}
	}

	/* Overridden days. */
	data.list = NULL;
	data.day = orig_day;
	
	g_hash_table_foreach (priv->days, 
			      (GHFunc) foreach_matching_day_add_to_list,
			      &data);
	
	for (l = data.list; l; l = l->next) {
		mrptime date = GPOINTER_TO_INT (l->data);
		
		/*g_print ("Got overriden day, %s\n",
		  mrp_time_format ("%H:%M %a %e %b", date));*/

		mrp_calendar_set_days (calendar, date, new_day, -1);
	}		

	g_list_free (data.list);
}

static void
calendar_emit_changed (MrpCalendar *calendar)
{
	MrpCalendarPriv *priv;
	GList          *l;

	priv = calendar->priv;

	g_signal_emit (calendar, signals[CALENDAR_CHANGED], 0, NULL);
	
	for (l = priv->children; l; l = l->next) {
		calendar_emit_changed (l->data);
	}
}

static gint
compare_intervals_func (MrpInterval *a, MrpInterval *b)
{
	mrptime at, bt;

	mrp_interval_get_absolute (a, 0, &at, NULL);
	mrp_interval_get_absolute (b, 0, &bt, NULL);
	
	if (at < bt) {
		return -1;
	}
	else if (at > bt) {
		return 1;
	} else {
		return 0;
	}
}

static GList *
calendar_clean_intervals (GList *list)
{
	GList *l, *sorted = NULL, *merged = NULL;
	MrpInterval *ival;
	mrptime t1, t2;
	mrptime start, end;

	for (l = list; l; l = l->next) {
		ival = l->data;

		mrp_interval_get_absolute (ival, 0, &t1, &t2);

		/* Clean out empty and backwards intervals. */
		if (t1 >= t2) {
			continue;
		}
		
		sorted = g_list_prepend (sorted, ival);
	}
	
	sorted = g_list_sort (sorted, (GCompareFunc) compare_intervals_func);

	start = -1;
	end = -1;
	for (l = sorted; l; l = l->next) {
		mrp_interval_get_absolute (l->data, 0, &t1, &t2);

		if (start == -1) {
			/* First interval. */
			start = t1;
			end = t2;
		}
		else if (t1 <= end) {
			/* Expand the current interval. */
			end = MAX (end, t2);
		} else {
			/* Store the current interval and start a new one. */
			ival = mrp_interval_new (start, end);
			
			merged = g_list_prepend (merged, ival);

			start = t1;
			end = t2;
		}

		/* Add the last interval if needed. */ 
		if (!l->next && start != -1 && end != -1) {
			ival = mrp_interval_new (start, end);
			merged = g_list_prepend (merged, ival);
		}
	}

	g_list_free (sorted);
	merged = g_list_reverse (merged);

	return merged;
}

/* Boxed types. */

GType
mrp_interval_get_type (void)
{
	static GType our_type = 0;
  
	if (our_type == 0) {
		our_type = g_boxed_type_register_static ("MrpInterval",
							 (GBoxedCopyFunc) mrp_interval_ref,
							 (GBoxedFreeFunc) mrp_interval_unref);
	}
	
	return our_type;
}


