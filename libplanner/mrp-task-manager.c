/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2004-2005 Imendio AB
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
#include <glib/gi18n.h>
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
	
	/* Whether the dependency graph is valid or needs to be rebuilt. */
	gboolean    needs_rebuild;

	/* Whether the task tree needs to be recalculated. */
	gboolean    needs_recalc;
	gboolean    in_recalc;

	GList      *depencency_list;
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

	/* FIXME: implement adding the task to the dependency graph instead. */
	manager->priv->needs_rebuild = TRUE;

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

	if (!mrp_task_manager_check_move (manager,
					  task,
					  parent,
					  error)) {
		return FALSE;
	}

	imrp_task_detach (task);
	imrp_task_reattach (task, sibling, parent, before);

	mrp_task_invalidate_cost (old_parent);
	mrp_task_invalidate_cost (parent);

	mrp_task_manager_rebuild (manager);
	
	imrp_project_task_moved (manager->priv->project, task);
	
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
	GNode       *child;
	gchar       *padding;
	MrpTask     *task;
	const gchar *name;

	padding = get_n_chars (2 * depth, ' ');
	
	for (child = g_node_first_child (node); child; child = g_node_next_sibling (child)) {
		task = (MrpTask *) child->data;

		if (MRP_IS_TASK (task)) {
			name = mrp_task_get_name (task);
			g_print ("%sName: %s   ", padding, name);

			if (imrp_task_peek_predecessors (task)) {
				GList *l;
				g_print (" <-[");
				for (l = imrp_task_peek_predecessors (task); l; l = l->next) {
					MrpTask *predecessor = mrp_relation_get_predecessor (l->data);

					if (MRP_IS_TASK (predecessor)) {
						name = mrp_task_get_name (predecessor);
						g_print ("%s, ", name);
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
						name = mrp_task_get_name (successor);
						g_print ("%s, ", name);
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
	GList       *list, *l;
	const gchar *name;

	g_return_if_fail (MRP_IS_TASK_MANAGER (manager));
	g_return_if_fail (manager->priv->root);

	g_print ("All tasks: ");
	list = mrp_task_manager_get_all_tasks (manager);
	for (l = list; l; l = l->next) {
		if (l != list) {
			g_print (", ");
		}

		if (MRP_IS_TASK (l->data)) {
			name = mrp_task_get_name (l->data);
			g_print ("%s", name);
		} else {
			g_print ("<unknown>");
		}
	}
	g_print ("\n");

	g_list_free (list);
}

/* ------------------------------------------------------------------------ */


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
		g_print ("Adding: %s\n", mrp_task_get_name (task));
		*output = g_list_prepend (*output, task);
	}
}

static void
dump_task_node (MrpTask *task)
{
	MrpTaskGraphNode *node;
	GList            *l;
	
	node = imrp_task_get_graph_node (task);

	g_print ("Task: %s\n", mrp_task_get_name (task));
	
	for (l = node->prev; l; l = l->next) {
		g_print (" from %s\n", mrp_task_get_name (l->data));
	}

	for (l = node->next; l; l = l->next) {
		g_print (" to %s\n", mrp_task_get_name (l->data));
	}
}

static void
dump_all_task_nodes (MrpTaskManager *manager)
{
	GList *tasks, *l;

	tasks = mrp_task_manager_get_all_tasks (manager);
	for (l = tasks; l; l = l->next) {
		dump_task_node (l->data);
	}

	g_list_free (tasks);
}

static void
add_predecessor_to_dependency_graph_recursive (MrpTask *task,
					       MrpTask *predecessor)
{
	MrpTaskGraphNode *predecessor_node;
	MrpTask          *child;
	MrpTaskGraphNode *child_node;
	
	predecessor_node = imrp_task_get_graph_node (predecessor);

	child = mrp_task_get_first_child (task);
	while (child) {
		child_node = imrp_task_get_graph_node (child);
		
		child_node->prev = g_list_append (child_node->prev, predecessor);
		predecessor_node->next = g_list_append (predecessor_node->next, child);
		
		if (mrp_task_get_n_children (child) > 0) {
			add_predecessor_to_dependency_graph_recursive (child, predecessor);
		}
		
		child = mrp_task_get_next_sibling (child);
	}
}

static void
add_predecessor_to_dependency_graph (MrpTaskManager *manager,
				     MrpTask        *task,
				     MrpTask        *predecessor)
{
	MrpTaskManagerPriv *priv;
	MrpTaskGraphNode   *task_node;
	MrpTaskGraphNode   *predecessor_node;
	
	priv = manager->priv;
	
	predecessor_node = imrp_task_get_graph_node (predecessor);

	task_node = imrp_task_get_graph_node (task);

	task_node->prev = g_list_append (task_node->prev, predecessor);
	predecessor_node->next = g_list_append (predecessor_node->next, task);

	/* Add dependencies from the predecessor to the task's children,
	 * recursively.
	 */
	add_predecessor_to_dependency_graph_recursive (task, predecessor);
}

static void
remove_predecessor_from_dependency_graph_recursive (MrpTask *task,
						    MrpTask *predecessor)
{
	MrpTaskGraphNode *predecessor_node;
	MrpTask          *child;
	MrpTaskGraphNode *child_node;
	
	predecessor_node = imrp_task_get_graph_node (predecessor);

	child = mrp_task_get_first_child (task);
	while (child) {
		child_node = imrp_task_get_graph_node (child);
		
		child_node->prev = g_list_remove (child_node->prev, predecessor);
		predecessor_node->next = g_list_remove (predecessor_node->next, child);
		
		if (mrp_task_get_n_children (child) > 0) {
			remove_predecessor_from_dependency_graph_recursive (child, predecessor);
		}
		
		child = mrp_task_get_next_sibling (child);
	}
}

static void
remove_predecessor_from_dependency_graph (MrpTaskManager *manager,
					  MrpTask        *task,
					  MrpTask        *predecessor)
{
	MrpTaskManagerPriv *priv;
	MrpTaskGraphNode   *task_node;
	MrpTaskGraphNode   *predecessor_node;
	
	priv = manager->priv;
	
	predecessor_node = imrp_task_get_graph_node (predecessor);

	task_node = imrp_task_get_graph_node (task);

	task_node->prev = g_list_remove (task_node->prev, predecessor);
	predecessor_node->next = g_list_remove (predecessor_node->next, task);
	
	/* Remove dependencies from the predecessor to the task's children,
	 * recursively.
	 */
	remove_predecessor_from_dependency_graph_recursive (task, predecessor);
}

static void
remove_parent_from_dependency_graph (MrpTaskManager *manager,
				     MrpTask        *task,
				     MrpTask        *parent)
{
	MrpTaskManagerPriv *priv;
	MrpTaskGraphNode   *task_node;
	MrpTaskGraphNode   *parent_node;

	priv = manager->priv;
	
	task_node = imrp_task_get_graph_node (task);
	parent_node = imrp_task_get_graph_node (parent);

	task_node->next = g_list_remove (task_node->next, parent);
	parent_node->prev = g_list_remove (parent_node->prev, task);
}

static void
add_parent_to_dependency_graph (MrpTaskManager *manager,
				MrpTask        *task,
				MrpTask        *parent)
{
	MrpTaskManagerPriv *priv;
	MrpTaskGraphNode   *task_node;
	MrpTaskGraphNode   *parent_node;

	priv = manager->priv;
	
	task_node = imrp_task_get_graph_node (task);
	parent_node = imrp_task_get_graph_node (parent);

	task_node->next = g_list_append (task_node->next, parent);
	parent_node->prev = g_list_append (parent_node->prev, task);
}

static void
remove_task_from_dependency_graph (MrpTaskManager *manager,
				   MrpTask        *task,
				   MrpTask        *parent)
{
	MrpTaskManagerPriv *priv;
	GList              *list, *l;
	MrpRelation        *relation;
	MrpTask            *predecessor;

	priv = manager->priv;
	
	/* Remove predecessors. */
	list = imrp_task_peek_predecessors (task);
	for (l = list; l; l = l->next) {
		relation = l->data;
		predecessor = mrp_relation_get_predecessor (relation);

		remove_predecessor_from_dependency_graph (manager, task, predecessor);
	}

	/* Remove the parent. */
	if (parent && parent != priv->root) {
		remove_parent_from_dependency_graph (manager, task, parent);
	}
}

static void
add_task_to_dependency_graph (MrpTaskManager *manager,
			      MrpTask        *task,
			      MrpTask        *parent)
{
	MrpTaskManagerPriv *priv;
	GList              *list, *l;
	MrpRelation        *relation;
	MrpTask            *predecessor;

	priv = manager->priv;
	
	if (task == priv->root) {
		return;
	}

	/* Add predecessors. */
	list = imrp_task_peek_predecessors (task);
	for (l = list; l; l = l->next) {
		relation = l->data;
		predecessor = mrp_relation_get_predecessor (relation);

		add_predecessor_to_dependency_graph (manager, task, predecessor);
	}
	
	/* Add the parent. */
	if (parent && parent != priv->root) {
		add_parent_to_dependency_graph (manager, task, parent);
	}
}

static gboolean
task_manager_unset_visited_func (MrpTask  *task,
				 gpointer  user_data)
{
	imrp_task_set_visited (task, FALSE);

	return FALSE;
}

static gboolean
task_manager_clean_graph_func (MrpTask  *task,
			       gpointer  user_data)
{
	MrpTaskGraphNode *node;

	imrp_task_set_visited (task, FALSE);

	node = imrp_task_get_graph_node (task);

	g_list_free (node->prev);
	node->prev = NULL;

	g_list_free (node->next);
	node->next = NULL;

	return FALSE;
}

static void
task_manager_build_dependency_graph (MrpTaskManager *manager)
{
	MrpTaskManagerPriv *priv;
	GList              *tasks;
	GList              *l;
	GList              *deps;
	MrpTask            *task;
	MrpTaskGraphNode   *node;
	GList              *queue;
	
	priv = manager->priv;

	/* Build a directed, acyclic graph, where relation links and children ->
	 * parent are graph links (children must be calculated before
	 * parents). Then do topological sorting on the graph to get the order
	 * to go through the tasks.
	 */

	mrp_task_manager_traverse (manager,
				   priv->root,
				   task_manager_clean_graph_func,
				   NULL);

	/* FIXME: Optimize by not getting all tasks but just traversing and
	 * adding them that way. Saves a constant factor.
	 */
	tasks = mrp_task_manager_get_all_tasks (manager);
	for (l = tasks; l; l = l->next) {
		add_task_to_dependency_graph (manager, l->data, mrp_task_get_parent (l->data));
	}

	/* Do a topological sort. Get the tasks without dependencies to start
	 * with.
	 */
	queue = NULL;
	for (l = tasks; l; l = l->next) {
		task = l->data;
		
		node = imrp_task_get_graph_node (task);

		if (node->prev == NULL) {
			queue = g_list_prepend (queue, task);
		}
	}

	deps = NULL;
	while (queue) {
		GList *next, *link;
		
		task = queue->data;

		link = queue;
		queue = g_list_remove_link (queue, link);

		link->next = deps;
		if (deps) {
			deps->prev = link;
		}
		deps = link;
		
		/* Remove this task from all the dependent tasks. */
		node = imrp_task_get_graph_node (task);
		for (next = node->next; next; next = next->next) {
			MrpTaskGraphNode *next_node;

			next_node = imrp_task_get_graph_node (next->data);
			next_node->prev = g_list_remove (next_node->prev, task);

			/* Add the task to the output queue if it has no
			 * dependencies left.
			 */
			if (next_node->prev == NULL) {
				queue = g_list_prepend (queue, next->data);
			}
		}
	}

	g_list_free (priv->depencency_list);
	priv->depencency_list = g_list_reverse (deps);

	g_list_free (queue);
	g_list_free (tasks);
	
	mrp_task_manager_traverse (manager,
				   priv->root,
				   task_manager_unset_visited_func,
				   NULL);

	manager->priv->needs_rebuild = FALSE;
	manager->priv->needs_recalc = TRUE;
}

/* Calcluate the earliest start time that a particular predesessor relation
 * allows given.
 */
static mrptime
task_manager_calc_relation (MrpTask	*task,
			    MrpRelation	*relation,
			    MrpTask	*predecessor)
{
	MrpRelationType type;
	mrptime         time;
	/*mrptime         start, finish;*/
	
	/* FIXME: This does not work correctly for FF and SF. The problem is
	 * that the start and finish times of task is not known at this stage,
	 * so we can't really use them.
	 */

	type = mrp_relation_get_relation_type (relation);
	
	switch (type) {
#if 0
	case MRP_RELATION_FF:
		/* finish-to-finish */
		start =  mrp_task_get_start (task);
		finish =  mrp_task_get_finish (task);

		time = mrp_task_get_finish (predecessor) +
			mrp_relation_get_lag (relation) - (finish - start);

		break;
		
	case MRP_RELATION_SF:
		/* start-to-finish */
		start = mrp_task_get_start (task);
		finish = mrp_task_get_finish (task);

		time = mrp_task_get_start (predecessor) +
			mrp_relation_get_lag (relation) - (finish - start);
		break;
#endif	
	case MRP_RELATION_SS:
		/* start-to-start */
		time = mrp_task_get_start (predecessor) +
			mrp_relation_get_lag (relation);
		break;
			
	case MRP_RELATION_FS:
	case MRP_RELATION_NONE:
	default:
		/* finish-to-start */
		time = mrp_task_get_finish (predecessor) +
			mrp_relation_get_lag (relation);
		break;
	}

	return time;
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

static void
task_manager_calculate_milestone_work_start (MrpTaskManager *manager,
					     MrpTask        *task,
					     mrptime         start)

{
	MrpTaskManagerPriv *priv;
	mrptime             t;
	mrptime             t1, t2;
	mrptime             work_start;
	GList              *unit_ivals, *l;
	UnitsInterval      *unit_ival;
	MrpTaskType         type;
	
	priv = manager->priv;

	type = mrp_task_get_task_type (task);
	g_return_if_fail (type == MRP_TASK_TYPE_MILESTONE);
	
	work_start = -1;
	
	t = mrp_time_align_day (start);

	while (1) {
		unit_ivals = task_manager_get_task_units_intervals (manager, task, t);

		/* If we don't get anywhere in 100 days, then the calendar must
		 * be broken, so we abort the scheduling of this task. It's not
		 * the best solution but fixes the issue for now.
		 */
		if (t - start > (60*60*24*100)) {
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

			work_start = t1;
			break;
		}

		if (work_start != -1) {
			break;
		}
		
		t += 60*60*24;
	}

	if (work_start == -1) {
		work_start = start;
	}
	
	imrp_task_set_work_start (task, work_start);
	
	g_list_foreach (unit_ivals, (GFunc) g_free, NULL);
	g_list_free (unit_ivals);
}

/* Calculate the finish time from the work needed for the task, and the effort
 * that the allocated resources add to the task. Uses the project calendar if no
 * resources are allocated. This function also sets the work_start property of
 * the task, which is the first time that actually has work scheduled, this can
 * differ from the start if start is inside a non-work period.
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

	/* Milestone tasks can be special cased, no duration. */
	type = mrp_task_get_task_type (task);
	if (type == MRP_TASK_TYPE_MILESTONE) {
		*duration = 0;
		task_manager_calculate_milestone_work_start (manager, task, start);
		return start;
	}
	
	work = mrp_task_get_work (task);
	sched = mrp_task_get_sched (task);

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
	imrp_task_set_work_start (task, work_start);

	g_list_foreach (unit_ivals, (GFunc) g_free, NULL);
	g_list_free (unit_ivals);

	return finish;
}

static void
task_manager_do_forward_pass_helper (MrpTaskManager *manager,
				     MrpTask        *task)
{
	MrpTaskManagerPriv *priv;
	mrptime             sub_start, sub_work_start, sub_finish;
	mrptime             old_start, old_finish;
	mrptime             new_start, new_finish;
	gint                duration;
	gint                old_duration;
	gint                work;
	mrptime             t1, t2;
	MrpTaskSched        sched;

	priv = manager->priv;
	
	old_start = mrp_task_get_start (task);
	old_finish = mrp_task_get_finish (task);
	old_duration = old_finish - old_start;
	
	if (mrp_task_get_n_children (task) > 0) {
		MrpTask *child;

		sub_start = -1;
		sub_work_start = -1;
		sub_finish = -1;
				
		child = mrp_task_get_first_child (task);
		while (child) {
			t1 = mrp_task_get_start (child);
			if (sub_start == -1) {
				sub_start = t1;
			} else {
				sub_start = MIN (sub_start, t1);
			}

			t2 = mrp_task_get_finish (child);
			if (sub_finish == -1) {
				sub_finish = t2;
			} else {
				sub_finish = MAX (sub_finish, t2);
			}
			
			t2 = mrp_task_get_work_start (child);
			if (sub_work_start == -1) {
				sub_work_start = t2;
			} else {
				sub_work_start = MIN (sub_work_start, t2);
			}

			child = mrp_task_get_next_sibling (child);
		}

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
		
		sched = mrp_task_get_sched (task);
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
}

static void
task_manager_do_forward_pass (MrpTaskManager *manager,
			      MrpTask        *start_task)
{
	MrpTaskManagerPriv *priv;
	GList              *l;

	priv = manager->priv;

	/* Do forward pass, start at the task and do all tasks that come after
	 * it in the dependency list. Note: we could try to skip tasks that are
	 * not dependent, but I don't think that's really worth it.
	*/

	if (start_task) {
		l = g_list_find (priv->depencency_list, start_task);
	} else {
		l = priv->depencency_list;
	}
	
	while (l) {
		task_manager_do_forward_pass_helper (manager, l->data);
		l = l->next;
	}

	/* FIXME: Might need to rework this if we make the forward/backward
	 * passes only recalculate tasks that depend on the changed task.
	 */
	task_manager_do_forward_pass_helper (manager, priv->root);
}

static void
task_manager_do_backward_pass (MrpTaskManager *manager)
{
	MrpTaskManagerPriv *priv;
	GList              *tasks, *l;
	GList              *successors, *s;
	mrptime             project_finish;
	mrptime             t1, t2;
	gint                duration;
	gboolean            critical;
	gboolean            was_critical;
	
	priv = manager->priv;

	project_finish = mrp_task_get_finish (priv->root);

	tasks = g_list_reverse (g_list_copy (priv->depencency_list));

	for (l = tasks; l; l = l->next) {
		MrpTask *task, *parent;

		task =  l->data;
		parent = mrp_task_get_parent (task);

		if (!parent || parent == priv->root) {
			t1 = project_finish;
		} else {
			t1 = MIN (project_finish, mrp_task_get_latest_finish (parent));
		}

		successors = imrp_task_peek_successors (task);
		for (s = successors; s; s = s->next) {
			MrpRelation *relation;
			MrpTask     *successor, *child;

			relation = s->data;
			successor = mrp_relation_get_successor (relation);
			
			child = mrp_task_get_first_child (successor);
			if (child) {
				/* If successor has children go through them
				 * instead of the successor itself.
				 */
				for (; child; child = mrp_task_get_next_sibling (child)) {
					successor = child;
					
					t2 = mrp_task_get_latest_start (successor) -
						mrp_relation_get_lag (relation);

					t1 = MIN (t1, t2);
				}
			} else {
				/* No children, check the real successor. */
				t2 = mrp_task_get_latest_start (successor) -
					mrp_relation_get_lag (relation);

				t1 = MIN (t1, t2);
			}
		}

		imrp_task_set_latest_finish (task, t1);

		/* Use the calendar duration to get the actual latest start, or
		 * calendars will make this break.
		 */
		duration = mrp_task_get_finish (task) - mrp_task_get_start (task);
		t1 -= duration;
		imrp_task_set_latest_start (task, t1);

		t2 = mrp_task_get_start (task);

		was_critical = mrp_task_get_critical (task);
		critical = (t1 == t2);

		/* FIXME: Bug in critical path for A -> B when B is SNET.
		 *
		 * The reason is that latest start for B becomes 00:00 instead
		 * of 17:00 the day before. So the slack becomes 7 hours
		 * (24-17).
		 */
#if 0
		g_print ("Task %s:\n", mrp_task_get_name (task));
		g_print ("  latest start   : "); mrp_time_debug_print (mrp_task_get_latest_start (task));
		g_print ("  latest finish  : "); mrp_time_debug_print (mrp_task_get_latest_finish (task));
	
#endif
		
		if (was_critical != critical) {
			g_object_set (task, "critical", critical, NULL);
		}
	}

	g_list_free (tasks);
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
		mrp_task_manager_recalc (manager, TRUE);
	}
}

gboolean
mrp_task_manager_get_block_scheduling (MrpTaskManager *manager)
{
	g_return_val_if_fail (MRP_IS_TASK_MANAGER (manager), FALSE);

	return manager->priv->block_scheduling;
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

	task_manager_build_dependency_graph (manager);
	
	priv->needs_rebuild = FALSE;
	priv->needs_recalc = TRUE;
}

void
mrp_task_manager_recalc (MrpTaskManager *manager,
			 gboolean        force)
{
	MrpTaskManagerPriv *priv;
	MrpProject         *project;

	g_return_if_fail (MRP_IS_TASK_MANAGER (manager));
	g_return_if_fail (manager->priv->root != NULL);

	priv = manager->priv;
	
	if (priv->block_scheduling) {
		return;
	}

	if (priv->in_recalc) {
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

	project = mrp_object_get_project (MRP_OBJECT (priv->root));
	if (!project) {
		return;
	}
	
	priv->in_recalc = TRUE;

	if (priv->needs_rebuild) {
		mrp_task_manager_rebuild (manager);
	}

	task_manager_do_forward_pass (manager, NULL);
	task_manager_do_backward_pass (manager);

	priv->needs_recalc = FALSE;
	priv->in_recalc = FALSE;
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
	mrp_task_invalidate_cost (mrp_assignment_get_task (assignment));
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
		
	mrp_task_invalidate_cost (task);
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
	
	mrp_task_invalidate_cost (task);
	manager->priv->needs_rebuild = TRUE;
	mrp_task_manager_recalc (manager, FALSE);
}

static gboolean
check_predecessor_traverse (MrpTaskManager *manager,
			    MrpTask        *task,
			    MrpTask        *end,
			    gint            length)
{
	MrpTaskGraphNode *node;
	GList            *l;

	if (length > 1 && task == end) {
		return FALSE;
	}

	/* Avoid endless loop. */
	if (imrp_task_get_visited (task)) {
		return TRUE;
	}

	imrp_task_set_visited (task, TRUE);
	
	node = imrp_task_get_graph_node (task);
	for (l = node->next; l; l = l->next) {
		if (!check_predecessor_traverse (manager, l->data, end, length + 1))
			return FALSE;
	}
	
	return TRUE;
}

gboolean
mrp_task_manager_check_predecessor (MrpTaskManager  *manager,
				    MrpTask         *task,
				    MrpTask         *predecessor,
				    GError         **error)
{
	gboolean retval;
	
	g_return_val_if_fail (MRP_IS_TASK_MANAGER (manager), FALSE);
	g_return_val_if_fail (MRP_IS_TASK (task), FALSE);
	g_return_val_if_fail (MRP_IS_TASK (predecessor), FALSE);

	if (manager->priv->needs_rebuild) {
		mrp_task_manager_rebuild (manager);
	}
	
	/* Add the predecessor to check. */
	add_predecessor_to_dependency_graph (manager, task, predecessor);
	
	if (0) {
		g_print ("--->\n");
		dump_all_task_nodes (manager);
		g_print ("<---\n");
	}
	
	mrp_task_manager_traverse (manager,
				   manager->priv->root,
				   task_manager_unset_visited_func,
				   NULL);
	
	retval = check_predecessor_traverse (manager, predecessor, predecessor, 1);

	/* Remove the predecessor again. */
	remove_predecessor_from_dependency_graph (manager, task, predecessor);
	
	if (!retval) {
		g_set_error (error,
			     MRP_ERROR,
			     MRP_ERROR_TASK_RELATION_FAILED,
			     _("Cannot add a predecessor, because it would result in a loop."));
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
	gboolean retval;
	
	g_return_val_if_fail (MRP_IS_TASK_MANAGER (manager), FALSE);
	g_return_val_if_fail (MRP_IS_TASK (task), FALSE);
	g_return_val_if_fail (MRP_IS_TASK (parent), FALSE);

	/* FIXME: Check this. */

	/* Remove the task from the old parent and add it to its new parent. */
	remove_task_from_dependency_graph (manager, task, mrp_task_get_parent (task));
	add_task_to_dependency_graph (manager, task, parent);
	
	if (0) {
		g_print ("--->\n");
		dump_all_task_nodes (manager);
		g_print ("<---\n");
	}
	
	mrp_task_manager_traverse (manager,
				   manager->priv->root,
				   task_manager_unset_visited_func,
				   NULL);
	
	retval = check_predecessor_traverse (manager, task, task, 1);

	/* Put the task back again. */
	remove_task_from_dependency_graph (manager, task, parent);
	add_task_to_dependency_graph (manager, task, mrp_task_get_parent (task));

	if (!retval) {
		g_set_error (error,
			     MRP_ERROR,
			     MRP_ERROR_TASK_MOVE_FAILED,
			     _("Cannot move the task, because it would result in a loop."));
		return FALSE;
	}

	return retval;
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
							    finish) *
			mrp_assignment_get_units (assignment) / 100;
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

