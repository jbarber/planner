/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2001-2003 CodeFactory AB
 * Copyright (C) 2001-2003 Richard Hult <richard@imendio.com>
 * Copyright (C) 2001-2002 Mikael Hallendal <micke@imendio.com>
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
#include <time.h>
#include <math.h>
#include <glib-object.h>
#include "mrp-intl.h"
#include "mrp-marshal.h"
#include "mrp-storage-module.h"
#include "mrp-task-manager.h"
#include "mrp-private.h"
#include "mrp-time.h"
#include "mrp-error.h"

struct _MrpTaskManagerPriv {
	MrpProject *project;
	MrpTask    *root;

	gboolean    block_scheduling;
	
	/* Whether the sorted tree is valid or needs to be rebuilt. */
	gboolean    needs_rebuild;

	/* Whether the task tree needs to be recalculated. */
	gboolean    needs_recalc;
};

typedef struct {
	MrpTaskTraverseFunc func;
	gpointer            user_data;
} MrpTaskTraverseData;

/* Properties */
enum {
	PROP_0,
	PROP_PROJECT,
};

static void task_manager_class_init       (MrpTaskManagerClass *klass);
static void task_manager_init             (MrpTaskManager      *tm);
static void task_manager_finalize         (GObject             *object);
static void task_manager_set_property     (GObject             *object,
					   guint                prop_id,
					   const GValue        *value,
					   GParamSpec          *pspec);
static void task_manager_get_property     (GObject             *object,
					   guint                prop_id,
					   GValue              *value,
					   GParamSpec          *pspec);
static void 
task_manager_task_duration_notify_cb      (MrpTask             *task,
					   GParamSpec          *spec,
					   MrpTaskManager      *manager);
static void
task_manager_task_constraint_notify_cb    (MrpTask             *task,
					   GParamSpec          *spec,
					   MrpTaskManager      *manager);
static void
task_manager_project_start_notify_cb      (MrpProject          *project,
					   GParamSpec          *spec,
					   MrpTaskManager      *manager);
static void
task_manager_task_relation_added_cb       (MrpTask             *task,
					   MrpRelation         *relation,
					   MrpTaskManager      *manager);
static void
task_manager_task_relation_removed_cb     (MrpTask             *task,
					   MrpRelation         *relation,
					   MrpTaskManager      *manager);
static void
task_manager_task_assignment_added_cb     (MrpTask             *task,
					   MrpAssignment       *assignment,
					   MrpTaskManager      *manager);
static void
task_manager_task_assignment_removed_cb   (MrpTask             *task,
					   MrpAssignment       *assignment,
					   MrpTaskManager      *manager);
static void
task_manager_task_relation_notify_cb      (MrpRelation         *relation,
					   GParamSpec          *spec,
					   MrpTaskManager      *manager);
static void
task_manager_assignment_units_notify_cb   (MrpAssignment       *assignment,
					   GParamSpec          *spec,
					   MrpTaskManager      *manager);
static void
task_manager_unlink_sorted_tree           (MrpTaskManager      *manager);

static void
task_manager_dump_task_tree               (GNode               *node);


static GObjectClass *parent_class;


GType
mrp_task_manager_get_type (void)
{
	static GType object_type = 0;

	if (!object_type) {
		static const GTypeInfo object_info = {
			sizeof (MrpTaskManagerClass),
			NULL,		/* base_init */
			NULL,		/* base_finalize */
			(GClassInitFunc) task_manager_class_init,
			NULL,		/* class_finalize */
			NULL,		/* class_data */
			sizeof (MrpTaskManager),
			0,              /* n_preallocs */
			(GInstanceInitFunc) task_manager_init,
		};

		object_type = g_type_register_static (G_TYPE_OBJECT,
						      "MrpTaskManager",
						      &object_info,
						      0);
	}

	return object_type;
}

static void
task_manager_class_init (MrpTaskManagerClass *klass)
{
	GObjectClass *object_class = (GObjectClass *) klass;

	parent_class = g_type_class_peek_parent (klass);

	object_class->finalize     = task_manager_finalize;
	object_class->set_property = task_manager_set_property;
	object_class->get_property = task_manager_get_property;

	/* Properties. */
	g_object_class_install_property (
		object_class,
		PROP_PROJECT,
		g_param_spec_object ("project",
				     "Project",
				     "The project that this task manager is attached to",
				     G_TYPE_OBJECT,
				     G_PARAM_READWRITE |
				     G_PARAM_CONSTRUCT_ONLY));
}

static void
task_manager_init (MrpTaskManager *man)
{
	MrpTaskManagerPriv *priv;
	
	man->priv = g_new0 (MrpTaskManagerPriv, 1);
	priv = man->priv;

	priv->needs_recalc = TRUE;
	priv->needs_rebuild = TRUE;
}

static void
task_manager_finalize (GObject *object)
{
	MrpTaskManager *manager = MRP_TASK_MANAGER (object);

	g_object_unref (manager->priv->root);

	g_free (manager->priv);

	if (G_OBJECT_CLASS (parent_class)->finalize) {
		(* G_OBJECT_CLASS (parent_class)->finalize) (object);
	}
}

static void
task_manager_set_property (GObject      *object,
			   guint         prop_id,
			   const GValue *value,
			   GParamSpec   *pspec)
{
	MrpTaskManager     *manager;
	MrpTaskManagerPriv *priv;
	
	manager = MRP_TASK_MANAGER (object);
	priv = manager->priv;
	
	switch (prop_id) {
	case PROP_PROJECT:
		priv->project = g_value_get_object (value);
		g_signal_connect (priv->project,
				  "notify::project-start",
				  G_CALLBACK (task_manager_project_start_notify_cb),
				  manager);
		
		mrp_task_manager_set_root (manager, mrp_task_new ());
		break;

	default:
		break;
	}
}

static void
task_manager_get_property (GObject    *object, 
			   guint       prop_id, 
			   GValue     *value,
			   GParamSpec *pspec)
{
	MrpTaskManager *manager;

	manager = MRP_TASK_MANAGER (object);
	
	switch (prop_id) {
	case PROP_PROJECT:
		g_value_set_object (value, manager->priv->project);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

MrpTaskManager *
mrp_task_manager_new (MrpProject *project)
{
	MrpTaskManager *manager;
	
	manager = g_object_new (MRP_TYPE_TASK_MANAGER,
				"project", project,
				NULL);
	
	return manager;
}

static void
task_manager_task_connect_signals (MrpTaskManager *manager,
				   MrpTask        *task)
{
	g_signal_connect (task,
			  "notify::duration",
			  G_CALLBACK (task_manager_task_duration_notify_cb),
			  manager);
	g_signal_connect (task,
			  "notify::constraint",
			  G_CALLBACK (task_manager_task_constraint_notify_cb),
			  manager);
	g_signal_connect (task,
			  "relation_added",
			  G_CALLBACK (task_manager_task_relation_added_cb),
			  manager);
	g_signal_connect (task,
			  "relation_removed",
			  G_CALLBACK (task_manager_task_relation_removed_cb),
			  manager);
	g_signal_connect (task,
			  "assignment_added",
			  G_CALLBACK (task_manager_task_assignment_added_cb),
			  manager);	
	g_signal_connect (task,
			  "assignment_removed",
			  G_CALLBACK (task_manager_task_assignment_removed_cb),
			  manager);	
}

/**
 * mrp_task_manager_insert_task:
 * @manager: A task manager
 * @parent: The parent to insert the task beneath.
 * @position: the position to place task at, with respect to its siblings.
 * If position is -1, task is inserted as the last child of parent.
 * @task: The task to insert.
 * 
 * Inserts a task beneath the parent at the given position.
 * 
 **/
void
mrp_task_manager_insert_task (MrpTaskManager *manager,
			      MrpTask        *parent,
			      gint            position,
			      MrpTask        *task)
{
	g_return_if_fail (MRP_IS_TASK_MANAGER (manager));
	g_return_if_fail (parent == NULL || MRP_IS_TASK (parent));
	g_return_if_fail (MRP_IS_TASK (task));

	if (parent == NULL) {
		parent = manager->priv->root;
	}

	g_object_set (task,
		      "project", manager->priv->project,
		      NULL);

	imrp_task_insert_child (parent, position, task);

	manager->priv->needs_recalc = TRUE;
	
	imrp_project_task_inserted (manager->priv->project, task);

	mrp_task_manager_recalc (manager, TRUE);

	task_manager_task_connect_signals (manager, task);
}

/**
 * mrp_task_manager_remove_task:
 * @manager: A task manager
 * @task: The task to remove.
 * 
 * Removes a task, or a task subtree if the task has children. The root task
 * (with id 0) cannot be removed.
 *
 **/
void
mrp_task_manager_remove_task (MrpTaskManager *manager,
			      MrpTask        *task)
{
	g_return_if_fail (MRP_IS_TASK_MANAGER (manager));
	g_return_if_fail (MRP_IS_TASK (task));

	if (task == manager->priv->root) {
		g_warning ("Can't remove root task");
		return;
	}

	g_object_set (task,
		      "project", NULL,
		      NULL);
	
	imrp_task_remove_subtree (task);

	manager->priv->needs_rebuild = TRUE;
	mrp_task_manager_recalc (manager, FALSE);
}

static gboolean
task_manager_get_all_tasks_cb (GNode *node, GList **list)
{
	/* Don't add the root. */
	if (node->parent != NULL) {
		*list = g_list_prepend (*list, node->data);
	}
	
	return FALSE; /* don't stop the traversal */
}

/**
 * mrp_task_manager_get_all_tasks:
 * @manager: A task manager
 * 
 * Gets all the tasks in the project.
 * 
 * Return value: A list of all the MrpTasks in the project.
 **/
GList *
mrp_task_manager_get_all_tasks (MrpTaskManager *manager)
{
	GList   *tasks;

	g_return_val_if_fail (MRP_IS_TASK_MANAGER (manager), NULL);

	if (manager->priv->root == NULL) {
		return NULL;
	}
	
	tasks = NULL;

	g_node_traverse (imrp_task_get_node (manager->priv->root),
			 G_PRE_ORDER,
			 G_TRAVERSE_ALL,
			 -1,
			 (GNodeTraverseFunc) task_manager_get_all_tasks_cb,
			 &tasks);

	tasks = g_list_reverse (tasks);
	
	return tasks;
}

static gboolean
task_manager_traverse_cb (GNode *node, MrpTaskTraverseData *data)
{
	return data->func (node->data, data->user_data);
}

/**
 * mrp_task_manager_traverse:
 * @manager: A task manager
 * @root: The task to start traversing
 * @func: A function to call for each traversed task
 * @user_data: Argument to pass to the callback
 * 
 * Calls %func for the subtree starting at %root, until @func returns %TRUE.
 * 
 **/
void
mrp_task_manager_traverse (MrpTaskManager      *manager,
			   MrpTask             *root,
			   MrpTaskTraverseFunc  func,
			   gpointer             user_data)
{
	MrpTaskTraverseData data;

	g_return_if_fail (MRP_IS_TASK_MANAGER (manager));
	g_return_if_fail (MRP_IS_TASK (root));

	data.func = func;
	data.user_data = user_data;
	
	g_node_traverse (imrp_task_get_node (root),
			 G_PRE_ORDER,
			 G_TRAVERSE_ALL,
			 -1,
			 (GNodeTraverseFunc) task_manager_traverse_cb,
			 &data);
}

void
mrp_task_manager_set_root (MrpTaskManager *manager,
			   MrpTask        *task)
{
	GList      *tasks, *l;
	MrpProject *project;
	
	g_return_if_fail (MRP_IS_TASK_MANAGER (manager));
	g_return_if_fail (task == NULL || MRP_IS_TASK (task));

	if (manager->priv->root != NULL) {
		imrp_task_remove_subtree (manager->priv->root);
	}

	manager->priv->root = task;

	project = manager->priv->project;
	
	tasks = mrp_task_manager_get_all_tasks (manager);
	for (l = tasks; l; l = l->next) {
		g_object_set (l->data, "project", project, NULL);
		
		task_manager_task_connect_signals (manager, l->data);
	}

	mrp_task_manager_recalc (manager, FALSE);

	g_object_set (task,
		      "project", project,
		      NULL);

	g_list_free (tasks);
}

MrpTask *
mrp_task_manager_get_root (MrpTaskManager *manager)
{
	g_return_val_if_fail (MRP_IS_TASK_MANAGER (manager), NULL);
	
	return manager->priv->root;
}

gboolean
mrp_task_manager_move_task (MrpTaskManager  *manager,
			    MrpTask         *task,
			    MrpTask         *sibling,
			    MrpTask         *parent,
			    gboolean         before,
			    GError         **error)
{
	MrpTask *old_parent;
	gint     old_pos;
	MrpTask *grand_parent;
	
	g_return_val_if_fail (MRP_IS_TASK_MANAGER (manager), FALSE);
	g_return_val_if_fail (MRP_IS_TASK (task), FALSE);
	g_return_val_if_fail (sibling == NULL || MRP_IS_TASK (sibling), FALSE);
	g_return_val_if_fail (MRP_IS_TASK (parent), FALSE);

	old_parent = mrp_task_get_parent (task);
	old_pos = mrp_task_get_position (task);

	grand_parent = mrp_task_get_parent (old_parent);

	imrp_task_detach (task);

	/* If we change parents, we need to special case a bit:
	 *
	 * 1. We rebuild the sorted tree completely, because it's easier than to
	 *    figure out what to change.
	 *
	 * 2. We must check for loops.
	 *
	 * 2a. Unless, we "unindent", i.e. just change depth one level. That can
	 *     never result in a loop and if we did check it would prevent
	 *     unindenting in some cases.
	 */
	if (parent != old_parent) {
		task_manager_unlink_sorted_tree (manager);
		
		if (parent != grand_parent &&
		    !mrp_task_manager_check_move (manager,
						  task,
						  parent,
						  error)) {
			imrp_task_reattach_pos (task, old_pos, old_parent);
			
			mrp_task_manager_rebuild (manager);
			
			return FALSE;
		}
	}

	imrp_task_reattach (task, sibling, parent, before);

	/* If we changed parents, we must rebuild, since we unlinked the sorted
	 * tree above.
	 */
	if (parent != old_parent) {
		mrp_task_manager_rebuild (manager);
	}
	
	imrp_project_task_moved (manager->priv->project,
				 task);
	
	mrp_task_manager_recalc (manager, FALSE);

	return TRUE;
}

/* ----------------------------------------------------------------------------
 * Test code. Remove at some stage, or move to test framework.
 */

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

		if (MRP_IS_TASK (task)) {
			g_object_get (task, "name", &name, NULL);
			g_print ("%sName: %s   ", padding, name);
			g_free (name);

			if (imrp_task_peek_predecessors (task)) {
				GList *l;
				g_print (" <-[");
				for (l = imrp_task_peek_predecessors (task); l; l = l->next) {
					MrpTask *predecessor = mrp_relation_get_predecessor (l->data);

					if (MRP_IS_TASK (predecessor)) {
						g_object_get (predecessor, "name", &name, NULL);
						g_print ("%s, ", name);
						g_free (name);
					} else {
						g_print ("<unknown>, ");
					}
				}
				g_print ("]");
			}

			if (imrp_task_peek_successors (task)) {
				GList *l;
				g_print (" ->[");
				for (l = imrp_task_peek_successors (task); l; l = l->next) {
					MrpTask *successor = mrp_relation_get_successor (l->data);

					if (MRP_IS_TASK (successor)) {
						g_object_get (successor, "name", &name, NULL);
						g_print ("%s, ", name);
						g_free (name);
					} else {
						g_print ("<unknown>, ");
					}
				}
				g_print ("]");
			}

			g_print ("\n");
		} else {
			g_print ("%s<unknown>\n", padding); 
		}
		
		dump_children (child, depth + 1);
	}

	g_free (padding);
}

static void
task_manager_dump_task_tree (GNode *node)
{
	g_return_if_fail (node != NULL);
	g_return_if_fail (node->parent == NULL);
	
	g_print ("------------------------------------------\n<Root>\n");

	dump_children (node, 1);

	g_print ("\n");
}

void
mrp_task_manager_dump_task_tree (MrpTaskManager *manager)
{
	g_return_if_fail (MRP_IS_TASK_MANAGER (manager));
	g_return_if_fail (manager->priv->root);

	task_manager_dump_task_tree (imrp_task_get_node (manager->priv->root));
}

void
mrp_task_manager_dump_task_list (MrpTaskManager *manager)
{
	GList *list, *l;
	gchar *name;

	g_return_if_fail (MRP_IS_TASK_MANAGER (manager));
	g_return_if_fail (manager->priv->root);

	g_print ("All tasks: ");
	list = mrp_task_manager_get_all_tasks (manager);
	for (l = list; l; l = l->next) {
		if (l != list) {
			g_print (", ");
		}

		if (MRP_IS_TASK (l->data)) {
			g_object_get (l->data, "name", &name, NULL);
			g_print ("%s", name);
			g_free (name);
		} else {
			g_print ("<unknown>");
		}
	}
	g_print ("\n");

	g_list_free (list);
}

/* ------------------------------------------------------------------------ */

static gboolean
task_manager_unlink_tree_recursive_cb (GNode *node, gpointer data)
{
	g_node_unlink (node);

	return FALSE;
}
	
static void
task_manager_unlink_sorted_tree (MrpTaskManager *manager)
{
	GNode *root;

	root = imrp_task_get_sorted_node (manager->priv->root);

	g_node_traverse (root,
			 G_POST_ORDER,
			 G_TRAVERSE_ALL,
			 -1,
			 (GNodeTraverseFunc) task_manager_unlink_tree_recursive_cb,
			 NULL);
}

/* Get the ancestor of task_a, that has the same parent as an ancestor or
 * task_b.
 */
static MrpTask *
task_manager_get_ancestor_with_same_parent (MrpTask *task_a, MrpTask *task_b)
{
	MrpTask *ancestor;
	gint     depth_a, depth_b, i;

	depth_a = imrp_task_get_depth (task_a);
	depth_b = imrp_task_get_depth (task_b);

	if (depth_a > depth_b) {
		for (i = depth_a; i > depth_b; i--) {
			task_a = mrp_task_get_parent (task_a);
		}
	}
	else if (depth_a < depth_b) {
		for (i = depth_b; i > depth_a; i--) {
			task_b = mrp_task_get_parent (task_b);
		}
	}
	
	ancestor = NULL;
	while (task_a != NULL && task_b != NULL) {

		if (mrp_task_get_parent (task_a) == mrp_task_get_parent (task_b)) {
			ancestor = task_a;
			break;
		}

		task_a = mrp_task_get_parent (task_a);
		task_b = mrp_task_get_parent (task_b);
	}
		
	return ancestor;
}

static void
task_manager_traverse_dependency_graph (MrpTaskManager  *manager,
					MrpTask         *task,
					GList          **output)
{
	GList       *l;
	MrpRelation *relation;
	MrpTask     *ancestor;
	MrpTask     *child;

	if (imrp_task_get_visited (task)) {
		/*g_warning ("Visited!!\n");*/
		return;
	}

	imrp_task_set_visited (task, TRUE);

	/* Follow successors. */
	for (l = imrp_task_peek_successors (task); l; l = l->next) {
		relation = l->data;

		task_manager_traverse_dependency_graph (manager,
							mrp_relation_get_successor (relation),
							output);

		/* Also follow the ancestor of the successor that has the same
		 * parent as an ancestor of the task. This way we make sure that
		 * predecessors are listed before the summary tasks of the
		 * successors.
		 */
		ancestor = task_manager_get_ancestor_with_same_parent (mrp_relation_get_successor (relation),
								       task);

		if (ancestor != NULL) {
			task_manager_traverse_dependency_graph (manager,
								ancestor,
								output);
		}
	}
	
	/* Follow parent -> child. */
	child = mrp_task_get_first_child (task);
	while (child) {
		task_manager_traverse_dependency_graph (manager, child, output);

		child = mrp_task_get_next_sibling (child);
	}

	if (task != manager->priv->root) {
		/*g_print ("Adding: %s\n", task->priv->name);*/
		*output = g_list_prepend (*output, task);
	}
}

static gboolean
task_manager_unset_visited_func (MrpTask  *task,
				 gpointer  user_data)
{
	imrp_task_set_visited (task, FALSE);

	return FALSE;
}

static void
task_manager_sort_tree (MrpTaskManager *manager)
{
	GList      *list, *l;
	GNode      *root, *node, *parent_node;
	GHashTable *hash;
	MrpTask    *task;

	/* Mark all nodes as unvisited. */
	mrp_task_manager_traverse (manager,
				   manager->priv->root,
				   task_manager_unset_visited_func,
				   NULL);
	
	list = NULL;
	task_manager_traverse_dependency_graph (manager,
						manager->priv->root,
						&list);

	hash = g_hash_table_new (NULL, NULL);

	root = imrp_task_get_sorted_node (manager->priv->root);

	/* Disconnect all the nodes. */
	task_manager_unlink_sorted_tree (manager);
	
	g_hash_table_insert (hash, manager->priv->root, root);
	
	for (l = list; l; l = l->next) {
		task = MRP_TASK (l->data);

		node = imrp_task_get_sorted_node (task);

		g_hash_table_insert (hash, task, node);

		parent_node = g_hash_table_lookup (hash,
						   mrp_task_get_parent (task));
		
		g_node_append (parent_node, node);
	}

	g_list_free (list);
	g_hash_table_destroy (hash);

	manager->priv->needs_rebuild = FALSE;
	manager->priv->needs_recalc = TRUE;
	/*dump_task_tree (root);*/
}

/* Calcluate the earliest start time that a particular predesessor relation
 * allows given.
 */
static mrptime
task_manager_calc_relation (MrpTask	*task,
			    MrpRelation	*relation,
			    MrpTask	*predecessor)
{
	switch (mrp_relation_get_relation_type (relation)) {
	case MRP_RELATION_FF:   /* finish-to-finish */
		return mrp_task_get_finish (predecessor) +
			mrp_relation_get_lag (relation) -
			(mrp_task_get_finish (task) -
			 mrp_task_get_start (task));
		
	case MRP_RELATION_SS:   /* start-to-start */
		return mrp_task_get_start (predecessor) +
			mrp_relation_get_lag (relation);
		
	case MRP_RELATION_SF:   /* start-to-finish */
		return mrp_task_get_start (predecessor) +
			mrp_relation_get_lag (relation) -
			(mrp_task_get_finish (task) -
			 mrp_task_get_start (task));
		
	case MRP_RELATION_NONE: /* unset */
	case MRP_RELATION_FS:   /* finish-to-start */
	default:
		return mrp_task_get_finish (predecessor) +
			mrp_relation_get_lag (relation);
	}
}

/* Calculate the start time of the task by finding the latest finish of it's
 * predecessors (plus any lag). Also take constraints into consideration.
 */
static mrptime
task_manager_calculate_task_start (MrpTaskManager *manager,
				   MrpTask        *task)
{
	MrpTaskManagerPriv *priv;
	MrpTask            *tmp_task;
	GList              *predecessors, *l;
	MrpRelation        *relation;
	MrpTask            *predecessor;
	mrptime             project_start;
	mrptime             start;
	mrptime             dep_start;
	MrpConstraint       constraint;

	priv = manager->priv;
	
	project_start = mrp_project_get_project_start (priv->project);
	start = project_start;

	tmp_task = task;
	while (tmp_task) {
		predecessors = imrp_task_peek_predecessors (tmp_task);
		for (l = predecessors; l; l = l->next) {
			relation = l->data;
			predecessor = mrp_relation_get_predecessor (relation);

			dep_start = task_manager_calc_relation (task,
								relation,
								predecessor);
			
			start = MAX (start, dep_start);
		}

		tmp_task = mrp_task_get_parent (tmp_task);
	}

	/* Take constraint types in consideration. */
	constraint = impr_task_get_constraint (task);
	switch (constraint.type) {
	case MRP_CONSTRAINT_SNET:
		/* Start-no-earlier-than. */
		start = MAX (start, constraint.time);
		break;

	case MRP_CONSTRAINT_MSO:
		/* Must-start-on. */
		start = MAX (project_start, constraint.time);
		break;
		
	case MRP_CONSTRAINT_ASAP:
		/* As-soon-as-possible, do nothing. */
		break;

	case MRP_CONSTRAINT_ALAP:
	case MRP_CONSTRAINT_FNLT:
	default:
		g_warning ("Constraint %d not implemented yet.", constraint.type);
		break;
	}

	return start;
}

typedef struct {
	gboolean is_start;
	mrptime  start;
	mrptime  end;
	gint     units;
} UnitsInterval;

#define UNIT_IVAL_GET_TIME(R) ((R->is_start?R->start:R->end))

static gint                                         
units_interval_sort_func (gconstpointer a, gconstpointer b)
{
	UnitsInterval *ai = *(UnitsInterval **) a;
	UnitsInterval *bi = *(UnitsInterval **) b;
	mrptime       at, bt;

	if (ai->is_start) {
		at = ai->start;
	} else {
		at = ai->end;
	}

	if (bi->is_start) {
		bt = bi->start;
	} else {
		bt = bi->end;
	}		
	
	if (at < bt) {
		return -1;
	}
	else if (at > bt) {
		return 1;
	} else {
		return 0;
	}
}

static UnitsInterval *
units_interval_new (MrpInterval *ival, gint units, gboolean is_start)
{
	UnitsInterval *unit_ival;

	unit_ival = g_new (UnitsInterval, 1);
	unit_ival->is_start = is_start;
	unit_ival->units = units;

	mrp_interval_get_absolute (ival, 0, &unit_ival->start, &unit_ival->end);

	return unit_ival;
}

/* Get all the intervals from all the assigned resource of this task, for a
 * certain day. Then we split them up in subintervals, at every point in time
 * where an interval is starting or ending.
 *
 * Then we need to merge all points that point at the same time and get the
 * total units at that point, the resulting list is the return value from this
 * function.
 */
static GList *
task_manager_get_task_units_intervals (MrpTaskManager *manager,
				       MrpTask        *task,
				       mrptime         date)
{
	MrpTaskManagerPriv *priv;
	MrpCalendar        *calendar;
	MrpDay             *day;
	GList              *ivals, *l;
	MrpInterval        *ival;
	UnitsInterval      *unit_ival, *new_unit_ival;
	GList              *unit_ivals = NULL;
	MrpAssignment      *assignment;
	MrpResource        *resource;
	GList              *assignments, *a;
	gint                units;
	mrptime             t;
	mrptime             poc;
	GPtrArray          *array;
	guint               len;
	gint                i;

	if (imrp_task_get_type (task) == MRP_TASK_TYPE_MILESTONE) {
		return NULL;
	}
	
	priv = manager->priv;

	assignments = mrp_task_get_assignments (task);

	array = g_ptr_array_new ();
	
	for (a = assignments; a; a = a->next) {
		assignment = a->data;
		
		resource = mrp_assignment_get_resource (assignment);
		units = mrp_assignment_get_units (assignment);

		calendar = mrp_resource_get_calendar (resource);
		if (!calendar) {
			calendar = mrp_project_get_calendar (priv->project);
		}

		day = mrp_calendar_get_day (calendar, date, TRUE);
		ivals = mrp_calendar_day_get_intervals (calendar, day, TRUE);

		for (l = ivals; l; l = l->next) {
			ival = l->data;

			/* Start of the interval. */
			unit_ival = units_interval_new (ival, units, TRUE);
			g_ptr_array_add (array, unit_ival);

			/* End of the interval. */
			unit_ival = units_interval_new (ival, units, FALSE);
			g_ptr_array_add (array, unit_ival);
		}
	}

	/* If the task is not allocated, we handle it as if we have one resource
	 * assigned to it, 100%, using the project calendar.
	 */
	if (!assignments) {
		calendar = mrp_project_get_calendar (priv->project);
		
		day = mrp_calendar_get_day (calendar, date, TRUE);
		ivals = mrp_calendar_day_get_intervals (calendar, day, TRUE);
		
		for (l = ivals; l; l = l->next) {
			ival = l->data;
			
			/* Start of the interval. */
			unit_ival = units_interval_new (ival, 100, TRUE);
			g_ptr_array_add (array, unit_ival);

			/* End of the interval. */
			unit_ival = units_interval_new (ival, 100, FALSE);
			g_ptr_array_add (array, unit_ival);
		}
	}
	
	g_ptr_array_sort (array, units_interval_sort_func);

	len = array->len;

	poc = -1;
	units = 0;
	for (i = 0; i < len; i++) {
		unit_ival = g_ptr_array_index (array, i);		

		/* Get the next point of change. */
		t = UNIT_IVAL_GET_TIME (unit_ival);

		if (t != poc) {
			/* Got a new point of change, the previous point is
			 * determined by now.
			 */
			if (poc != -1) {
				if (units > 0) {
					new_unit_ival = g_new (UnitsInterval, 1);
					new_unit_ival->units = units;
					new_unit_ival->start = poc;
					new_unit_ival->end = t;
					
					unit_ivals = g_list_prepend (unit_ivals, new_unit_ival);
				}
			}
				
			poc = t;
		}

		if (unit_ival->is_start) {
			units += unit_ival->units;
		} else {
			units -= unit_ival->units;
		}
	}

	for (i = 0; i < array->len; i++) {
		g_free (array->pdata[i]);
	}
	
	g_ptr_array_free (array, TRUE);

	return g_list_reverse (unit_ivals);
}

/* Calculate the finish time from the work needed for the task, and the effort
 * that the allocated resources add to the task. Uses the project calendar if no
 * resources are allocated.
 */
static mrptime
task_manager_calculate_task_finish (MrpTaskManager *manager,
				    MrpTask        *task,
				    mrptime         start,
				    gint           *duration)
{
	MrpTaskManagerPriv *priv;
	mrptime             finish;
	mrptime             t;
	mrptime             t1, t2;
	mrptime             work_start;
	gint                work;
	gint                effort;
	gint                delta;
	GList              *unit_ivals, *l;
	UnitsInterval      *unit_ival;
	MrpTaskType         type;
	MrpTaskSched        sched;
	
	priv = manager->priv;

	if (task == priv->root) {
		g_warning ("Tried to get duration of root task.");
		return 0;
	}

	type = imrp_task_get_type (task);

	if (0 && type == MRP_TASK_TYPE_MILESTONE) {
		if (duration) {
			*duration = 0;
		}
		return start;
	}
	
	work = mrp_task_get_work (task);

	sched = imrp_task_get_sched (task);
	if (sched == MRP_TASK_SCHED_FIXED_WORK) {
		*duration = 0;
	} else {
		*duration = mrp_task_get_duration (task);
	}
	
	effort = 0;

	finish = start;
	work_start = -1;
	
	t = mrp_time_align_day (start);

	while (1) {
		unit_ivals = task_manager_get_task_units_intervals (manager, task, t);

		/* If we don't get anywhere in 100 days, then the calendar must
		 * be broken, so we abort the scheduling of this task. It's not
		 * the best solution but fixes the issue for now.
		 */
		if (effort == 0 && t - start > (60*60*24*100)) {
			break;
		}

		if (!unit_ivals) {
			t += 60*60*24;
			continue;
		}
		
		for (l = unit_ivals; l; l = l->next) { 
			unit_ival = l->data;

			t1 = t + unit_ival->start;
			t2 = t + unit_ival->end;
			
			/* Skip any intervals before the task starts. */
			if (t2 < start) {
				continue;
			}

			/* Don't add time before the start of the task. */
			t1 = MAX (t1, start);

			if (t1 == t2) {
				continue;
			}
			
			if (work_start == -1) {
				work_start = t1;
			}

			/* Effort added by this interval. */
			if (sched == MRP_TASK_SCHED_FIXED_WORK) {
				delta = floor (0.5 + (double) unit_ival->units * (t2 - t1) / 100.0);

				*duration += (t2 - t1);
				
				if (effort + delta >= work) {
					finish = t1 + floor (0.5 + (work - effort) / unit_ival->units * 100.0);

					/* Subtract the spill. */
					*duration -= floor (0.5 + (effort + delta - work) / unit_ival->units * 100.0);

					goto done;
				}
			}
			else if (sched == MRP_TASK_SCHED_FIXED_DURATION) {
				delta = t2 - t1;
				
				if (effort + delta >= *duration) {
					/* Done, make sure we don't spill. */
					finish = t1 + *duration - effort;
					goto done;
				}
			} else {
				delta = 0;
				g_assert_not_reached ();
			}
			
			effort += delta;
		}
		
		t += 60*60*24;
	}

 done:

	if (work_start == -1) {
		work_start = start;
	}

	if (type == MRP_TASK_TYPE_MILESTONE) {
		imrp_task_set_work_start (task, start);
	} else { 
		imrp_task_set_work_start (task, work_start);
	}
	
	g_list_foreach (unit_ivals, (GFunc) g_free, NULL);
	g_list_free (unit_ivals);

	return finish;
}

static void
task_manager_do_forward_pass (MrpTaskManager *manager,
			      MrpTask        *task,
			      mrptime        *start,
			      mrptime        *finish,
			      mrptime        *work_start)
{
	MrpTaskManagerPriv *priv;
	GNode              *child;
	mrptime             sub_start, sub_work_start, sub_finish;
	mrptime             old_start, old_finish;
	mrptime             new_start, new_finish;
	mrptime             tmp_time;
	gint                duration;
	gint                old_duration;
	gint                work;
	mrptime             t1, t2;
	MrpTaskSched        sched;

	priv = manager->priv;
	
	old_start = mrp_task_get_start (task);
	old_finish = mrp_task_get_finish (task);
	old_duration = old_finish - old_start;

	/*{ gchar *name; g_object_get (task, "name", &name, NULL);
	g_print ("task %s\n", name);
	}*/
	
	if (g_node_n_children (imrp_task_get_sorted_node (task)) > 0) {
		/* Summary task. */
		sub_start = -1;
		sub_work_start = -1;
		sub_finish = -1;
		
		child = g_node_first_child (imrp_task_get_sorted_node (task));
		for (; child; child = g_node_next_sibling (child)) {
			task_manager_do_forward_pass (manager,
						      child->data,
						      &sub_start,
						      &sub_finish,
						      &sub_work_start);
		}
		
		/* Now the whole subtree has been traversed, set start/finish
		 * from the children.
		 */
		imrp_task_set_start (task, sub_start);
		imrp_task_set_work_start (task, sub_work_start);
		imrp_task_set_finish (task, sub_finish);

		/* FIXME: should work be the sum of the work of the children
		 * here? The problem is that summary tasks really display
		 * duration in the work field. That's why we set duration to the
		 * same value as work below.
		 */
		
		work = mrp_task_manager_calculate_task_work (manager,
							     task,
							     sub_start,
							     sub_finish);
		imrp_task_set_work (task, work);
		imrp_task_set_duration (task, work);
	} else {
		/* Non-summary task. */
		
		t1 = task_manager_calculate_task_start (manager, task);
		t2 = task_manager_calculate_task_finish (manager, task, t1, &duration);
		
		imrp_task_set_start (task, t1);
		imrp_task_set_finish (task, t2);

		sched = imrp_task_get_sched (task);
		if (sched == MRP_TASK_SCHED_FIXED_WORK) {
			imrp_task_set_duration (task, duration);
		} else {
			duration = mrp_task_get_duration (task);
			work = mrp_task_get_work (task);
			
			/* Update resource units for fixed duration. */
			if (duration > 0) {
				GList         *assignments, *a;
				MrpAssignment *assignment;
				gint           n, units;
				
				assignments = mrp_task_get_assignments (task);
				
				n = g_list_length (assignments);
				units = floor (0.5 + 100.0 * (gdouble) work / duration / n);
				
				for (a = assignments; a; a = a->next) {
					assignment = a->data;
					
					g_signal_handlers_block_by_func (assignment,
									 task_manager_assignment_units_notify_cb,
									 manager);
					
					g_object_set (assignment, "units", units, NULL);
					
					g_signal_handlers_unblock_by_func (assignment,
									   task_manager_assignment_units_notify_cb,
									   manager);
					
					
				}
			}
		}		
	}
	
	new_start = mrp_task_get_start (task);
	if (old_start != new_start) {
		g_object_notify (G_OBJECT (task), "start");
	}
	
	new_finish = mrp_task_get_finish (task);
	if (old_finish != new_finish) {
		g_object_notify (G_OBJECT (task), "finish");
	}

	if (old_duration != (new_finish - new_start)) {
		g_object_notify (G_OBJECT (task), "duration");
	}
	
	if (*start == -1) {
		*start = new_start;
	} else {
		*start = MIN (*start, new_start);
	}
	
	if (*finish == -1) {
		*finish = new_finish;
	} else {
		*finish = MAX (*finish, new_finish);
	}

	tmp_time = mrp_task_get_work_start (task);
	if (*work_start == -1) {
		*work_start = tmp_time;
	} else {
		*work_start = MIN (*work_start, tmp_time);
	}
}

typedef struct {
	GSList  *list;
	MrpTask *root;
} GetSortedData;

static gboolean
traverse_get_sorted_tasks (GNode         *node,
			   GetSortedData *data)
{
	if (node->data != data->root) {
		data->list = g_slist_prepend (data->list, node->data);
	}
	
	return FALSE;
}

static void
task_manager_do_backward_pass (MrpTaskManager *manager)
{
	MrpTaskManagerPriv *priv;
	GNode              *root;
	GetSortedData       data;
	GSList             *tasks, *l;
	GList              *successors, *s;
	mrptime             project_finish;
	mrptime             t1, t2;
	gint                duration;
	
	priv = manager->priv;

	root = imrp_task_get_sorted_node (priv->root);

	data.list = NULL;
	data.root = root->data;

	g_node_traverse (root,
			 G_POST_ORDER,
			 G_TRAVERSE_ALL,
			 -1,
			 (GNodeTraverseFunc) traverse_get_sorted_tasks,
			 &data);

	tasks = data.list;

	project_finish = mrp_task_get_finish (priv->root);
	
	for (l = tasks; l; l = l->next) {
		MrpTask *task = l->data;
		MrpTask *parent = mrp_task_get_parent (task);

		if ((NULL == parent) || (parent == manager->priv->root)) {
			t1 = project_finish;
		} else {
			t1 = MIN (project_finish, mrp_task_get_latest_finish (parent));
		}

		successors = imrp_task_peek_successors (task);
		for (s = successors; s; s = s->next) {
			MrpRelation *relation;
			MrpTask     *successor;

			relation = s->data;
			successor = mrp_relation_get_successor (relation);
			
			t2 = mrp_task_get_latest_start (successor) -
				mrp_relation_get_lag (relation);
			
			t1 = MIN (t1, t2);
		}

		imrp_task_set_latest_finish (task, t1);

		/* Use the calendar duration to get the actual latest start, or
		 * calendars will make this break.
		 */
		duration = mrp_task_get_finish (task) - mrp_task_get_start (task);
		t1 -= duration;
		imrp_task_set_latest_start (task, t1);
			
		t2 = mrp_task_get_start (task);

		g_object_set (task, "critical", t1 == t2, NULL);
	}

	g_slist_free (tasks);
}

void
mrp_task_manager_set_block_scheduling (MrpTaskManager *manager, gboolean block)
{
	MrpTaskManagerPriv *priv;

	g_return_if_fail (MRP_IS_TASK_MANAGER (manager));

	priv = manager->priv;
	
	if (priv->block_scheduling == block) {
		return;
	}
	
	priv->block_scheduling = block;

	if (!block) {
		/* FIXME: why do we need two recalcs here, see bug #500. */
		mrp_task_manager_recalc (manager, TRUE);
		mrp_task_manager_recalc (manager, TRUE);
	}
}

void
mrp_task_manager_rebuild (MrpTaskManager *manager)
{
	MrpTaskManagerPriv *priv;

	g_return_if_fail (MRP_IS_TASK_MANAGER (manager));
	g_return_if_fail (manager->priv->root != NULL);

	priv = manager->priv;
	
	if (priv->block_scheduling) {
		return;
	}

	task_manager_sort_tree (manager);
	priv->needs_rebuild = FALSE;
	priv->needs_recalc = TRUE;
}

void
mrp_task_manager_recalc (MrpTaskManager *manager,
			 gboolean        force)
{
	MrpTaskManagerPriv *priv;
	MrpProject         *project;
	mrptime             start, finish, work_start;

	g_return_if_fail (MRP_IS_TASK_MANAGER (manager));
	g_return_if_fail (manager->priv->root != NULL);

	priv = manager->priv;

	if (priv->block_scheduling) {
		return;
	}
	
	priv->needs_recalc |= force;
	
	if (!priv->needs_recalc && !priv->needs_rebuild) {
		return;
	}
	
	/* If we don't have any children yet, or if the root is not inserted
	 * properly into the project yet, just postpone the recalc.
	 */
	if (mrp_task_get_n_children (priv->root) == 0) {
		return;
	}

	g_object_get (priv->root, "project", &project, NULL);
	if (!project) {
		return;
	}
	
	if (priv->needs_rebuild) {
		mrp_task_manager_rebuild (manager);
	}

	start = MRP_TIME_MAX;
	work_start = MRP_TIME_MAX;
	finish = 0;
	
	task_manager_do_forward_pass (manager, priv->root, &start, &finish, &work_start);
	task_manager_do_backward_pass (manager);

	priv->needs_recalc = FALSE;
}

static void
task_manager_task_duration_notify_cb (MrpTask        *task,
				      GParamSpec     *spec,
				      MrpTaskManager *manager)
{
	mrp_task_manager_recalc (manager, TRUE);
}

static void
task_manager_task_constraint_notify_cb (MrpTask        *task,
					GParamSpec     *spec,
					MrpTaskManager *manager)
{
	mrp_task_manager_recalc (manager, TRUE);
}

static void
task_manager_project_start_notify_cb (MrpProject     *project,
				      GParamSpec     *spec,
				      MrpTaskManager *manager)
{
	mrp_task_manager_recalc (manager, TRUE);
}

static void
task_manager_task_relation_notify_cb (MrpRelation    *relation,
				      GParamSpec     *spec,
				      MrpTaskManager *manager)
{
	mrp_task_manager_recalc (manager, TRUE);	
}

static void
task_manager_assignment_units_notify_cb (MrpAssignment  *assignment,
					 GParamSpec     *spec,
					 MrpTaskManager *manager)
{
	mrp_task_manager_recalc (manager, TRUE);	
}

static void
task_manager_task_relation_added_cb (MrpTask        *task,
				     MrpRelation    *relation,
				     MrpTaskManager *manager)
{
	/* We get a signal on both the predecessor and the successor, it's
	 * enough with one rebuild.
	 */
	if (task == mrp_relation_get_predecessor (relation)) {
		return;
	}
	g_signal_connect_object (relation,
				 "notify",
				 G_CALLBACK (task_manager_task_relation_notify_cb),
				 manager, 0);

	manager->priv->needs_rebuild = TRUE;
	mrp_task_manager_recalc (manager, FALSE);
}

static void
task_manager_task_relation_removed_cb (MrpTask        *task,
				       MrpRelation    *relation,
				       MrpTaskManager *manager)
{
	/* We get a signal on both the predecessor and the successor, it's
	 * enough with one rebuild.
	 */
	if (task == mrp_relation_get_predecessor (relation)) {
		return;
	}
	
	g_signal_handlers_disconnect_by_func (relation,
					      task_manager_task_relation_notify_cb,
					      manager);
	
	manager->priv->needs_rebuild = TRUE;
	mrp_task_manager_recalc (manager, FALSE);
}

static void
task_manager_task_assignment_added_cb (MrpTask        *task,
				       MrpAssignment  *assignment,
				       MrpTaskManager *manager)
{
	g_signal_connect_object (assignment, "notify::units",
				 G_CALLBACK (task_manager_assignment_units_notify_cb),
				 manager, 0);
		
	manager->priv->needs_rebuild = TRUE;
	mrp_task_manager_recalc (manager, FALSE);
}

static void
task_manager_task_assignment_removed_cb (MrpTask        *task,
					 MrpAssignment  *assignment,
					 MrpTaskManager *manager)
{
	g_signal_handlers_disconnect_by_func (assignment, 
					      task_manager_assignment_units_notify_cb,
					      manager);
	
	manager->priv->needs_rebuild = TRUE;
	mrp_task_manager_recalc (manager, FALSE);
}

static gboolean
task_manager_check_successor_loop (MrpTask *task, MrpTask *end_task)
{
	GList       *successors, *l;
	MrpRelation *relation;
	MrpTask     *child;

	if (task == end_task) {
		return FALSE;
	}
	
	successors = imrp_task_peek_successors (task);
	for (l = successors; l; l = l->next) {
		relation = l->data;
		if (!task_manager_check_successor_loop (mrp_relation_get_successor (relation),
							end_task)) {
			return FALSE;
		}
	}

	child = mrp_task_get_first_child (task);
	while (child) {
		if (!task_manager_check_successor_loop (child, end_task)) {
			return FALSE;
		}
		
		child = mrp_task_get_next_sibling (child);
	}
	
	return TRUE;
}

static gboolean
task_manager_check_predecessor_loop (MrpTask *task, MrpTask *end_task)
{
	GList       *predecessors, *l;
	MrpRelation *relation;
	MrpTask     *child;

	if (task == end_task) {
		return FALSE;
	}
	
	predecessors = imrp_task_peek_predecessors (task);
	for (l = predecessors; l; l = l->next) {
		relation = l->data;
		if (!task_manager_check_predecessor_loop (mrp_relation_get_predecessor (relation),
							  end_task)) {
			return FALSE;
		}
	}

	child = mrp_task_get_first_child (task);
	while (child) {
		if (!task_manager_check_predecessor_loop (child, end_task)) {
			return FALSE;
		}
		
		child = mrp_task_get_next_sibling (child);
	}
	
	return TRUE;
}

gboolean
mrp_task_manager_check_predecessor (MrpTaskManager  *manager,
				    MrpTask         *task,
				    MrpTask         *predecessor,
				    GError         **error)
{
	gint     depth_task, depth_predecessor;
	gint     i;
	MrpTask *task_ancestor;
	MrpTask *predecessor_ancestor;

	g_return_val_if_fail (MRP_IS_TASK_MANAGER (manager), FALSE);
	g_return_val_if_fail (MRP_IS_TASK (task), FALSE);
	g_return_val_if_fail (MRP_IS_TASK (predecessor), FALSE);

	/* "Inbreed" check, i.e. a relation between a task and it's ancestor. */
	depth_task = imrp_task_get_depth (task);
	depth_predecessor = imrp_task_get_depth (predecessor);

	task_ancestor = task;
	predecessor_ancestor = predecessor;

	if (depth_task < depth_predecessor) {
		for (i = depth_predecessor; i > depth_task; i--) {
			predecessor_ancestor = mrp_task_get_parent (predecessor_ancestor);
		}
	} else if (depth_predecessor < depth_task) {
		for (i = depth_task; i > depth_predecessor; i--) {
			task_ancestor = mrp_task_get_parent (task_ancestor);
		}
	}

	if (task_ancestor == predecessor_ancestor) {
		g_set_error (error,
			     MRP_ERROR,
			     MRP_ERROR_TASK_RELATION_FAILED,
			     _("Can not add a predecessor relation between a task and its ancestor."));
		return FALSE;
	}

	/* Loop checking. */
	if (!task_manager_check_successor_loop (task, predecessor)) {
		g_set_error (error,
			     MRP_ERROR,
			     MRP_ERROR_TASK_RELATION_FAILED,
			     _("Can not add a predecessor, because it would result in a loop."));
		return FALSE;
	}

	if (!task_manager_check_predecessor_loop (predecessor, task)) {
		g_set_error (error,
			     MRP_ERROR,
			     MRP_ERROR_TASK_RELATION_FAILED,
			     _("Can not add a predecessor, because it would result in a loop."));
		return FALSE;
	}

	return TRUE;
}

gboolean
mrp_task_manager_check_move (MrpTaskManager  *manager,
			     MrpTask         *task,
			     MrpTask         *parent,
			     GError         **error)
{
	gboolean success;
	
	g_return_val_if_fail (MRP_IS_TASK_MANAGER (manager), FALSE);
	g_return_val_if_fail (MRP_IS_TASK (task), FALSE);
	g_return_val_if_fail (MRP_IS_TASK (parent), FALSE);

	/* If there is a link between the task's subtree and the new parent or
	 * any of its ancestors, the move will result in a loop.
	 */
	success = mrp_task_manager_check_predecessor (manager,
						      task,
						      parent,
						      NULL) &&
		mrp_task_manager_check_predecessor (manager,
						    parent,
						    task,
						    NULL);

	if (!success) {
		g_set_error (error,
			     MRP_ERROR,
			     MRP_ERROR_TASK_MOVE_FAILED,
			     _("Can not move the task, since it would result in a loop."));
		return FALSE;
	}

	return TRUE;
}

static gint
task_manager_get_work_for_calendar (MrpTaskManager *manager,
				    MrpCalendar    *calendar,
				    mrptime         start,
				    mrptime         finish)
{
	MrpTaskManagerPriv *priv;
	mrptime             t;
	mrptime             t1, t2;
	gint                work;
	MrpDay             *day;
	GList              *ivals, *l;
	MrpInterval        *ival;

	priv = manager->priv;

	work = 0;

	/* Loop through the intervals of the calendar and add up the work, until
	 * the finish time is hit.
	 */
	t = mrp_time_align_day (start);

	while (t < finish) {
		day = mrp_calendar_get_day (calendar, t, TRUE);
		ivals = mrp_calendar_day_get_intervals (calendar, day, TRUE);

		for (l = ivals; l; l = l->next) {
			ival = l->data;
			
			mrp_interval_get_absolute (ival, t, &t1, &t2);
			
			/* Skip intervals that are before the task. */
			if (t2 < start) {
				continue;
			}
			
			/* Stop if the interval starts after the task. */
			if (t1 >= finish) {
				break;
			}
			
			/* Don't add time outside the task. */
			t1 = MAX (t1, start);
			t2 = MIN (t2, finish);
			
			work += t2 - t1;
		}
		
		t += 24*60*60;
	}

	return work;
}

/* Calculate the work needed to achieve the specified start and finish, with the
 * allocated resources' calendars in consideration.
 */
gint
mrp_task_manager_calculate_task_work (MrpTaskManager *manager,
				      MrpTask        *task,
				      mrptime         start,
				      mrptime         finish)
{
	MrpTaskManagerPriv *priv;
	gint                work = 0;
	MrpAssignment      *assignment;
	MrpResource        *resource;
	GList              *assignments, *a;
	MrpCalendar        *calendar;

	priv = manager->priv;

	if (task == priv->root) {
		return 0;
	}

	if (start == -1) {
		/* FIXME: why did we use task_manager_calculate_task_start
		 * (manager, task) here?? Shouldn't be needed...
		 */
		start = mrp_task_get_start (task);
	}

	if (finish <= start) {
		return 0;
	}
	
	/* Loop through the intervals of the assigned resources' calendars (or
	 * the project calendar if no resources are assigned), and add up the
	 * work.
	 */

	assignments = mrp_task_get_assignments (task);
	for (a = assignments; a; a = a->next) {
		assignment = a->data;

		resource = mrp_assignment_get_resource (assignment);

		calendar = mrp_resource_get_calendar (resource);
		if (!calendar) {
			calendar = mrp_project_get_calendar (priv->project);
		}

		work += task_manager_get_work_for_calendar (manager,
							    calendar,
							    start,
							    finish);
	}
	
	if (!assignments) {
		calendar = mrp_project_get_calendar (priv->project);

		work = task_manager_get_work_for_calendar (manager,
							   calendar,
							   start,
							   finish);
	}

	return work;
}
