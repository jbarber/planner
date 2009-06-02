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

#include "mrp-private.h"
#include <glib/gi18n.h>
#include "mrp-day.h"

struct _MrpDay {
        MrpProject *project;
        gint        id;
        gchar      *name;
        gchar      *description;
        gint        ref_count;
};

static MrpDay *work_day     = NULL;
static MrpDay *nonwork_day  = NULL;
static MrpDay *use_base_day = NULL;

static void  day_free           (MrpDay *day);

static void
day_free (MrpDay *day)
{
        g_free (day->name);
        g_free (day->description);
        g_free (day);
}

void
imrp_day_setup_defaults (void)
{
	if (!work_day && !nonwork_day && !use_base_day) {
		work_day = mrp_day_add (NULL,
					_("Working"),
					_("A default working day"));
		nonwork_day = mrp_day_add (NULL,
					   _("Nonworking"),
					   _("A default non working day"));
		use_base_day = mrp_day_add (NULL,
					    _("Use base"),
					    _("Use day from base calendar"));
	}
}

/**
 * mrp_day_add:
 * @project: an #MrpProject
 * @name: the name of the day type
 * @description: human readable description of the day type
 *
 * Adds a new day type to @project.
 *
 * Return value: the newly created #MrpDay
 **/
MrpDay *
mrp_day_add (MrpProject *project, const gchar *name, const gchar *description)
{
        MrpDay *day;

        g_return_val_if_fail (name != NULL, NULL);

        day = g_new0 (MrpDay, 1);

        day->project   = project;
        day->ref_count = 1;
        day->name = g_strdup (name);
	day->id = g_quark_from_string (name);

        if (description) {
                day->description = g_strdup (description);
        }

        if (project) {
		imrp_project_add_calendar_day (project, day);
	}

        return day;
}

/**
 * mrp_day_get_all:
 * @project: an #MrpProject
 *
 * Fetches a list of all available day types in @project.
 *
 * Return value: the list of all available day types in @project
 **/
GList *
mrp_day_get_all (MrpProject *project)
{
	return imrp_project_get_calendar_days (project);
}

/**
 * mrp_day_remove:
 * @project: an #MrpProject
 * @day: an #MrpDay
 *
 * Remove @day from available day types in @project
 **/
void
mrp_day_remove (MrpProject *project, MrpDay *day)
{
	imrp_project_remove_calendar_day (project, day);
}

/**
 * mrp_day_get_id:
 * @day: an #MrpDay
 *
 * Fetches the id of @day
 *
 * Return value: the id of @day
 **/
gint
mrp_day_get_id (MrpDay *day)
{
        g_return_val_if_fail (day != NULL, -1);

        return day->id;
}

/**
 * mrp_day_get_name:
 * @day: an #MrpDay
 *
 * Fetches the name of @day
 *
 * Return value: the name of @day
 **/
const gchar *
mrp_day_get_name (MrpDay *day)
{
        g_return_val_if_fail (day != NULL, NULL);

        return day->name;
}

/**
 * mrp_day_set_name:
 * @day: an #MrpDay
 * @name: the new name
 *
 * Sets the name of @day to @name and emits the "day-changed" signal
 **/
void
mrp_day_set_name (MrpDay *day, const gchar *name)
{
        g_return_if_fail (day != NULL);

        g_free (day->name);
        day->name = g_strdup (name);

	if (day->project) {
		g_signal_emit_by_name (day->project,
				       "day_changed",
				       day);
	}
}

/**
 * mrp_day_get_description:
 * @day: an #MrpDay
 *
 * Fetches the description of @day
 *
 * Return value: the description of @day
 **/
const gchar *
mrp_day_get_description (MrpDay *day)
{
        g_return_val_if_fail (day != NULL, NULL);

        return day->description;
}

/**
 * mrp_day_set_description:
 * @day: an #MrpDay
 * @description: the new description
 *
 * Sets the description of @day to @description and emits the "day-changed"
 * signal
 **/
void
mrp_day_set_description (MrpDay *day, const gchar *description)
{
        g_return_if_fail (day != NULL);

        g_free (day->description);
        day->description = g_strdup (description);

	if (day->project) {
		g_signal_emit_by_name (day->project,
				       "day_changed",
				       day);
	}
}

/**
 * mrp_day_ref:
 * @day: #MrpDay
 *
 * Add a reference to @day. User should call this when storing a reference to
 * a day.
 *
 * Return value: the day
 **/
MrpDay *
mrp_day_ref (MrpDay *day)
{
        g_return_val_if_fail (day != NULL, NULL);

        day->ref_count++;

        return day;
}

/**
 * mrp_day_unref:
 * @day: an #MrpDay
 *
 * Remove a reference from property. If the reference count reaches 0 the
 * property will be freed. User should not use it's reference after calling
 * mrp_day_unref().
 **/
void
mrp_day_unref (MrpDay *day)
{
        g_return_if_fail (day != NULL);

        day->ref_count--;
        if (day->ref_count <= 0) {
                day_free (day);
        }
}

/**
 * mrp_day_get_work:
 *
 * Fetches the builtin day type work.
 *
 * Return value: the builtin day type work
 **/
MrpDay *
mrp_day_get_work (void)
{
        return work_day;
}

/**
 * mrp_day_get_nonwork:
 *
 * Fetches the builtin day type nonwork.
 *
 * Return value: the builtin day type nowork
 **/
MrpDay *
mrp_day_get_nonwork (void)
{
        return nonwork_day;
}

/**
 * mrp_day_get_use_base:
 *
 * Fetches the builtin day type user base
 *
 * Return value: the builtin day type use base
 **/
MrpDay *
mrp_day_get_use_base (void)
{
        return use_base_day;
}


/* Boxed types. */

GType
mrp_day_get_type (void)
{
	static GType our_type = 0;

	if (our_type == 0) {
		our_type = g_boxed_type_register_static ("MrpDay",
							 (GBoxedCopyFunc) mrp_day_ref,
							 (GBoxedFreeFunc) mrp_day_unref);
	}

	return our_type;
}


