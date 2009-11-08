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
#include "planner-group-model.h"

struct _PlannerGroupModelPriv {
	MrpProject *project;
};

#define G_LIST(x) ((GList *) x)

static void     mgm_init                     (PlannerGroupModel        *model);
static void     mgm_class_init               (PlannerGroupModelClass   *klass);
static void     mgm_finalize                 (GObject             *object);

static gint     mgm_get_n_columns            (GtkTreeModel        *tree_model);
static GType    mgm_get_column_type          (GtkTreeModel        *tree_model,
					      gint                 index);

static void     mgm_get_value                (GtkTreeModel        *tree_model,
					      GtkTreeIter         *iter,
					      gint                 column,
					      GValue              *value);
static void     mgm_group_notify_cb          (MrpGroup            *group,
					      GParamSpec          *pspec,
					      PlannerGroupModel        *model);
static void     mgm_group_added_cb           (MrpProject          *project,
					      MrpGroup            *resource,
					      PlannerGroupModel        *model);

static void     mgm_group_removed_cb         (MrpProject          *project,
					      MrpGroup            *resource,
					      PlannerGroupModel        *model);

static void     mgm_default_group_changed_cb (MrpProject          *project,
					      MrpGroup            *group,
					      PlannerGroupModel        *model);

static PlannerListModelClass *parent_class = NULL;


GType
planner_group_model_get_type (void)
{
        static GType rm_type = 0;

        if (!rm_type) {
                static const GTypeInfo rm_info =
                        {
                                sizeof (PlannerGroupModelClass),
                                NULL,		/* base_init */
                                NULL,		/* base_finalize */
                                (GClassInitFunc) mgm_class_init,
                                NULL,		/* class_finalize */
                                NULL,		/* class_data */
                                sizeof (PlannerGroupModel),
                                0,
                                (GInstanceInitFunc) mgm_init,
                        };

                rm_type = g_type_register_static (PLANNER_TYPE_LIST_MODEL,
                                                  "PlannerGroupModel",
                                                  &rm_info, 0);
        }

        return rm_type;
}

static void
mgm_class_init (PlannerGroupModelClass *klass)
{
        GObjectClass     *object_class;
	PlannerListModelClass *lm_class;

        parent_class = g_type_class_peek_parent (klass);
        object_class = G_OBJECT_CLASS (klass);
	lm_class     = PLANNER_LIST_MODEL_CLASS (klass);

        object_class->finalize = mgm_finalize;

	lm_class->get_n_columns   = mgm_get_n_columns;
	lm_class->get_column_type = mgm_get_column_type;
	lm_class->get_value       = mgm_get_value;
}

static void
mgm_init (PlannerGroupModel *model)
{
        PlannerGroupModelPriv *priv;

        priv = g_new0 (PlannerGroupModelPriv, 1);

	priv->project = NULL;

        model->priv = priv;
}


static void
mgm_finalize (GObject *object)
{
	PlannerGroupModel *model = PLANNER_GROUP_MODEL (object);
	GList             *l, *groups;

	g_return_if_fail (model->priv != NULL);
	g_return_if_fail (MRP_IS_PROJECT (model->priv->project));

	groups = planner_list_model_get_data (PLANNER_LIST_MODEL (model));
	for (l = groups; l; l = l->next) {
		g_signal_handlers_disconnect_by_func (MRP_GROUP (l->data),
						      mgm_group_notify_cb,
						      model);
	}
	g_signal_handlers_disconnect_by_func (model->priv->project,
					      mgm_group_added_cb,
					      model);
	g_signal_handlers_disconnect_by_func (model->priv->project,
					      mgm_group_removed_cb,
					      model);
	g_signal_handlers_disconnect_by_func (model->priv->project,
					      mgm_default_group_changed_cb,
					      model);

	g_object_unref (model->priv->project);
	g_free (model->priv);
	model->priv = NULL;

        if (G_OBJECT_CLASS (parent_class)->finalize) {
                (* G_OBJECT_CLASS (parent_class)->finalize) (object);
        }
}

static gint
mgm_get_n_columns (GtkTreeModel *tree_model)
{
        return NUMBER_OF_GROUP_COLS;
}

static GType
mgm_get_column_type (GtkTreeModel *tree_model,
		      gint          column)
{
        switch (column) {
        case GROUP_COL_NAME:
        case GROUP_COL_MANAGER_NAME:
        case GROUP_COL_MANAGER_PHONE:
        case GROUP_COL_MANAGER_EMAIL:
                return G_TYPE_STRING;

        case GROUP_COL_GROUP_DEFAULT:
                return G_TYPE_BOOLEAN;

	case GROUP_COL:
		return MRP_TYPE_GROUP;

	default:
		return G_TYPE_INVALID;
        }
}

static void
mgm_get_value (GtkTreeModel *tree_model,
		GtkTreeIter  *iter,
		gint          column,
		GValue       *value)
{
        gchar                 *str = NULL;
	MrpGroup              *group, *default_group;
	PlannerGroupModelPriv *priv;
	gboolean               is_default;

        g_return_if_fail (PLANNER_IS_GROUP_MODEL (tree_model));
        g_return_if_fail (iter != NULL);

	priv = PLANNER_GROUP_MODEL (tree_model)->priv;
        group = MRP_GROUP (planner_list_model_get_object (
				   PLANNER_LIST_MODEL (tree_model), iter));

        switch (column) {
        case GROUP_COL_NAME:
                mrp_object_get (group, "name", &str, NULL);
		g_value_init (value, G_TYPE_STRING);
		g_value_set_string (value, str);
		g_free (str);
                break;

	case GROUP_COL_GROUP_DEFAULT:
		g_object_get (priv->project,
			      "default-group", &default_group,
			      NULL);

		is_default = (group == default_group);

                g_value_init (value, G_TYPE_BOOLEAN);
                g_value_set_boolean (value, is_default);

                break;

        case GROUP_COL_MANAGER_NAME:
                mrp_object_get (group, "manager_name", &str, NULL);
                g_value_init (value, G_TYPE_STRING);
		g_value_set_string (value, str);
		g_free (str);

                break;

	case GROUP_COL_MANAGER_PHONE:
                mrp_object_get (group, "manager_phone", &str, NULL);
                g_value_init (value, G_TYPE_STRING);
		g_value_set_string (value, str);
		g_free (str);

                break;

	case GROUP_COL_MANAGER_EMAIL:
                mrp_object_get (group, "manager_email", &str, NULL);
                g_value_init (value, G_TYPE_STRING);
		g_value_set_string (value, str);
		g_free (str);

                break;

	case GROUP_COL:
                g_value_init (value, MRP_TYPE_GROUP);
		g_value_set_object (value, group);

                break;

	default:
                g_assert_not_reached ();
        }
}

static void
mgm_group_notify_cb (MrpGroup *group, GParamSpec *pspec, PlannerGroupModel *model)
{
	GtkTreeModel *tree_model;
	GtkTreePath  *path;
	GtkTreeIter   iter;

	g_return_if_fail (PLANNER_IS_GROUP_MODEL (model));
	g_return_if_fail (MRP_IS_GROUP (group));

	tree_model = GTK_TREE_MODEL (model);

	path = planner_list_model_get_path (PLANNER_LIST_MODEL (model),
				       MRP_OBJECT (group));

	if (path) {
		gtk_tree_model_get_iter (tree_model, &iter, path);
		gtk_tree_model_row_changed (GTK_TREE_MODEL (model),
					    path, &iter);

		gtk_tree_path_free (path);
	}
}

static void
mgm_group_added_cb (MrpProject   *project,
		    MrpGroup     *group,
		    PlannerGroupModel *model)
{
	g_return_if_fail (PLANNER_IS_GROUP_MODEL (model));
	g_return_if_fail (MRP_IS_GROUP (group));

	planner_list_model_append (PLANNER_LIST_MODEL (model), MRP_OBJECT (group));

	g_signal_connect (group, "notify",
			  G_CALLBACK (mgm_group_notify_cb),
			  model);
}

static void
mgm_group_removed_cb (MrpProject   *project,
		      MrpGroup     *group,
		      PlannerGroupModel *model)
{
	g_return_if_fail (PLANNER_IS_GROUP_MODEL (model));
	g_return_if_fail (MRP_IS_GROUP (group));

	g_signal_handlers_disconnect_by_func (group,
					      mgm_group_notify_cb,
					      model);

	planner_list_model_remove (PLANNER_LIST_MODEL (model), MRP_OBJECT (group));
}

static void
mgm_default_group_changed_cb (MrpProject   *project,
			      MrpGroup     *group,
			      PlannerGroupModel *model)
{
	GtkTreePath *path;
	GtkTreeIter  iter;
	gint         i;
	GList       *groups;

	g_return_if_fail (PLANNER_IS_GROUP_MODEL (model));

	if (group == NULL)
		return;

	groups = planner_list_model_get_data (PLANNER_LIST_MODEL (model));

	i = g_list_index (groups, group);

	path = gtk_tree_path_new ();
	gtk_tree_path_append_index (path, i);

	gtk_tree_model_get_iter (GTK_TREE_MODEL (model), &iter, path);

	gtk_tree_model_row_changed (GTK_TREE_MODEL (model), path, &iter);

	gtk_tree_path_free (path);
}

PlannerGroupModel *
planner_group_model_new (MrpProject *project)
{
        PlannerGroupModel     *model;
        PlannerGroupModelPriv *priv;
	GList                 *groups;

        model = g_object_new (PLANNER_TYPE_GROUP_MODEL, NULL);

        priv = model->priv;

        groups = mrp_project_get_groups (project);
	planner_list_model_set_data (PLANNER_LIST_MODEL (model), groups);

	priv->project = g_object_ref (project);

	g_signal_connect_object (project,
				 "group_added",
				 G_CALLBACK (mgm_group_added_cb),
				 model, 0);

	g_signal_connect_object (project,
				 "group_removed",
				 G_CALLBACK (mgm_group_removed_cb),
				 model, 0);

	g_signal_connect_object (project,
				 "default_group_changed",
				 G_CALLBACK (mgm_default_group_changed_cb),
				 model, 0);

        return model;
}
