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
#include <string.h>

#include "planner-list-model.h"

struct _PlannerListModelPriv {
	GList       *data_list;
        gint         stamp;
	gulong       empty_notify_handler;
};

#define G_LIST(x) ((GList *) x)

static void          mlm_init                 (PlannerListModel          *model);
static void          mlm_class_init           (PlannerListModelClass     *klass);
static void          mlm_tree_model_init      (GtkTreeModelIface    *iface);

static void          mlm_finalize             (GObject              *object);
static gboolean      mlm_get_iter             (GtkTreeModel         *treemodel,
					       GtkTreeIter          *iter,
					       GtkTreePath          *path);
static GtkTreePath * mlm_get_path             (GtkTreeModel         *treemodel,
					       GtkTreeIter          *iter);
static gboolean      mlm_iter_next            (GtkTreeModel         *treemodel,
					       GtkTreeIter          *iter);
static gboolean      mlm_iter_children        (GtkTreeModel         *treemodel,
					       GtkTreeIter          *iter,
					       GtkTreeIter          *parent);
static gboolean      mlm_iter_has_child       (GtkTreeModel         *treemodel,
					       GtkTreeIter          *iter);
static gint          mlm_iter_n_children      (GtkTreeModel         *treemodel,
					       GtkTreeIter          *iter);
static gboolean      mlm_iter_nth_child       (GtkTreeModel         *treemodel,
					       GtkTreeIter          *iter,
					       GtkTreeIter          *parent,
					       gint                  n);
static gboolean      mlm_iter_parent          (GtkTreeModel         *treemodel,
					       GtkTreeIter          *iter,
					       GtkTreeIter          *child);
static gint          mlm_get_n_columns        (GtkTreeModel         *tree_model);
static GType         mlm_get_column_type      (GtkTreeModel         *tree_model,
					       gint                  column);

static void          mlm_get_value            (GtkTreeModel         *tree_model,
					       GtkTreeIter          *iter,
					       gint                  column,
					       GValue               *value);


static GObjectClass *parent_class = NULL;


GType
planner_list_model_get_type (void)
{
        static GType type = 0;

        if (!type) {
                static const GTypeInfo info =
                        {
                                sizeof (PlannerListModelClass),
                                NULL,		/* base_init */
                                NULL,		/* base_finalize */
                                (GClassInitFunc) mlm_class_init,
                                NULL,		/* class_finalize */
                                NULL,		/* class_data */
                                sizeof (PlannerListModel),
                                0,
                                (GInstanceInitFunc) mlm_init,
                        };

                static const GInterfaceInfo tree_model_info =
                        {
                                (GInterfaceInitFunc) mlm_tree_model_init,
                                NULL,
                                NULL
                        };

                type = g_type_register_static (G_TYPE_OBJECT,
					       "PlannerListModel",
					       &info, 0);

                g_type_add_interface_static (type,
                                             GTK_TYPE_TREE_MODEL,
                                             &tree_model_info);
        }

        return type;
}

static void
mlm_class_init (PlannerListModelClass *klass)
{
        GObjectClass *object_class;

        parent_class = g_type_class_peek_parent (klass);
        object_class = (GObjectClass*) klass;

        object_class->finalize = mlm_finalize;

	klass->get_n_columns   = NULL;
	klass->get_column_type = NULL;
	klass->get_value       = NULL;
}

static void
mlm_tree_model_init (GtkTreeModelIface *iface)
{
        iface->get_iter        = mlm_get_iter;
        iface->get_path        = mlm_get_path;
        iface->iter_next       = mlm_iter_next;
        iface->iter_children   = mlm_iter_children;
        iface->iter_has_child  = mlm_iter_has_child;
        iface->iter_n_children = mlm_iter_n_children;
        iface->iter_nth_child  = mlm_iter_nth_child;
        iface->iter_parent     = mlm_iter_parent;

	/* Calls the class->function so that subclasses can override this */
	iface->get_n_columns   = mlm_get_n_columns;
	iface->get_column_type = mlm_get_column_type;
	iface->get_value       = mlm_get_value;
}

static void
mlm_init (PlannerListModel *model)
{
        PlannerListModelPriv *priv;

        priv = g_new0 (PlannerListModelPriv, 1);

	do {
		priv->stamp = g_random_int ();
	} while (priv->stamp == 0);

        model->priv               = priv;
	priv->data_list           = NULL;
}

static void
mlm_finalize (GObject *object)
{
	PlannerListModel     *model = PLANNER_LIST_MODEL (object);
	PlannerListModelPriv *priv  = model->priv;

	if (priv->data_list) {
 		g_list_free (priv->data_list);
		priv->data_list = NULL;
	}

	g_free (model->priv);
	model->priv = NULL;

	if (G_OBJECT_CLASS (parent_class)->finalize) {
		(* G_OBJECT_CLASS (parent_class)->finalize) (object);
	}
}

static gboolean
mlm_get_iter (GtkTreeModel *tree_model,
              GtkTreeIter  *iter,
              GtkTreePath  *path)
{
        PlannerListModel     *model;
	PlannerListModelPriv *priv;
        GList           *node;
        gint             i;

        g_return_val_if_fail (PLANNER_IS_LIST_MODEL (tree_model), FALSE);
        g_return_val_if_fail (gtk_tree_path_get_depth (path) > 0, FALSE);

        model = PLANNER_LIST_MODEL (tree_model);
	priv  = model->priv;

        i = gtk_tree_path_get_indices (path)[0];

        if (i >= g_list_length (priv->data_list)) {
                return FALSE;
        }

        node = g_list_nth (priv->data_list, i);

        iter->stamp     = model->priv->stamp;
        iter->user_data = node;

        return TRUE;
}

static GtkTreePath *
mlm_get_path (GtkTreeModel *tree_model,
              GtkTreeIter  *iter)
{
        PlannerListModel     *model = PLANNER_LIST_MODEL (tree_model);
        PlannerListModelPriv *priv;
        GtkTreePath     *path;
        GList           *node;
        gint             i = 0;

        g_return_val_if_fail (PLANNER_IS_LIST_MODEL (tree_model), NULL);
        g_return_val_if_fail (model->priv->stamp == iter->stamp, NULL);

        priv = model->priv;

        for (node = priv->data_list; node; node = node->next)
        {
                if ((gpointer)node->data == (gpointer)iter->user_data)
                        break;
                i++;
        }

        if (node == NULL) {
                return NULL;
        }

        path = gtk_tree_path_new ();
        gtk_tree_path_append_index (path, i);

        return path;
}

static gboolean
mlm_iter_next (GtkTreeModel  *tree_model,
               GtkTreeIter   *iter)
{
	PlannerListModel *model = PLANNER_LIST_MODEL (tree_model);

        g_return_val_if_fail (PLANNER_IS_LIST_MODEL (tree_model), FALSE);
        g_return_val_if_fail (model->priv->stamp == iter->stamp, FALSE);

        iter->user_data = G_LIST(iter->user_data)->next;

        return (iter->user_data != NULL);
}

static gboolean
mlm_iter_children (GtkTreeModel *tree_model,
                   GtkTreeIter  *iter,
                   GtkTreeIter  *parent)
{
	PlannerListModel     *model;
        PlannerListModelPriv *priv;

        g_return_val_if_fail (PLANNER_IS_LIST_MODEL (tree_model), FALSE);

        model = PLANNER_LIST_MODEL (tree_model);
        priv  = model->priv;

        /* this is a list, nodes have no children */
        if (parent) {
                return FALSE;
        }

        /* but if parent == NULL we return the list itself as children of the
         * "root"
         */

        if (priv->data_list) {
                iter->stamp = priv->stamp;
                iter->user_data = priv->data_list;
                return TRUE;
        }

        return FALSE;
}

static gboolean
mlm_iter_has_child (GtkTreeModel *tree_model,
                    GtkTreeIter  *iter)
{
        return FALSE;
}

static gint
mlm_iter_n_children (GtkTreeModel *tree_model,
                     GtkTreeIter  *iter)
{
	PlannerListModel     *model;
        PlannerListModelPriv *priv;

        g_return_val_if_fail (PLANNER_IS_LIST_MODEL (tree_model), -1);

	model = PLANNER_LIST_MODEL (tree_model);
        priv  = model->priv;

        if (iter == NULL) {
                return g_list_length (priv->data_list);
        }

        g_return_val_if_fail (priv->stamp == iter->stamp, -1);

        return 0;
}

static gboolean
mlm_iter_nth_child (GtkTreeModel *tree_model,
                    GtkTreeIter  *iter,
                    GtkTreeIter  *parent,
                    gint          n)
{
        PlannerListModel     *model;
	PlannerListModelPriv *priv;
        GList           *child;

        g_return_val_if_fail (PLANNER_IS_LIST_MODEL (tree_model), FALSE);

        model = PLANNER_LIST_MODEL (tree_model);
	priv  = model->priv;

        if (parent) {
                return FALSE;
        }

        child = g_list_nth (priv->data_list, n);

        if (child) {
                iter->stamp     = model->priv->stamp;
                iter->user_data = child;
                return TRUE;
        }

        return FALSE;
}

static gboolean
mlm_iter_parent (GtkTreeModel *tree_model,
                 GtkTreeIter  *iter,
                 GtkTreeIter  *child)
{
        return FALSE;
}

static gint
mlm_get_n_columns (GtkTreeModel *tree_model)
{
	PlannerListModelClass *klass;

	klass = PLANNER_LIST_MODEL_GET_CLASS (tree_model);

	if (klass->get_n_columns) {
		return klass->get_n_columns (tree_model);
	}

	g_warning ("You have to override get_n_columns!");

	return -1;
}

static GType
mlm_get_column_type (GtkTreeModel *tree_model, gint column)
{
	PlannerListModelClass *klass;

	klass = PLANNER_LIST_MODEL_GET_CLASS (tree_model);

	if (klass->get_column_type) {
		return klass->get_column_type (tree_model, column);
	}

	g_warning ("You have to override get_column_type!");

	return G_TYPE_INVALID;
}


static void
mlm_get_value (GtkTreeModel *tree_model,
	       GtkTreeIter  *iter,
	       gint          column,
	       GValue       *value)
{
	PlannerListModelClass *klass;

	klass = PLANNER_LIST_MODEL_GET_CLASS (tree_model);

	if (klass->get_value) {
		klass->get_value (tree_model, iter, column, value);
	}
}

void
planner_list_model_append (PlannerListModel *model, MrpObject *object)
{
	PlannerListModelPriv *priv;
	GtkTreePath     *path;
	GtkTreeIter      iter;
	gint             i;

	g_return_if_fail (PLANNER_IS_LIST_MODEL (model));
	g_return_if_fail (MRP_IS_OBJECT (object));

	priv = model->priv;

 	priv->data_list = g_list_append (priv->data_list,
					 g_object_ref (object));

	i = g_list_index (priv->data_list, object);

	path = gtk_tree_path_new ();
	gtk_tree_path_append_index (path, i);

	gtk_tree_model_get_iter (GTK_TREE_MODEL (model), &iter, path);

	gtk_tree_model_row_inserted (GTK_TREE_MODEL (model), path, &iter);

	gtk_tree_path_free (path);
}

void
planner_list_model_remove (PlannerListModel *model, MrpObject *object)
{
	PlannerListModelPriv *priv;
        GtkTreePath     *path;
        gint             i;

	g_return_if_fail (PLANNER_IS_LIST_MODEL (model));
	g_return_if_fail (MRP_IS_OBJECT (object));

	priv = model->priv;

	i = g_list_index (priv->data_list, object);

	priv->data_list = g_list_remove (priv->data_list, object);

	g_object_unref (object);

	path = gtk_tree_path_new ();
	gtk_tree_path_append_index (path, i);

	gtk_tree_model_row_deleted (GTK_TREE_MODEL (model), path);

	gtk_tree_path_free (path);
}

void
planner_list_model_update (PlannerListModel *model, MrpObject *object)
{
	PlannerListModelPriv *priv;
        GtkTreePath     *path;
	GtkTreeIter      iter;
        gint             i;

	g_return_if_fail (PLANNER_IS_LIST_MODEL (model));
	g_return_if_fail (MRP_IS_OBJECT (object));

	priv = model->priv;

	i = g_list_index (priv->data_list, object);

	path = gtk_tree_path_new ();
	gtk_tree_path_append_index (path, i);

	gtk_tree_model_get_iter (GTK_TREE_MODEL (model), &iter, path);

	gtk_tree_model_row_changed (GTK_TREE_MODEL (model), path, &iter);

	gtk_tree_path_free (path);
}

GtkTreePath *
planner_list_model_get_path (PlannerListModel *model, MrpObject *object)
{
	PlannerListModelPriv *priv;
	GtkTreePath     *path;
	gint             index = -1;

	g_return_val_if_fail (PLANNER_IS_LIST_MODEL (model), NULL);
	g_return_val_if_fail (MRP_IS_OBJECT (object), NULL);

	priv = model->priv;

	index = g_list_index (priv->data_list, object);

	if (index >= 0) {
		path = gtk_tree_path_new ();
		gtk_tree_path_append_index (path, index);
		return path;
	}

	return NULL;
}

MrpObject *
planner_list_model_get_object (PlannerListModel *model, GtkTreeIter *iter)
{
	return MRP_OBJECT (((GList *) iter->user_data)->data);
}

void
planner_list_model_set_data (PlannerListModel *model, GList *data)
{
	PlannerListModelPriv *priv;
	GList           *old_list;
	GList           *node;

	g_return_if_fail (PLANNER_IS_LIST_MODEL (model));

	priv = model->priv;

	/* Remove the old entries */

	if (priv->data_list) {
		old_list = g_list_copy (priv->data_list);

		for (node = old_list; node; node = node->next) {
			planner_list_model_remove (model, MRP_OBJECT (node->data));
		}

		g_list_free (old_list);
	}

	/* Add the new ones */

	for (node = data; node; node = node->next) {
		planner_list_model_append (model, MRP_OBJECT (node->data));
	}
}

GList *
planner_list_model_get_data (PlannerListModel *model)
{
	g_return_val_if_fail (PLANNER_IS_LIST_MODEL (model), NULL);

	return model->priv->data_list;
}
