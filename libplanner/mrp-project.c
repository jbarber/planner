/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2003-2004 Imendio HB
 * Copyright (C) 2001-2003 CodeFactory AB
 * Copyright (C) 2001-2003 Richard Hult <richard@imendio.com>
 * Copyright (C) 2001-2002 Mikael Hallendal <micke@imendio.com>
 * Copyright (C) 2001-2002 Alvaro del Castillo <acs@barrapunto.com>
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
#include <gsf/gsf-input-memory.h>
#include <gsf/gsf-input-stdio.h>
#include "mrp-error.h"
#include "mrp-intl.h"
#include "mrp-marshal.h"
#include "mrp-task-manager.h"
#include "mrp-storage-module-factory.h"
#include "mrp-storage-module.h"
#include "mrp-private.h"
#include "mrp-time.h"
#include "mrp-property.h"
#include "mrp-resource.h"
#include "mrp-project.h"

struct _MrpProjectPriv {
	MrpApplication   *app;
	gchar            *uri;

	MrpTaskManager   *task_manager;

	GList            *resources;
	GList            *groups;
	
	MrpStorageModule *primary_storage;

	mrptime           project_start;

	gchar            *organization;
	gchar            *manager;
	gchar            *name;
	
	/*GList          *storage_modules;*/        /* <MrpStorageModule> */
	gboolean          needs_saving;
	gboolean          empty;
	
	MrpGroup         *default_group;
	
	/* Property stuff */
	GParamSpecPool   *property_pool;

	/* Calendar stuff */
	MrpCalendar      *root_calendar;
	MrpCalendar      *calendar;
	GHashTable       *day_types;

	/* Project phases */
	GList            *phases;
	gchar            *phase;
};

/* Properties */
enum {
	PROP_0,
	PROP_PROJECT_START,
	PROP_NAME,
	PROP_ORGANIZATION,
	PROP_MANAGER,
	PROP_DEFAULT_GROUP,
	PROP_CALENDAR,
	PROP_PHASES,
	PROP_PHASE,
};

/* Signals */
enum {
	LOADED,
	RESOURCE_ADDED,
	RESOURCE_REMOVED,
	GROUP_ADDED,
	GROUP_REMOVED,
	DEFAULT_GROUP_CHANGED,
	TASK_INSERTED,
	TASK_REMOVED,
	TASK_MOVED,
	NEEDS_SAVING_CHANGED,
	PROPERTY_ADDED,
	PROPERTY_CHANGED,
	PROPERTY_REMOVED,
	CALENDAR_TREE_CHANGED,
	DAY_ADDED,
	DAY_REMOVED,
	DAY_CHANGED,
	LAST_SIGNAL
};


static void     project_class_init                (MrpProjectClass  *klass);
static void     project_init                      (MrpProject       *project);
static void     project_finalize                  (GObject          *object);
static void     project_set_property              (GObject          *object,
						   guint             prop_id,
						   const GValue     *value,
						   GParamSpec       *pspec);
static void     project_get_property              (GObject          *object,
						   guint             prop_id,
						   GValue           *value,
						   GParamSpec       *pspec);
static void     project_connect_object            (MrpObject        *object,
						   MrpProject       *project);
static void     project_setup_default_calendar    (MrpProject       *project);
static void     project_calendar_changed          (MrpCalendar      *calendar,
						   MrpProject       *project);
static void     project_set_calendar              (MrpProject       *project,
						   MrpCalendar      *calendar);
static gboolean project_load_from_sql             (MrpProject       *project,
						   const gchar      *uri,
						   GError          **error);
static gboolean project_set_storage               (MrpProject       *project,
						   const gchar      *storage_name);
#if 0
static void     project_dump_task_tree            (MrpProject       *project);
#endif

static GObjectClass *parent_class;
static guint signals[LAST_SIGNAL];

GType
mrp_project_get_type (void)
{
	static GType object_type = 0;

	if (!object_type) {
		static const GTypeInfo object_info = {
			sizeof (MrpProjectClass),
			NULL,		/* base_init */
			NULL,		/* base_finalize */
			(GClassInitFunc) project_class_init,
			NULL,		/* class_finalize */
			NULL,		/* class_data */
			sizeof (MrpProject),
			0,              /* n_preallocs */
			(GInstanceInitFunc) project_init,
		};

		object_type = g_type_register_static (MRP_TYPE_OBJECT, 
						      "MrpProject", 
						      &object_info, 0);
	}

	return object_type;
}

static void
project_class_init (MrpProjectClass *klass)
{
	GObjectClass *object_class = (GObjectClass *) klass;

	parent_class = g_type_class_peek_parent (klass);

	object_class->finalize     = project_finalize;
	object_class->set_property = project_set_property;
	object_class->get_property = project_get_property;

	signals[LOADED] = g_signal_new
		("loaded",
		 G_TYPE_FROM_CLASS (klass),
		 G_SIGNAL_RUN_LAST,
		 0, /*G_STRUCT_OFFSET (MrpProjectClass, method), */ 
		 NULL, NULL,
		 mrp_marshal_VOID__VOID,
		 G_TYPE_NONE, 0);

	signals[RESOURCE_ADDED] = g_signal_new
		("resource_added",
		 G_TYPE_FROM_CLASS (klass),
		 G_SIGNAL_RUN_LAST,
		 0, /*G_STRUCT_OFFSET (MrpProjectClass, method), */
		 NULL, NULL,
		 mrp_marshal_VOID__OBJECT,
		 G_TYPE_NONE,
		 1, MRP_TYPE_RESOURCE);

	signals[RESOURCE_REMOVED] = g_signal_new
		("resource_removed",
		 G_TYPE_FROM_CLASS (klass),
		 G_SIGNAL_RUN_LAST,
		 0, /*G_STRUCT_OFFSET (MrpProjectClass, method), */
		 NULL, NULL,
		 mrp_marshal_VOID__OBJECT,
		 G_TYPE_NONE,
		 1, MRP_TYPE_RESOURCE);
	
	signals[GROUP_ADDED] = g_signal_new
		("group_added",
		 G_TYPE_FROM_CLASS (klass),
		 G_SIGNAL_RUN_LAST,
		 0, /*G_STRUCT_OFFSET (MrpProjectClass, method), */
		 NULL, NULL,
		 mrp_marshal_VOID__OBJECT,
		 G_TYPE_NONE,
		 1, MRP_TYPE_GROUP);

	signals[GROUP_REMOVED] = g_signal_new
		("group_removed",
		 G_TYPE_FROM_CLASS (klass),
		 G_SIGNAL_RUN_LAST,
		 0, /*G_STRUCT_OFFSET (MrpProjectClass, method), */
		 NULL, NULL,
		 mrp_marshal_VOID__OBJECT,
		 G_TYPE_NONE,
		 1, MRP_TYPE_GROUP);

	signals[DEFAULT_GROUP_CHANGED] = g_signal_new
		("default_group_changed",
		 G_TYPE_FROM_CLASS (klass),
		 G_SIGNAL_RUN_LAST,
		 0, /*G_STRUCT_OFFSET (MrpProjectClass, method), */
		 NULL, NULL,
		 mrp_marshal_VOID__OBJECT,
		 G_TYPE_NONE,
		 1, MRP_TYPE_GROUP);
	
	signals[TASK_INSERTED] = g_signal_new
		("task_inserted",
		 G_TYPE_FROM_CLASS (klass),
		 G_SIGNAL_RUN_LAST,
		 0,
		 NULL, NULL,
		 mrp_marshal_VOID__OBJECT,
		 G_TYPE_NONE,
		 1, MRP_TYPE_TASK);

	signals[TASK_REMOVED] = g_signal_new
		("task_removed",
		 G_TYPE_FROM_CLASS (klass),
		 G_SIGNAL_RUN_LAST,
		 0,
		 NULL, NULL,
		 mrp_marshal_VOID__OBJECT,
		 G_TYPE_NONE,
		 1, MRP_TYPE_TASK);

	signals[TASK_MOVED] = g_signal_new
		("task_moved",
		 G_TYPE_FROM_CLASS (klass),
		 G_SIGNAL_RUN_LAST,
		 0,
		 NULL, NULL,
		 mrp_marshal_VOID__OBJECT,
		 G_TYPE_NONE,
		 1, MRP_TYPE_TASK);

	signals[NEEDS_SAVING_CHANGED] = g_signal_new
		("needs_saving_changed",
		 G_TYPE_FROM_CLASS (klass),
		 G_SIGNAL_RUN_LAST,
		 0,
		 NULL, NULL,
		 mrp_marshal_VOID__BOOLEAN,
		 G_TYPE_NONE,
		 1, G_TYPE_BOOLEAN);

	signals[PROPERTY_ADDED] = g_signal_new
		("property_added",
		 G_TYPE_FROM_CLASS (klass),
		 G_SIGNAL_RUN_LAST,
		 0,
		 NULL, NULL,
		 mrp_marshal_VOID__LONG_POINTER,
		 G_TYPE_NONE,
		 2, G_TYPE_LONG, G_TYPE_POINTER);
		
	signals[PROPERTY_CHANGED] = g_signal_new
		("property_changed",
		 G_TYPE_FROM_CLASS (klass),
		 G_SIGNAL_RUN_LAST,
		 0,
		 NULL, NULL,
		 mrp_marshal_VOID__POINTER,
		 G_TYPE_NONE,
		 1, G_TYPE_POINTER);
		
	signals[PROPERTY_REMOVED] = g_signal_new
		("property_removed",
		 G_TYPE_FROM_CLASS (klass),
		 G_SIGNAL_RUN_LAST,
		 0,
		 NULL, NULL,
		 mrp_marshal_VOID__POINTER,
		 G_TYPE_NONE,
		 1, G_TYPE_POINTER);

	signals[CALENDAR_TREE_CHANGED] = g_signal_new 
		("calendar_tree_changed",
		 G_TYPE_FROM_CLASS (klass),
		 G_SIGNAL_RUN_LAST,
		 0,
		 NULL, NULL,
		 mrp_marshal_VOID__OBJECT,
		 G_TYPE_NONE,
		 1, MRP_TYPE_CALENDAR);
	
	signals[DAY_ADDED] = g_signal_new
		("day_added",
		 G_TYPE_FROM_CLASS (klass),
		 G_SIGNAL_RUN_LAST,
		 0,
		 NULL, NULL,
		 mrp_marshal_VOID__POINTER,
		 G_TYPE_NONE,
		 1, G_TYPE_POINTER);
	
	signals[DAY_REMOVED] = g_signal_new
		("day_removed",
		 G_TYPE_FROM_CLASS (klass),
		 G_SIGNAL_RUN_LAST,
		 0,
		 NULL, NULL,
		 mrp_marshal_VOID__POINTER,
		 G_TYPE_NONE,
		 1, G_TYPE_POINTER);
	
	signals[DAY_CHANGED] = g_signal_new
		("day_changed",
		 G_TYPE_FROM_CLASS (klass),
		 G_SIGNAL_RUN_LAST,
		 0,
		 NULL, NULL,
		 mrp_marshal_VOID__POINTER,
		 G_TYPE_NONE,
		 1, G_TYPE_POINTER);

	/* Properties. */
	g_object_class_install_property (object_class,
					 PROP_PROJECT_START,
					 mrp_param_spec_time ("project-start",
							      "Project start",
							      "The start date of the project",
							      G_PARAM_READWRITE));

	g_object_class_install_property (object_class,
					 PROP_NAME,
					 g_param_spec_string ("name",
							      "Name",
							      "The name of the project",
							      "",
							      G_PARAM_READWRITE));

	g_object_class_install_property (object_class,
					 PROP_ORGANIZATION,
					 g_param_spec_string ("organization",
							      "Organization",
							      "The organization behind the project",
							      "",
							      G_PARAM_READWRITE));
	g_object_class_install_property (object_class,
					 PROP_MANAGER,
					 g_param_spec_string ("manager",
							      "Manager",
							      "The manager of the project",
							      "",
							      G_PARAM_READWRITE));

	g_object_class_install_property (object_class,
					 PROP_DEFAULT_GROUP,
					 g_param_spec_object ("default-group",
							      "Default Group",
							      "Default group for new resources",
							      MRP_TYPE_GROUP,
							      G_PARAM_READWRITE));

	g_object_class_install_property (object_class,
					 PROP_CALENDAR,
					 g_param_spec_object ("calendar",
							      "Calendar",
							      "The calendar used in the project",
							      MRP_TYPE_CALENDAR,
							      G_PARAM_READWRITE));

	g_object_class_install_property (object_class,
					 PROP_PHASES,
					 g_param_spec_pointer ("phases",
							       "Project Phases",
							       "The various phases the project can be in",
							       G_PARAM_READWRITE));
	
	g_object_class_install_property (object_class,
					 PROP_PHASE,
					 g_param_spec_string ("phase",
							      "Project Phase",
							      "The phase the project is in",
							      "",
							      G_PARAM_READWRITE));
}

static void
project_init (MrpProject *project)
{
	MrpProjectPriv *priv;
	MrpProperty    *property;
	MrpCalendar    *calendar;
	
	project->priv = g_new0 (MrpProjectPriv, 1);

	priv = project->priv;

	priv->needs_saving  = FALSE;
	priv->empty         = TRUE;
	priv->project_start = mrp_time_align_day (mrp_time_current_time ());
	priv->resources     = NULL;
	priv->groups        = NULL;
	priv->organization  = g_strdup ("");
	priv->manager       = g_strdup ("");
	priv->name          = g_strdup ("");

	priv->property_pool = g_param_spec_pool_new (TRUE);
	priv->task_manager  = mrp_task_manager_new (project);

	priv->root_calendar = g_object_new (MRP_TYPE_CALENDAR,
					    "name", "-",
					    "project", project,
					    NULL);

	calendar = mrp_calendar_derive (_("Default"), priv->root_calendar);
	project_set_calendar (project, calendar);
	
	project_setup_default_calendar (project);
	priv->day_types = g_hash_table_new_full (
		NULL, NULL, NULL,
		(GDestroyNotify) mrp_day_unref);
	
        /* Move these into the cost plugin */
	property = mrp_property_new ("cost", 
				     MRP_PROPERTY_TYPE_COST,
				     "Cost",
				     "standard cost for a resource",
				     TRUE);
	mrp_project_add_property (project,
				  MRP_TYPE_RESOURCE,
				  property,
                                  TRUE);

	/* FIXME: Add back this when it's used. */
/*	property = mrp_property_new ("cost_overtime", 
	MRP_PROPERTY_TYPE_COST,
	"Overtime Cost",
	"overtime cost for a resource");
	mrp_project_add_property (project,
	MRP_TYPE_RESOURCE,
	property,
	FALSE);		
*/

	/* FIXME: Small temporary hack to get custom properties working on
	 * project.
	 */
	g_object_set (project, "project", project, NULL);

	priv->default_group = NULL;
}

static void
project_finalize (GObject *object)
{
	MrpProject *project = MRP_PROJECT (object);

	g_object_unref (project->priv->primary_storage);
	g_object_unref (project->priv->task_manager);
	
	g_free (project->priv->uri);
	g_free (project->priv);

	if (G_OBJECT_CLASS (parent_class)->finalize) {
		(* G_OBJECT_CLASS (parent_class)->finalize) (object);
	}
}

static void
project_set_property (GObject      *object,
		      guint         prop_id,
		      const GValue *value,
		      GParamSpec   *pspec)
{
	MrpProject     *project;
	MrpProjectPriv *priv;
	MrpGroup       *group;
	MrpCalendar    *calendar;
	
	project = MRP_PROJECT (object);
	priv    = project->priv;
	
	switch (prop_id) {
	case PROP_PROJECT_START:
		priv->project_start = g_value_get_long (value);
		imrp_project_set_needs_saving (project, TRUE);
		break;

	case PROP_ORGANIZATION:
		g_free (priv->organization);
		priv->organization = g_strdup (g_value_get_string (value));
		imrp_project_set_needs_saving (project, TRUE);
		break;

	case PROP_MANAGER:
		g_free (priv->manager);
		priv->manager = g_strdup (g_value_get_string (value));
		imrp_project_set_needs_saving (project, TRUE);
		break;

	case PROP_NAME:
		g_free (priv->name);
		priv->name = g_strdup (g_value_get_string (value));
		imrp_project_set_needs_saving (project, TRUE);
		break;

	case PROP_DEFAULT_GROUP:
		group = g_value_get_object (value);
		if (priv->default_group == group) {
			break;
		}
		
		if (priv->default_group) {
			g_object_unref (priv->default_group);
		}
		
		priv->default_group = group;

		if (priv->default_group) {
			g_object_ref (priv->default_group);
		}

		/* FIXME: isn't the notify enough, why do we have a signal for
		 * this one, but not the others?
		 */
		g_signal_emit (project,
			       signals[DEFAULT_GROUP_CHANGED], 
			       0,
			       priv->default_group);
		imrp_project_set_needs_saving (project, TRUE);
		break;

	case PROP_CALENDAR:
		calendar = g_value_get_object (value);
		project_set_calendar (project, calendar);
		imrp_project_set_needs_saving (project, TRUE);
		break;

	case PROP_PHASES: 
		mrp_string_list_free (priv->phases);
		priv->phases = mrp_string_list_copy (g_value_get_pointer (value));
		imrp_project_set_needs_saving (project, TRUE);
		break;

	case PROP_PHASE:
		g_free (priv->phase);
		priv->phase = g_value_dup_string (value);
		imrp_project_set_needs_saving (project, TRUE);
		break;

	default:
		break;
	}
}

static void
project_get_property (GObject    *object,
		      guint       prop_id,
		      GValue     *value,
		      GParamSpec *pspec)
{
	MrpProject     *project;
	MrpProjectPriv *priv;
	
	project = MRP_PROJECT (object);
	priv    = project->priv;
	
	switch (prop_id) {
	case PROP_PROJECT_START:
		g_value_set_long (value, priv->project_start);
		break;

	case PROP_ORGANIZATION:
		g_value_set_string (value, priv->organization);
		break;

	case PROP_NAME:
		g_value_set_string (value, priv->name);
		break;

	case PROP_MANAGER:
		g_value_set_string (value, priv->manager);
		break;

	case PROP_DEFAULT_GROUP:
		g_value_set_object (value, priv->default_group);
		break;

	case PROP_CALENDAR:
		g_value_set_object (value, priv->calendar);
		break;

	case PROP_PHASES:
		g_value_set_pointer (value, mrp_string_list_copy (priv->phases));
		break;

	case PROP_PHASE:
		g_value_set_string (value, priv->phase);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break; 
	}
}

static void
project_connect_object (MrpObject *object, MrpProject *project)
{
	g_object_set (object, "project", project, NULL);
}

/**
 * mrp_project_new:
 * @app: #MrpApplication that creates the new project.
 * 
 * Creates a new #MrpProject.
 * 
 * Return value: the #MrpProject
 **/
MrpProject *
mrp_project_new (MrpApplication *app)
{
	MrpProject      *project;
	MrpProjectPriv  *priv;

	project = g_object_new (MRP_TYPE_PROJECT, NULL);
	priv = project->priv;
	
	priv->app = app;
	
	project_set_storage (project, "mrproject-1");

	project->priv->app = app;

	/* FIXME: Is this right? The creating of the default calendar changes
	 * the project and sets the needs_saving flag. Not the cleanest solution
	 * but it works: unset it after creating the project. Bug #416.
	 */
	imrp_project_set_needs_saving (project, FALSE);
	priv->empty = TRUE;
	
	return project;
}

/**
 * mrp_project_load:
 * @project: an #MrpProject
 * @uri: the URI where project should be read from
 * @error: location to store error, or %NULL
 * 
 * Loads a project stored at @uri into @project.
 * 
 * Return value: Returns %TRUE on success, otherwise %FALSE.
 **/
gboolean
mrp_project_load (MrpProject *project, const gchar *uri, GError **error)
{
	MrpProjectPriv *priv;
	GsfInput       *input;
	GList          *l;
	MrpCalendar    *old_default_calendar;
	
	g_return_val_if_fail (MRP_IS_PROJECT (project), FALSE);
	g_return_val_if_fail (uri != NULL, FALSE);

	priv = project->priv;

	/* Hack. Check if we are dealing with an SQL uri. */
	if (strncmp (uri, "sql://", 6) == 0) {
		return project_load_from_sql (project, uri, error);
	}
	
	/* Small hack for now. We will load the project, then remove the 
	 * default calendar.
	 */
	old_default_calendar = priv->calendar;

 	input = GSF_INPUT (gsf_input_mmap_new (uri, NULL));
	
 	if (!input) {
		input = GSF_INPUT (gsf_input_stdio_new (uri, error));
	}

	if (!input) {
		return FALSE;
	}

	mrp_task_manager_set_block_scheduling (priv->task_manager, TRUE);
	
	l = imrp_application_get_all_file_readers (priv->app);
	for (; l; l = l->next) {
		MrpFileReader *reader = l->data;

		if (mrp_file_reader_read (reader, input, project, error)) {
			g_object_unref (input);
			
			g_signal_emit (project, signals[LOADED], 0, NULL);
			imrp_project_set_needs_saving (project, FALSE);

			g_free (priv->uri);
			priv->uri = g_strdup (uri);

			/* Remove old calendar. */
			mrp_calendar_remove (old_default_calendar);

			mrp_task_manager_set_block_scheduling (priv->task_manager, FALSE);

			/* FIXME: See bug #416. */
			imrp_project_set_needs_saving (project, FALSE);
			
			return TRUE;
		}
	}

	mrp_task_manager_set_block_scheduling (priv->task_manager, FALSE);

	g_object_unref (input);

	g_set_error (error, 
		     MRP_ERROR,
		     MRP_ERROR_NO_FILE_MODULE,
		     _("Couldn't find a suitable file module for loading '%s'"),
		     uri);

	return FALSE;
}

static gboolean
project_do_save (MrpProject   *project,
		 const gchar  *uri,
		 gboolean      force,
		 GError      **error)
{
	MrpProjectPriv *priv;
	
	priv = project->priv;

	/* A small hack for now: special case SQL URIs. */
	if (strncmp (uri, "sql://", 6) == 0) {
		if (!project_set_storage (project, "sql")) {
			g_set_error (error, MRP_ERROR,
				     MRP_ERROR_NO_FILE_MODULE,
				     _("No support for SQL storage built into this version of Planner."));
			return FALSE;
		}
	} else {
		project_set_storage (project, "mrproject-1");
	}
	
	return mrp_storage_module_save (priv->primary_storage, uri, force, error);
}

/**
 * mrp_project_save:
 * @project: an #MrpProject
 * @force: overwrite changes done by someone else if necessary
 * @error: location to store error, or %NULL
 *
 * Saves a project.
 * 
 * Return value: %TRUE on success, otherwise %FALSE
 **/
gboolean
mrp_project_save (MrpProject *project, gboolean force, GError **error)
{
	MrpProjectPriv *priv;
	
	g_return_val_if_fail (MRP_IS_PROJECT (project), FALSE);
	
	priv = project->priv;

	if (!priv->needs_saving) {
		return TRUE;
	}
	
	/* Sanity check the URI. */
	if (priv->uri == NULL) {
		g_set_error (error,
			     MRP_ERROR, MRP_ERROR_INVALID_URI,
			     _("Invalid URI."));
			     
		return FALSE;
	}

	/* Another hack for handling sql correctly. If we have non-sql, we
	 * always want to force.
	 */
	if (strncmp (priv->uri, "sql://", 6) != 0) {
		force = TRUE;
	}
	
	if (!project_do_save (project, priv->uri, force, error)) {
		return FALSE;
	}

	imrp_project_set_needs_saving (project, FALSE);

	return TRUE;
}

/**
 * mrp_project_save_as:
 * @project: an #MrpProject
 * @uri: URI to save to
 * @force: overwrite an existing file if necessary
 * @error: location to store error, or %NULL
 * 
 * Saves a project to a specific URI.
 * 
 * Return value: %TRUE on success, otherwise %FALSE
 **/
gboolean
mrp_project_save_as (MrpProject   *project,
		     const gchar  *uri,
		     gboolean      force,
		     GError      **error)
{
	MrpProjectPriv *priv;
	gboolean        is_sql;
	gchar          *real_uri;
	
	g_return_val_if_fail (MRP_IS_PROJECT (project), FALSE);
	g_return_val_if_fail (uri != NULL && uri[0] != '\0', FALSE);
	
	priv = project->priv;

	if (strncmp (uri, "sql://", 6) == 0) {
		is_sql = TRUE;

		real_uri = g_strdup (uri);
	} else {
		is_sql = FALSE;

		/* Hack for now. */
		if (!strstr (uri, ".mrproject") && !strstr (uri, ".planner")) {
			real_uri = g_strconcat (uri, ".planner", NULL);
		} else {
			real_uri = g_strdup (uri);
		}
	}
	
	if (!project_do_save (project, real_uri, force, error)) {
		g_free (real_uri);
		return FALSE;
	}

	g_free (priv->uri);
	
	/* A small hack for now: check if it's SQL and update the URI to include
	 * the newly assigned id.
	 */
	if (is_sql) {
		gchar *new_uri;
		
		new_uri = g_object_get_data (G_OBJECT (priv->primary_storage), "uri");
		priv->uri = g_strdup (new_uri);
	} else {
		priv->uri = g_strdup (real_uri);
	}
	
	g_free (real_uri);
	
	imrp_project_set_needs_saving (project, FALSE);
	
	return TRUE;
}


/**
 * mrp_project_export:
 * @project: an #MrpProject
 * @uri: URI to export to
 * @identifier: string identifying which export module to use
 * @force: overwrite an existing file if necessary
 * @error: location to store error, or %NULL
 * 
 * Exports a project to a specific URI. @identifier is used to identify which
 * module to use. It can be either the identifier string of a module or the 
 * mime type you whish to export to.
 * 
 * Return value: %TRUE on success, otherwise %FALSE.
 **/
gboolean
mrp_project_export (MrpProject   *project,
		    const gchar  *uri,
		    const gchar  *identifier,
		    gboolean      force,   
		    GError      **error)
{
	MrpProjectPriv *priv;
	GList          *l;
	
	g_return_val_if_fail (MRP_IS_PROJECT (project), FALSE);
	g_return_val_if_fail (uri != NULL && uri[0] != '\0', FALSE);

	priv = project->priv;
	
	l = imrp_application_get_all_file_writers (priv->app);
	for (; l; l = l->next) {
		MrpFileWriter *writer = l->data;

		if (g_ascii_strcasecmp (writer->identifier, identifier) == 0) {
			return mrp_file_writer_write (writer, project,
						      uri, force, error);
		}
	}

	l = imrp_application_get_all_file_writers (priv->app);
	for (; l; l = l->next) {
		MrpFileWriter *writer = l->data;

		if (g_ascii_strcasecmp (writer->mime_type, identifier) == 0) {
			return mrp_file_writer_write (writer, project,
						      uri, force, error);
		}
	}

	g_set_error (error,
		     MRP_ERROR,
		     MRP_ERROR_EXPORT_UNSUPPORTED,
		     _("Unable to find file writer identified by '%s'"),
		     identifier);

	return FALSE;
}

/**
 * mrp_project_load_from_xml:
 * @project: an #MrpProject
 * @str: XML string with project data to read from
 * @error: location to store error, or %NULL
 * 
 * Loads a project from XML data into @project.
 * 
 * Return value: Returns %TRUE on success, otherwise %FALSE.
 **/
gboolean
mrp_project_load_from_xml (MrpProject *project, const gchar *str, GError **error)
{
	MrpProjectPriv *priv;
	GList          *l;
	MrpCalendar    *old_default_calendar;
	
	g_return_val_if_fail (MRP_IS_PROJECT (project), FALSE);
	g_return_val_if_fail (str != NULL, FALSE);

	priv = project->priv;

	/* Small hack for now. We will load the project, then remove the 
	 * default calendar.
	 */
	old_default_calendar = priv->calendar;
	
	mrp_task_manager_set_block_scheduling (priv->task_manager, TRUE);
	
	l = imrp_application_get_all_file_readers (priv->app);
	for (; l; l = l->next) {
		MrpFileReader *reader = l->data;

		if (mrp_file_reader_read_string (reader, str, project, error)) {
			g_signal_emit (project, signals[LOADED], 0, NULL);
			imrp_project_set_needs_saving (project, FALSE);

			/*g_free (priv->uri);*/
			priv->uri = NULL;

			/* Remove old calendar. */
			mrp_calendar_remove (old_default_calendar);

			mrp_task_manager_set_block_scheduling (priv->task_manager, FALSE);

			/* FIXME: See bug #416. */
			imrp_project_set_needs_saving (project, FALSE);
			
			return TRUE;
		}
	}

	mrp_task_manager_set_block_scheduling (priv->task_manager, FALSE);

	g_set_error (error, 
		     MRP_ERROR,
		     MRP_ERROR_NO_FILE_MODULE,
		     _("Couldn't find a suitable file module for loading project"));

	return FALSE;
}

/**
 * mrp_project_save_to_xml:
 * @project: an #MrpProject
 * @str: location to store XML string
 * @error: location to store error, or %NULL
 * 
 * Saves a project as XML to a string buffer.
 * 
 * Return value: %TRUE on success, otherwise %FALSE
 **/
gboolean
mrp_project_save_to_xml (MrpProject   *project,
			 gchar       **str,
			 GError      **error)
{
	MrpProjectPriv *priv;
	
	g_return_val_if_fail (MRP_IS_PROJECT (project), FALSE);
	g_return_val_if_fail (str != NULL, FALSE);
	
	priv = project->priv;

	return mrp_storage_module_to_xml (priv->primary_storage, str, error);
}

static gboolean
project_load_from_sql (MrpProject   *project,
		       const gchar  *uri,
		       GError      **error)
{
	MrpProjectPriv *priv;
	MrpCalendar    *old_default_calendar;

	priv = project->priv;

	if (!project_set_storage (project, "sql")) {
		g_set_error (error, MRP_ERROR,
			     MRP_ERROR_NO_FILE_MODULE,
			     _("No support for SQL storage built into this version of Planner."));
		return FALSE;
	}

	mrp_task_manager_set_block_scheduling (priv->task_manager, TRUE);

	if (mrp_storage_module_load (priv->primary_storage, uri, error)) {
		old_default_calendar = priv->calendar;
	
		g_signal_emit (project, signals[LOADED], 0, NULL);
		imrp_project_set_needs_saving (project, FALSE);
		
		g_free (priv->uri);
		priv->uri = g_strdup (uri);
		
		/* Remove old calendar. */
		mrp_calendar_remove (old_default_calendar);
		
		mrp_task_manager_set_block_scheduling (priv->task_manager, FALSE);

		/* FIXME: See bug #416. */
		imrp_project_set_needs_saving (project, FALSE);
		
		return TRUE;
	}
	
	mrp_task_manager_set_block_scheduling (priv->task_manager, FALSE);
	
	return FALSE;
}

static gboolean
project_set_storage (MrpProject  *project,
		     const gchar *storage_name)
{
	MrpProjectPriv          *priv;
	MrpStorageModuleFactory *factory;
	MrpStorageModule        *module;

	priv = project->priv;

	factory = mrp_storage_module_factory_get (storage_name);
	if (!factory) {
		return FALSE;
	}
	
	module = mrp_storage_module_factory_create_module (factory);
	if (!module) {
		return FALSE;
	}

	g_type_module_unuse (G_TYPE_MODULE (factory));
	
	imrp_storage_module_set_project (module, project);

	if (priv->primary_storage) {
		g_object_unref (priv->primary_storage);
	}
	
	priv->primary_storage = module;

	return TRUE;
}

/**
 * mrp_project_close:
 * @project: an #MrpProject
 * 
 * Closes a project.
 **/
void
mrp_project_close (MrpProject *project)
{
	/* FIXME: impl close. */
}

MrpTaskManager *
imrp_project_get_task_manager (MrpProject *project)
{
	g_return_val_if_fail (MRP_IS_PROJECT (project), NULL);

	return project->priv->task_manager;
}

/**
 * mrp_project_get_resource_by_name:
 * @project: an #MrpProject
 * @name: name to search for
 * 
 * Retrieves the first resource in the list that match the name.
 * 
 * Return value: an #MrpResource or %NULL if not found
 **/
MrpResource *
mrp_project_get_resource_by_name (MrpProject *project, const gchar *name)
{
	GList *resources, *l;
	
	g_return_val_if_fail (MRP_IS_PROJECT (project), NULL);
	g_return_val_if_fail (name != NULL, NULL);
	
	/* Should we use a hash table to store these in as well? */
	resources = mrp_project_get_resources (project);

	for (l = resources; l; l = l->next) {
		const gchar *rname;
		
		rname = mrp_resource_get_name (MRP_RESOURCE (l->data));
		if (strcmp (name, rname) == 0) {
			return MRP_RESOURCE (l->data);
		}
	}

	return NULL;
}

/**
 * mrp_project_get_resources:
 * @project: an #MrpProject
 * 
 * Fetches the list of resources in @project. This list should not be freed and
 * if caller needs to manipulate it, a copy needs to be made first.
 * 
 * Return value: the resource list of @project
 **/
GList *
mrp_project_get_resources (MrpProject *project)
{
	g_return_val_if_fail (MRP_IS_PROJECT (project), NULL);

	return project->priv->resources;
}

void
imrp_project_set_resources (MrpProject *project,
			    GList      *resources)
{
	g_return_if_fail (MRP_IS_PROJECT (project));
	
	project->priv->resources = resources;

	g_list_foreach (project->priv->resources, 
			(GFunc) project_connect_object,
			project);
}

/**
 * mrp_project_add_resource:
 * @project: an #MrpProject
 * @resource: #MrpResource to add
 * 
 * Adds @resource to the list of resources in @project.
 **/
void      
mrp_project_add_resource (MrpProject *project, MrpResource *resource)
{
	MrpProjectPriv  *priv;
	MrpGroup        *group;
	MrpResourceType  type;
	
	g_return_if_fail (MRP_IS_PROJECT (project));
	g_return_if_fail (MRP_IS_RESOURCE (resource));
	
	priv = project->priv;

	priv->resources = g_list_prepend (priv->resources, resource);

	g_object_get (resource, "group", &group, NULL);
	
	if (!group) {
		g_object_set (resource, "group", priv->default_group, NULL);
	}

	g_object_get (resource, "type", &type, NULL);
	
	if (type == MRP_RESOURCE_TYPE_NONE) {
		g_object_set (resource, "type", MRP_RESOURCE_TYPE_WORK, NULL);
	}

	
	project_connect_object (MRP_OBJECT (resource), project);
	

	g_signal_emit (project, signals[RESOURCE_ADDED], 0, resource);

	imrp_project_set_needs_saving (project, TRUE);
}

/**
 * mrp_project_remove_resource:
 * @project: an #MrpProject
 * @resource: #MrpResource to remove
 * 
 * Removes @resource from @project.
 **/
void
mrp_project_remove_resource (MrpProject *project, MrpResource *resource)
{
	MrpProjectPriv *priv;
	
	g_return_if_fail (MRP_IS_PROJECT (project));
	g_return_if_fail (MRP_IS_RESOURCE (resource));
	
	priv = project->priv;
	
	mrp_object_removed (MRP_OBJECT (resource));

	priv->resources = g_list_remove (priv->resources, resource);

	g_signal_emit (project, signals[RESOURCE_REMOVED], 0, resource);

	imrp_project_set_needs_saving (project, TRUE);
}

/**
 * mrp_project_get_group_by_name:
 * @project: an #MrpProject
 * @name: a name to look for
 * 
 * Retrieves the first group with name that matches @name
 * 
 * Return value: an #MrpGroup or %NULL if not found
 **/
MrpGroup *
mrp_project_get_group_by_name (MrpProject *project, const gchar *name)
{
	GList *groups, *l;
	
	g_return_val_if_fail (MRP_IS_PROJECT (project), NULL);
	g_return_val_if_fail (name != NULL, NULL);

	groups = mrp_project_get_groups (project);
	for (l = groups; l; l = l->next) {
		const gchar *gname;

		gname = mrp_group_get_name (MRP_GROUP (l->data));
		if (strcmp (gname, name) == 0) {
			return MRP_GROUP (l->data);
		}
	}

	return NULL;
}

/**
 * mrp_project_get_groups:
 * @project: an #MrpProject
 * 
 * Fetches the list of groups in @project. The list should not be freed and if
 * caller needs to manipulate it, a copy needs to be made first.
 *
 * Return value: the group list of @project
 **/
GList *
mrp_project_get_groups (MrpProject *project)
{
	g_return_val_if_fail (MRP_IS_PROJECT (project), NULL);
	
	return project->priv->groups;
}

void
imrp_project_set_groups (MrpProject *project,
			 GList      *groups)
{
	g_return_if_fail (MRP_IS_PROJECT (project));
	
	project->priv->groups = groups;

	g_list_foreach (project->priv->groups, 
			(GFunc) project_connect_object,
			project);
	
}

/**
 * mrp_project_add_group:
 * @project: an #MrpProject
 * @group: #MrpGroup to remove
 * 
 * Adds @group to the list of groups in @project.
 **/
void
mrp_project_add_group (MrpProject *project, MrpGroup *group)
{
	MrpProjectPriv *priv;
	
	g_return_if_fail (MRP_IS_PROJECT (project));
	g_return_if_fail (MRP_IS_GROUP (group));
	
	priv = project->priv;

	priv->groups = g_list_prepend (priv->groups, group);

	g_object_set (group, "project", project, NULL);

	project_connect_object (MRP_OBJECT (group), project);

	g_signal_emit (project, signals[GROUP_ADDED], 0, group);

	imrp_project_set_needs_saving (project, TRUE);
}

/**
 * mrp_project_remove_group:
 * @project: an #MrpProject
 * @group: #MrpGroup to remove
 * 
 * Removes @group from @project.
 **/
void
mrp_project_remove_group (MrpProject *project, MrpGroup *group)
{
	MrpProjectPriv *priv;
	
	g_return_if_fail (MRP_IS_PROJECT (project));
	g_return_if_fail (MRP_IS_GROUP (group));
	
	priv = project->priv;
	
	if (priv->default_group && priv->default_group == group) {
		priv->default_group = NULL;
	}
	
	priv->groups = g_list_remove (priv->groups, group);

	g_signal_emit (project, signals[GROUP_REMOVED], 0, group);

	mrp_object_removed (MRP_OBJECT (group));

	imrp_project_set_needs_saving (project, TRUE);
}

/**
 * mrp_project_is_empty:
 * @project: an #MrpProject
 * 
 * Checks whether a project is empty.
 * 
 * Return value: %TRUE if @project is empty, otherwise %FALSE
 **/
gboolean
mrp_project_is_empty (MrpProject *project)
{
	return project->priv->empty;
}

/**
 * mrp_project_needs_saving:
 * @project: an #MrpProject
 * 
 * Checks if @project needs saving
 * 
 * Return value: %TRUE if project has been altered since last save, 
 * otherwise %FALSE
 **/
gboolean
mrp_project_needs_saving (MrpProject *project)
{
	return project->priv->needs_saving;
}

/**
 * mrp_project_get_uri:
 * @project: an #MrpProject
 * 
 * Fetches the URI from @project.
 * 
 * Return value: the URI of @project
 **/
const gchar *    
mrp_project_get_uri (MrpProject *project)
{
	g_return_val_if_fail (MRP_IS_PROJECT (project), NULL);

	return project->priv->uri;
}

/**
 * mrp_project_get_project_start:
 * @project: an #MrpProject
 * 
 * Fetches the project start from @project.
 * 
 * Return value: the project start
 **/
mrptime
mrp_project_get_project_start (MrpProject *project)
{
	g_return_val_if_fail (MRP_IS_PROJECT (project), MRP_TIME_INVALID);

	return project->priv->project_start;
}

/**
 * mrp_project_set_project_start:
 * @project: an #MrpProject
 * @start: project start time
 * 
 * Set the project start.
 **/
void
mrp_project_set_project_start (MrpProject *project,
			       mrptime     start)
{
	g_return_if_fail (MRP_IS_PROJECT (project));

	project->priv->project_start = start;
}
/**
 * imrp_project_add_calendar_day:
 * @project: an #MrpProject
 * @day: #MrpDay to add
 * 
 * This adds a calendar day type to the list of available day types.
 * 
 * Return value: %TRUE on success, %FALSE if a day type of the same id 
 * was already in the list.
 **/
gboolean
imrp_project_add_calendar_day (MrpProject *project, MrpDay *day)
{
	MrpProjectPriv *priv;
	MrpDay         *tmp_day;
	
	g_return_val_if_fail (MRP_IS_PROJECT (project), -1);
	g_return_val_if_fail (day != NULL, -1);

	priv = project->priv;
	
	tmp_day = (MrpDay *) g_hash_table_lookup (priv->day_types,
						  GINT_TO_POINTER (mrp_day_get_id (day)));
		
	if (tmp_day) {
		g_warning ("Trying to add already present day type: '%s'",
			   mrp_day_get_name (tmp_day));
		return FALSE;
	}

	g_hash_table_insert (priv->day_types,
			     GINT_TO_POINTER (mrp_day_get_id (day)), 
			     mrp_day_ref (day));

	g_signal_emit (project,
		       signals[DAY_ADDED],
		       0,
		       day);

	imrp_project_set_needs_saving (project, TRUE);

	return TRUE;
}

/**
 * mrp_project_get_calendar_day_by_id:
 * @project: an #MrpProject
 * 
 * Semi-private function. You most likely won't need this outside of
 * Planner. Returns the day type associated with @id.
 * 
 * Return value: A day type.
 **/
MrpDay *
mrp_project_get_calendar_day_by_id (MrpProject *project, gint id)
{
	MrpProjectPriv *priv;
	MrpDay         *day;
	
	g_return_val_if_fail (MRP_IS_PROJECT (project), NULL);

	priv = project->priv;
	
	day = g_hash_table_lookup (priv->day_types, GINT_TO_POINTER (id));

	return day;
}

static void
foreach_day_add_to_list (gpointer key, MrpDay *day, GList **list)
{
	*list = g_list_prepend (*list, day);
}

/**
 * imrp_project_get_calendar_days:
 * @project: an #MrpProject
 * 
 * Returns a new list of the day types in @project. This list needs to be 
 * saved by the caller.
 * 
 * Return value: A newly allocated list containing the day types in @project.
 **/
GList *
imrp_project_get_calendar_days (MrpProject *project)
{
	MrpProjectPriv *priv;
	GList          *ret_val = NULL;

	g_return_val_if_fail (MRP_IS_PROJECT (project), NULL);

	priv = project->priv;

	g_hash_table_foreach (priv->day_types,
			      (GHFunc) foreach_day_add_to_list,
			      &ret_val);
	
	return ret_val;
}

static void
project_day_fallback_when_removed (MrpCalendar *calendar,
				   MrpDay      *day)
{
	GList       *children, *l;
	MrpCalendar *child;
		
	children = mrp_calendar_get_children (calendar);
	for (l = children; l; l = l->next) {
		child = l->data;

		imrp_calendar_replace_day (child, day, mrp_day_get_work ());
		project_day_fallback_when_removed (child, day);
	}
}

/**
 * imrp_project_remove_calendar_day:
 * @project: an #MrpProject
 * @day: #MrpDay to remove
 * 
 * Removes @day from list of available day types in @project.
 **/
void
imrp_project_remove_calendar_day (MrpProject *project, MrpDay *day)
{
	MrpProjectPriv *priv;

	g_return_if_fail (MRP_IS_PROJECT (project));
	g_return_if_fail (day != NULL);
	
	priv = project->priv;

	/* Check if the day type is used in a calendar, if so fallback to
	 * working day.
	 */
	project_day_fallback_when_removed (priv->root_calendar, day);
	
	g_signal_emit (project,
		       signals[DAY_REMOVED],
		       0,
		       day);
	
	g_hash_table_remove (priv->day_types, 
			     GINT_TO_POINTER (mrp_day_get_id (day)));

	imrp_project_set_needs_saving (project, TRUE);
}

/**
 * imrp_project_signal_calendar_tree_changed:
 * @project: an #MrpProject
 * 
 * Emits the "calendar_tree_changed" signal. Used from calendar to notify user
 * that it has changed, so that the user doesn't have to connect to every 
 * calendar.  
 **/
void
imrp_project_signal_calendar_tree_changed (MrpProject *project)
{
	MrpProjectPriv *priv;
	
	if (!project) {
		return;
	}
	
	priv = project->priv;

	g_signal_emit (project, 
		       signals[CALENDAR_TREE_CHANGED],
		       0,
		       priv->root_calendar);
}

/**
 * imrp_project_set_needs_saving:
 * @project: an #MrpProject
 * @needs_saving: a #gboolean
 * 
 * Sets the needs_saving flag on @project and if it changed emit the 
 * "needs_saving_changed" signal.
 **/
void
imrp_project_set_needs_saving (MrpProject *project, gboolean needs_saving) 
{
	MrpProjectPriv *priv;
	
	g_return_if_fail (MRP_IS_PROJECT (project));

	priv = project->priv;

	if (needs_saving == project->priv->needs_saving) {
		return;
	}

	if (needs_saving == TRUE) {
		priv->empty = FALSE;
	}

	project->priv->needs_saving = needs_saving;

	g_signal_emit (project, 
		       signals[NEEDS_SAVING_CHANGED], 
		       0, 
		       needs_saving);
}

/* Task related functions.
 */

typedef struct {
	const gchar *name;
	MrpTask     *task;
} FindTaskByNameData;
	
static gboolean 
find_task_by_name_traverse_func (MrpTask *task, FindTaskByNameData *data)
{
	const gchar *name;

	name = mrp_task_get_name (task);
	if (strcmp (name, data->name) == 0) {
		data->task = task;
		return TRUE;
	}
	
	return FALSE;
}

/**
 * mrp_project_get_task_by_name:
 * @project: an #MrpProject
 * @name: the name to look for
 * 
 * Retrieves the first task with name matching @name. Uses task_traverse to traverse all tasks.
 * 
 * Return value: an #MrpTask or %NULL if not found.
 **/
MrpTask *
mrp_project_get_task_by_name (MrpProject *project, const gchar *name) 
{
	FindTaskByNameData *data;
	MrpTask            *ret_val;
	
	data = g_new0 (FindTaskByNameData, 1);
	data->name = name;
	data->task = NULL;

	mrp_project_task_traverse (project,
				   mrp_project_get_root_task (project),
				   (MrpTaskTraverseFunc) find_task_by_name_traverse_func,
				   data);

	ret_val = data->task;
	g_free (data);
	
	return ret_val;
}

/**
 * mrp_project_get_all_tasks:
 * @project: an #MrpProject
 * 
 * Returns a new list of the tasks in @project. The caller needs to free the 
 * list with g_list_free(), but not the values in it.
 * 
 * Return value: a newly allocated list of the tasks
 **/
GList *
mrp_project_get_all_tasks (MrpProject *project)
{
	g_return_val_if_fail (MRP_IS_PROJECT (project), NULL);

	return mrp_task_manager_get_all_tasks (project->priv->task_manager);
}

/**
 * mrp_project_insert_task:
 * @project: an #MrpProject
 * @parent: #MrpTask that will be parent to inserted task
 * @position: position among children to insert task
 * @task: #MrpTask to insert
 * 
 * Insert @task in the task tree with @parent at @position among other 
 * children.
 **/
void
mrp_project_insert_task (MrpProject *project,
			 MrpTask    *parent,
			 gint        position,
			 MrpTask    *task)
{
	g_return_if_fail (MRP_IS_PROJECT (project));

	mrp_task_manager_insert_task (project->priv->task_manager,
				      parent,
				      position,
				      task);

	g_object_set (MRP_OBJECT (task), "project", project, NULL);
}

/**
 * mrp_project_remove_task:
 * @project: an #MrpProject
 * @task: #MrpTask to remove
 * 
 * Removes @task from the task tree in @project.
 **/
void
mrp_project_remove_task (MrpProject *project,
			 MrpTask    *task)
{
	g_return_if_fail (MRP_IS_PROJECT (project));

	mrp_object_removed (MRP_OBJECT (task));

	mrp_task_manager_remove_task (project->priv->task_manager,
				      task);
	
	g_signal_emit (project, signals[TASK_REMOVED], 0, task);

	imrp_project_set_needs_saving (project, TRUE);
}

/**
 * mrp_project_move_task:
 * @project: an #MrpProject
 * @task: #MrpTask to move
 * @sibling: #MrpTask that @task will be placed next to, can be %NULL
 * @parent: #MrpTask the new parent
 * @before: whether to put @task before or after @sibling.
 * @error: location to store error, or NULL
 * 
 * Move the task in the task tree. If @sibling is %NULL task will be placed 
 * first among the children of @parent if @before is %TRUE, otherwise it will 
 * be placed last. If @sibling is set, @task will be placed before or after 
 * @sibling depending on the value of @before.
 * 
 * Return value: %TRUE on success, otherwise %FALSE
 **/
gboolean
mrp_project_move_task (MrpProject  *project,
		       MrpTask     *task,
		       MrpTask     *sibling,
		       MrpTask     *parent,
		       gboolean     before,
		       GError     **error)
{
	g_return_val_if_fail (MRP_IS_PROJECT (project), FALSE);
	g_return_val_if_fail (MRP_IS_TASK (task), FALSE);
	g_return_val_if_fail (sibling == NULL || MRP_IS_TASK (sibling), FALSE);
	g_return_val_if_fail (MRP_IS_TASK (parent), FALSE);

	return mrp_task_manager_move_task (project->priv->task_manager,
					   task,
					   sibling,
					   parent,
					   before,
					   error);
}

/**
 * imrp_project_task_moved:
 * @project: an #MrpProject
 * @task: #MrpTask that was moved
 * 
 * Signals "task-moved" and sets the "needs_saving" flag on project.
 **/
void
imrp_project_task_moved (MrpProject *project,
			 MrpTask    *task)
{
	g_signal_emit (project, signals[TASK_MOVED], 0, task);

	imrp_project_set_needs_saving (project, TRUE);
}

/**
 * mrp_project_get_root_task:
 * @project: an #MrpProject
 * 
 * Fetches the root task from @project.
 * 
 * Return value: the root task
 **/
MrpTask *
mrp_project_get_root_task (MrpProject *project)
{
	g_return_val_if_fail (MRP_IS_PROJECT (project), NULL);

	return mrp_task_manager_get_root (project->priv->task_manager);
}

/**
 * mrp_project_task_traverse:
 * @project: an #MrpProject
 * @root: #MrpTask indicates where traversing will begin.
 * @func: the function to call for each task
 * @user_data: user data passed to the function
 * 
 * Calls @func on each task under @root in the task tree. @user_data is passed
 * to @func. If @func returns %TRUE, the traversal is stopped.
 **/
void
mrp_project_task_traverse (MrpProject          *project,
			   MrpTask             *root,
			   MrpTaskTraverseFunc  func,
			   gpointer             user_data)
{
	g_return_if_fail (MRP_IS_PROJECT (project));

	mrp_task_manager_traverse (project->priv->task_manager,
				   root,
				   func,
				   user_data);
}

/**
 * mrp_project_reschedule:
 * @project: an #MrpProject
 * 
 * Reschedules the project, calculating task start/end/duration etc.
 **/
void
mrp_project_reschedule (MrpProject *project)
{
	g_return_if_fail (MRP_IS_PROJECT (project));

	mrp_task_manager_recalc (project->priv->task_manager, TRUE);
}

/**
 * mrp_project_calculate_task_work:
 * @project: an #MrpProject
 * @task: an #MrpTask
 * @start: a start time, or if %-1, the task start time is to be used 
 * @finish: a finish time
 * 
 * Calculates the work needed to achieve the given start and finish time, with
 * the allocated resources' calendards in consideration.
 * 
 * Return value: The calculated work.
 **/
gint
mrp_project_calculate_task_work (MrpProject *project, 
				 MrpTask    *task, 
				 mrptime     start,
				 mrptime     finish)
{
	g_return_val_if_fail (MRP_IS_PROJECT (project), 0);
	g_return_val_if_fail (MRP_IS_TASK (task), 0);
	g_return_val_if_fail (start == -1 || start <= finish, 0);
	g_return_val_if_fail (finish >= 0, 0);

	return mrp_task_manager_calculate_task_work (
		project->priv->task_manager,
		task, start, finish);
}

/**
 * imrp_project_task_inserted:
 * @project: an #MrpProject
 * @task: #MrpTask that was inserted
 * 
 * Signals "task-inserted" and sets the "needs_saving" flag on project.
 **/
void
imrp_project_task_inserted (MrpProject *project, MrpTask *task)
{
	g_return_if_fail (MRP_IS_PROJECT (project));

	g_signal_emit (project, signals[TASK_INSERTED], 0, task);

	imrp_project_set_needs_saving (project, TRUE);
}

/* Debug function. */
#if 0
static void
project_dump_task_tree (MrpProject *project)
{
	g_return_if_fail (MRP_IS_PROJECT (project));

	mrp_task_manager_dump_task_tree (project->priv->task_manager);
}
#endif

/**
 * mrp_project_get_property:
 * @project: an #MrpProject
 * @name: the name of the property
 * @object_type: object type the property belongs to
 * 
 * Fetches an #MrpProperty that corresponds to @name and @object_type. This is
 * mainly for language bindings and should not be used for other cases.
 * 
 * Return value: An #MrpProperty, if found, otherwise %NULL.
 **/
MrpProperty *
mrp_project_get_property (MrpProject  *project,
			  const gchar *name,
			  GType        object_type)
{
	MrpProjectPriv *priv;
	MrpProperty    *property;
	
	g_return_val_if_fail (MRP_IS_PROJECT (project), NULL);

	priv = project->priv;

	property = MRP_PROPERTY (g_param_spec_pool_lookup (priv->property_pool,
							   name,
							   object_type,
							   TRUE));
	
	if (!property) {
		g_warning ("%s: object of type `%s' has no property named `%s'",
			   G_STRLOC,
			   g_type_name (object_type),
			   name);
		return NULL;
	}

	return property;
}

/**
 * imrp_project_property_changed:
 * @project: an #MrpProject
 * @property: an #MrpProperty
 * 
 * Signals "property-changed" and sets the "needs_saving" flag on @project.
 **/
void
imrp_project_property_changed (MrpProject *project, MrpProperty *property)
{
	g_return_if_fail (MRP_IS_PROJECT (project));
	g_return_if_fail (property != NULL);

	g_signal_emit (project, signals[PROPERTY_CHANGED], 0, property);
	imrp_project_set_needs_saving (project, TRUE);
}

/**
 * mrp_project_get_properties_from_type:
 * @project: an #MrpProject
 * @object_type: a #GType
 * 
 * Fetches a list of the properties belonging to @project and applies to
 * @object_type. The list should not be freed and needs to be copied before 
 * modified.
 * 
 * Return value: The list of properties.
 **/
GList *
mrp_project_get_properties_from_type (MrpProject *project, GType object_type)
{
	MrpProjectPriv *priv;
	GList          *properties;
	
	g_return_val_if_fail (MRP_IS_PROJECT (project), NULL);

	priv = project->priv;

	properties = g_param_spec_pool_list_owned (priv->property_pool,
						   object_type);

	return properties;
}

/**
 * mrp_project_add_property:
 * @project: an #MrpProject
 * @object_type: the owner type
 * @property: an #MrpProperty
 * @user_defined: whether the property is defined through a user interface
 * 
 * Add a custom property to @project. The @object_type specifies what kind of 
 * objects the property applies to. @user_defined specifies whether the 
 * property is created by the user or by some plugin. 
 **/
void
mrp_project_add_property (MrpProject  *project,
			  GType        object_type,
			  MrpProperty *property,
                          gboolean     user_defined)
{
	MrpProjectPriv *priv;

	g_return_if_fail (MRP_IS_PROJECT (project));
	
	priv = project->priv;

	/* FIXME: Check for name conflicts. */
	if (g_param_spec_pool_lookup (priv->property_pool,
				      G_PARAM_SPEC(property)->name,
				      object_type,
				      TRUE)) {
		g_warning ("%s: Param '%s' already exists for object '%s'.",
			   G_STRLOC,
			   G_PARAM_SPEC (property)->name,
			   g_type_name (object_type));
		return;
	}

	mrp_property_set_user_defined (property, user_defined);

	g_param_spec_pool_insert (priv->property_pool,
				  G_PARAM_SPEC (property),
				  object_type);

	imrp_property_set_project (property, project);

	g_signal_emit (project, signals[PROPERTY_ADDED], 0, 
		       object_type, 
		       property);

        if (user_defined) {
                imrp_project_set_needs_saving (project, TRUE);
        }
}

/**
 * mrp_project_remove_property:
 * @project: an #MrpProject
 * @object_type: a #GType specifing object type to remove property from
 * @name: the name of the property
 * 
 * Removes the property corresponding to @object_type and @name from @project.
 **/
void
mrp_project_remove_property (MrpProject  *project,
			     GType        object_type,
			     const gchar *name)
{
	MrpProjectPriv *priv;
	MrpProperty    *property;
	
	g_return_if_fail (MRP_IS_PROJECT (project));
	
	priv = project->priv;

	property = mrp_project_get_property (project, name, object_type);

	if (!property) {
		g_warning ("%s: object of type '%s' has no property named '%s'",
			   G_STRLOC,
			   g_type_name (object_type),
			   name);
		return;
	}

	g_signal_emit (project, signals[PROPERTY_REMOVED], 0, property);
	
	g_param_spec_pool_remove (priv->property_pool,
				  G_PARAM_SPEC (property));

	imrp_project_set_needs_saving (project, TRUE);
}

/**
 * mrp_project_has_property:
 * @project: an #MrpProperty
 * @owner_type: a #GType specifing object type look for property on
 * @name: the name of the property
 * 
 * Checks if @project has a property named @name applying to object of type 
 * @object_type.
 * 
 * Return value: %TRUE if property @name exists on objects of type @object_type
 **/
gboolean
mrp_project_has_property (MrpProject  *project,
			  GType        owner_type,
			  const gchar *name)
{
	MrpProjectPriv *priv;

	g_return_val_if_fail (MRP_IS_PROJECT (project), FALSE);
	g_return_val_if_fail (name != NULL, FALSE);
	
	priv = project->priv;

	return NULL != g_param_spec_pool_lookup (priv->property_pool,
						 name,
						 owner_type,
						 TRUE);
}

/**
 * mrp_project_get_root_calendar:
 * @project: an #MrpProject
 * 
 * Fetches the root calendar of @project.
 * 
 * Return value: the root calendar of @project
 **/
MrpCalendar *
mrp_project_get_root_calendar (MrpProject *project)
{
	MrpProjectPriv *priv;
	
	g_return_val_if_fail (MRP_IS_PROJECT (project), NULL);
	
	priv = project->priv;
	
	return priv->root_calendar;
}

/**
 * mrp_project_get_calendar:
 * @project: an #MrpProject
 * 
 * Fetches the calendar used by @project. 
 * 
 * Return value: the calendar used by @project
 **/
MrpCalendar *
mrp_project_get_calendar (MrpProject *project)
{
	MrpProjectPriv *priv;
	
	g_return_val_if_fail (MRP_IS_PROJECT (project), NULL);
	
	priv = project->priv;
	
	return priv->calendar;
}

static void
project_setup_default_calendar (MrpProject *project)
{
	MrpProjectPriv *priv;
	MrpInterval    *ival;
	GList          *ivals = NULL;

	priv = project->priv;
	
	mrp_calendar_set_default_days (
		priv->calendar,
		MRP_CALENDAR_DAY_MON, mrp_day_get_work (),
		MRP_CALENDAR_DAY_TUE, mrp_day_get_work (),
		MRP_CALENDAR_DAY_WED, mrp_day_get_work (),
		MRP_CALENDAR_DAY_THU, mrp_day_get_work (),
		MRP_CALENDAR_DAY_FRI, mrp_day_get_work (),
		MRP_CALENDAR_DAY_SAT, mrp_day_get_nonwork (),
		MRP_CALENDAR_DAY_SUN, mrp_day_get_nonwork (),
		-1);
	
	ival = mrp_interval_new (8 * 60*60, 12 * 60*60);
	ivals = g_list_append (ivals, ival);
	
	ival = mrp_interval_new (13 * 60*60, 17 * 60*60);
	ivals = g_list_append (ivals, ival);
	
	mrp_calendar_day_set_intervals (priv->calendar, mrp_day_get_work (), ivals);

	g_list_foreach (ivals, (GFunc) mrp_interval_unref, NULL);
	g_list_free (ivals);
}

static void
project_calendar_changed (MrpCalendar *calendar,
			  MrpProject  *project)
{
	MrpProjectPriv *priv;

	priv = project->priv;

	mrp_task_manager_recalc (priv->task_manager, TRUE);
}

static void
project_set_calendar (MrpProject  *project,
		      MrpCalendar *calendar)
{
	MrpProjectPriv *priv;

	priv = project->priv;

	if (priv->calendar) {
		g_signal_handlers_disconnect_by_func (priv->calendar,
						      project_calendar_changed,
						      project);
		g_object_unref (priv->calendar);
	}

	if (calendar) {
		priv->calendar = g_object_ref (calendar);

		g_signal_connect_object (calendar,
					 "calendar_changed",
					 G_CALLBACK (project_calendar_changed),
					 project,
					 0);
	}

	mrp_task_manager_recalc (priv->task_manager, TRUE);
}
