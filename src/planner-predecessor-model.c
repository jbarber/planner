/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2002 CodeFactory AB
 * Copyright (C) 2002 Richard Hult <richard@imendio.com>
 * Copyright (C) 2002 Mikael Hallendal <micke@imendio.com>
 * Copyright (C) 2002 Alvaro del Castillo <acs@barrapunto.com>
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
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include "planner-predecessor-model.h"

struct _PlannerPredecessorModelPriv {
	MrpProject *project;
	MrpTask    *task;
};

#define G_LIST(x) ((GList *) x)

static void       mpm_init                   (PlannerPredecessorModel      *model);
static void       mpm_class_init             (PlannerPredecessorModelClass *class);

static void       mpm_finalize               (GObject              *object);
static gint       mpm_get_n_columns          (GtkTreeModel         *treemodel);
static GType      mpm_get_column_type        (GtkTreeModel         *treemodel,
					      gint                  index);

static void       mpm_get_value              (GtkTreeModel         *treemodel,
					      GtkTreeIter          *iter,
					      gint                  column,
					      GValue               *value);

static void       mpm_predecessor_changed_cb (MrpRelation          *relation,
					      GParamSpec           *pspec,
					      PlannerPredecessorModel   *model);

static PlannerListModelClass *parent_class = NULL;


GType
planner_predecessor_model_get_type (void)
{
        static GType type = 0;

        if (!type) {
                static const GTypeInfo info = {
                                sizeof (PlannerPredecessorModelClass),
                                NULL,		/* base_init */
                                NULL,		/* base_finalize */
                                (GClassInitFunc) mpm_class_init,
                                NULL,		/* class_finalize */
                                NULL,		/* class_data */
                                sizeof (PlannerPredecessorModel),
                                0,
                                (GInstanceInitFunc) mpm_init,
                        };

                type = g_type_register_static (PLANNER_TYPE_LIST_MODEL,
					       "PlannerPredecessorModel",
					       &info, 0);
        }

        return type;
}

static void
mpm_class_init (PlannerPredecessorModelClass *klass)
{
        GObjectClass     *object_class;
	PlannerListModelClass *lm_class;

        parent_class = g_type_class_peek_parent (klass);
        object_class = G_OBJECT_CLASS (klass);
	lm_class     = PLANNER_LIST_MODEL_CLASS (klass);

        object_class->finalize = mpm_finalize;

        lm_class->get_n_columns   = mpm_get_n_columns;
        lm_class->get_column_type = mpm_get_column_type;
        lm_class->get_value       = mpm_get_value;
}

static void
mpm_init (PlannerPredecessorModel *model)
{
        PlannerPredecessorModelPriv *priv;

        priv = g_new0 (PlannerPredecessorModelPriv, 1);

	priv->project = NULL;

        model->priv = priv;
}

static void
mpm_finalize (GObject *object)
{
	PlannerPredecessorModel *model = PLANNER_PREDECESSOR_MODEL (object);

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
mpm_get_n_columns (GtkTreeModel *tree_model)
{
        return NUMBER_OF_PREDECESSOR_COLS;
}

static GType
mpm_get_column_type (GtkTreeModel *tree_model,
                     gint          column)
{
        switch (column) {
        case PREDECESSOR_COL_NAME:
                return G_TYPE_STRING;
        case PREDECESSOR_COL_TYPE:
                return G_TYPE_INT;
        case PREDECESSOR_COL_LAG:
                return G_TYPE_INT;
        default:
		return G_TYPE_INVALID;
        }
}

static void
mpm_get_value (GtkTreeModel *tree_model,
               GtkTreeIter  *iter,
               gint          column,
               GValue       *value)
{
	PlannerPredecessorModel     *model;
	MrpTask                *task;
	MrpRelation            *relation;
	PlannerPredecessorModelPriv *priv;
	gchar                  *str;

        g_return_if_fail (PLANNER_IS_PREDECESSOR_MODEL (tree_model));
        g_return_if_fail (iter != NULL);

	model = PLANNER_PREDECESSOR_MODEL (tree_model);
	priv = model->priv;
	task = G_LIST (iter->user_data)->data;
	relation = mrp_task_get_relation (priv->task, task);

        switch (column) {
        case PREDECESSOR_COL_NAME:
                g_object_get (task, "name", &str, NULL);
		if (str == NULL) {
			str = g_strdup ("");
		}
                g_value_init (value, G_TYPE_STRING);
                g_value_set_string (value, str);
                g_free (str);
                break;

	case PREDECESSOR_COL_TYPE:
                g_value_init (value, G_TYPE_STRING);
		/* i18n: finish-to-start, not sure if it should be translated... */
		switch (mrp_relation_get_relation_type (relation)) {
		case MRP_RELATION_FS:
			g_value_set_string (value, _("FS"));
			break;
		case MRP_RELATION_FF:
			g_value_set_string (value, _("FF"));
			break;
		case MRP_RELATION_SS:
			g_value_set_string (value, _("SS"));
			break;
		case MRP_RELATION_SF:
			g_value_set_string (value, _("SF"));
			break;
		default:
			g_warning("Unknown relation type %d",
				  mrp_relation_get_relation_type (relation));
		}
		break;

	case PREDECESSOR_COL_LAG:
                g_value_init (value, G_TYPE_INT);
                g_value_set_int (value, mrp_relation_get_lag (relation));
                break;

        default:
                g_warning ("Bad column %d requested", column);
        }
}

static void
mpm_predecessor_notify_cb (MrpTask            *task,
			   GParamSpec         *pspec,
			   PlannerPredecessorModel *model)
{
	g_return_if_fail (PLANNER_IS_PREDECESSOR_MODEL (model));
	g_return_if_fail (MRP_IS_TASK (task));

	planner_list_model_update (PLANNER_LIST_MODEL (model), MRP_OBJECT (task));
}

static void
mpm_relation_added_cb (MrpProject         *project,
		       MrpRelation        *relation,
		       PlannerPredecessorModel *model)
{
	MrpTask *predecessor;

	g_return_if_fail (PLANNER_IS_PREDECESSOR_MODEL (model));

	predecessor = mrp_relation_get_predecessor (relation);

	/* We're not interested in the case where our task is the
	 * predecessor.
	 */
	if (model->priv->task == predecessor) {
		return;
	}

	planner_list_model_append (PLANNER_LIST_MODEL (model),
			      MRP_OBJECT (predecessor));

	g_signal_connect_object (predecessor,
				 "notify",
				 G_CALLBACK (mpm_predecessor_notify_cb),
				 model, 0);
}

static void
mpm_relation_removed_cb (MrpProject         *project,
			 MrpRelation        *relation,
			 PlannerPredecessorModel *model)
{
	MrpTask *predecessor;

	g_return_if_fail (PLANNER_IS_PREDECESSOR_MODEL (model));

	/* We're not interested in the case where our task is the
	 * predecessor.
	 */
	predecessor = mrp_relation_get_predecessor (relation);

	if (model->priv->task == predecessor) {
		return;
	}

	planner_list_model_remove (PLANNER_LIST_MODEL (model),
			      MRP_OBJECT (predecessor));

	g_signal_handlers_disconnect_by_func (predecessor,
					      mpm_predecessor_notify_cb,
					      model);
}

static void
mpm_predecessor_changed_cb (MrpRelation *relation, GParamSpec *pspec,
			    PlannerPredecessorModel *model)
{
	MrpTask *predecessor;

	predecessor = mrp_relation_get_predecessor (relation);

	planner_list_model_update (PLANNER_LIST_MODEL (model), MRP_OBJECT (predecessor));
}

static void
mpm_connect_to_relation (MrpRelation *relation, PlannerPredecessorModel *model)
{
	g_signal_connect_object (relation, "notify::predecessor",
				 G_CALLBACK (mpm_predecessor_changed_cb),
				 model,
				 0);
	g_signal_connect_object (relation, "notify::lag",
				 G_CALLBACK (mpm_predecessor_changed_cb),
				 model,
				 0);
	g_signal_connect_object (relation, "notify::type",
				 G_CALLBACK (mpm_predecessor_changed_cb),
				 model,
				 0);
}

GtkTreeModel *
planner_predecessor_model_new (MrpTask *task)
{
        PlannerPredecessorModel     *model;
        PlannerPredecessorModelPriv *priv;
	GList                  *list, *l;
	GList                  *tasks;
	MrpTask                *predecessor;

        model = g_object_new (PLANNER_TYPE_PREDECESSOR_MODEL, NULL);
        priv = model->priv;

        priv->task = g_object_ref (task);

	g_object_get (priv->task, "project", &priv->project, NULL);

 	list = mrp_task_get_predecessor_relations (task);
	tasks = NULL;

	for (l = list; l; l = l->next) {
		MrpRelation *relation = l->data;

		predecessor = mrp_relation_get_predecessor (relation);

		tasks = g_list_prepend (tasks, predecessor);

		g_signal_connect_object (predecessor,
					 "notify",
					 G_CALLBACK (mpm_predecessor_notify_cb),
					 model, 0);

		mpm_connect_to_relation (relation, model);
	}

	tasks = g_list_reverse (tasks);

	planner_list_model_set_data (PLANNER_LIST_MODEL (model), tasks);
	g_list_free (tasks);

	g_signal_connect_object (priv->task,
				 "relation-added",
				 G_CALLBACK (mpm_relation_added_cb),
				 model, 0);

	g_signal_connect_object (priv->task,
				 "relation-removed",
				 G_CALLBACK (mpm_relation_removed_cb),
				 model, 0);

        return GTK_TREE_MODEL (model);
}
