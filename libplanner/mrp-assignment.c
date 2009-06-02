/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
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

#include <glib/gi18n.h>
#include "mrp-task.h"
#include "mrp-resource.h"
#include "mrp-marshal.h"
#include "mrp-assignment.h"

struct _MrpAssignmentPriv {
	MrpTask     *task;
	MrpResource *resource;

	gint         units;
};

/* Properties */
enum {
        PROP_0,
        PROP_TASK,
        PROP_RESOURCE,
        PROP_UNITS
};


static void assignment_class_init        (MrpAssignmentClass *klass);
static void assignment_init              (MrpAssignment      *assignment);
static void assignment_finalize          (GObject            *object);
static void assignment_set_property      (GObject            *object,
					  guint               prop_id,
					  const GValue       *value,
					  GParamSpec         *pspec);
static void assignment_get_property      (GObject            *object,
					  guint               prop_id,
					  GValue             *value,
					  GParamSpec         *pspec);

static MrpObjectClass *parent_class;

GType
mrp_assignment_get_type (void)
{
	static GType type = 0;

	if (!type) {
		static const GTypeInfo info = {
			sizeof (MrpAssignmentClass),
			NULL,		/* base_init */
			NULL,		/* base_finalize */
			(GClassInitFunc) assignment_class_init,
			NULL,		/* class_finalize */
			NULL,		/* class_data */
			sizeof (MrpAssignment),
			0,              /* n_preallocs */
			(GInstanceInitFunc) assignment_init,
		};

		type = g_type_register_static (MRP_TYPE_OBJECT,
					       "MrpAssignment",
					       &info, 0);
	}

	return type;
}

static void
assignment_class_init (MrpAssignmentClass *klass)
{
        GObjectClass   *object_class     = G_OBJECT_CLASS (klass);

        parent_class = MRP_OBJECT_CLASS (g_type_class_peek_parent (klass));

        object_class->finalize     = assignment_finalize;
        object_class->set_property = assignment_set_property;
        object_class->get_property = assignment_get_property;

	/* Properties */
        g_object_class_install_property (object_class,
                                         PROP_TASK,
                                         g_param_spec_object ("task",
							      "Task",
							      "The task",
							      MRP_TYPE_TASK,
							      G_PARAM_READWRITE));
        g_object_class_install_property (object_class,
                                         PROP_RESOURCE,
                                         g_param_spec_object ("resource",
							      "Resource",
							      "The resource that is assigned to the task",
							      MRP_TYPE_RESOURCE,
							      G_PARAM_READWRITE));
        g_object_class_install_property (object_class,
                                         PROP_UNITS,
                                         g_param_spec_int ("units",
							   "Units",
							   "Number of units assignment",
							   -1,
							   G_MAXINT,
							   0,
							   G_PARAM_READWRITE));
}

static void
assignment_init (MrpAssignment *assignment)
{
        MrpAssignmentPriv *priv;

        priv = g_new0 (MrpAssignmentPriv, 1);

        assignment->priv = priv;
}

static void
assignment_finalize (GObject *object)
{
        MrpAssignment     *assignment = MRP_ASSIGNMENT (object);
        MrpAssignmentPriv *priv;

        priv = assignment->priv;

	if (priv->task) {
		g_object_unref (priv->task);
		priv->task = NULL;
	}

	if (priv->resource) {
		g_object_unref (priv->resource);
		priv->resource = NULL;
	}

        if (G_OBJECT_CLASS (parent_class)->finalize) {
                (* G_OBJECT_CLASS (parent_class)->finalize) (object);
        }
}

static void
assignment_set_property (GObject      *object,
			 guint         prop_id,
			 const GValue *value,
			 GParamSpec   *pspec)
{
	MrpAssignment     *assignment;
	MrpAssignmentPriv *priv;

	assignment = MRP_ASSIGNMENT (object);
	priv       = assignment->priv;

	/* FIXME: See bug #138368 about this. The assignment doesn't have a
	 * project pointer so we can't emit changed on it. We cheat for now and
	 * use the resource/task in those cases.
	 */

	switch (prop_id) {
	case PROP_TASK:
		if (priv->task) {
			g_object_unref (priv->task);
		}
		priv->task = g_object_ref (g_value_get_object (value));
		mrp_object_changed (MRP_OBJECT (priv->task));
		break;

	case PROP_RESOURCE:
		if (priv->resource) {
			g_object_unref (priv->resource);
		}
		priv->resource = g_object_ref (g_value_get_object (value));
		mrp_object_changed (MRP_OBJECT (priv->resource));
		break;

	case PROP_UNITS:
		priv->units = g_value_get_int (value);
		mrp_object_changed (MRP_OBJECT (priv->resource));
		break;

	default:
		break;
	}
}

static void
assignment_get_property (GObject    *object,
			 guint       prop_id,
			 GValue     *value,
			 GParamSpec *pspec)
{
	MrpAssignment     *assignment;
	MrpAssignmentPriv *priv;

	assignment = MRP_ASSIGNMENT (object);
	priv       = assignment->priv;

	switch (prop_id) {
	case PROP_TASK:
		g_value_set_object (value, priv->task);
		break;
	case PROP_RESOURCE:
		g_value_set_object (value, priv->resource);
		break;
	case PROP_UNITS:
		g_value_set_int (value, priv->units);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

/**
 * mrp_assignment_new:
 *
 * Creates a new, empty, assignment. You most often don't want to create an
 * assignment explicitly like this, but using mrp_resource_assign() instead.
 *
 * Return value: Newly created assignment.
 **/
MrpAssignment *
mrp_assignment_new (void)
{
        MrpAssignment *assignment;

        assignment = g_object_new (MRP_TYPE_ASSIGNMENT, NULL);

        return assignment;
}

/**
 * mrp_assignment_get_task:
 * @assignment: an #MrpAssignment
 *
 * Retrieves the #MrpTask associated with @assignment.
 *
 * Return value: the task associated with the assignment object. The reference
 * count of the task is not increased.
 **/
MrpTask *
mrp_assignment_get_task (MrpAssignment *assignment)
{
	g_return_val_if_fail (MRP_IS_ASSIGNMENT (assignment), NULL);

	return assignment->priv->task;
}

/**
 * mrp_assignment_get_resource:
 * @assignment: an #MrpAssignment
 *
 * Retrieves the #MrpResource associated with @assignment.
 *
 * Return value: the resource associated with the assignment object. The reference
 * count of the resource is not increased.
 **/
MrpResource *
mrp_assignment_get_resource (MrpAssignment *assignment)
{
	g_return_val_if_fail (MRP_IS_ASSIGNMENT (assignment), NULL);

	return assignment->priv->resource;
}

/**
 * mrp_assignment_get_units:
 * @assignment: an #MrpAssignment
 *
 * Retrieves the number of units that the resource is assigned with to the
 * task. 100 means 100%, etc.
 *
 * Return value: number of units of the assignment.
 **/
gint
mrp_assignment_get_units (MrpAssignment *assignment)
{
	g_return_val_if_fail (MRP_IS_ASSIGNMENT (assignment), -1);

	return assignment->priv->units;
}
