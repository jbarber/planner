/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2002-2003 CodeFactory AB
 * Copyright (C) 2002-2003 Richard Hult <richard@imendio.com>
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

#include <config.h>
#include <string.h>
#include <glib/gi18n.h>
#include "mrp-types.h"
#include "mrp-marshal.h"
#include "mrp-relation.h"
#include "mrp-task.h"

struct _MrpRelationPriv {
	MrpTask         *successor;
	MrpTask         *predecessor;

	MrpRelationType  type;
	gint             lag;
};

/* Properties */
enum {
	PROP_0,
	PROP_PREDECESSOR,
	PROP_SUCCESSOR,
	PROP_TYPE,
	PROP_LAG
};

/* Signals */
enum {
	CHANGED, /* ? */
	LAST_SIGNAL
};


static void   relation_class_init                (MrpRelationClass  *klass);
static void   relation_init                      (MrpRelation       *relation);
static void   relation_finalize                  (GObject          *object);
static void   relation_set_property              (GObject          *object,
						  guint             prop_id,
						  const GValue     *value,
						  GParamSpec       *pspec);
static void   relation_get_property              (GObject          *object,
						  guint             prop_id,
						  GValue           *value,
						  GParamSpec       *pspec);

static MrpObjectClass *parent_class;
static guint signals[LAST_SIGNAL];

GType
mrp_relation_get_type (void)
{
	static GType object_type = 0;

	if (!object_type) {
		static const GTypeInfo object_info = {
			sizeof (MrpRelationClass),
			NULL,		/* base_init */
			NULL,		/* base_finalize */
			(GClassInitFunc) relation_class_init,
			NULL,		/* class_finalize */
			NULL,		/* class_data */
			sizeof (MrpRelation),
			0,              /* n_preallocs */
			(GInstanceInitFunc) relation_init,
		};

		object_type = g_type_register_static (MRP_TYPE_OBJECT,
						      "MrpRelation",
						      &object_info, 0);
	}

	return object_type;
}

static void
relation_class_init (MrpRelationClass *klass)
{
	GObjectClass *object_class = (GObjectClass *) klass;

	parent_class = g_type_class_peek_parent (klass);

	object_class->finalize     = relation_finalize;
	object_class->set_property = relation_set_property;
	object_class->get_property = relation_get_property;

	signals[CHANGED] = g_signal_new
		("changed",
		 G_TYPE_FROM_CLASS (klass),
		 G_SIGNAL_RUN_LAST,
		 0, /*G_STRUCT_OFFSET (MrpRelationClass, method), */
		 NULL, NULL,
		 mrp_marshal_VOID__VOID,
		 G_TYPE_NONE,
		 0);

	/* Properties. */
	g_object_class_install_property (object_class,
					 PROP_SUCCESSOR,
					 g_param_spec_object ("successor",
							      "Successor",
							      "The successor in the relation",
							      MRP_TYPE_TASK,
							      G_PARAM_READWRITE |
							      G_PARAM_CONSTRUCT_ONLY));
	g_object_class_install_property (object_class,
					 PROP_PREDECESSOR,
					 g_param_spec_object ("predecessor",
							      "Predecessor",
							      "The predecessor in the relation",
							      MRP_TYPE_TASK,
							      G_PARAM_READWRITE |
							      G_PARAM_CONSTRUCT_ONLY));

	g_object_class_install_property (object_class,
					 PROP_TYPE,
					 g_param_spec_enum ("type",
							    "Type",
							    "The type of relation",
							    MRP_TYPE_RELATION_TYPE,
							    MRP_RELATION_FS,
							    G_PARAM_READWRITE));

	g_object_class_install_property (object_class,
					 PROP_LAG,
					 g_param_spec_int ("lag",
							   "Lag",
							   "Lag between the predecessor and successor",
							   -G_MAXINT, G_MAXINT, 0,
							   G_PARAM_READWRITE));
}

static void
relation_init (MrpRelation *relation)
{
	MrpRelationPriv *priv;

	relation->priv = g_new0 (MrpRelationPriv, 1);

	priv = relation->priv;

	priv->type = MRP_RELATION_FS;
	priv->lag  = 0;
}

static void
relation_finalize (GObject *object)
{
	MrpRelation *relation = MRP_RELATION (object);

	g_object_unref (relation->priv->successor);
	g_object_unref (relation->priv->predecessor);

	if (G_OBJECT_CLASS (parent_class)->finalize) {
		(* G_OBJECT_CLASS (parent_class)->finalize) (object);
	}
}

static void
relation_set_property (GObject      *object,
		       guint         prop_id,
		       const GValue *value,
		       GParamSpec   *pspec)
{
	MrpRelation     *relation;
	MrpRelationPriv *priv;
	MrpTask         *task;
	gboolean         changed = FALSE;

	relation = MRP_RELATION (object);
	priv    = relation->priv;

	switch (prop_id) {
	case PROP_SUCCESSOR:
		priv->successor = g_value_get_object (value);
		if (priv->successor) {
			g_object_ref (priv->successor);
			changed = TRUE;
		}
		break;

	case PROP_PREDECESSOR:
		priv->predecessor = g_value_get_object (value);
		if (priv->predecessor) {
			g_object_ref (priv->predecessor);
			changed = TRUE;
		}
		break;

	case PROP_TYPE:
		priv->type = (MrpRelationType) g_value_get_enum (value);
		changed = TRUE;
		break;

	case PROP_LAG:
		priv->lag = g_value_get_int (value);
		changed = TRUE;
		break;

	default:
		break;
	}

	if (changed) {
		task = mrp_relation_get_predecessor (relation);
		/* If we get called from the constructor, we don't always have
		 * one of these.
		 */
		if (task == NULL) {
			task = mrp_relation_get_successor (relation);
		}
		if (task != NULL) {
			mrp_object_changed (MRP_OBJECT (task));
		}
	}
}

static void
relation_get_property (GObject    *object,
		       guint       prop_id,
		       GValue     *value,
		       GParamSpec *pspec)
{
	MrpRelation     *relation;
	MrpRelationPriv *priv;

	relation = MRP_RELATION (object);
	priv    = relation->priv;

	switch (prop_id) {
	case PROP_SUCCESSOR:
		g_value_set_object (value, priv->successor);
		break;

	case PROP_PREDECESSOR:
		g_value_set_object (value, priv->predecessor);
		break;

	case PROP_TYPE:
		g_value_set_enum (value, priv->type);
		break;

	case PROP_LAG:
		g_value_set_int (value, priv->lag);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

/**
 * mrp_relation_get_predecessor:
 * @relation: an #MrpRelation
 *
 * Retrieves the predecessor of @relation.
 *
 * Return value: the predecessor task.
 **/
MrpTask *
mrp_relation_get_predecessor (MrpRelation *relation)
{
	g_return_val_if_fail (MRP_IS_RELATION (relation), NULL);

	return relation->priv->predecessor;
}

/**
 * mrp_relation_get_successor:
 * @relation: an #MrpRelation
 *
 * Retrieves the successor of @relation.
 *
 * Return value: the successor task.
 **/
MrpTask *
mrp_relation_get_successor (MrpRelation *relation)
{
	g_return_val_if_fail (MRP_IS_RELATION (relation), NULL);

	return relation->priv->successor;
}

/**
 * mrp_relation_get_lag:
 * @relation: an #MrpRelation
 *
 * Retrieves the lag between the predecessor and successor in @relation.
 *
 * Return value: Lag time in seconds.
 **/
gint
mrp_relation_get_lag (MrpRelation *relation)
{
	g_return_val_if_fail (MRP_IS_RELATION (relation), 0);

	return relation->priv->lag;
}

/**
 * mrp_relation_get_relation_type:
 * @relation: an #MrpRelation
 *
 * Retrieves the relation type of @relation.
 *
 * Return value: the #MrpRelationType of the relation.
 **/
/* Cumbersome name, but mrp_relation_get_type is already taken :) */
MrpRelationType
mrp_relation_get_relation_type (MrpRelation *relation)
{
	g_return_val_if_fail (MRP_IS_RELATION (relation), MRP_RELATION_NONE);

	return relation->priv->type;
}
