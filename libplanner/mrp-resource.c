/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2004 Imendio HB
 * Copyright (C) 2002-2003 CodeFactory AB
 * Copyright (C) 2002-2003 Richard Hult <richard@imendio.com>
 * Copyright (C) 2002 Mikael Hallendal <micke@imendio.com>
 * Copyright (C) 2004 Alvaro del Castillo <acs@barrapunto.com>
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
#include "mrp-private.h"
#include "mrp-marshal.h"
#include "mrp-intl.h"
#include "mrp-assignment.h"
#include "mrp-group.h"
#include "mrp-task.h"
#include "mrp-resource.h"


struct _MrpResourcePriv {
	gchar           *name;
	gchar		*short_name;
        MrpGroup        *group;
        MrpResourceType  type;
        gint             units;
        gchar           *email;
        gchar           *note;
	GList           *assignments;
	
	MrpCalendar     *calendar;
};

/* Properties */
enum {
        PROP_0,
        PROP_NAME,
	PROP_SHORT_NAME,
        PROP_GROUP,
        PROP_TYPE,
        PROP_UNITS,
        PROP_EMAIL,
        PROP_NOTE,
	PROP_CALENDAR
};

/* Signals */
enum {
	ASSIGNMENT_ADDED,
	ASSIGNMENT_REMOVED,
	LAST_SIGNAL
};


static void resource_class_init            (MrpResourceClass *klass);
static void resource_init                  (MrpResource      *resource);
static void resource_finalize              (GObject          *object);
static void resource_set_property          (GObject          *object,
					    guint             prop_id,
					    const GValue     *value,
					    GParamSpec       *pspec);
static void resource_get_property          (GObject          *object,
					    guint             prop_id,
					    GValue           *value,
					    GParamSpec       *pspec);
static void resource_calendar_changed      (MrpCalendar      *calendar,
					    MrpResource      *resource);
static void resource_removed               (MrpObject        *object);
static void resource_assignment_removed_cb (MrpAssignment    *assignment,
					    MrpResource      *resource);
static void resource_group_removed_cb      (MrpGroup         *group,
					    MrpResource      *resource);


static MrpObjectClass *parent_class;
static guint signals[LAST_SIGNAL];


GType
mrp_resource_get_type (void)
{
	static GType object_type = 0;

	if (!object_type) {
		static const GTypeInfo object_info = {
			sizeof (MrpResourceClass),
			NULL,		/* base_init */
			NULL,		/* base_finalize */
			(GClassInitFunc) resource_class_init,
			NULL,		/* class_finalize */
			NULL,		/* class_data */
			sizeof (MrpResource),
			0,              /* n_preallocs */
			(GInstanceInitFunc) resource_init,
		};

		object_type = g_type_register_static (MRP_TYPE_OBJECT,
                                                      "MrpResource", 
                                                      &object_info, 0);
	}

	return object_type;
}

static void
resource_class_init (MrpResourceClass *klass)
{
        GObjectClass   *object_class = G_OBJECT_CLASS (klass);
        MrpObjectClass *mrp_object_class = MRP_OBJECT_CLASS (klass);
	
        parent_class = MRP_OBJECT_CLASS (g_type_class_peek_parent (klass));
        
        object_class->finalize     = resource_finalize;
        object_class->set_property = resource_set_property;
        object_class->get_property = resource_get_property;
	
	mrp_object_class->removed  = resource_removed;
        
	/* Properties */
        g_object_class_install_property (object_class,
                                         PROP_NAME,
                                         g_param_spec_string ("name",
                                                              "Name",
                                                              "The name of the resource",
                                                              NULL,
                                                              G_PARAM_READWRITE));
        g_object_class_install_property (object_class,
                                         PROP_SHORT_NAME,
                                         g_param_spec_string ("short_name",
                                                              "Short name",
                                                              "The shorter name, initials or nickname of the resource",
                                                              NULL,
                                                              G_PARAM_READWRITE));
        g_object_class_install_property (object_class,
                                         PROP_GROUP,
                                         g_param_spec_object ("group",
							      "Group",
							      "The group that the resource belongs to",
							      MRP_TYPE_GROUP,
							      G_PARAM_READWRITE));
        g_object_class_install_property (object_class,
                                         PROP_TYPE,
                                         g_param_spec_int ("type",
                                                           "Type",
                                                           "The type of resource this is",
                                                           MRP_RESOURCE_TYPE_NONE,
                                                           MRP_RESOURCE_TYPE_MATERIAL,
                                                           MRP_RESOURCE_TYPE_WORK,
                                                           G_PARAM_READWRITE));
        
        g_object_class_install_property (object_class,
                                         PROP_UNITS,
                                         g_param_spec_int ("units",
                                                           "Units",
                                                           "The amount of units this resource has",
                                                           -1,
                                                           G_MAXINT,
                                                           0,
                                                           G_PARAM_READWRITE));
        
        g_object_class_install_property (object_class,
                                         PROP_EMAIL,
                                         g_param_spec_string ("email",
                                                              "Email",
                                                              "The email address of the resource",
                                                              NULL,
                                                              G_PARAM_READWRITE));
        g_object_class_install_property (object_class,
                                         PROP_NOTE,
                                         g_param_spec_string ("note",
                                                              "Note",
							      "Resource note",
							      "",
							      G_PARAM_READWRITE));
	g_object_class_install_property (object_class,
                                         PROP_CALENDAR,
                                         g_param_spec_pointer ("calendar",
							       "Calendar",
							       "The calendar this resource uses",
							       G_PARAM_READWRITE));
                
	/* Signals */
	signals[ASSIGNMENT_ADDED] = 
		g_signal_new ("assignment_added",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      0,
			      NULL, NULL,
			      mrp_marshal_VOID__OBJECT,
			      G_TYPE_NONE,
			      1, MRP_TYPE_ASSIGNMENT);

	signals[ASSIGNMENT_REMOVED] = 
		g_signal_new ("assignment_removed",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      0,
			      NULL, NULL,
			      mrp_marshal_VOID__OBJECT,
			      G_TYPE_NONE,
			      1, MRP_TYPE_ASSIGNMENT);
}

static void
resource_init (MrpResource *resource)
{
        MrpResourcePriv *priv;
        
        priv = g_new0 (MrpResourcePriv, 1);

	priv->assignments = NULL;
	priv->type        = MRP_RESOURCE_TYPE_NONE;
        priv->name        = g_strdup ("");
	priv->short_name    = g_strdup ("");
	priv->group       = NULL;
	priv->email       = g_strdup ("");
	priv->note        = g_strdup ("");   
        resource->priv = priv;
}

static void
resource_finalize (GObject *object)
{
        MrpResource     *resource = MRP_RESOURCE (object);
        MrpResourcePriv *priv;
        
        priv = resource->priv;

        g_free (priv->name);
	g_free (priv->short_name);
        g_free (priv->email);
        g_free (priv->note);
	if (priv->group) {
		g_object_unref (priv->group);
	}
	if (priv->calendar) {
		g_object_unref (priv->calendar);
	}
	
	g_free (priv);
	resource->priv = NULL;

        if (G_OBJECT_CLASS (parent_class)->finalize) {
                (* G_OBJECT_CLASS (parent_class)->finalize) (object);
        }
}

static void
resource_set_property (GObject      *object,
		       guint         prop_id,
		       const GValue *value,
		       GParamSpec   *pspec)
{
	MrpResource     *resource;
	MrpResourcePriv *priv;
	gboolean         changed = FALSE;
	const gchar     *str;
	gint             i_val;
	MrpGroup        *group;
	MrpCalendar     *calendar;
	MrpProject      *project;
	
	resource = MRP_RESOURCE (object);
	priv     = resource->priv;
	
	switch (prop_id) {
	case PROP_NAME:
		str = g_value_get_string (value);
		if (!priv->name || strcmp (priv->name, str)) {
			g_free (priv->name);
			priv->name = g_strdup (str);
			changed = TRUE;
		}
		break;
		
	case PROP_SHORT_NAME: 
		str = g_value_get_string (value);
		if (!priv->short_name || strcmp (priv->short_name, str)) {
			g_free (priv->short_name);
			priv->short_name = g_strdup (str);
			changed = TRUE;
		}
		break;

	case PROP_GROUP:
		if (priv->group != NULL) {
			g_object_unref (priv->group);
			g_signal_handlers_disconnect_by_func 
				(priv->group,
				 resource_group_removed_cb,
				 resource);
			
		}

		group = g_value_get_object (value);
		if (group != NULL) {
			g_object_ref (group);
			g_signal_connect (G_OBJECT (group),
					  "removed",
					  G_CALLBACK (resource_group_removed_cb),
					  resource);
		}
		if (group != priv->group) {
			changed = TRUE;
		}
		priv->group = group;
		break;
	case PROP_TYPE:
		i_val = g_value_get_int (value);
		
		if (priv->type != i_val) {
			priv->type = i_val;
			changed = TRUE;
		}
		break;
	case PROP_UNITS:
		i_val = g_value_get_int (value);
		
		if (priv->units != i_val) {
			priv->units = i_val;
			changed = TRUE;
		}
		break;
	case PROP_EMAIL:
		str = g_value_get_string (value);
		
		if (!priv->email || strcmp (priv->email, str)) {
			g_free (priv->email);
			priv->email = g_strdup (str);
			changed = TRUE;
		}
		break;			
	case PROP_NOTE:
		str = g_value_get_string (value);
		
		if (!priv->note || strcmp (priv->note, str)) {
			g_free (priv->note);
			priv->note = g_strdup (str);
			changed = TRUE;
		}
		
		break;
	case PROP_CALENDAR:
		calendar = g_value_get_pointer (value);
		if (calendar != priv->calendar) {
			changed = TRUE;
		} else {
			break;
		}
		
		if (priv->calendar != NULL) {
			g_signal_handlers_disconnect_by_func (priv->calendar,
							      resource_calendar_changed,
							      resource);
			g_object_unref (priv->calendar);
		}
		
		if (calendar != NULL) {
			g_object_ref (calendar);
			
			g_signal_connect_object (calendar,
						 "calendar_changed",
						 G_CALLBACK (resource_calendar_changed),
						 resource,
						 0);
		}

		priv->calendar = calendar;

		/* Make sure the project is rescheduled if necessary. */
		if (priv->assignments) {
			project = mrp_object_get_project (MRP_OBJECT (resource));
			if (project) {
				mrp_project_reschedule (project);
			}
		}
		break;

	default:
		break;
	}

	if (changed) {
		mrp_object_changed (MRP_OBJECT (object));
	}
}

static void
resource_get_property (GObject    *object,
		       guint       prop_id,
		       GValue     *value,
		       GParamSpec *pspec)
{
	MrpResource     *resource;
	MrpResourcePriv *priv;
	
	resource = MRP_RESOURCE (object);
	priv     = resource->priv;
	
	switch (prop_id) {
	case PROP_NAME:
		g_value_set_string (value, priv->name);
		break;
	case PROP_SHORT_NAME:
		g_value_set_string (value, priv->short_name);
		break;
	case PROP_GROUP:
		g_value_set_object (value, priv->group);
		break;
	case PROP_TYPE:
		g_value_set_int (value, priv->type);
		break;
	case PROP_UNITS:
		g_value_set_int (value, priv->units);
		break;
	case PROP_EMAIL:
		g_value_set_string (value, priv->email);
		break;
	case PROP_NOTE:
		g_value_set_string (value, priv->note);
		break;
	case PROP_CALENDAR:
		g_value_set_pointer (value, priv->calendar);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
resource_calendar_changed (MrpCalendar *calendar,
			   MrpResource *resource)
{
	MrpProject *project;

	project = mrp_object_get_project (MRP_OBJECT (resource));

	if (!project) {
		return;
	}
			
	mrp_project_reschedule (project);
}

static void
resource_remove_assignment_foreach (MrpAssignment *assignment, 
				    MrpResource   *resource)
{
	g_return_if_fail (MRP_IS_ASSIGNMENT (assignment));
	
	g_signal_handlers_disconnect_by_func (MRP_OBJECT (assignment),
					      resource_assignment_removed_cb,
					      resource);

	g_object_unref (assignment);

	mrp_object_removed (MRP_OBJECT (assignment));
}

static void
resource_removed (MrpObject *object)
{
	MrpResource     *resource;
	MrpResourcePriv *priv;
	
	g_return_if_fail (MRP_IS_RESOURCE (object));
	
	resource = MRP_RESOURCE (object);
	priv     = resource->priv;
	
	g_list_foreach (priv->assignments,
			(GFunc) resource_remove_assignment_foreach,
			resource);
	
	g_list_free (priv->assignments);
	priv->assignments = NULL;

        if (MRP_OBJECT_CLASS (parent_class)->removed) {
                (* MRP_OBJECT_CLASS (parent_class)->removed) (object);
        }
}

static void
resource_group_removed_cb (MrpGroup     *group,
			   MrpResource  *resource)
{
	g_return_if_fail (MRP_IS_RESOURCE (resource));
	g_return_if_fail (MRP_IS_GROUP (group));

	mrp_object_set (MRP_OBJECT (resource), "group", NULL, NULL);
}

static void
resource_assignment_removed_cb (MrpAssignment *assignment, 
				MrpResource *resource)
{
	MrpResourcePriv *priv;
	MrpTask         *task;
 	
	g_return_if_fail (MRP_IS_RESOURCE (resource));
	g_return_if_fail (MRP_IS_ASSIGNMENT (assignment));
	
	priv = resource->priv;

	task = mrp_assignment_get_task (assignment);
	
	if (!task) {
		g_warning ("Task not found in resource's assignment list");
		return;
	}

	priv->assignments = g_list_remove (priv->assignments, assignment);

	g_signal_emit (resource, signals[ASSIGNMENT_REMOVED], 
		       0, 
		       assignment);

	g_object_unref (assignment);

	mrp_object_changed (MRP_OBJECT (resource));
}

/**
 * mrp_resource_add_assignment:
 * @resource: an #MrpResource
 * @assignment: an #MrpAssignment
 *
 * Adds an assignment to @resource. This increases the reference count of
 * @assignment.
 * 
 **/
void
imrp_resource_add_assignment (MrpResource *resource, MrpAssignment *assignment)
{
	MrpResourcePriv *priv;
	MrpTask         *task;
	
	g_return_if_fail (MRP_IS_RESOURCE (resource));
	g_return_if_fail (MRP_IS_ASSIGNMENT (assignment));
	
	priv = resource->priv;
	
	task = mrp_assignment_get_task (assignment);

	priv->assignments = g_list_prepend (priv->assignments, 
					    g_object_ref (assignment));
	
	g_signal_connect (G_OBJECT (assignment),
			  "removed",
			  G_CALLBACK (resource_assignment_removed_cb),
			  resource);
	
	g_signal_emit (resource, signals[ASSIGNMENT_ADDED], 0, assignment);

	mrp_object_changed (MRP_OBJECT (resource));
}

/**
 * mrp_resource_new:
 *
 * Creates a new empty resource.
 * 
 * Return value: the newly created resource.
 **/
MrpResource *
mrp_resource_new (void)
{
        MrpResource *resource;
        
        resource = g_object_new (MRP_TYPE_RESOURCE, NULL);
        
        return resource;
}

/**
 * mrp_resource_get_name:
 * @resource: an #MrpResource
 * 
 * Retrives the name of @resource.
 * 
 * Return value: the name
 **/
const gchar *
mrp_resource_get_name (MrpResource *resource)
{
	g_return_val_if_fail (MRP_IS_RESOURCE (resource), NULL);
		
	return resource->priv->name;
}

/**
 * mrp_resource_set_name:
 * @resource: an #MrpResource
 * @name: new name of @resource
 * 
 * Sets the name of @resource.
 **/
void mrp_resource_set_name (MrpResource *resource, const gchar *name)
{
	g_return_if_fail (MRP_IS_RESOURCE (resource));

	mrp_object_set (MRP_OBJECT (resource), "name", name, NULL);
}

/**
 * mrp_resource_get_short_name:
 * @resource: an #MrpResource
 * 
 * Retrives the short_name of @resource.
 * 
 * Return value: the short name
 **/
const gchar *
mrp_resource_get_short_name (MrpResource *resource)
{
	g_return_val_if_fail (MRP_IS_RESOURCE (resource), NULL);
		
	return resource->priv->short_name;
}

/**
 * mrp_resource_set_short_name:
 * @resource: an #MrpResource
 * @name: new short name of @resource
 * 
 * Sets the short name of @resource.
 **/
void mrp_resource_set_short_name (MrpResource *resource, const gchar *name)
{
	g_return_if_fail (MRP_IS_RESOURCE (resource));

	mrp_object_set (MRP_OBJECT (resource), "short_name", name, NULL);
}

/**
 * mrp_resource_assign:
 * @resource: an #MrpResource
 * @task: an #MrpTask
 * @units: the amount of units of assignment
 *
 * Assigns @resource to @task by the given amount of @units. A value of 100
 * units corresponds to fulltime assignment.
 * 
 **/
void
mrp_resource_assign (MrpResource *resource,
		     MrpTask     *task,
		     gint         units)
{
	MrpAssignment   *assignment;
	
	g_return_if_fail (MRP_IS_RESOURCE (resource));
	g_return_if_fail (MRP_IS_TASK (task));

	assignment = g_object_new (MRP_TYPE_ASSIGNMENT,
				   "resource", resource,
				   "task", task,
				   "units", units,
				   NULL);

	imrp_resource_add_assignment (resource, assignment);
	imrp_task_add_assignment (task, assignment);

	g_object_unref (assignment);
}

/**
 * mrp_resource_get_assignments:
 * @resource: an #MrpResource.
 * 
 * Retrieves the assignments that this resource has. If caller needs to 
 * manipulate the returned list, a copy of it needs to be made.
 * 
 * Return value: The assignments of @resource. It should not be freed.
 **/
GList *
mrp_resource_get_assignments (MrpResource *resource)
{
	g_return_val_if_fail (MRP_IS_RESOURCE (resource), NULL);
	
	return resource->priv->assignments;
}

/**
 * mrp_resource_get_assigned_tasks:
 * @resource: an #MrpResource
 * 
 * Retrieves a list of all the tasks that this resource is assigned to. It is
 * basically a convenience wrapper around mrp_resource_get_assignments().
 * 
 * Return value: A list of the tasks that this resource is assigned to. Needs to
 * be freed when not used anymore.
 **/
GList *
mrp_resource_get_assigned_tasks (MrpResource *resource)
{
	GList         *list = NULL;
	GList         *l;
	MrpAssignment *assignment;
	MrpTask       *task;
	
	g_return_val_if_fail (MRP_IS_RESOURCE (resource), NULL);
	
	for (l = resource->priv->assignments; l; l = l->next) {
		assignment = l->data;
		task       = mrp_assignment_get_task (assignment);

		list = g_list_prepend (list, task);
	}

	list = g_list_sort (list, mrp_task_compare);

	return list;
}

/**
 * mrp_resource_compare:
 * @a: an #MrpResource
 * @b: an #MrpResource
 * 
 * Comparison routine for resources. It is suitable for sorting, and only
 * compares the resource name.
 * 
 * Return value: -1 if @a is less than @b, 1 id @a is greater than @b, and 1 if
 * equal.
 **/
gint
mrp_resource_compare (gconstpointer a, gconstpointer b)
{
	return strcmp (MRP_RESOURCE(a)->priv->name, 
		       MRP_RESOURCE(b)->priv->name);
}

/**
 * mrp_resource_get_calendar:
 * @resource: an #MrpResource
 * 
 * Retrieves the calendar that is used for @resource. If no calendar is set,
 * %NULL is returned, which means the project default calendar.
 * 
 * Return value: a #MrpCalendar, or %NULL if no specific calendar is set.
 **/
MrpCalendar *
mrp_resource_get_calendar (MrpResource *resource)
{
	MrpResourcePriv *priv;
	
	g_return_val_if_fail (MRP_IS_RESOURCE (resource), NULL);

	priv = resource->priv;
	
	return priv->calendar;
}

/**
 * mrp_resource_set_calendar:
 * @resource: an #MrpResource
 * @calendar: the #MrpCalendar to set, or %NULL 
 *
 * Sets the calendar to use for @resource. %NULL means to use the project
 * default calendar.
 * 
 **/
void
mrp_resource_set_calendar (MrpResource *resource, MrpCalendar *calendar)
{
	g_return_if_fail (MRP_IS_RESOURCE (resource));

	g_object_set (resource, "calendar", calendar, NULL);
}
