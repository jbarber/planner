/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2002 CodeFactory AB
 * Copyright (C) 2002 Richard Hult <richard@imendio.com>
 * Copyright (C) 2002 Mikael Hallendal <micke@imendio.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <config.h>
#include <gtk/gtktreemodel.h>
#include <glib/gi18n.h>
#include <libplanner/mrp-assignment.h>
#include <libplanner/mrp-resource.h>
#include "planner-assignment-model.h"

struct _PlannerAssignmentModelPriv {
	MrpProject *project;
	MrpTask    *task;
};

#define G_LIST(x) ((GList *) x)

static void       mam_init                  (PlannerAssignmentModel      *model);
static void       mam_class_init            (PlannerAssignmentModelClass *class);

static void       mam_finalize              (GObject              *object);
static gint       mam_get_n_columns         (GtkTreeModel         *treemodel);
static GType      mam_get_column_type       (GtkTreeModel         *treemodel,
					     gint                  index);

static void       mam_get_value             (GtkTreeModel         *treemodel,
					     GtkTreeIter          *iter,
					     gint                  column,
					     GValue               *value);
static void       mam_assignment_changed_cb (MrpTask              *task,
					     MrpAssignment        *assignment,
					     PlannerAssignmentModel    *model);
static void       mam_resource_added_cb     (MrpProject           *project, 
					     MrpResource          *assignment,
					     PlannerAssignmentModel    *model);
static void       mam_resource_removed_cb   (MrpProject           *project, 
					     MrpResource          *resource,
					     PlannerAssignmentModel    *model);
static void       mam_resource_notify_cb    (MrpResource          *resource,
					     GParamSpec           *pspec,
					     PlannerAssignmentModel    *model);


static PlannerListModelClass *parent_class = NULL;


GType
planner_assignment_model_get_type (void)
{
        static GType type = 0;
        
        if (!type) {
                static const GTypeInfo info =
                        {
                                sizeof (PlannerAssignmentModelClass),
                                NULL,		/* base_init */
                                NULL,		/* base_finalize */
                                (GClassInitFunc) mam_class_init,
                                NULL,		/* class_finalize */
                                NULL,		/* class_data */
                                sizeof (PlannerAssignmentModel),
                                0,
                                (GInstanceInitFunc) mam_init,
                        };

                type = g_type_register_static (PLANNER_TYPE_LIST_MODEL,
					       "PlannerAssignmentModel",
					       &info, 0);
        }
        
        return type;
}

static void
mam_class_init (PlannerAssignmentModelClass *klass)
{
        GObjectClass     *object_class;
	PlannerListModelClass *lm_class;
	
        parent_class = g_type_class_peek_parent (klass);
        object_class = G_OBJECT_CLASS (klass);
	lm_class     = PLANNER_LIST_MODEL_CLASS (klass);
	
        object_class->finalize = mam_finalize;

        lm_class->get_n_columns   = mam_get_n_columns;
        lm_class->get_column_type = mam_get_column_type;
        lm_class->get_value       = mam_get_value;
}

static void
mam_init (PlannerAssignmentModel *model)
{
        PlannerAssignmentModelPriv *priv;
        
        priv = g_new0 (PlannerAssignmentModelPriv, 1);
        
	priv->project = NULL;

        model->priv = priv;
}

static void
mam_finalize (GObject *object)
{
	PlannerAssignmentModel *model = PLANNER_ASSIGNMENT_MODEL (object);

        if (model->priv) {
		if (model->priv->project) {
			g_object_unref (model->priv->project);
		}
		if (model->priv->task) {
			g_object_unref (model->priv->task);
		}

                g_free (model->priv);
                model->priv = NULL;
        }
        
	if (G_OBJECT_CLASS (parent_class)->finalize) {
		(* G_OBJECT_CLASS (parent_class)->finalize) (object);
	}
}

static gint
mam_get_n_columns (GtkTreeModel *tree_model)
{
        return NUMBER_OF_RESOURCE_ASSIGNMENT_COLS;
}

static GType
mam_get_column_type (GtkTreeModel *tree_model,
                     gint          column)
{
        switch (column) {
        case RESOURCE_ASSIGNMENT_COL_NAME:
                return G_TYPE_STRING;
        case RESOURCE_ASSIGNMENT_COL_UNITS:
                return G_TYPE_INT;
        case RESOURCE_ASSIGNMENT_COL_COST_STD:
        case RESOURCE_ASSIGNMENT_COL_COST_OVT:
                return G_TYPE_FLOAT;
	case RESOURCE_ASSIGNMENT_COL_ASSIGNED:
		return G_TYPE_BOOLEAN;
	case RESOURCE_ASSIGNMENT_COL_ASSIGNED_UNITS:
		return G_TYPE_INT;
	case RESOURCE_ASSIGNMENT_COL_ASSIGNED_UNITS_EDITABLE:
		return G_TYPE_BOOLEAN;
        default:
		return G_TYPE_INVALID;
        }
}

static void
mam_get_value (GtkTreeModel *tree_model,
               GtkTreeIter  *iter,
               gint          column,
               GValue       *value)
{	
	PlannerAssignmentModel *model;
	MrpAssignment     *assignment;
	MrpResource       *resource;
        gchar             *str;
        gint               units;
        gfloat             rate;
	
        g_return_if_fail (PLANNER_IS_ASSIGNMENT_MODEL (tree_model));
        g_return_if_fail (iter != NULL);

	model    = PLANNER_ASSIGNMENT_MODEL (tree_model);
	resource = MRP_RESOURCE (G_LIST(iter->user_data)->data);

        switch (column) {
        case RESOURCE_ASSIGNMENT_COL_NAME:
                g_object_get (resource, "name", &str, NULL);
		if (str == NULL) {
			str = g_strdup ("");
		}
                g_value_init (value, G_TYPE_STRING);
                g_value_set_string (value, str);
                g_free (str);
                break;
        case RESOURCE_ASSIGNMENT_COL_UNITS:
                g_object_get (resource, "units", &units, NULL);
                g_value_init (value, G_TYPE_INT);
                g_value_set_int (value, units);
                break;
        case RESOURCE_ASSIGNMENT_COL_COST_STD:
                mrp_object_get (resource, "cost", &rate, NULL);
                g_value_init (value, G_TYPE_FLOAT);
                g_value_set_int (value, rate);
                break;
        case RESOURCE_ASSIGNMENT_COL_COST_OVT:
                mrp_object_get (resource, "cost_overtime", &rate, NULL);
                g_value_init (value, G_TYPE_FLOAT);
                g_value_set_int (value, rate);
                break;
	case RESOURCE_ASSIGNMENT_COL_ASSIGNED:
		g_value_init (value, G_TYPE_BOOLEAN);
		if (mrp_task_get_assignment (model->priv->task, resource)) {
			g_value_set_boolean (value, TRUE);
		} else {
			g_value_set_boolean (value, FALSE);
		}
		break;
	case RESOURCE_ASSIGNMENT_COL_ASSIGNED_UNITS:
		g_value_init (value, G_TYPE_INT);
		assignment = mrp_task_get_assignment (model->priv->task, resource);
		if (assignment) {
			g_value_set_int (value, mrp_assignment_get_units (assignment));
		} else {
			g_value_set_int (value, 0);
		}
		break;
	case RESOURCE_ASSIGNMENT_COL_ASSIGNED_UNITS_EDITABLE:
		g_value_init (value, G_TYPE_BOOLEAN);
		assignment = mrp_task_get_assignment (model->priv->task, resource);
		if (assignment) {
			MrpTaskSched sched;
			
			g_object_get (model->priv->task, "sched", &sched, NULL);
			g_value_set_boolean (value, sched == MRP_TASK_SCHED_FIXED_WORK);
		} else {
			g_value_set_boolean (value, FALSE);
		}
		break;
        default:
                g_warning ("Bad column %d requested", column);
        }
}

static void
mam_assignment_changed_cb (MrpTask           *task,
			   MrpAssignment     *assignment,
			   PlannerAssignmentModel *model)
{
	MrpResource *resource;
	
	g_return_if_fail (PLANNER_IS_ASSIGNMENT_MODEL (model));
	g_return_if_fail (MRP_IS_ASSIGNMENT (assignment));
	
	resource = mrp_assignment_get_resource (assignment);

	planner_list_model_update (PLANNER_LIST_MODEL (model), MRP_OBJECT (resource));
}

static void
mam_resource_added_cb (MrpProject        *project, 
		       MrpResource       *resource,
		       PlannerAssignmentModel *model)
{
	g_return_if_fail (PLANNER_IS_ASSIGNMENT_MODEL (model));
	g_return_if_fail (MRP_IS_RESOURCE (resource));

	planner_list_model_append (PLANNER_LIST_MODEL (model), MRP_OBJECT (resource));
	
	g_signal_connect_object (resource, 
				 "notify",
				 G_CALLBACK (mam_resource_notify_cb),
				 model, 0);
}

static void
mam_resource_removed_cb (MrpProject        *project, 
			 MrpResource       *resource,
			 PlannerAssignmentModel *model)
{
	g_return_if_fail (PLANNER_IS_ASSIGNMENT_MODEL (model));
	g_return_if_fail (MRP_IS_RESOURCE (resource));

	planner_list_model_remove (PLANNER_LIST_MODEL (model), MRP_OBJECT (resource));

	g_signal_handlers_disconnect_by_func (resource,
                                              mam_resource_notify_cb,
                                              model);
}

static void
mam_resource_notify_cb (MrpResource       *resource,
			GParamSpec        *pspec,
			PlannerAssignmentModel *model)
{
	g_return_if_fail (PLANNER_IS_ASSIGNMENT_MODEL (model));
	g_return_if_fail (MRP_IS_RESOURCE (resource));
	
	planner_list_model_update (PLANNER_LIST_MODEL (model), MRP_OBJECT (resource));
}

PlannerAssignmentModel *
planner_assignment_model_new (MrpTask *task)
{
        PlannerAssignmentModel     *model;
        PlannerAssignmentModelPriv *priv;
	GList                 *node;
	GList                 *resources;
	
        model = g_object_new (PLANNER_TYPE_ASSIGNMENT_MODEL, NULL);
        
	g_return_val_if_fail (PLANNER_IS_ASSIGNMENT_MODEL (model), NULL);

        priv = model->priv;

        priv->task    = g_object_ref (task);
	g_object_get (priv->task, "project", &priv->project, NULL);

	resources = mrp_project_get_resources (priv->project);
	planner_list_model_set_data (PLANNER_LIST_MODEL (model), resources);

	for (node = resources; node; node = node->next) {
		g_signal_connect_object (node->data,
					 "notify",
					 G_CALLBACK (mam_resource_notify_cb),
					 model, 0);
	}

	g_signal_connect_object (priv->task,
				 "assignment_added",
				 G_CALLBACK (mam_assignment_changed_cb),
				 model, 0);
	
	g_signal_connect_object (priv->task,
				 "assignment_removed",
				 G_CALLBACK (mam_assignment_changed_cb),
				 model, 0);

	g_signal_connect_object (priv->project, 
				 "resource_added", 
				 G_CALLBACK (mam_resource_added_cb), 
				 model, 0);

	g_signal_connect_object (priv->project, 
				 "resource_removed", 
				 G_CALLBACK (mam_resource_removed_cb), 
				 model, 0);

        return model;
}
