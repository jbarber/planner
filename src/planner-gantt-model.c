/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2004 Imendio HB
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

struct _PlannerGanttModelPriv {
	MrpProject *project;
	GHashTable *task2node;
	GNode      *tree;

	GHashTable *task2cache;
	gint        wbs_stamp;
};

typedef struct {
	gchar *wbs;
	gint   wbs_stamp;
	
	gchar *start;
	gchar *finish;
	gchar *duration;
	gchar *work;
} ValueCache;


static void         gantt_model_init                 (PlannerGanttModel      *model);
static void         gantt_model_class_init           (PlannerGanttModelClass *class);
static void         gantt_model_tree_model_init      (GtkTreeModelIface      *iface);
static gboolean     gantt_model_get_iter             (GtkTreeModel           *model,
						      GtkTreeIter            *iter,
						      GtkTreePath            *path);
static void         gantt_model_task_notify_cb       (MrpTask                *task,
						      GParamSpec             *pspec,
						      PlannerGanttModel      *model);
static void         gantt_model_task_prop_changed_cb (MrpTask                *task,
						      MrpProperty            *property,
						      GValue                 *value,
						      PlannerGanttModel      *model);
static GtkTreePath *gantt_model_get_path_from_node   (PlannerGanttModel      *model,
						      GNode                  *node);
static const gchar *value_cache_get_wbs              (PlannerGanttModel      *model,
						      MrpTask                *task);
static ValueCache * value_cache_get                  (PlannerGanttModel      *model,
						      MrpTask                *task);
static void         value_cache_clear                (PlannerGanttModel      *model,
						      MrpTask                *task);
static void         value_cache_clear_cache_wbs      (PlannerGanttModel      *model);
static void         value_cache_free                 (ValueCache             *cache);


static GObjectClass *parent_class;
static guint signals[LAST_SIGNAL];

GType
planner_gantt_model_get_type (void)
{
	static GType type = 0;

	if (!type) {
		static const GTypeInfo info = {
			sizeof (PlannerGanttModelClass),
			NULL,		/* base_init */
			NULL,		/* base_finalize */
			(GClassInitFunc) gantt_model_class_init,
			NULL,		/* class_finalize */
			NULL,		/* class_data */
			sizeof (PlannerGanttModel),
			0,
			(GInstanceInitFunc) gantt_model_init
		};

		static const GInterfaceInfo tree_model_info = {
			(GInterfaceInitFunc) gantt_model_tree_model_init,
			NULL,
			NULL
		};

		type = g_type_register_static (G_TYPE_OBJECT,
					       "PlannerGanttModel",
					       &info, 0);
		
		g_type_add_interface_static (type,
					     GTK_TYPE_TREE_MODEL,
					     &tree_model_info);
   	}

	return type;
}

static void
gantt_model_connect_to_task_signals (PlannerGanttModel *model, MrpTask *task)
{
	g_signal_connect_object (task,
				 "notify",
				 G_CALLBACK (gantt_model_task_notify_cb),
				 model,
				 0);

	g_signal_connect_object (task,
				 "prop_changed",
				 G_CALLBACK (gantt_model_task_prop_changed_cb),
				 model,
				 0);
}

static void
gantt_model_task_inserted_cb (MrpProject        *project,
			      MrpTask           *task,
			      PlannerGanttModel *model)
{
	GtkTreePath *path;
	GtkTreePath *parent_path;
	GtkTreeIter  iter;
	GNode       *node;
	GNode       *parent_node;
	MrpTask     *parent;
	gint         pos;
	gboolean     has_child_toggled;

	value_cache_clear_cache_wbs (model);
	
	node = g_node_new (task);

	g_hash_table_insert (model->priv->task2node, task, node);

	parent = mrp_task_get_parent (task);
	pos = mrp_task_get_position (task);

	parent_node = g_hash_table_lookup (model->priv->task2node, parent);

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
#if 0
	if (g_node_n_nodes (model->priv->tree, G_TRAVERSE_ALL) !=
	    g_hash_table_size (model->priv->task2node)) {
		g_warning ("Gantt model corrupt.");
	}
#endif
	
	g_signal_emit (model, signals[TASK_ADDED], 0, task);
}

static gboolean
traverse_remove_subtree (GNode             *node,
			 PlannerGanttModel *model)
{
	g_signal_handlers_disconnect_by_func (node->data,
					      gantt_model_task_notify_cb,
					      model);
	g_signal_handlers_disconnect_by_func (node->data,
					      gantt_model_task_prop_changed_cb,
					      model);
	
	g_hash_table_remove (model->priv->task2node, node->data);

	return FALSE;
}

static void
gantt_model_remove_subtree (PlannerGanttModel *model,
			    GNode             *node)
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
gantt_model_task_removed_cb (MrpProject        *project,
			     MrpTask           *task,
			     PlannerGanttModel *model)
{
	GNode       *node;
	GNode       *parent_node;
	GtkTreePath *path;
	GtkTreePath *parent_path;
	GtkTreeIter  iter;
	gboolean     has_child_toggled;

	g_signal_emit (model, signals[TASK_REMOVED], 0, task);

	node = g_hash_table_lookup (model->priv->task2node, task);
	if (!node) {
		/* This happens if we are already removed when a parent was
		 * removed. 
		 */
		return; 
	}

	value_cache_clear_cache_wbs (model);

	g_signal_handlers_disconnect_by_func (task,
					      gantt_model_task_notify_cb,
					      model);
	g_signal_handlers_disconnect_by_func (task,
					      gantt_model_task_prop_changed_cb,
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
#if 0
	if (g_node_n_nodes (model->priv->tree, G_TRAVERSE_ALL) !=
	    g_hash_table_size (model->priv->task2node)) {
		g_warning ("Gantt model corrupt.");
	}
#endif
}

static void
gantt_model_reattach_subtasks (GtkTreeModel *tree_model,
			       MrpTask      *task)
{
	PlannerGanttModel     *model;
	PlannerGanttModelPriv *priv;
	MrpTask               *child;
	GNode                 *node;
	GNode                 *parent_node;
	GtkTreePath           *path;
	GtkTreeIter            iter;
	gint                   pos;
	gboolean               has_child_toggled;
	
	model = PLANNER_GANTT_MODEL (tree_model);
	priv = model->priv;
	
	parent_node = g_hash_table_lookup (priv->task2node, task);

	/* Traverse the subtasks. */
	child = mrp_task_get_first_child (task);
	while (child) {
		node = g_hash_table_lookup (priv->task2node, child);
		pos = mrp_task_get_position (child);
		g_node_insert (parent_node, pos, node);
		
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
gantt_model_task_moved_cb (MrpProject        *project,
			   MrpTask           *task,
			   PlannerGanttModel *model)
{
	MrpTask     *parent;
	GtkTreePath *path;
	GtkTreePath *parent_path;
	GtkTreeIter  iter;
	GNode       *node;
	GNode       *parent_node;
	gint         pos;
	gboolean     has_child_toggled;

	value_cache_clear_cache_wbs (model);

	path = planner_gantt_model_get_path_from_task (model, task);
	gtk_tree_model_get_iter (GTK_TREE_MODEL (model), &iter, path);

	/* Notify views. */
	gtk_tree_model_row_deleted (GTK_TREE_MODEL (model), path);
	gtk_tree_path_free (path);

	node = g_hash_table_lookup (model->priv->task2node, task);

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
	parent_node = g_hash_table_lookup (model->priv->task2node, parent);

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
gantt_model_task_notify_cb (MrpTask           *task,
			    GParamSpec        *pspec,
			    PlannerGanttModel *model)
{
	GtkTreeModel *tree_model;
	GtkTreePath  *path;
	GtkTreeIter   iter;

	tree_model = GTK_TREE_MODEL (model);

	if (strcmp (pspec->name, "start") == 0 ||
	    strcmp (pspec->name, "finish") == 0 ||
	    strcmp (pspec->name, "duration") == 0 ||
	    strcmp (pspec->name, "work") == 0) {
		value_cache_clear (model, task);
	}
	
	path = planner_gantt_model_get_path_from_task (model, task);
	gtk_tree_model_get_iter (tree_model, &iter, path);
	gtk_tree_model_row_changed (tree_model, path, &iter);

	gtk_tree_path_free (path);
}

static void
gantt_model_task_prop_changed_cb (MrpTask           *task,
				  MrpProperty       *property,
				  GValue            *value,
				  PlannerGanttModel *model)
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
traverse_insert_task_into_hash (GNode             *node,
				PlannerGanttModel *model)
{
	g_hash_table_insert (model->priv->task2node,
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
gantt_model_setup_task_tree (PlannerGanttModel *model)
{
	MrpTask *root_task;
	GNode   *root_node;

	root_task = mrp_project_get_root_task (model->priv->project); 
	root_node = g_node_new (root_task);

	traverse_setup_tree (root_task, root_node);

	return root_node;
}

PlannerGanttModel *
planner_gantt_model_new (MrpProject *project)
{
	PlannerGanttModel     *model;
	PlannerGanttModelPriv *priv;
	GList                 *tasks, *l;
	
	model = PLANNER_GANTT_MODEL (g_object_new (PLANNER_TYPE_GANTT_MODEL, NULL));
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
	PlannerGanttModel *model = PLANNER_GANTT_MODEL (object);

	g_node_destroy (model->priv->tree);
	g_hash_table_destroy (model->priv->task2node);
	g_hash_table_destroy (model->priv->task2cache);
	
	g_free (model->priv);
	model->priv = NULL;

	if (G_OBJECT_CLASS (parent_class)->finalize) {
		G_OBJECT_CLASS (parent_class)->finalize (object);
	}
}

static void
gantt_model_class_init (PlannerGanttModelClass *klass)
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
	case COL_WBS:
		return G_TYPE_STRING;
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
	PlannerGanttModel *gantt_model;
	GtkTreeIter        parent;
	gint              *indices;
	gint               depth, i;
	
	gantt_model = PLANNER_GANTT_MODEL (tree_model);
	
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
gantt_model_get_path_from_node (PlannerGanttModel *model,
				GNode             *node)
{
	GtkTreePath *path;
	GNode       *parent;
	GNode       *child;
	gint         i = 0;
	
	g_return_val_if_fail (PLANNER_IS_GANTT_MODEL (model), NULL);
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
planner_gantt_model_get_path_from_task (PlannerGanttModel *model,
					MrpTask           *task)
{
	GNode *node;
	
	g_return_val_if_fail (PLANNER_IS_GANTT_MODEL (model), NULL);
	g_return_val_if_fail (MRP_IS_TASK (task), NULL);

	node = g_hash_table_lookup (model->priv->task2node, task);

	if (!node) {
		return NULL;
	}
	
	return gantt_model_get_path_from_node (model, node);
}

static GtkTreePath *
gantt_model_get_path (GtkTreeModel *tree_model,
		      GtkTreeIter  *iter)
{
	GNode *node;
	
	g_return_val_if_fail (iter != NULL, NULL);
	g_return_val_if_fail (iter->user_data != NULL, NULL);
	g_return_val_if_fail (iter->stamp == PLANNER_GANTT_MODEL (tree_model)->stamp, NULL);

	node = iter->user_data;

	return gantt_model_get_path_from_node (PLANNER_GANTT_MODEL (tree_model), node);
}

static void
gantt_model_get_value (GtkTreeModel *tree_model,
		       GtkTreeIter  *iter,
		       gint          column,
		       GValue       *value)
{
	GNode       *node;
	MrpTask     *task;
	MrpProject  *project;
	mrptime      t1, t2;
	gint         duration;
	MrpTaskType  type;
	const gchar *name;
	const gchar *cached_str;
	
	g_return_if_fail (iter != NULL);

	node = iter->user_data;
	task = node->data;

	switch (column) {
	case COL_WBS:
		cached_str = value_cache_get_wbs (PLANNER_GANTT_MODEL (tree_model),
						  task);
		
		g_value_init (value, G_TYPE_STRING);
		g_value_set_string (value, cached_str);
		break;
		
	case COL_NAME:
		name = mrp_task_get_name (task);
		if (name == NULL) {
			name = "";
		}
		
		g_value_init (value, G_TYPE_STRING);
		g_value_set_string (value, name);
		break;

	case COL_START:
		g_value_init (value, G_TYPE_LONG);
		g_value_set_long (value, mrp_task_get_work_start (task));
		break;

	case COL_FINISH:
		g_value_init (value, G_TYPE_LONG);
		g_value_set_long (value, mrp_task_get_finish (task));
		break;
		
	case COL_DURATION:
		g_value_init (value, G_TYPE_INT);
		g_value_set_int (value, mrp_task_get_duration (task));
		break;
		
	case COL_WORK:
		g_value_init (value, G_TYPE_INT);
		g_value_set_int (value, mrp_task_get_work (task));
		break;

	case COL_SLACK:
		t1 = mrp_task_get_finish (task);
		t2 = mrp_task_get_latest_finish (task);
		project = mrp_object_get_project (MRP_OBJECT (task));
		
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
		type = mrp_task_get_task_type (task);
		
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
		node = PLANNER_GANTT_MODEL (tree_model)->priv->tree;
	}
	
	child = g_node_first_child (node);

	if (child == NULL) {
		iter->user_data = NULL;
		return FALSE;
	}
	
	iter->user_data = child;
	iter->stamp = PLANNER_GANTT_MODEL (tree_model)->stamp;
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
		node = PLANNER_GANTT_MODEL (tree_model)->priv->tree;
	}

	return g_node_n_children (node);
}

static gboolean
gantt_model_iter_nth_child (GtkTreeModel *tree_model,
			    GtkTreeIter  *iter,
			    GtkTreeIter  *parent_iter,
			    gint          n)
{
	PlannerGanttModel *model;
	GNode        *parent;
	GNode        *child;

	g_return_val_if_fail (parent_iter == NULL || parent_iter->user_data != NULL, FALSE);

	model = PLANNER_GANTT_MODEL (tree_model);
	
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
	iter->stamp = PLANNER_GANTT_MODEL (tree_model)->stamp;

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
gantt_model_init (PlannerGanttModel *model)
{
	PlannerGanttModelPriv *priv;

	priv = g_new0 (PlannerGanttModelPriv, 1);
	model->priv = priv;

	priv->task2node = g_hash_table_new (NULL, NULL);
	priv->task2cache = g_hash_table_new_full (NULL, NULL,
						  NULL,
						  (GDestroyNotify) value_cache_free);
	
	do {
		model->stamp = g_random_int ();
	} while (model->stamp == 0);
}

MrpProject *
planner_gantt_model_get_project (PlannerGanttModel *model)
{
	g_return_val_if_fail (PLANNER_IS_GANTT_MODEL (model), NULL);
	
	return model->priv->project;
}

MrpTask *
planner_gantt_model_get_task (PlannerGanttModel *model,
			      GtkTreeIter  *iter)
{
	MrpTask *task;

	task = ((GNode *) iter->user_data)->data; 

	if (task == NULL) {
		/* Shouldn't really happen. */
		return NULL;
	} else { 
		return MRP_TASK (task);
	}
}

MrpTask *
planner_gantt_model_get_task_from_path (PlannerGanttModel *model,
					GtkTreePath       *path)
{
	GtkTreeIter  iter;
	MrpTask     *task = NULL;

	g_return_val_if_fail (PLANNER_IS_GANTT_MODEL (model), NULL);
	
	if (gtk_tree_model_get_iter (GTK_TREE_MODEL (model), &iter, path)) {
		gtk_tree_model_get (GTK_TREE_MODEL (model), &iter, 
				    COL_TASK, &task,
				    -1);
	}
	
	return task;
}

MrpTask *
planner_gantt_model_get_indent_task_target (PlannerGanttModel *model,
					    MrpTask           *task)
{
	GNode *node;
	GNode *sibling;

	g_return_val_if_fail (PLANNER_IS_GANTT_MODEL (model), NULL);
	g_return_val_if_fail (MRP_IS_TASK (task), NULL);

	node = g_hash_table_lookup (model->priv->task2node, task);

	sibling = g_node_prev_sibling (node);

	/* If we're the first child we can't indent. */
	if (sibling == NULL || sibling == node) {
		return NULL;
	}

	return sibling->data;
}

static const gchar *
value_cache_get_wbs (PlannerGanttModel *model,
		     MrpTask           *task)
{
	ValueCache *cache;
	MrpTask    *tmp_task;
	gchar      *str;
	GString    *string;
	gint        pos;

	cache = value_cache_get (model, task);
	if (!cache->wbs) {
		goto update_cache;
	}
	
	if (cache->wbs_stamp != model->priv->wbs_stamp) {
		goto update_cache;
	}

	return cache->wbs;
	
 update_cache:
	string = g_string_sized_new (24);

	pos = -1;
	tmp_task = task;
	while (tmp_task) {
		if (pos != -1) {
			g_string_prepend_c (string, '.');
		}
		
		pos = mrp_task_get_position (tmp_task) + 1;
		
		str = g_strdup_printf ("%d", pos);
		g_string_prepend (string, str);
		g_free (str);
		
		tmp_task = mrp_task_get_parent (tmp_task);
		
		/* Skip the root. */
		if (mrp_task_get_parent (tmp_task) == NULL)
			break;
	}

	g_free (cache->wbs);

	cache->wbs = g_string_free (string, FALSE);
	cache->wbs_stamp = model->priv->wbs_stamp;
	
	return cache->wbs;
}

static void
value_cache_free (ValueCache *cache)
{
	g_free (cache->wbs);
	g_free (cache->start);
	g_free (cache->finish);
	g_free (cache->duration);
	g_free (cache->work);

	g_free (cache);
}

static ValueCache *
value_cache_get (PlannerGanttModel *model,
		 MrpTask           *task)
{
	ValueCache *cache;

	cache = g_hash_table_lookup (model->priv->task2cache, task);
	if (!cache) {
		cache = g_new0 (ValueCache, 1);
		g_hash_table_insert (model->priv->task2cache,
				     task,
				     cache);
	}	

	return cache;
}

static void
value_cache_clear (PlannerGanttModel *model,
		   MrpTask           *task)
{
	g_hash_table_remove (model->priv->task2cache, task);
}

static void
value_cache_clear_cache_wbs (PlannerGanttModel *model)
{
	model->priv->wbs_stamp++;
}



