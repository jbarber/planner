/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2001-2003 CodeFactory AB
 * Copyright (C) 2001-2003 Richard Hult <richard@imendio.com>
 * Copyright (C) 2001-2002 Mikael Hallendal <micke@imendio.com>
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
#include <string.h>
#include <time.h>
#include <glib.h>
#include <gtk/gtktreemodel.h>
#include <gtk/gtktreednd.h>
#include <libgnome/gnome-i18n.h>
#include <libplanner/mrp-task.h>
#include "planner-marshal.h"
#include "planner-gantt-model.h"

enum {
	TASK_ADDED,
	TASK_REMOVED,
	LAST_SIGNAL
};

struct _MgGanttModelPriv {
	MrpProject *project;
	GHashTable *task2node_hash; /* Task -> Node mapping. */
	GNode      *tree;
};

static void      gantt_model_init             (MgGanttModel           *model);
static void      gantt_model_class_init       (MgGanttModelClass      *class);
static void      gantt_model_tree_model_init  (GtkTreeModelIface      *iface);
#ifdef DND
static void      gantt_model_drag_source_init (GtkTreeDragSourceIface *iface);
static void      gantt_model_drag_dest_init   (GtkTreeDragDestIface   *iface);
#endif
static gboolean  gantt_model_get_iter         (GtkTreeModel           *model,
					       GtkTreeIter            *iter,
					       GtkTreePath            *path);
static void      gantt_model_task_notify_cb   (MrpTask                *task,
					       GParamSpec             *pspec,
					       MgGanttModel           *model);
static GtkTreePath *
gantt_model_get_path_from_node                (MgGanttModel           *model,
					       GNode                  *node);
static void      dump_tree                    (GNode                  *node);


static GObjectClass *parent_class;
static guint signals[LAST_SIGNAL];

GType
planner_gantt_model_get_type (void)
{
	static GType type = 0;

	if (!type) {
		static const GTypeInfo info = {
			sizeof (MgGanttModelClass),
			NULL,		/* base_init */
			NULL,		/* base_finalize */
			(GClassInitFunc) gantt_model_class_init,
			NULL,		/* class_finalize */
			NULL,		/* class_data */
			sizeof (MgGanttModel),
			0,
			(GInstanceInitFunc) gantt_model_init
		};

		static const GInterfaceInfo tree_model_info = {
			(GInterfaceInitFunc) gantt_model_tree_model_init,
			NULL,
			NULL
		};

#ifdef DND
		static const GInterfaceInfo drag_source_info = {
			(GInterfaceInitFunc) gantt_model_drag_source_init,
			NULL,
			NULL
		};

		static const GInterfaceInfo drag_dest_info = {
			(GInterfaceInitFunc) gantt_model_drag_dest_init,
			NULL,
			NULL
		};
#endif    
		type = g_type_register_static (G_TYPE_OBJECT,
					       "MgGanttModel",
					       &info, 0);
		
		g_type_add_interface_static (type,
					     GTK_TYPE_TREE_MODEL,
					     &tree_model_info);

#ifdef DND
		g_type_add_interface_static (type,
					     GTK_TYPE_TREE_DRAG_SOURCE,
					     &drag_source_info);

		g_type_add_interface_static (type,
					     GTK_TYPE_TREE_DRAG_DEST,
					     &drag_dest_info);
#endif
   	}

	return type;
}

static void
gantt_model_connect_to_task_signals (MgGanttModel *model, MrpTask *task)
{
	g_signal_connect_object (task,
				 "notify",
				 G_CALLBACK (gantt_model_task_notify_cb),
				 model,
				 0);
}

static void
gantt_model_task_inserted_cb (MrpProject   *project,
			      MrpTask      *task,
			      MgGanttModel *model)
{
	GtkTreePath *path;
	GtkTreePath *parent_path;
	GtkTreeIter  iter;
	GNode       *node;
	GNode       *parent_node;
	MrpTask     *parent;
	gint         pos;
	gboolean     has_child_toggled;

	node = g_node_new (task);

	g_hash_table_insert (model->priv->task2node_hash, task, node);

	parent = mrp_task_get_parent (task);
	pos = mrp_task_get_position (task);

	parent_node = g_hash_table_lookup (model->priv->task2node_hash, parent);

	has_child_toggled = (g_node_n_children (parent_node) == 0);

	g_node_insert (parent_node, pos, node);
	
	if (has_child_toggled && parent_node->parent != NULL) {
		parent_path = gantt_model_get_path_from_node (model, parent_node);
		gantt_model_get_iter (GTK_TREE_MODEL (model), &iter, parent_path);
		gtk_tree_model_row_has_child_toggled (GTK_TREE_MODEL (model),
						      parent_path,
						      &iter);
		gtk_tree_path_free (parent_path);
	}

	path = gantt_model_get_path_from_node (model, node);
	gantt_model_get_iter (GTK_TREE_MODEL (model), &iter, path);
	gtk_tree_model_row_inserted (GTK_TREE_MODEL (model), path, &iter);

	gtk_tree_path_free (path);
	
	gantt_model_connect_to_task_signals (model, task);

	/* Sanity check. */
	if (g_node_n_nodes (model->priv->tree, G_TRAVERSE_ALL)
	    != g_hash_table_size (model->priv->task2node_hash)) {
		g_warning ("Gantt model corrupt.");
	}

	g_signal_emit (model, signals[TASK_ADDED], 0, task);
}

static gboolean
traverse_remove_subtree (GNode        *node,
			 MgGanttModel *model)
{
	g_signal_handlers_disconnect_by_func (node->data,
					      gantt_model_task_notify_cb,
					      model);
	
	g_hash_table_remove (model->priv->task2node_hash, node->data);

	return FALSE;
}

static void
gantt_model_remove_subtree (MgGanttModel *model,
			    GNode        *node)
{	
	g_node_unlink (node);

	g_node_traverse (node,
			 G_POST_ORDER,
			 G_TRAVERSE_ALL,
			 -1,
			 (GNodeTraverseFunc) traverse_remove_subtree,
			 model);

	g_node_destroy (node);
}

static void
gantt_model_task_removed_cb (MrpProject   *project,
			     MrpTask      *task,
			     MgGanttModel *model)
{
	GNode       *node;
	GNode       *parent_node;
	GtkTreePath *path;
	GtkTreePath *parent_path;
	GtkTreeIter  iter;
	gboolean     has_child_toggled;

	g_signal_emit (model, signals[TASK_REMOVED], 0, task);
	
	node = g_hash_table_lookup (model->priv->task2node_hash, task);
	if (!node) {
		/* This happens if we are already removed when a parent was
		 * removed. 
		 */
		return; 
	}
	
	g_signal_handlers_disconnect_by_func (task,
					      gantt_model_task_notify_cb,
					      model);
	
	parent_node = node->parent;
	
	path = gantt_model_get_path_from_node (model, node);
	gtk_tree_model_get_iter (GTK_TREE_MODEL (model), &iter, path);

	has_child_toggled = (g_node_n_children (parent_node) == 1);

	gantt_model_remove_subtree (model, node);
	
	gtk_tree_model_row_deleted (GTK_TREE_MODEL (model), path);

	if (has_child_toggled && parent_node->parent != NULL) {
		parent_path = gantt_model_get_path_from_node (model, parent_node);
		gantt_model_get_iter (GTK_TREE_MODEL (model), &iter, parent_path);
		gtk_tree_model_row_has_child_toggled (GTK_TREE_MODEL (model),
						      parent_path,
						      &iter);
		gtk_tree_path_free (parent_path);
	}
	
	gtk_tree_path_free (path);

	/* Sanity check. */
	if (g_node_n_nodes (model->priv->tree, G_TRAVERSE_ALL)
	    != g_hash_table_size (model->priv->task2node_hash)) {
		g_warning ("Gantt model corrupt.");
	}
	
}

static void
gantt_model_reattach_subtasks (GtkTreeModel *tree_model,
			       MrpTask      *task)
{
	MgGanttModel     *model;
	MgGanttModelPriv *priv;
	MrpTask          *child;
	GNode            *node;
	GNode            *parent_node;
	GtkTreePath      *path;
	GtkTreeIter       iter;
	gint              pos;

	model = MG_GANTT_MODEL (tree_model);
	priv = model->priv;
	
	parent_node = g_hash_table_lookup (priv->task2node_hash, task);

	/* Traverse the subtasks. */
	child = mrp_task_get_first_child (task);
	while (child) {
		node = g_hash_table_lookup (priv->task2node_hash, child);
		pos = mrp_task_get_position (child);
		g_node_insert (parent_node, pos, node);
		
#if 1
		{
			gboolean has_child_toggled;

			has_child_toggled = (g_node_n_children (parent_node) == 1);
			
			/* Emit has_child_toggled if necessary. */
			if (has_child_toggled) {
				GtkTreePath *parent_path;
				
				parent_path = gantt_model_get_path_from_node (model, parent_node);
				gantt_model_get_iter (GTK_TREE_MODEL (model), &iter, parent_path);
				gtk_tree_model_row_has_child_toggled (GTK_TREE_MODEL (model),
								      parent_path,
								      &iter);
				gtk_tree_path_free (parent_path);
			}
		}
#endif
		
		path = planner_gantt_model_get_path_from_task (model, child);
		gtk_tree_model_get_iter (tree_model, &iter, path);
		gtk_tree_model_row_inserted (tree_model, path, &iter);

		gtk_tree_path_free (path);

		gantt_model_reattach_subtasks (tree_model, child);

		child = mrp_task_get_next_sibling (child);
	}
}

static gboolean
gantt_model_unlink_subtree_cb (GNode *node, gpointer data)
{
	g_node_unlink (node);
	
	return FALSE;
}

static void
gantt_model_unlink_subtree_recursively (GNode *node)
{
	/* Remove the tasks one by one using post order so we don't mess with
	 * the tree while traversing it.
	 */
	g_node_traverse (node,
			 G_POST_ORDER,
			 G_TRAVERSE_ALL,
			 -1,
			 (GNodeTraverseFunc) gantt_model_unlink_subtree_cb,
			 NULL);
}

static void
gantt_model_task_moved_cb (MrpProject   *project,
			   MrpTask      *task,
			   MgGanttModel *model)
{
	MrpTask     *parent;
	GtkTreePath *path;
	GtkTreePath *parent_path;
	GtkTreeIter  iter;
	GNode       *node;
	GNode       *parent_node;
	gint         pos;
	gboolean     has_child_toggled;

	path = planner_gantt_model_get_path_from_task (model, task);
	gtk_tree_model_get_iter (GTK_TREE_MODEL (model), &iter, path);

	/* Notify views. */
	gtk_tree_model_row_deleted (GTK_TREE_MODEL (model), path);
	gtk_tree_path_free (path);

	node = g_hash_table_lookup (model->priv->task2node_hash, task);

	parent_node = node->parent;
	has_child_toggled = (g_node_n_children (parent_node) == 1);

	/* Unlink the subtree from the original position in the tree. */
	gantt_model_unlink_subtree_recursively (node);

	/* Emit has_child_toggled if necessary. */
	if (has_child_toggled) {
		parent_path = gantt_model_get_path_from_node (model, parent_node);
		gantt_model_get_iter (GTK_TREE_MODEL (model), &iter, parent_path);
		gtk_tree_model_row_has_child_toggled (GTK_TREE_MODEL (model),
						      parent_path,
						      &iter);
		gtk_tree_path_free (parent_path);
	}

	/* Get the new parent task and node. */
	parent = mrp_task_get_parent (task);
	parent_node = g_hash_table_lookup (model->priv->task2node_hash, parent);

	/* Re-insert the task at the new position. */
	pos = mrp_task_get_position (task);
	g_node_insert (parent_node, pos, node);

	has_child_toggled = (g_node_n_children (parent_node) == 1);

	/* Emit has_child_toggled if necessary. */
	if (has_child_toggled) {
		parent_path = gantt_model_get_path_from_node (model, parent_node);
		gantt_model_get_iter (GTK_TREE_MODEL (model), &iter, parent_path);
		gtk_tree_model_row_has_child_toggled (GTK_TREE_MODEL (model),
						      parent_path,
						      &iter);
		gtk_tree_path_free (parent_path);
	}

	/* Get the path for the new position and notify views. */
	path = planner_gantt_model_get_path_from_task (model, task);
	gtk_tree_model_get_iter (GTK_TREE_MODEL (model), &iter, path);

	gtk_tree_model_row_inserted (GTK_TREE_MODEL (model), path, &iter);

	gtk_tree_path_free (path);

	/* Handle any subtasks. */
	gantt_model_reattach_subtasks (GTK_TREE_MODEL (model), task);
}

static void
gantt_model_task_notify_cb (MrpTask      *task,
			    GParamSpec   *pspec,
			    MgGanttModel *model)
{
	GtkTreeModel *tree_model;
	GtkTreePath  *path;
	GtkTreeIter   iter;

	tree_model = GTK_TREE_MODEL (model);
	
	path = planner_gantt_model_get_path_from_task (model, task);
	gtk_tree_model_get_iter (tree_model, &iter, path);
	gtk_tree_model_row_changed (tree_model, path, &iter);

	gtk_tree_path_free (path);
}

static gboolean
traverse_insert_task_into_hash (GNode        *node,
				MgGanttModel *model)
{
	g_hash_table_insert (model->priv->task2node_hash,
			     node->data,
			     node);

	return FALSE;
}

static void
traverse_setup_tree (MrpTask  *task,
		     GNode    *node)
{
	MrpTask *child_task;
	GNode   *child_node;

	child_task = mrp_task_get_first_child (task);
	while (child_task) {
		child_node = g_node_new (child_task);
		g_node_append (node, child_node);

		traverse_setup_tree (child_task, child_node);
		
		child_task = mrp_task_get_next_sibling (child_task);
	}
}

static GNode *
gantt_model_setup_task_tree (MgGanttModel *model)
{
	MrpTask *root_task;
	GNode   *root_node;

	root_task = mrp_project_get_root_task (model->priv->project); 
	root_node = g_node_new (root_task);

	traverse_setup_tree (root_task, root_node);

	return root_node;
}

MgGanttModel *
planner_gantt_model_new (MrpProject *project)
{
	MgGanttModel     *model;
	MgGanttModelPriv *priv;
	GList            *tasks, *l;
	
	model = MG_GANTT_MODEL (g_object_new (MG_TYPE_GANTT_MODEL, NULL));
	priv = model->priv;

	priv->project = project;
	priv->tree = gantt_model_setup_task_tree (model);

	g_node_traverse (priv->tree,
			 G_PRE_ORDER,
			 G_TRAVERSE_ALL,
			 -1,
			 (GNodeTraverseFunc) traverse_insert_task_into_hash,
			 model);
	
	g_signal_connect_object (project,
				 "task-inserted",
				 G_CALLBACK (gantt_model_task_inserted_cb),
				 model,
				 0);
	
	g_signal_connect_object (project,
				 "task-removed",
				 G_CALLBACK (gantt_model_task_removed_cb),
				 model,
				 0);

	g_signal_connect_object (project,
				 "task-moved",
				 G_CALLBACK (gantt_model_task_moved_cb),
				 model,
				 0);

	tasks = mrp_project_get_all_tasks (project);
	for (l = tasks; l; l = l->next) {
		gantt_model_connect_to_task_signals (model, l->data);
	}
	
	g_list_free (tasks);
	
	return model;
}

static void
gantt_model_finalize (GObject *object)
{
	MgGanttModel *model = MG_GANTT_MODEL (object);

	g_node_destroy (model->priv->tree);
	g_hash_table_destroy (model->priv->task2node_hash);
	
	g_free (model->priv);
	model->priv = NULL;

	if (G_OBJECT_CLASS (parent_class)->finalize) {
		G_OBJECT_CLASS (parent_class)->finalize (object);
	}
}

static void
gantt_model_class_init (MgGanttModelClass *klass)
{
	GObjectClass *object_class;

	object_class = (GObjectClass *) klass;

	parent_class = g_type_class_peek_parent (klass);
	
	object_class->finalize = gantt_model_finalize;

	signals[TASK_ADDED] =
		g_signal_new ("task-added",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      0,
			      NULL, NULL,
			      planner_marshal_VOID__OBJECT,
			      G_TYPE_NONE,
			      1, MRP_TYPE_TASK);
	signals[TASK_REMOVED] = 
		g_signal_new ("task-removed",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      0,
			      NULL, NULL,
			      planner_marshal_VOID__OBJECT,
			      G_TYPE_NONE,
			      1, MRP_TYPE_TASK);
}

static gint
gantt_model_get_n_columns (GtkTreeModel *tree_model)
{
	return NUM_COLS;
}

static GType
gantt_model_get_column_type (GtkTreeModel *tree_model,
			     gint          column)
{
	switch (column) {
	case COL_NAME:
		return G_TYPE_STRING;
	case COL_START:
		return G_TYPE_LONG;
	case COL_FINISH:
		return G_TYPE_LONG;
	case COL_DURATION:
		return G_TYPE_INT;
	case COL_WORK:
		return G_TYPE_INT;
	case COL_SLACK:
		return G_TYPE_INT;
	case COL_WEIGHT:
		return G_TYPE_INT;
	case COL_EDITABLE:
		return G_TYPE_BOOLEAN;
	case COL_TASK:
		return MRP_TYPE_TASK;
	case COL_COST:
		return G_TYPE_LONG;	
	default:
		return G_TYPE_INVALID;
	}
}

static gboolean
gantt_model_get_iter (GtkTreeModel *tree_model,
		      GtkTreeIter  *iter,
		      GtkTreePath  *path)
{
	MgGanttModel *gantt_model;
	GtkTreeIter   parent;
	gint         *indices;
	gint          depth, i;
	
	gantt_model = MG_GANTT_MODEL (tree_model);
	
	indices = gtk_tree_path_get_indices (path);
	depth = gtk_tree_path_get_depth (path);

	g_return_val_if_fail (depth > 0, FALSE);

	parent.stamp = gantt_model->stamp;
	parent.user_data = gantt_model->priv->tree;

	if (!gtk_tree_model_iter_nth_child (tree_model, iter, &parent, indices[0])) {
		return FALSE;
	}

	for (i = 1; i < depth; i++) {
		parent = *iter;
		if (!gtk_tree_model_iter_nth_child (tree_model, iter, &parent, indices[i])) {
			return FALSE;
		}
	}

	return TRUE;
}

GtkTreePath *
gantt_model_get_path_from_node (MgGanttModel *model,
				GNode        *node)
{
	GtkTreePath *path;
	GNode       *parent;
	GNode       *child;
	gint         i = 0;
	
	g_return_val_if_fail (MG_IS_GANTT_MODEL (model), NULL);
	g_return_val_if_fail (node != NULL, NULL);

	parent = node->parent;
	
	if (parent == NULL && node == model->priv->tree) {
		return gtk_tree_path_new_root ();
	}
	
	g_assert (parent != NULL);

	if (parent == model->priv->tree) {
		path = gtk_tree_path_new ();
		child = g_node_first_child (model->priv->tree);
	} else {
		path = gantt_model_get_path_from_node (model, parent);
		child = g_node_first_child (parent);
	}

	if (path == NULL) {
		return NULL;
	}
	
	if (child == NULL) {
		gtk_tree_path_free (path);
		return NULL;
	}

	for (; child; child = g_node_next_sibling (child)) {
		if (child == node) {
			break;
		}
		i++;
	}

	if (child == NULL) {
		/* We couldn't find node, meaning it's prolly not ours */
		/* Perhaps I should do a g_return_if_fail here. */
		gtk_tree_path_free (path);
		return NULL;
	}
	
	gtk_tree_path_append_index (path, i);

	return path;
}

GtkTreePath *
planner_gantt_model_get_path_from_task (MgGanttModel *model,
				   MrpTask      *task)
{
	GNode *node;
	
	g_return_val_if_fail (MG_IS_GANTT_MODEL (model), NULL);
	g_return_val_if_fail (MRP_IS_TASK (task), NULL);

	node = g_hash_table_lookup (model->priv->task2node_hash, task);

	/* Help debugging. */
	g_return_val_if_fail (node != NULL, NULL);
	
	return gantt_model_get_path_from_node (model, node);
}

static GtkTreePath *
gantt_model_get_path (GtkTreeModel *tree_model,
		      GtkTreeIter  *iter)
{
	GNode *node;
	
	g_return_val_if_fail (iter != NULL, NULL);
	g_return_val_if_fail (iter->user_data != NULL, NULL);
	g_return_val_if_fail (iter->stamp == MG_GANTT_MODEL (tree_model)->stamp, NULL);

	node = iter->user_data;

	return gantt_model_get_path_from_node (MG_GANTT_MODEL (tree_model), node);
}

static void
gantt_model_get_value (GtkTreeModel *tree_model,
		       GtkTreeIter  *iter,
		       gint          column,
		       GValue       *value)
{
	GNode      *node;
	MrpTask    *task;
	MrpProject *project;
	gchar      *str;
	mrptime     t, t1, t2;
	gint        duration;
	MrpTaskType type;

	g_return_if_fail (iter != NULL);

	node = iter->user_data;
	task = node->data;

	switch (column) {
	case COL_NAME:
		g_object_get (task, "name", &str, NULL);
		if (str == NULL) {
			str = g_strdup ("");
		}
		
		g_value_init (value, G_TYPE_STRING);
		g_value_set_string (value, str);

		g_free (str);
		break;

	case COL_START:
		g_object_get (task, "start", &t, NULL);
		
		g_value_init (value, G_TYPE_LONG);
		g_value_set_long (value, t);
		break;

	case COL_FINISH:
		g_object_get (task, "finish", &t, NULL);

		g_value_init (value, G_TYPE_LONG);
		g_value_set_long (value, t);
		break;
		
	case COL_DURATION:
		g_object_get (task, "duration", &duration, NULL);

		g_value_init (value, G_TYPE_INT);
		g_value_set_int (value, duration);
		break;
		
	case COL_WORK:
		g_object_get (task, "work", &duration, NULL);

		g_value_init (value, G_TYPE_INT);
		g_value_set_int (value, duration);
		break;

	case COL_SLACK:
		g_object_get (task,
			      "finish", &t1,
			      "latest-finish", &t2,
			      "project", &project,
			      NULL);

		/* We don't support negative slack yet. */
		if (t2 >= t1) {
			duration = mrp_project_calculate_task_work (
				project, task, t1, t2);
		} else {
			duration = 0;
		}
		
		g_value_init (value, G_TYPE_INT);
		g_value_set_int (value, duration);
		break;

	case COL_WEIGHT:
		g_value_init (value, G_TYPE_INT);
		if (g_node_n_children (node) > 0) {
			g_value_set_int (value, PANGO_WEIGHT_BOLD);
		} else {
			g_value_set_int (value, PANGO_WEIGHT_NORMAL);
		}
		break;

	case COL_EDITABLE:
		g_object_get (task, "type", &type, NULL);
		
		g_value_init (value, G_TYPE_BOOLEAN);
		if (g_node_n_children (node) > 0) {
			g_value_set_boolean (value, FALSE);
		} else {
			g_value_set_boolean (value, TRUE);
		}
		break;

	case COL_TASK:
		g_value_init (value, MRP_TYPE_TASK);
		g_value_set_object (value, task);

		break;

	case COL_COST:
		g_value_init (value, G_TYPE_FLOAT);
		g_value_set_float (value, mrp_task_get_cost (task));

		break;

	default:
		g_warning ("Bad column %d requested", column);
	}
}

static gboolean
gantt_model_iter_next (GtkTreeModel *tree_model,
		       GtkTreeIter  *iter)
{
	GNode *node, *next;

	node = iter->user_data;

	next = g_node_next_sibling (node);

	if (next == NULL) {
		iter->user_data = NULL;
		return FALSE;
	}
	
	iter->user_data = next;
	return TRUE;
}

static gboolean
gantt_model_iter_children (GtkTreeModel *tree_model,
			   GtkTreeIter  *iter,
			   GtkTreeIter  *parent)
{
	GNode *node, *child;

	/* Handle iter == NULL (or gail will crash), even though the docs
	 * doesn't say we need to.
	 */
	if (parent) {
		node = parent->user_data;
	} else {
		node = MG_GANTT_MODEL (tree_model)->priv->tree;
	}
	
	child = g_node_first_child (node);

	if (child == NULL) {
		iter->user_data = NULL;
		return FALSE;
	}
	
	iter->user_data = child;
	iter->stamp = MG_GANTT_MODEL (tree_model)->stamp;
	return TRUE;
}

static gboolean
gantt_model_iter_has_child (GtkTreeModel *tree_model,
			    GtkTreeIter  *iter)
{
	GNode *node;
	
	node = iter->user_data;

	return (g_node_n_children (node) > 0);
}

static gint
gantt_model_iter_n_children (GtkTreeModel *tree_model,
			     GtkTreeIter  *iter)
{
	GNode *node;

	if (iter) {
		node = iter->user_data;
	} else {
		node = MG_GANTT_MODEL (tree_model)->priv->tree;
	}

	return g_node_n_children (node);
}

static gboolean
gantt_model_iter_nth_child (GtkTreeModel *tree_model,
			    GtkTreeIter  *iter,
			    GtkTreeIter  *parent_iter,
			    gint          n)
{
	MgGanttModel *model;
	GNode        *parent;
	GNode        *child;

	g_return_val_if_fail (parent_iter == NULL || parent_iter->user_data != NULL, FALSE);

	model = MG_GANTT_MODEL (tree_model);
	
	if (parent_iter == NULL) {
		parent = model->priv->tree;
	} else {
		parent = parent_iter->user_data;
	}

	child = g_node_nth_child (parent, n);

	if (child) {
		iter->user_data = child;
		iter->stamp = model->stamp;
		return TRUE;
	} else {
		iter->user_data = NULL;
		return FALSE;
	}
}

static gboolean
gantt_model_iter_parent (GtkTreeModel *tree_model,
			 GtkTreeIter  *iter,
			 GtkTreeIter  *child)
{
	GNode *node_task;
	GNode *node_parent;
  
	node_task = child->user_data;
  
	node_parent = node_task->parent;

	if (node_parent == NULL) {
		iter->user_data = NULL;
		return FALSE;
	}

	iter->user_data = node_parent;
	iter->stamp = MG_GANTT_MODEL (tree_model)->stamp;

	return TRUE;
}

static void
gantt_model_tree_model_init (GtkTreeModelIface *iface)
{
	iface->get_n_columns = gantt_model_get_n_columns;
	iface->get_column_type = gantt_model_get_column_type;
	iface->get_iter = gantt_model_get_iter;
	iface->get_path = gantt_model_get_path;
	iface->get_value = gantt_model_get_value;
	iface->iter_next = gantt_model_iter_next;
	iface->iter_children = gantt_model_iter_children;
	iface->iter_has_child = gantt_model_iter_has_child;
	iface->iter_n_children = gantt_model_iter_n_children;
	iface->iter_nth_child = gantt_model_iter_nth_child;
	iface->iter_parent = gantt_model_iter_parent;
}

static void
gantt_model_init (MgGanttModel *model)
{
	MgGanttModelPriv *priv;

	priv = g_new0 (MgGanttModelPriv, 1);
	model->priv = priv;

	priv->task2node_hash = g_hash_table_new (NULL, NULL);
	
	do {
		model->stamp = g_random_int ();
	} while (model->stamp == 0);
}

/* DND */
#ifdef DND
static gboolean
gantt_model_drag_data_delete (GtkTreeDragSource *drag_source,
			      GtkTreePath       *path)
{
	GtkTreeIter iter;
	
	g_return_val_if_fail (MG_IS_GANTT_MODEL (drag_source), FALSE);

	g_print ("delete\n");
	
	if (gtk_tree_model_get_iter (GTK_TREE_MODEL (drag_source),
				     &iter,
				     path)) {
		mrp_project_remove_task (MG_GANTT_MODEL (drag_source)->priv->project,
					 iter.user_data);
		return TRUE;
	} else {
		return FALSE;
	}
}

static gboolean
gantt_model_drag_data_get (GtkTreeDragSource *drag_source,
			   GtkTreePath       *path,
			   GtkSelectionData  *selection_data)
{
	g_return_val_if_fail (MG_IS_GANTT_MODEL (drag_source), FALSE);

	/* Note that we don't need to handle the GTK_TREE_MODEL_ROW
	 * target, because the default handler does it for us, but
	 * we do anyway for the convenience of someone maybe overriding the
	 * default handler.
	 */
	
	if (gtk_tree_set_row_drag_data (selection_data,
					GTK_TREE_MODEL (drag_source),
					path)) {
		return TRUE;
	} else {
		/* FIXME handle text targets at least. */
	}

	return FALSE;
}

static gboolean
gantt_model_drag_data_received (GtkTreeDragDest   *drag_dest,
				GtkTreePath       *dest,
				GtkSelectionData  *selection_data)
{
	GtkTreeModel *tree_model;
	MgGanttModel *gantt_model;
	GtkTreeModel *src_model = NULL;
	GtkTreePath *src_path = NULL;
	gboolean retval = FALSE;

	g_return_val_if_fail (MG_IS_GANTT_MODEL (drag_dest), FALSE);

	g_print ("received\n");
	
	tree_model = GTK_TREE_MODEL (drag_dest);
	gantt_model = MG_GANTT_MODEL (drag_dest);

	if (gtk_tree_get_row_drag_data (selection_data,
					&src_model,
					&src_path) &&
	    src_model == tree_model) {
		/* Copy the given row to a new position */
		GtkTreeIter src_iter;
		GtkTreeIter dest_iter;
		GtkTreePath *prev;
		
		if (!gtk_tree_model_get_iter (src_model,
					      &src_iter,
					      src_path)) {
			goto out;
		}

		/* Get the path to insert _after_ (dest is the path to insert _before_) */
		prev = gtk_tree_path_copy (dest);
		
		if (!gtk_tree_path_prev (prev)) {
			GtkTreeIter dest_parent;
			GtkTreePath *parent;
			GtkTreeIter *dest_parent_p;
			
			/* dest was the first spot at the current depth; which means
			 * we are supposed to prepend.
			 */
			
			/* Get the parent, NULL if parent is the root */
			dest_parent_p = NULL;
			parent = gtk_tree_path_copy (dest);
			if (gtk_tree_path_up (parent) &&
			    gtk_tree_path_get_depth (parent) > 0) {
				gtk_tree_model_get_iter (tree_model,
							 &dest_parent,
							 parent);
				dest_parent_p = &dest_parent;
			}
			gtk_tree_path_free (parent);
			parent = NULL;
			
			/*gtk_tree_store_prepend (GTK_TREE_STORE (tree_model),
			  &dest_iter,
			  dest_parent_p);*/
			
			retval = TRUE;
		} else {
			if (gtk_tree_model_get_iter (GTK_TREE_MODEL (tree_model),
						     &dest_iter,
						     prev)) {
				GtkTreeIter tmp_iter = dest_iter;
				
				if (GPOINTER_TO_INT (g_object_get_data (G_OBJECT (tree_model),
									"gtk-tree-model-drop-append"))) {
					GtkTreeIter parent;
					
					if (gtk_tree_model_iter_parent (GTK_TREE_MODEL (tree_model), &parent, &tmp_iter))
						/*gtk_tree_store_append (GTK_TREE_STORE (tree_model),
						  &dest_iter, &parent);*/
						;
					else
						/*gtk_tree_store_append (GTK_TREE_STORE (tree_model),
						  &dest_iter, NULL);*/
						;
				}
				else
					/*gtk_tree_store_insert_after (GTK_TREE_STORE (tree_model),
					  &dest_iter,
					  NULL,
					  &tmp_iter);*/
					;
				retval = TRUE;
				
			}
		}
		
		g_object_set_data (G_OBJECT (tree_model),
				   "gtk-tree-model-drop-append",
				   NULL);
		
		gtk_tree_path_free (prev);
		
		/* If we succeeded in creating dest_iter, walk src_iter tree branch,
		 * duplicating it below dest_iter.
		 */

		if (retval) {
			/*recursive_node_copy (tree_store,
			  &src_iter,
			  &dest_iter);*/
		}
	} else {
		/* FIXME maybe add some data targets eventually, or handle text
		 * targets in the simple case.
		 */
	}
	
 out:
	
	if (src_path) {
		gtk_tree_path_free (src_path);
	}
	
	return retval;
}

static gboolean
gantt_model_row_drop_possible (GtkTreeDragDest  *drag_dest,
			       GtkTreePath      *dest_path,
			       GtkSelectionData *selection_data)
{
	GtkTreeModel *src_model = NULL;
	GtkTreePath  *src_path = NULL;
	GtkTreePath  *tmp = NULL;
	gboolean      retval = FALSE;
	GtkTreeIter   iter;
	
	if (!gtk_tree_get_row_drag_data (selection_data,
					 &src_model,
					 &src_path)) {
		goto out;
	}
	
	/* can only drag to ourselves */
	if (src_model != GTK_TREE_MODEL (drag_dest)) {
		goto out;
	}
	
	/* Can't drop into ourself. */
	if (gtk_tree_path_is_ancestor (src_path,
				       dest_path)) {
		goto out;
	}

	/* Can't drop if dest_path's parent doesn't exist */
	if (gtk_tree_path_get_depth (dest_path) > 1) {
		tmp = gtk_tree_path_copy (dest_path);
		gtk_tree_path_up (tmp);
		
		if (!gtk_tree_model_get_iter (GTK_TREE_MODEL (drag_dest),
					      &iter, tmp)) {
			goto out;
		}
	}

	/* Can otherwise drop anywhere. */
	retval = TRUE;

 out:
	
	if (src_path) {
		gtk_tree_path_free (src_path);
	}
	if (tmp) {
		gtk_tree_path_free (tmp);
	}
	
	return retval;
}

static void
gantt_model_drag_source_init (GtkTreeDragSourceIface *iface)
{
	iface->drag_data_delete = gantt_model_drag_data_delete;
	iface->drag_data_get = gantt_model_drag_data_get;
}

static void
gantt_model_drag_dest_init (GtkTreeDragDestIface *iface)
{
	iface->drag_data_received = gantt_model_drag_data_received;
	iface->row_drop_possible = gantt_model_row_drop_possible;
}
#endif

MrpProject *
planner_gantt_model_get_project (MgGanttModel *model)
{
	g_return_val_if_fail (MG_IS_GANTT_MODEL (model), NULL);
	
	return model->priv->project;
}

MrpTask *
planner_gantt_model_get_task (MgGanttModel *model,
			 GtkTreeIter  *iter)
{
	MrpTask *task;

	task = ((GNode *) iter->user_data)->data; 

	if (task == NULL) {
		g_warning ("Eeek");
		return NULL;
	} else { 
		return MRP_TASK (task);
	}
}

MrpTask *
planner_gantt_model_get_indent_task_target (MgGanttModel *model,
				       MrpTask      *task)
{
	GNode *node;
	GNode *sibling;

	g_return_val_if_fail (MG_IS_GANTT_MODEL (model), NULL);
	g_return_val_if_fail (MRP_IS_TASK (task), NULL);

	node = g_hash_table_lookup (model->priv->task2node_hash, task);

	sibling = g_node_prev_sibling (node);

	/* If we're the first child we can't indent. */
	if (sibling == NULL || sibling == node) {
		return NULL;
	}

	return sibling->data;
}



/* ------------------ test */
static gchar*
get_n_chars (gint n, gchar c)
{
	GString *str;
	gchar   *ret;
	gint     i;

	str = g_string_new ("");
	
	for (i = 0; i < n; i++) {
		g_string_append_c (str, c);
	}

	ret = str->str;
	g_string_free (str, FALSE);
	
	return ret;
}

static void
dump_children (GNode *node, gint depth)
{
	GNode   *child;
	gchar   *padding = get_n_chars (2 * depth, ' ');
	MrpTask *task;
	gchar   *name;

	for (child = g_node_first_child (node); child; child = g_node_next_sibling (child)) {
		task = (MrpTask *) child->data;

		g_object_get (task, "name", &name, NULL);
		g_print ("%sName: %s\n", padding, name);
		g_free (name);
		
		dump_children (child, depth + 1);
	}

	g_free (padding);
}

static void
dump_tree (GNode *node)
{
	g_return_if_fail (node != NULL);
	g_return_if_fail (node->parent == NULL);

	return;
	
	g_print ("------------------------------------------\n<Root>\n");

	dump_children (node, 1);

	g_print ("\n");

	if (0) {
		dump_tree (NULL);
	}
}

