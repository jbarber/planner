/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2003-2004 Imendio HB
 * Copyright (C) 2002-2003 CodeFactory AB
 * Copyright (C) 2002-2003 Richard Hult <richard@imendio.com>
 * Copyright (C) 2002      Mikael Hallendal <micke@imendio.com>
 * Copyright (C) 2002-2004 Alvaro del Castillo <acs@barrapunto.com>
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
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtktreeview.h>
#include <gtk/gtktreeselection.h>
#include <gtk/gtkcellrenderertext.h>
#include <gtk/gtkmenu.h>
#include <gtk/gtkmain.h>
#include <gtk/gtkitemfactory.h>
#include <gtk/gtkiconfactory.h>
#include <gtk/gtkstock.h>
#include <gtk/gtkmessagedialog.h>
#include <libgnome/gnome-i18n.h>
#include "planner-format.h"
#include "planner-marshal.h"
#include "planner-cell-renderer-date.h"
#include "planner-task-dialog.h"
#include "planner-task-input-dialog.h"
#include "planner-property-dialog.h"
#include "planner-task-tree.h"
#include "planner-gantt-model.h"
#include "planner-task-popup.h"
#include "planner-task-cmd.h"

#define WARN_TASK_DIALOGS 10
#define MAX_TASK_DIALOGS 25

enum {
	SELECTION_CHANGED,
	RELATION_ADDED,
	RELATION_REMOVED,
	LAST_SIGNAL
};

struct _PlannerTaskTreePriv {
	GtkItemFactory *popup_factory;
	gboolean        custom_properties;
	MrpProject     *project;
	GHashTable     *property_to_column;

	PlannerWindow   *main_window;

	gboolean         highlight_critical;
	
	/* Keep the dialogs here so that we can just raise the dialog if it's
	 * opened twice for the same task.
	 */
	GHashTable     *task_dialogs;
	GtkTreePath    *anchor;
};

typedef struct {
	PlannerTaskTree *tree;
	MrpProperty     *property;
} ColPropertyData;

typedef enum {
	UNIT_NONE,
	UNIT_MONTH,
	UNIT_WEEK,
	UNIT_DAY,
	UNIT_HOUR,
	UNIT_MINUTE
} Unit;

typedef struct {
	gchar *name;
	Unit   unit;
} Units;

static void        task_tree_class_init                (PlannerTaskTreeClass *klass);
static void        task_tree_init                      (PlannerTaskTree      *tree);
static void        task_tree_finalize                  (GObject              *object);
static void        task_tree_setup_tree_view           (GtkTreeView          *tree,
							MrpProject           *project,
							PlannerGanttModel    *model);
static void        task_tree_add_column                (GtkTreeView          *tree,
							gint                  column,
							const gchar          *title);
static void        task_tree_name_data_func            (GtkTreeViewColumn    *tree_column,
							GtkCellRenderer      *cell,
							GtkTreeModel         *tree_model,
							GtkTreeIter          *iter,
							gpointer              data);
static void        task_tree_start_data_func           (GtkTreeViewColumn    *tree_column,
							GtkCellRenderer      *cell,
							GtkTreeModel         *tree_model,
							GtkTreeIter          *iter,
							gpointer              data);
static void        task_tree_finish_data_func          (GtkTreeViewColumn    *tree_column,
							GtkCellRenderer      *cell,
							GtkTreeModel         *tree_model,
							GtkTreeIter          *iter,
							gpointer              data);
static void        task_tree_duration_data_func        (GtkTreeViewColumn    *tree_column,
							GtkCellRenderer      *cell,
							GtkTreeModel         *tree_model,
							GtkTreeIter          *iter,
							gpointer              data);
static void        task_tree_cost_data_func            (GtkTreeViewColumn    *tree_column,
							GtkCellRenderer      *cell,
							GtkTreeModel         *tree_model,
							GtkTreeIter          *iter,
							gpointer              data);
static void        task_tree_work_data_func            (GtkTreeViewColumn    *tree_column,
							GtkCellRenderer      *cell,
							GtkTreeModel         *tree_model,
							GtkTreeIter          *iter,
							gpointer              data);
static void        task_tree_block_selection_changed   (PlannerTaskTree      *tree);
static void        task_tree_unblock_selection_changed (PlannerTaskTree      *tree);
static void        task_tree_selection_changed_cb      (GtkTreeSelection     *selection,
							PlannerTaskTree      *tree);
static void        task_tree_relation_added_cb         (MrpTask              *task,
							MrpRelation          *relation,
							PlannerTaskTree      *tree);
static void        task_tree_relation_removed_cb       (MrpTask              *task,
							MrpRelation          *relation,
							PlannerTaskTree      *tree);
static void        task_tree_row_inserted              (GtkTreeModel         *model,
							GtkTreePath          *path,
							GtkTreeIter          *iter,
							GtkTreeView          *tree);
static void        task_tree_task_added_cb             (PlannerGanttModel    *model,
							MrpTask              *task,
							PlannerTaskTree      *tree);
static void        task_tree_task_removed_cb           (PlannerGanttModel    *model,
							MrpTask              *task,
							PlannerTaskTree      *tree);
static gint        task_tree_parse_time_string         (PlannerTaskTree      *tree,
							const gchar          *str);
static MrpProject *task_tree_get_project               (PlannerTaskTree      *tree);
static MrpTask *   task_tree_get_task_from_path        (PlannerTaskTree      *tree,
                                                        GtkTreePath          *path);


static GtkTreeViewClass *parent_class = NULL;
static guint signals[LAST_SIGNAL];

/*
 * Commands
 */

typedef struct {
	PlannerCmd         base;

	PlannerTaskTree   *tree;
	MrpProject        *project;
	
	GtkTreePath       *path;
	
	gchar             *property;  
	GValue            *value;
	GValue            *old_value;
} TaskCmdEditProperty;

static gboolean
task_cmd_edit_property_do (PlannerCmd *cmd_base)
{
	TaskCmdEditProperty *cmd;
	MrpTask             *task;
	
	cmd = (TaskCmdEditProperty*) cmd_base;

	task = task_tree_get_task_from_path (cmd->tree, cmd->path);

	g_object_set_property (G_OBJECT (task),
			       cmd->property, cmd->value);

	return TRUE;
}

static void
task_cmd_edit_property_undo (PlannerCmd *cmd_base)
{
	TaskCmdEditProperty *cmd;
	MrpTask             *task;
	
	cmd = (TaskCmdEditProperty*) cmd_base;

	task = task_tree_get_task_from_path (cmd->tree, cmd->path);
	
	g_object_set_property (G_OBJECT (task),
			       cmd->property,
			       cmd->old_value);
}

static void
task_cmd_edit_property_free (PlannerCmd *cmd_base)
{
	TaskCmdEditProperty *cmd;
	
	cmd = (TaskCmdEditProperty*) cmd_base;

	g_free (cmd->property);
	g_value_unset (cmd->value);
	g_value_unset (cmd->old_value);

	g_free (cmd->value);
	g_free (cmd->old_value);
}

static PlannerCmd *
task_cmd_edit_property (PlannerTaskTree *tree,
			MrpTask         *task,
			const gchar     *property,
			const GValue    *value)
{
	PlannerTaskTreePriv *priv = tree->priv;
	PlannerCmd          *cmd_base;
	TaskCmdEditProperty *cmd;
	PlannerGanttModel   *model;

	cmd_base = planner_cmd_new (TaskCmdEditProperty,
				    _("Edit task property"),
				    task_cmd_edit_property_do,
				    task_cmd_edit_property_undo,
				    task_cmd_edit_property_free);

	cmd = (TaskCmdEditProperty *) cmd_base;

	cmd->tree = tree;
	cmd->project = task_tree_get_project (tree);

	model = PLANNER_GANTT_MODEL (gtk_tree_view_get_model (GTK_TREE_VIEW (tree)));
	
	cmd->path = planner_gantt_model_get_path_from_task (model, task);
	
	cmd->property = g_strdup (property);

	cmd->value = g_new0 (GValue, 1);
	g_value_init (cmd->value, G_VALUE_TYPE (value));
	g_value_copy (value, cmd->value);

	cmd->old_value = g_new0 (GValue, 1);
	g_value_init (cmd->old_value, G_VALUE_TYPE (value));

	g_object_get_property (G_OBJECT (task),
			       cmd->property,
			       cmd->old_value);

	planner_cmd_manager_insert_and_do (planner_window_get_cmd_manager (priv->main_window),
					   cmd_base);
	
	return cmd_base;
}

typedef struct {
	PlannerCmd       base;

	PlannerTaskTree *tree;
	MrpProject      *project;

	GtkTreePath     *path;
	MrpTask         *task;
	GList           *children;
	GList           *successors;
	GList           *predecessors;
	GList           *assignments;
} TaskCmdRemove;

static gboolean 
is_task_in_project (MrpTask *task, PlannerTaskTree *tree)
{
	PlannerGanttModel *model; 
	GtkTreePath       *path;

	model = PLANNER_GANTT_MODEL (gtk_tree_view_get_model (GTK_TREE_VIEW (tree)));
	path = planner_gantt_model_get_path_from_task (model, task);

	if (path != NULL) {
		gtk_tree_path_free (path);
		return TRUE;
	} else {	
		return FALSE;
	}
}

static void
task_cmd_save_assignments (TaskCmdRemove *cmd)
{
	GList *l;

	cmd->assignments = g_list_copy (mrp_task_get_assignments (cmd->task));
	g_list_foreach (cmd->assignments, (GFunc) g_object_ref, NULL);

	if (g_getenv ("PLANNER_DEBUG_UNDO_TASK")) {
		for (l = cmd->assignments; l; l = l->next) {
			g_message ("Assignment save %s is done by %s (%d)", 
				   mrp_task_get_name (cmd->task), 
				   mrp_resource_get_name (mrp_assignment_get_resource (l->data)),
				   mrp_assignment_get_units (l->data));		
		}
	}
}

static void
task_cmd_restore_assignments (TaskCmdRemove *cmd)
{
	GList         *l;
	MrpAssignment *assignment;
	MrpResource   *resource;		

	for (l = cmd->assignments; l; l = l->next) {
		assignment = l->data;
		resource = mrp_assignment_get_resource (assignment);

		if (g_getenv ("PLANNER_DEBUG_UNDO_TASK"))
			g_message ("Resource recover: %s is done by %s",
				   mrp_task_get_name (cmd->task),
				   mrp_resource_get_name (mrp_assignment_get_resource (l->data)));
		
		mrp_resource_assign (resource, cmd->task,
				     mrp_assignment_get_units (assignment));
	}
}


static void
task_cmd_save_relations (TaskCmdRemove *cmd)
{
	GList *l;

	cmd->predecessors = g_list_copy (mrp_task_get_predecessor_relations (cmd->task));
	g_list_foreach (cmd->predecessors, (GFunc) g_object_ref, NULL);

	if (g_getenv ("PLANNER_DEBUG_UNDO_TASK")) {
		for (l = cmd->predecessors; l; l = l->next) {
			g_message ("Predecessor save %s -> %s", 
				   mrp_task_get_name (mrp_relation_get_predecessor (l->data)), 
				   mrp_task_get_name (mrp_relation_get_successor (l->data)));
		}
	}

	cmd->successors = g_list_copy (mrp_task_get_successor_relations (cmd->task));
	g_list_foreach (cmd->successors, (GFunc) g_object_ref, NULL);

	if (g_getenv ("PLANNER_DEBUG_UNDO_TASK")) {
		for (l = cmd->successors; l; l = l->next) {
			g_message ("Successor save %s -> %s", 
				   mrp_task_get_name (mrp_relation_get_predecessor (l->data)), 
				   mrp_task_get_name (mrp_relation_get_successor (l->data)));
		}
	}
}

static void
task_cmd_restore_relations (TaskCmdRemove *cmd)
{
	GList       *l;
	MrpRelation *relation;
	MrpTask     *rel_task;
	GError      *error;
		

	for (l = cmd->predecessors; l; l = l->next) {
		relation = l->data;
		rel_task = mrp_relation_get_predecessor (relation);

		if (!is_task_in_project (rel_task, cmd->tree)) {
			continue;
		}

		if (g_getenv ("PLANNER_DEBUG_UNDO_TASK")) {
			g_message ("Predecessor recover: %s -> %s",
				   mrp_task_get_name (mrp_relation_get_predecessor (l->data)),
				   mrp_task_get_name (mrp_relation_get_successor (l->data)));
		}
		
		mrp_task_add_predecessor (cmd->task, rel_task, 
					  mrp_relation_get_relation_type (relation),
					  mrp_relation_get_lag (relation), &error);
	}

	for (l = cmd->successors; l; l = l->next) {
		relation = l->data;
		rel_task = mrp_relation_get_successor (relation);

		if (!is_task_in_project (rel_task, cmd->tree)) {
			continue;	
		}

		if (g_getenv ("PLANNER_DEBUG_UNDO_TASK"))
			g_message ("Successor recover: %s -> %s",
				   mrp_task_get_name (mrp_relation_get_predecessor (l->data)),
				   mrp_task_get_name (mrp_relation_get_successor (l->data)));
		
		mrp_task_add_predecessor (rel_task, cmd->task,
					  mrp_relation_get_relation_type (relation),
					  mrp_relation_get_lag (relation), &error);
		
		if (g_getenv ("PLANNER_DEBUG_UNDO_TASK")) {
			if (error) {
				g_message ("Recover OK");
			} else {
				g_message ("Recover KO: %s", error->message);
			}
		}
	}
}

static void 
task_cmd_save_children (TaskCmdRemove *cmd)
{
	MrpTask *child;

	child = mrp_task_get_first_child (cmd->task);

	while (child) {
		TaskCmdRemove     *cmd_child;
		GtkTreePath       *path;
		PlannerGanttModel *model;
		
		model = PLANNER_GANTT_MODEL (gtk_tree_view_get_model (GTK_TREE_VIEW (cmd->tree)));
		
		path = planner_gantt_model_get_path_from_task (model, child);
		
		/* We don't insert this command in the undo manager */
		cmd_child = g_new0 (TaskCmdRemove, 1);
		cmd_child->tree = cmd->tree;
		cmd_child->project = task_tree_get_project (cmd->tree);		
		cmd_child->path = gtk_tree_path_copy (path);		
		cmd_child->task = g_object_ref (child);
		task_cmd_save_relations (cmd_child);
		task_cmd_save_assignments (cmd_child);
		
		cmd->children = g_list_append (cmd->children, cmd_child);
		
		task_cmd_save_children (cmd_child);

		child = mrp_task_get_next_sibling (child);
	}

	if (g_getenv ("PLANNER_DEBUG_UNDO_TASK")) {
		if (cmd->children != NULL) {
			GList *l;
			for (l = cmd->children; l; l = l->next) {
				TaskCmdRemove *cmd_child = l->data;
				g_message ("Child saved: %s", mrp_task_get_name (cmd_child->task));
			}
		}
	}
	
}

static void
task_cmd_restore_children (TaskCmdRemove *cmd)
{
	PlannerGanttModel *model;
	gint               position, depth;
	GtkTreePath       *path;
	MrpTask           *parent;
	GList             *l;

	for (l = cmd->children; l; l = l->next) {
		TaskCmdRemove *cmd_child;			
		
		cmd_child = l->data;

		path = gtk_tree_path_copy (cmd_child->path);
		model = PLANNER_GANTT_MODEL (gtk_tree_view_get_model 
					     (GTK_TREE_VIEW (cmd_child->tree)));
		
		depth = gtk_tree_path_get_depth (path);
		position = gtk_tree_path_get_indices (path)[depth - 1];
		
		if (depth > 1) {
			gtk_tree_path_up (path);
			parent = task_tree_get_task_from_path (cmd_child->tree, path);
		} else {
			parent = NULL;
		}
		
		gtk_tree_path_free (path);
		
		mrp_project_insert_task (cmd_child->project,
					 parent,
					 position,
					 cmd_child->task);
		
		task_cmd_restore_children (cmd_child);
		task_cmd_restore_relations (cmd_child);
		task_cmd_restore_assignments (cmd_child);
	}
}

static gboolean
task_cmd_remove_do (PlannerCmd *cmd_base)
{
	TaskCmdRemove *cmd;
	gint           children;

	cmd = (TaskCmdRemove*) cmd_base;

	task_cmd_save_relations (cmd);
	task_cmd_save_assignments (cmd);

	children = mrp_task_get_n_children (cmd->task);

	if (children > 0 && cmd->children == NULL) {
		task_cmd_save_children (cmd);
	}

	mrp_project_remove_task (cmd->project, cmd->task);

	return TRUE;
}

static void
task_cmd_remove_undo (PlannerCmd *cmd_base)
{
	PlannerGanttModel *model;
	TaskCmdRemove     *cmd;
	gint               position, depth;
	GtkTreePath       *path;
	MrpTask           *parent;
	MrpTask           *child_parent;
	
	cmd = (TaskCmdRemove*) cmd_base;

	path = gtk_tree_path_copy (cmd->path);
	model = PLANNER_GANTT_MODEL (gtk_tree_view_get_model (GTK_TREE_VIEW (cmd->tree)));

	depth = gtk_tree_path_get_depth (path);
	position = gtk_tree_path_get_indices (path)[depth - 1];

 	if (depth > 1) {
		gtk_tree_path_up (path);
		parent = task_tree_get_task_from_path (cmd->tree, path);
	} else {
		parent = NULL;
	}
	
	gtk_tree_path_free (path);

	mrp_project_insert_task (cmd->project,
				 parent,
				 position,
				 cmd->task);

	child_parent = planner_gantt_model_get_indent_task_target (model, cmd->task);

	if (cmd->children != NULL) task_cmd_restore_children (cmd);

	task_cmd_restore_relations (cmd);
	task_cmd_restore_assignments (cmd);
}

static void
task_cmd_remove_free (PlannerCmd *cmd_base)
{
	TaskCmdRemove *cmd;
	GList         *l;

	cmd = (TaskCmdRemove*) cmd_base;

	for (l = cmd->children; l; l = l->next) {
		task_cmd_remove_free (l->data);
	}

	g_object_unref (cmd->task);

	g_list_free (cmd->children);

	g_list_foreach (cmd->predecessors, (GFunc) g_object_unref, NULL);
	g_list_free (cmd->predecessors);

	g_list_foreach (cmd->successors, (GFunc) g_object_unref, NULL);
	g_list_free (cmd->successors);

	g_list_foreach (cmd->assignments, (GFunc) g_object_unref, NULL);
	g_list_free (cmd->assignments);
		
	gtk_tree_path_free (cmd->path);
}

static PlannerCmd *
task_cmd_remove (PlannerTaskTree *tree,
		 GtkTreePath     *path,
		 MrpTask         *task)
{
	PlannerTaskTreePriv *priv = tree->priv;
	PlannerCmd          *cmd_base;
	TaskCmdRemove       *cmd;
	
	cmd_base = planner_cmd_new (TaskCmdRemove,
				    _("Remove task"),
				    task_cmd_remove_do,
				    task_cmd_remove_undo,
				    task_cmd_remove_free);
	
	cmd = (TaskCmdRemove *) cmd_base;

	cmd->tree = tree;
	cmd->project = task_tree_get_project (tree);

	cmd->path = gtk_tree_path_copy (path);

	cmd->task = g_object_ref (task);

	planner_cmd_manager_insert_and_do (planner_window_get_cmd_manager (priv->main_window),
					   cmd_base);

	return cmd_base;
}

typedef struct {
	PlannerCmd     base;

	MrpTask       *task;
	MrpConstraint *constraint;
	MrpConstraint *constraint_old;
} TaskCmdConstraint;

static gboolean
task_cmd_constraint_do (PlannerCmd *cmd_base)
{
	TaskCmdConstraint *cmd;

	cmd = (TaskCmdConstraint*) cmd_base;

	g_object_set (cmd->task,
		      "constraint", cmd->constraint,
		      NULL);

	return TRUE;
}

static void
task_cmd_constraint_undo (PlannerCmd *cmd_base)
{
	TaskCmdConstraint *cmd;

	cmd = (TaskCmdConstraint*) cmd_base;

	g_object_set (cmd->task, "constraint", 
		      cmd->constraint_old, NULL);
	
}

static void
task_cmd_constraint_free (PlannerCmd *cmd_base)
{
	TaskCmdConstraint *cmd;

	cmd = (TaskCmdConstraint*) cmd_base;

	g_object_unref (cmd->task);
	g_free (cmd->constraint);
	g_free (cmd->constraint_old);
}


static PlannerCmd *
task_cmd_constraint (PlannerTaskTree *tree,
		     MrpTask         *task,
		     MrpConstraint    constraint)
{
	PlannerTaskTreePriv *priv = tree->priv;
	PlannerCmd          *cmd_base;
	TaskCmdConstraint   *cmd;

	cmd_base = planner_cmd_new (TaskCmdConstraint,
				    _("Apply constraint to task"),
				    task_cmd_constraint_do,
				    task_cmd_constraint_undo,
				    task_cmd_constraint_free);

	cmd = (TaskCmdConstraint *) cmd_base;

	cmd->task = g_object_ref (task);
	cmd->constraint = g_new0 (MrpConstraint, 1);
	cmd->constraint->time = constraint.time;
	cmd->constraint->type = constraint.type;

	g_object_get (task, "constraint", 
		      &cmd->constraint_old, NULL);

	planner_cmd_manager_insert_and_do (planner_window_get_cmd_manager (priv->main_window),
					   cmd_base);

	return cmd_base;
}

static gboolean
task_cmd_constraint_reset_do (PlannerCmd *cmd_base)
{
	TaskCmdConstraint *cmd;

	cmd = (TaskCmdConstraint*) cmd_base;
	mrp_task_reset_constraint (cmd->task);
	return TRUE;
}

static void
task_cmd_constraint_reset_undo (PlannerCmd *cmd_base)
{
	TaskCmdConstraint *cmd;

	cmd = (TaskCmdConstraint*) cmd_base;

	g_object_set (cmd->task, "constraint", 
		      cmd->constraint_old, NULL);
	
}

static void
task_cmd_constraint_reset_free (PlannerCmd *cmd_base)
{
	TaskCmdConstraint *cmd;

	cmd = (TaskCmdConstraint*) cmd_base;
	g_object_unref (cmd->task);
	g_free (cmd->constraint_old);
}

static PlannerCmd *
task_cmd_reset_constraint (PlannerTaskTree *tree,
			   MrpTask         *task)
{
	PlannerTaskTreePriv *priv = tree->priv;
	PlannerCmd          *cmd_base;
	TaskCmdConstraint   *cmd;

	cmd_base = planner_cmd_new (TaskCmdConstraint,
				    _("Reset task constraint"),
				    task_cmd_constraint_reset_do,
				    task_cmd_constraint_reset_undo,
				    task_cmd_constraint_reset_free);

	cmd = (TaskCmdConstraint *) cmd_base;

	cmd->task = g_object_ref (task);

	g_object_get (task, "constraint", 
		      &cmd->constraint_old, NULL);

	planner_cmd_manager_insert_and_do (planner_window_get_cmd_manager (priv->main_window),
					   cmd_base);

	return cmd_base;
}

typedef struct {
	PlannerCmd     base;

	MrpProject *project;
	MrpTask    *task;
	MrpTask    *parent;
	MrpTask    *parent_old;
	MrpTask    *sibling;
	gboolean    before;
	gboolean    before_old;
	GError     *error;
} TaskCmdMove;

static gboolean
task_cmd_move_do (PlannerCmd *cmd_base)
{
	TaskCmdMove *cmd;
	GError      *error = NULL;
	gboolean     retval;

	cmd = (TaskCmdMove*) cmd_base;

	if (g_getenv ("PLANNER_DEBUG_UNDO_TASK")) {
		if (cmd->before) {
			g_message ("DO: Moving %s (parent %s) before %s",
				   mrp_task_get_name (cmd->task),
				   mrp_task_get_name (cmd->parent),
				   mrp_task_get_name (cmd->sibling));
		} else {
			g_message ("DO: Moving %s (parent %s) after %s",
				   mrp_task_get_name (cmd->task),
				   mrp_task_get_name (cmd->parent),
				   mrp_task_get_name (cmd->sibling));	
		}
	}

	retval = mrp_project_move_task (cmd->project,
					cmd->task,
					cmd->sibling,
					cmd->parent,
					cmd->before,
					&error);

	if (!retval) {
		cmd->error = error;
	}

	return retval;
}

static void
task_cmd_move_undo (PlannerCmd *cmd_base)
{
	TaskCmdMove *cmd;
	GError      *error;
	gboolean     result;

	cmd = (TaskCmdMove*) cmd_base;

	if (g_getenv ("PLANNER_DEBUG_UNDO_TASK")) {
		if (cmd->before_old) {
			g_message ("UNDO: Moving %s (parent %s) before %s",
				   mrp_task_get_name (cmd->task),
				   mrp_task_get_name (cmd->parent_old),
				   mrp_task_get_name (cmd->sibling));
		} else {
			g_message ("UNDO: Moving %s (parent %s) after %s",
				   mrp_task_get_name (cmd->task),
				   mrp_task_get_name (cmd->parent_old),
				   mrp_task_get_name (cmd->sibling));	
		}
	}

	result = mrp_project_move_task (cmd->project,
					cmd->task,
					cmd->sibling,
					cmd->parent_old,
					cmd->before_old,
					&error);
	g_assert (result);
}

static void
task_cmd_move_free (PlannerCmd *cmd_base)
{
	TaskCmdMove *cmd;

	cmd = (TaskCmdMove*) cmd_base;

	g_object_unref (cmd->project);
	g_object_unref (cmd->task);
	if (cmd->parent != NULL) {
		g_object_unref (cmd->parent);
	}
	g_object_unref (cmd->parent_old);
	if (cmd->sibling != NULL) {
		g_object_unref (cmd->sibling);
	}
}


static PlannerCmd *
task_cmd_move (PlannerTaskTree *tree,
	       MrpTask         *task,
	       MrpTask         *sibling,
	       MrpTask         *parent,
	       gboolean         before,
	       GError         **error)
{
	PlannerTaskTreePriv *priv = tree->priv;
	PlannerCmd          *cmd_base;
	TaskCmdMove         *cmd;
	MrpTask             *parent_old;


	cmd_base = planner_cmd_new (TaskCmdMove,
				    _("Move task"),
				    task_cmd_move_do,
				    task_cmd_move_undo,
				    task_cmd_move_free);

	cmd = (TaskCmdMove *) cmd_base;
	
	cmd->project = g_object_ref (tree->priv->project);
	cmd->task = g_object_ref (task);

	if (parent != NULL) {
		cmd->parent = g_object_ref (parent);
	}
	
	parent_old = mrp_task_get_parent (task);

	if (parent_old != NULL) {
		cmd->parent_old = g_object_ref (parent_old);
	}

	if (sibling != NULL) {
		cmd->sibling = g_object_ref (sibling);
	}

	cmd->before = before;

	/* do Up task -> undo Down task*/
	if (sibling != NULL && before) { 
		cmd->before_old = FALSE;
	} 
	/* do Down task -> undo Up task */
	else if (sibling != NULL && !before) {
		cmd->before_old = TRUE;	
	}

	planner_cmd_manager_insert_and_do (planner_window_get_cmd_manager 
					   (priv->main_window),
					   cmd_base);

	if (cmd->error) {
		g_propagate_error (error, cmd->error);
		return NULL;
	} 

	return cmd_base;
}

typedef struct {
	PlannerCmd   base;

	MrpTask      *task;
	MrpProperty  *property;  
	GValue       *value;
	GValue       *old_value;
} TaskCmdEditCustomProperty;

static gboolean
task_cmd_edit_custom_property_do (PlannerCmd *cmd_base)
{
	TaskCmdEditCustomProperty *cmd;
	cmd = (TaskCmdEditCustomProperty*) cmd_base;

	mrp_object_set_property (MRP_OBJECT (cmd->task),
				 cmd->property,
				 cmd->value);

	return TRUE;
}

static void
task_cmd_edit_custom_property_undo (PlannerCmd *cmd_base)
{
	TaskCmdEditCustomProperty *cmd;

	cmd = (TaskCmdEditCustomProperty*) cmd_base;

	/* FIXME: delay in the UI when setting the property */
	mrp_object_set_property (MRP_OBJECT (cmd->task),
				 cmd->property,
				 cmd->old_value);

}

static void
task_cmd_edit_custom_property_free (PlannerCmd *cmd_base)
{
	TaskCmdEditCustomProperty *cmd;

	cmd = (TaskCmdEditCustomProperty*) cmd_base;

	g_value_unset (cmd->value);
	g_value_unset (cmd->old_value);
	g_free (cmd->value);
	g_free (cmd->old_value);

	g_object_unref (cmd->task);
}

static PlannerCmd *
task_cmd_edit_custom_property (PlannerTaskTree  *tree,
			       MrpTask          *task,
			       MrpProperty      *property,
			       const GValue     *value)
{
	PlannerCmd                *cmd_base;
	PlannerTaskTreePriv       *priv = tree->priv;
	TaskCmdEditCustomProperty *cmd;

	cmd_base = planner_cmd_new (TaskCmdEditCustomProperty,
				    _("Edit task custom property"),
				    task_cmd_edit_custom_property_do,
				    task_cmd_edit_custom_property_undo,
				    task_cmd_edit_custom_property_free);
	

	cmd = (TaskCmdEditCustomProperty *) cmd_base;

	cmd->property = property;
	cmd->task = g_object_ref (task);

	cmd->value = g_new0 (GValue, 1);
	g_value_init (cmd->value, G_VALUE_TYPE (value));
	g_value_copy (value, cmd->value);

	cmd->old_value = g_new0 (GValue, 1);
	g_value_init (cmd->old_value, G_VALUE_TYPE (value));

	mrp_object_get_property (MRP_OBJECT (cmd->task),
				 cmd->property,
				 cmd->old_value);

	/* FIXME: if old and new value are the same, do nothing 
	   How we can compare values?
	*/

	planner_cmd_manager_insert_and_do (planner_window_get_cmd_manager (priv->main_window),
					   cmd_base);

	return cmd_base;
}


GType
planner_task_tree_get_type (void)
{
	static GType type = 0;

	if (!type) {
		static const GTypeInfo info = {
			sizeof (PlannerTaskTreeClass),
			NULL,		/* base_init */
			NULL,		/* base_finalize */
			(GClassInitFunc) task_tree_class_init,
			NULL,		/* class_finalize */
			NULL,		/* class_data */
			sizeof (PlannerTaskTree),
			0,              /* n_preallocs */
			(GInstanceInitFunc) task_tree_init
		};

		type = g_type_register_static (GTK_TYPE_TREE_VIEW, "PlannerTaskTree",
					       &info, 0);
	}
	
	return type;
}

static void
task_tree_class_init (PlannerTaskTreeClass *klass)
{
	GObjectClass *o_class;

	parent_class = g_type_class_peek_parent (klass);

	o_class = (GObjectClass *) klass;

	o_class->finalize = task_tree_finalize;

	signals[SELECTION_CHANGED] =
		g_signal_new ("selection-changed",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      0,
			      NULL, NULL,
			      planner_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);
	signals[RELATION_ADDED] =
		g_signal_new ("relation-added",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      0,
			      NULL, NULL,
			      planner_marshal_VOID__OBJECT_OBJECT,
			      G_TYPE_NONE, 
			      2, MRP_TYPE_TASK, MRP_TYPE_RELATION);
	signals[RELATION_REMOVED] = 
		g_signal_new ("relation-removed",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      0,
			      NULL, NULL,
			      planner_marshal_VOID__OBJECT_OBJECT,
			      G_TYPE_NONE, 
			      2, MRP_TYPE_TASK, MRP_TYPE_RELATION);
}

static void
task_tree_init (PlannerTaskTree *tree)
{
	PlannerTaskTreePriv *priv;
	
	priv = g_new0 (PlannerTaskTreePriv, 1);
	tree->priv = priv;

	priv->property_to_column = g_hash_table_new (NULL, NULL);
	
	priv->popup_factory = planner_task_popup_new (tree);
	
	priv->anchor = NULL;
}

static void
task_tree_finalize (GObject *object)
{
	PlannerTaskTree     *tree;
	PlannerTaskTreePriv *priv;

	tree = PLANNER_TASK_TREE (object);
	priv = tree->priv;
	
	g_hash_table_destroy (priv->property_to_column);

	g_free (priv);

	if (G_OBJECT_CLASS (parent_class)->finalize) {
		(* G_OBJECT_CLASS (parent_class)->finalize) (object);
	}
	
	planner_task_tree_set_anchor (tree, NULL);
}

static void
task_tree_block_selection_changed (PlannerTaskTree *tree)
{
	GtkTreeSelection *selection;
	
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree));
	
	g_signal_handlers_block_by_func (selection,
					 task_tree_selection_changed_cb,
					 tree);
}

static void
task_tree_unblock_selection_changed (PlannerTaskTree *tree)
{
	GtkTreeSelection *selection;
	
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree));
	
	g_signal_handlers_unblock_by_func (selection,
					   task_tree_selection_changed_cb,
					   tree);
}

static void 
task_tree_selection_changed_cb (GtkTreeSelection *selection,
				PlannerTaskTree       *tree)
{
	g_return_if_fail (GTK_IS_TREE_SELECTION (selection));
	g_return_if_fail (PLANNER_IS_TASK_TREE (tree));

	g_signal_emit (tree, signals[SELECTION_CHANGED], 0, NULL);
}

static void
task_tree_relation_added_cb (MrpTask     *task, 
			     MrpRelation *relation, 
			     PlannerTaskTree  *tree)
{
	g_return_if_fail (MRP_IS_TASK (task));
	g_return_if_fail (MRP_IS_RELATION (relation));
	
	g_signal_emit (tree, signals[RELATION_ADDED], 0, task, relation);
}

static void
task_tree_relation_removed_cb (MrpTask     *task,
			       MrpRelation *relation,
			       PlannerTaskTree  *tree)
{
	g_return_if_fail (MRP_IS_TASK (task));
	g_return_if_fail (MRP_IS_RELATION (relation));
	
	g_signal_emit (tree, signals[RELATION_REMOVED], 0, task, relation);
}

static void
task_tree_row_inserted (GtkTreeModel *model,
			GtkTreePath  *path,
			GtkTreeIter  *iter,
			GtkTreeView  *tree)
{
	GtkTreePath *parent;

	parent = gtk_tree_path_copy (path);
	
	gtk_tree_path_up (parent);

	gtk_tree_view_expand_row (tree,
				  parent,
				  FALSE);

	gtk_tree_path_free (parent);
}

static void
task_tree_task_added_cb (PlannerGanttModel *model, MrpTask *task, PlannerTaskTree *tree)
{
	g_object_ref (task);

	g_signal_connect (task, "relation_added", 
			  G_CALLBACK (task_tree_relation_added_cb),
			  tree);
	g_signal_connect (task, "relation_removed",
			  G_CALLBACK (task_tree_relation_removed_cb),
			  tree);
}

static void
task_tree_task_removed_cb (PlannerGanttModel *model,
			   MrpTask      *task,
			   PlannerTaskTree   *tree)
{
	g_signal_handlers_disconnect_by_func (task,
					      task_tree_relation_added_cb,
					      tree);
	g_signal_handlers_disconnect_by_func (task,
					      task_tree_relation_removed_cb,
					      tree);
	g_object_unref (task);
}

static void
task_tree_tree_view_popup_menu (GtkWidget  *widget,
				PlannerTaskTree *tree)
{
	gint x, y;

	gdk_window_get_pointer (widget->window, &x, &y, NULL);

	gtk_item_factory_popup (tree->priv->popup_factory,
				x, y,
				0,
				gtk_get_current_event_time ());
}

static gboolean
task_tree_tree_view_button_press_event (GtkTreeView     *tree_view,
					GdkEventButton  *event,
					PlannerTaskTree *tree)
{
	GtkTreePath         *path;
	GtkTreeView         *tv;
	PlannerTaskTreePriv *priv;
	GtkItemFactory      *factory;
	GtkTreeIter          iter;
	
	tv = GTK_TREE_VIEW (tree);
	priv = tree->priv;
	factory = priv->popup_factory;

	if (event->button == 3) {
		gtk_widget_grab_focus (GTK_WIDGET (tree));

		/* Select our row. */
		if (gtk_tree_view_get_path_at_pos (tv, event->x, event->y, &path, NULL, NULL, NULL)) {
			gtk_tree_model_get_iter (gtk_tree_view_get_model (tree_view), &iter, path);
			if (!gtk_tree_selection_iter_is_selected (gtk_tree_view_get_selection (tree_view), &iter)) {
				gtk_tree_selection_unselect_all (gtk_tree_view_get_selection (tv));
				gtk_tree_selection_select_path (gtk_tree_view_get_selection (tv), path);
			}
			
			gtk_widget_set_sensitive (
				gtk_item_factory_get_widget_by_action (factory, PLANNER_TASK_POPUP_SUBTASK), TRUE);
			gtk_widget_set_sensitive (
				gtk_item_factory_get_widget_by_action (factory, PLANNER_TASK_POPUP_REMOVE), TRUE);
			gtk_widget_set_sensitive (
				gtk_item_factory_get_widget_by_action (factory, PLANNER_TASK_POPUP_UNLINK), TRUE);
			gtk_widget_set_sensitive (
				gtk_item_factory_get_widget_by_action (factory, PLANNER_TASK_POPUP_EDIT_RESOURCES), TRUE);
			gtk_widget_set_sensitive (
				gtk_item_factory_get_widget_by_action (factory, PLANNER_TASK_POPUP_EDIT_TASK), TRUE);
			
			planner_task_tree_set_anchor (tree, path);
			
		} else {
			gtk_tree_selection_unselect_all (gtk_tree_view_get_selection (tv));

			gtk_widget_set_sensitive (
				gtk_item_factory_get_widget_by_action (factory, PLANNER_TASK_POPUP_SUBTASK), FALSE);
			gtk_widget_set_sensitive (
				gtk_item_factory_get_widget_by_action (factory, PLANNER_TASK_POPUP_REMOVE), FALSE);
			gtk_widget_set_sensitive (
				gtk_item_factory_get_widget_by_action (factory, PLANNER_TASK_POPUP_UNLINK), FALSE);
			gtk_widget_set_sensitive (
				gtk_item_factory_get_widget_by_action (factory, PLANNER_TASK_POPUP_EDIT_RESOURCES), FALSE);
			gtk_widget_set_sensitive (
				gtk_item_factory_get_widget_by_action (factory, PLANNER_TASK_POPUP_EDIT_TASK), FALSE);
			
			planner_task_tree_set_anchor (tree, NULL);
		}
		
		gtk_item_factory_popup (factory, event->x_root, event->y_root,
					event->button, event->time);
		return TRUE;
	}
	else if (event->button == 1) {
		if (!(event->state & GDK_SHIFT_MASK)) {
			if (gtk_tree_view_get_path_at_pos (tv, event->x, event->y, &path, NULL, NULL, NULL)) {
				planner_task_tree_set_anchor (tree, path);
			}
		}
		return FALSE;
	}
	
	return FALSE;
}

/* Note: we must make sure that this matches the treeview behavior. It's a bit
 * hackish but seems to be the only way to match the selection behavior of the
 * gantt chart with the treeview.
 */
static gboolean
task_tree_tree_view_key_release_event (GtkTreeView *tree_view,
					GdkEventKey *event,
					PlannerTaskTree *tree)
{
	if (!(event->state & (GDK_SHIFT_MASK | GDK_CONTROL_MASK))) {
		if (event->keyval & (GDK_Up | GDK_Page_Up | GDK_KP_Up |
			GDK_KP_Page_Up | GDK_Pointer_Up | GDK_ISO_Move_Line_Up |
			GDK_Down | GDK_Page_Down | GDK_KP_Down | GDK_KP_Page_Down |
			GDK_Pointer_Down | GDK_ISO_Move_Line_Down)) {
			
			GtkTreeSelection *selection = gtk_tree_view_get_selection (tree_view);
			GList *list = gtk_tree_selection_get_selected_rows (selection, NULL);
			
			if (g_list_length (list) == 1) {
				planner_task_tree_set_anchor (tree, list->data);
			}
						
			g_list_free (list);
		}
	}
	
	return FALSE;
}

static void
task_tree_wbs_data_func (GtkTreeViewColumn *tree_column,
			 GtkCellRenderer   *cell,
			 GtkTreeModel      *tree_model,
			 GtkTreeIter       *iter,
			 gpointer           data)
{
	PlannerTaskTree *tree;
	gchar           *str;
	
	tree = PLANNER_TASK_TREE (data);
	
	gtk_tree_model_get (tree_model,
			    iter,
			    COL_WBS, &str,
			    -1);

	g_object_set (cell,
		      "text", str,
		      NULL);
	
	g_free (str);
}

static void
task_tree_name_data_func (GtkTreeViewColumn *tree_column,
			  GtkCellRenderer   *cell,
			  GtkTreeModel      *tree_model,
			  GtkTreeIter       *iter,
			  gpointer           data)
{
	PlannerTaskTree *tree;
	MrpTask         *task;
	gint             weight;
	gboolean         critical;

	tree = PLANNER_TASK_TREE (data);
	
	gtk_tree_model_get (tree_model,
			    iter,
			    COL_TASK, &task,
			    COL_WEIGHT, &weight,
			    -1);

	if (tree->priv->highlight_critical) {
		critical = mrp_task_get_critical (task);
	} else {
		critical = FALSE;
	}

	g_object_set (cell,
		      "text", mrp_task_get_name (task), 
		      "weight", weight,
		      "foreground", critical ? "indian red" : NULL,
		      NULL);
}

static void
task_tree_start_data_func (GtkTreeViewColumn *tree_column,
			   GtkCellRenderer   *cell,
			   GtkTreeModel      *tree_model,
			   GtkTreeIter       *iter,
			   gpointer           data)
{
	glong     start;
	gint      weight;
	gboolean  editable;
	gchar    *str;
 
	gtk_tree_model_get (tree_model,
			    iter,
			    COL_START, &start,
			    COL_WEIGHT, &weight,
			    COL_EDITABLE, &editable, 
			    -1);

	str = planner_format_date (start);

	g_object_set (cell, 
		      "text", str,
		      "weight", weight,
		      "editable", editable,
		      NULL);
	g_free (str);
}

static void
task_tree_finish_data_func (GtkTreeViewColumn *tree_column,
			    GtkCellRenderer   *cell,
			    GtkTreeModel      *tree_model,
			    GtkTreeIter       *iter,
			    gpointer           data)
{
	glong  start;
	gchar *str;
	gint   weight;

	gtk_tree_model_get (tree_model,
			    iter,
			    COL_FINISH, &start,
			    COL_WEIGHT, &weight,
			    -1);

	str = planner_format_date (start);
	
	g_object_set (cell,
		      "text", str,
		      "weight", weight,
		      NULL);
	g_free (str);
}

static void
task_tree_duration_data_func (GtkTreeViewColumn *tree_column,
			      GtkCellRenderer   *cell,
			      GtkTreeModel      *tree_model,
			      GtkTreeIter       *iter,
			      gpointer           data)
{
	PlannerTaskTree     *task_tree;
	PlannerTaskTreePriv *priv;
	MrpCalendar         *calendar;
	gint                 hours_per_day;
	gint                 duration;
	gchar               *str;
	gint                 weight;
	gboolean             editable;

	task_tree = PLANNER_TASK_TREE (data);
	priv = task_tree->priv;
	
	gtk_tree_model_get (tree_model,
			    iter,
			    COL_DURATION, &duration,
			    COL_WEIGHT, &weight,
			    COL_EDITABLE, &editable,
			    -1);

	calendar = mrp_project_get_calendar (priv->project);
	
	hours_per_day = mrp_calendar_day_get_total_work (
		calendar, mrp_day_get_work ()) / (60*60);
	
	str = planner_format_duration (duration, hours_per_day);

	g_object_set (cell, 
		      "text", str,
		      "weight", weight,
		      "editable", editable,
		      NULL);

	g_free (str);
}

static void
task_tree_cost_data_func (GtkTreeViewColumn *tree_column,
			  GtkCellRenderer   *cell,
			  GtkTreeModel      *tree_model,
			  GtkTreeIter       *iter,
			  gpointer           data)
{
	gfloat  cost;
	gchar  *str;
	gint    weight;

	gtk_tree_model_get (tree_model,
			    iter,
			    COL_COST, &cost,
			    COL_WEIGHT, &weight,
			    -1);

	str = planner_format_float (cost, 2, FALSE);

	g_object_set (cell,
		      "text", str,
		      "weight", weight,
		      NULL);

	g_free (str);
}

static void
task_tree_work_data_func (GtkTreeViewColumn *tree_column,
			  GtkCellRenderer   *cell,
			  GtkTreeModel      *tree_model,
			  GtkTreeIter       *iter,
			  gpointer           data)
{
	PlannerTaskTree *tree;
	gint             work;
	gint             hours_per_day;
	MrpTask         *task;
	MrpTaskType      type;
	gint             weight;
	gboolean         editable;

	g_return_if_fail (PLANNER_IS_TASK_TREE (data));
	tree = PLANNER_TASK_TREE (data);

	hours_per_day = mrp_calendar_day_get_total_work (
		mrp_project_get_calendar (tree->priv->project),
		mrp_day_get_work ()) / (60*60);

	/* FIXME */
	if (hours_per_day == 0) {
		hours_per_day = 8;
	}

	gtk_tree_model_get (tree_model,
			    iter,
			    COL_WORK, &work,
			    COL_TASK, &task,
			    COL_WEIGHT, &weight,
			    COL_EDITABLE, &editable,
			    -1);

	type = mrp_task_get_task_type (task);

	g_object_set (cell, 
		      "weight", weight,
		      "editable", editable,
		      NULL);

	if (type == MRP_TASK_TYPE_MILESTONE) {
		g_object_set (cell, "text", "", NULL);
	} else {
		gchar *str = planner_format_duration (work, hours_per_day);
		g_object_set (cell, "text", str, NULL);
		g_free (str);
	}
}

static void
task_tree_slack_data_func (GtkTreeViewColumn *tree_column,
			   GtkCellRenderer   *cell,
			   GtkTreeModel      *tree_model,
			   GtkTreeIter       *iter,
			   gpointer           data)
{
	PlannerTaskTree *tree = data;
	gint             slack;
	gint             hours_per_day;
	gchar           *str;
	gint             weight;

	hours_per_day = mrp_calendar_day_get_total_work (
		mrp_project_get_calendar (tree->priv->project),
		mrp_day_get_work ()) / (60*60);

	/* FIXME */
	if (hours_per_day == 0) {
		hours_per_day = 8;
	}
	
	gtk_tree_model_get (tree_model, iter,
			    COL_SLACK, &slack,
			    COL_WEIGHT, &weight,
			    -1);
	
	str = planner_format_duration (slack, hours_per_day);

	g_object_set (cell, 
		      "text", str,
		      "weight", weight, 
		      NULL);
	g_free (str);
}

static void
task_tree_name_edited (GtkCellRendererText *cell,
		       gchar               *path_string,
		       gchar               *new_text,
		       gpointer             data)
{
	PlannerTaskTree     *task_tree;
	PlannerTaskTreePriv *priv;
	GtkTreeView         *view;
	GtkTreeModel        *model;
	GtkTreePath         *path;
	GtkTreeIter          iter;
	MrpTask             *task;
	GValue               value = { 0 };

	task_tree = PLANNER_TASK_TREE (data);
	priv = task_tree->priv;
	
	view = GTK_TREE_VIEW (data);
	model = gtk_tree_view_get_model (view);

	path = gtk_tree_path_new_from_string (path_string);
	gtk_tree_model_get_iter (model, &iter, path);

	gtk_tree_model_get (model, &iter, 
			    COL_TASK, &task,
			    -1);

	if (strcmp (mrp_task_get_name (task), new_text) != 0) {
		g_value_init (&value, G_TYPE_STRING);
		g_value_set_string (&value, new_text);
		
		task_cmd_edit_property (task_tree,
					task,
					"name",
					&value);

		g_value_unset (&value);
	}
	
	/*planner_cmd_manager_end_transaction (planner_window_get_cmd_manager (priv->main_window));*/
		
	gtk_tree_path_free (path);
}

static void
task_tree_start_edited (GtkCellRendererText *cell,
			gchar               *path_string,
			gchar               *new_text,
			gpointer             data)
{
	PlannerTaskTree         *tree = data;
	PlannerCellRendererDate *date;
	GtkTreeView             *view;
	GtkTreeModel            *model;
	GtkTreePath             *path;
	GtkTreeIter              iter;
	MrpTask                 *task;
	MrpConstraint            constraint;

	view = GTK_TREE_VIEW (data);
	model = gtk_tree_view_get_model (view);
	date = PLANNER_CELL_RENDERER_DATE (cell);

	path = gtk_tree_path_new_from_string (path_string);
	gtk_tree_model_get_iter (model, &iter, path);

	gtk_tree_model_get (model, &iter, 
			    COL_TASK, &task,
			    -1);

	constraint.time = date->time;
	constraint.type = date->type;
	
	task_cmd_constraint (tree, task, constraint);
	
	/* g_object_set (task, "constraint", &constraint, NULL); */

	gtk_tree_path_free (path);
}

static void
task_tree_start_show_popup (PlannerCellRendererDate *cell,
			    const gchar        *path_string,
			    gint                x1,
			    gint                y1,
			    gint                x2,
			    gint                y2,
			    GtkTreeView        *tree_view)
{
	GtkTreeModel  *model;
	GtkTreeIter    iter;
	GtkTreePath   *path;
	MrpTask       *task;
	mrptime        start;
	MrpConstraint *constraint;

	model = gtk_tree_view_get_model (tree_view);
	
	path = gtk_tree_path_new_from_string (path_string);
	gtk_tree_model_get_iter (model, &iter, path);
	gtk_tree_model_get (model, &iter, 
			    COL_TASK, &task,
			    -1);

	g_object_get (task,
		      "constraint", &constraint,
		      NULL);

	cell->type = constraint->type;
	
	if (cell->type == MRP_CONSTRAINT_ASAP) {
		start = mrp_task_get_start (task);
		cell->time = start;
	} else {
		cell->time = constraint->time;
	}

	g_free (constraint);
	gtk_tree_path_free (path);
}

static void
task_tree_property_date_show_popup (PlannerCellRendererDate *cell,
				    const gchar        *path_string,
				    gint                x1,
				    gint                y1,
				    gint                x2,
				    gint                y2,
				    GtkTreeView        *tree_view)
{
	
	if (cell->time == MRP_TIME_INVALID) {
		cell->time = mrp_time_current_time ();
	}
}

static void
task_tree_duration_edited (GtkCellRendererText *cell,
			   gchar               *path_string,
			   gchar               *new_text,
			   gpointer             data)
{
	PlannerTaskTree *tree = data;
	GtkTreeView     *view = data;
	GtkTreeModel    *model;
	GtkTreePath     *path;
	GtkTreeIter      iter;
	gfloat           flt;
	gint             duration;
	gint             seconds_per_day;
	gchar           *ptr;
	MrpTask         *task;
	GValue           value = { 0 };
	
	model = gtk_tree_view_get_model (view);
	path = gtk_tree_path_new_from_string (path_string);	
	gtk_tree_model_get_iter (model, &iter, path);

	seconds_per_day = mrp_calendar_day_get_total_work (
		mrp_project_get_calendar (tree->priv->project),
		mrp_day_get_work ());
		
	flt = g_ascii_strtod (new_text, &ptr);
	if (ptr != NULL) {
		duration = flt * seconds_per_day;
		gtk_tree_model_get (model, &iter, 
				    COL_TASK, &task,
				    -1);

		g_value_init (&value, G_TYPE_INT);
		g_value_set_int (&value, duration);
		
		task_cmd_edit_property (PLANNER_TASK_TREE (view),
					task,
					"duration",
					&value);
	}
	
	gtk_tree_path_free (path);
}

static void
task_tree_work_edited (GtkCellRendererText *cell,
		       gchar               *path_string,
		       gchar               *new_text,
		       gpointer             data)
{
	GtkTreeView  *view;
	GtkTreeModel *model;
	GtkTreePath  *path;
	GtkTreeIter   iter;
	gint          work;
	MrpTask      *task;
	GValue        value = { 0 };
	
	view = GTK_TREE_VIEW (data);
	
	model = gtk_tree_view_get_model (view);
	path = gtk_tree_path_new_from_string (path_string);	
	gtk_tree_model_get_iter (model, &iter, path);

	work = task_tree_parse_time_string (PLANNER_TASK_TREE (view), new_text);
	
	gtk_tree_model_get (model, &iter, 
			    COL_TASK, &task,
			    -1);

	g_value_init (&value, G_TYPE_INT);
	g_value_set_int (&value, work);
	
	task_cmd_edit_property (PLANNER_TASK_TREE (view),
				task,
				"work",
				&value);
	
	gtk_tree_path_free (path);
}

static void
task_tree_property_data_func (GtkTreeViewColumn *tree_column,
			      GtkCellRenderer   *cell,
			      GtkTreeModel      *tree_model,
			      GtkTreeIter       *iter,
			      gpointer           data)
{
	MrpObject       *object;
	MrpProperty     *property = data;
	MrpPropertyType  type;
	gchar           *svalue;
	gint             ivalue;
	gfloat           fvalue;
	mrptime          tvalue;
	gint             work;

	gtk_tree_model_get (tree_model,
			    iter,
			    COL_TASK,
			    &object,
			    -1);

	/* FIXME: implement mrp_object_get_property like
	 * g_object_get_property that takes a GValue. 
	 */
	type = mrp_property_get_property_type (property);

	switch (type) {
	case MRP_PROPERTY_TYPE_STRING:
		mrp_object_get (object,
				mrp_property_get_name (property), &svalue,
				NULL);
		
		if (svalue == NULL) {
			svalue = g_strdup ("");
		}		

		break;
	case MRP_PROPERTY_TYPE_INT:
		mrp_object_get (object,
				mrp_property_get_name (property), &ivalue,
				NULL);
		svalue = g_strdup_printf ("%d", ivalue);
		break;

	case MRP_PROPERTY_TYPE_FLOAT:
		mrp_object_get (object,
				mrp_property_get_name (property), &fvalue,
				NULL);

		svalue = planner_format_float (fvalue, 4, FALSE);
		break;

	case MRP_PROPERTY_TYPE_DATE:
		mrp_object_get (object,
				mrp_property_get_name (property), &tvalue,
				NULL); 
		svalue = planner_format_date (tvalue);
		break;
		
	case MRP_PROPERTY_TYPE_DURATION:
		mrp_object_get (object,
				mrp_property_get_name (property), &ivalue,
				NULL); 

/*		work = mrp_calendar_day_get_total_work (
		mrp_project_get_calendar (tree->priv->project),
		mrp_day_get_work ());
*/
		work = 8*60*60;

		svalue = planner_format_duration (ivalue, work / (60*60));
		break;
		
	case MRP_PROPERTY_TYPE_COST:
		mrp_object_get (object,
				mrp_property_get_name (property), &fvalue,
				NULL); 

		svalue = planner_format_float (fvalue, 2, FALSE);
		break;
				
	default:
		g_warning ("Type not implemented.");
		break;
	}

	g_object_set (cell, "text", svalue, NULL);
	g_free (svalue);
}

static GValue
task_view_custom_property_set_value (MrpProperty         *property,
				     gchar               *new_text,
				     GtkCellRendererText *cell) 
{
	PlannerCellRendererDate *date;
	GValue                   value = { 0 };
	MrpPropertyType          type;
	gfloat                   fvalue;

	/* FIXME: implement mrp_object_set_property like
	 * g_object_set_property that takes a GValue. 
	 */
	type = mrp_property_get_property_type (property);

	switch (type) {
	case MRP_PROPERTY_TYPE_STRING:
		g_value_init (&value, G_TYPE_STRING);
		g_value_set_string (&value, new_text);

		break;
	case MRP_PROPERTY_TYPE_INT:
		g_value_init (&value, G_TYPE_INT);
		g_value_set_int (&value, atoi (new_text));

		break;
	case MRP_PROPERTY_TYPE_FLOAT:
		fvalue = g_ascii_strtod (new_text, NULL);
		g_value_init (&value, G_TYPE_FLOAT);
		g_value_set_float (&value, fvalue);

		break;

	case MRP_PROPERTY_TYPE_DURATION:
		/* FIXME: support reading units etc... */
		g_value_init (&value, G_TYPE_INT);
		g_value_set_int (&value, atoi (new_text) *8*60*60);

		break;
		

	case MRP_PROPERTY_TYPE_DATE:
		date = PLANNER_CELL_RENDERER_DATE (cell);
		/* FIXME: Currently custom properties can't be dates. Why? */
		/* mrp_object_set (MRP_OBJECT (task),
				mrp_property_get_name (property), 
				&(date->time),
				NULL);*/
		break;
	case MRP_PROPERTY_TYPE_COST:
		fvalue = g_ascii_strtod (new_text, NULL);
		g_value_init (&value, G_TYPE_FLOAT);
		g_value_set_float (&value, fvalue);

		break;	
				
	default:
		g_assert_not_reached ();
		break;
	}

	return value;
}

static void  
task_tree_property_value_edited (GtkCellRendererText *cell, 
				 gchar               *path_str,
				 gchar               *new_text, 
				 ColPropertyData     *data)
{
	PlannerCmd              *cmd;
	GtkTreePath             *path;
	GtkTreeIter              iter;
	GtkTreeModel            *model;
	MrpProperty             *property;
	MrpTask                 *task;
	GValue                   value = { 0 };
	
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (data->tree));
	property = data->property;	

	path = gtk_tree_path_new_from_string (path_str);
	gtk_tree_model_get_iter (model, &iter, path);

	task = planner_gantt_model_get_task (PLANNER_GANTT_MODEL (model), &iter);

	value = task_view_custom_property_set_value (property, new_text, cell);

	cmd = task_cmd_edit_custom_property (data->tree, task, property, &value);

	g_value_unset (&value);

	gtk_tree_path_free (path);
}

static void
task_tree_property_added (MrpProject      *project,
			  GType            object_type,
			  MrpProperty     *property,
			  PlannerTaskTree *task_tree)
{
	GtkTreeView         *tree;
	PlannerTaskTreePriv *priv;
	MrpPropertyType      type;
	GtkTreeViewColumn   *col;	
	GtkCellRenderer     *cell;
	ColPropertyData     *data;

	priv = task_tree->priv;
	
	tree = GTK_TREE_VIEW (task_tree);

	data = g_new0 (ColPropertyData, 1);

	type = mrp_property_get_property_type (property);

	/* The costs are edited in resources view 
	   if (type == MRP_PROPERTY_TYPE_COST) {
	   return;
	   } */

	if (object_type != MRP_TYPE_TASK) {
		return;
	}
	
	if (type == MRP_PROPERTY_TYPE_DATE) {
		cell = planner_cell_renderer_date_new (FALSE);
		g_signal_connect (cell,
				  "show_popup",
				  G_CALLBACK (task_tree_property_date_show_popup),
				  tree);
	} else {
		cell = gtk_cell_renderer_text_new ();			
	}

	g_object_set (cell, "editable", TRUE, NULL);

	g_signal_connect_data (cell,
			       "edited",
			       G_CALLBACK (task_tree_property_value_edited),
			       data,
			       (GClosureNotify) g_free,
			       0);

	col = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_resizable (col, TRUE);
	gtk_tree_view_column_set_title (col, 
					mrp_property_get_label (property));

	g_hash_table_insert (priv->property_to_column, property, col);
	
	data->property = property;
	data->tree = task_tree;

	gtk_tree_view_column_pack_start (col, cell, TRUE);

	gtk_tree_view_column_set_cell_data_func (col,
						 cell,
						 task_tree_property_data_func,
						 property,
						 NULL);
	g_object_set_data (G_OBJECT (col),
			   "data-func", task_tree_property_data_func);
	g_object_set_data (G_OBJECT (col),
			   "user-data", property);

	gtk_tree_view_append_column (tree, col);
}

static void
task_tree_property_removed (MrpProject       *project,
			    MrpProperty      *property,
			    PlannerTaskTree  *task_tree)
{
	PlannerTaskTreePriv *priv;
	GtkTreeViewColumn   *col;

	priv = task_tree->priv;
	
	col = g_hash_table_lookup (priv->property_to_column, property);
	if (col) {
		g_hash_table_remove (priv->property_to_column, property);

		gtk_tree_view_remove_column (GTK_TREE_VIEW (task_tree), col);
	}
}

static void
task_tree_property_changed (MrpProject      *project,
			    MrpProperty     *property,
			    PlannerTaskTree *task_tree)
{
	PlannerTaskTreePriv *priv;
	GtkTreeViewColumn   *col;

	priv = task_tree->priv;
	
	col = g_hash_table_lookup (priv->property_to_column, property);
	if (col) {
		gtk_tree_view_column_set_title (col, 
				mrp_property_get_label (property));
	}
}

void
planner_task_tree_set_model (PlannerTaskTree   *tree,
			     PlannerGanttModel *model)
{
	gtk_tree_view_set_model (GTK_TREE_VIEW (tree),
				 GTK_TREE_MODEL (model));

	g_signal_connect (model,
			  "row-inserted",
			  G_CALLBACK (task_tree_row_inserted),
			  tree);

	g_signal_connect (model,
			  "task-added",
			  G_CALLBACK (task_tree_task_added_cb),
			  tree);
	
	g_signal_connect (model,
			  "task-removed",
			  G_CALLBACK (task_tree_task_removed_cb),
			  tree);

	gtk_tree_view_expand_all (GTK_TREE_VIEW (tree));
}

static void
task_tree_setup_tree_view (GtkTreeView       *tree,
			   MrpProject        *project,
			   PlannerGanttModel *model)
{
	PlannerTaskTree  *task_tree;
	GtkTreeSelection *selection;

	task_tree = PLANNER_TASK_TREE (tree);
	
	planner_task_tree_set_model (task_tree, model);

	gtk_tree_view_set_rules_hint (tree, TRUE);
	gtk_tree_view_set_reorderable (tree, TRUE);
	
	g_signal_connect (tree,
			  "popup_menu",
			  G_CALLBACK (task_tree_tree_view_popup_menu),
			  tree);
	
	g_signal_connect (tree,
			  "button_press_event",
			  G_CALLBACK (task_tree_tree_view_button_press_event),
			  tree);

	g_signal_connect (tree,
			  "key_release_event",
			  G_CALLBACK (task_tree_tree_view_key_release_event),
			  tree);

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree));
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_MULTIPLE);
	g_signal_connect (selection, "changed",
			  G_CALLBACK (task_tree_selection_changed_cb),
			  tree);
	
	if (task_tree->priv->custom_properties) {
		g_signal_connect (project,
				  "property_added",
				  G_CALLBACK (task_tree_property_added),
				  tree);
		
		g_signal_connect (project,
				  "property_removed",
				  G_CALLBACK (task_tree_property_removed),
				  tree);

		g_signal_connect (project,
				  "property_changed",
				  G_CALLBACK (task_tree_property_changed),
				  tree);
	}
}

static void
task_tree_add_column (GtkTreeView *tree,
		      gint         column,
		      const gchar *title)
{
	GtkTreeViewColumn *col;
	GtkCellRenderer   *cell;

	switch (column) {
	case COL_WBS:
		cell = gtk_cell_renderer_text_new ();

		col = gtk_tree_view_column_new_with_attributes (title,
								cell,
								NULL);
		gtk_tree_view_column_set_cell_data_func (col,
							 cell,
							 task_tree_wbs_data_func,
							 tree, NULL);
		g_object_set_data (G_OBJECT (col),
				   "data-func", task_tree_wbs_data_func);
		g_object_set_data (G_OBJECT (col),
				   "user-data", tree);
		
		gtk_tree_view_column_set_resizable (col, TRUE);
		gtk_tree_view_column_set_min_width (col, 50);
		gtk_tree_view_append_column (tree, col);
		break;

	case COL_NAME:
		cell = gtk_cell_renderer_text_new ();
		g_object_set (cell, "editable", TRUE, NULL);

		g_signal_connect (cell,
				  "edited",
				  G_CALLBACK (task_tree_name_edited),
				  tree);

		col = gtk_tree_view_column_new_with_attributes (title,
								cell,
								NULL);
		gtk_tree_view_column_set_cell_data_func (col,
							 cell,
							 task_tree_name_data_func,
							 tree, NULL);
		g_object_set_data (G_OBJECT (col),
				   "data-func", task_tree_name_data_func);
		g_object_set_data (G_OBJECT (col),
				   "user-data", tree);
		
		gtk_tree_view_column_set_resizable (col, TRUE);
		gtk_tree_view_column_set_min_width (col, 100);
		gtk_tree_view_append_column (tree, col);

		gtk_tree_view_set_expander_column (tree, col);
                                             
		break;

	case COL_START:
		cell = planner_cell_renderer_date_new (TRUE);
		g_signal_connect (cell,
				  "edited",
				  G_CALLBACK (task_tree_start_edited),
				  tree);
		g_signal_connect (cell,
				  "show-popup",
				  G_CALLBACK (task_tree_start_show_popup),
				  tree);
		
		col = gtk_tree_view_column_new_with_attributes (title,
								cell,
								NULL);
		gtk_tree_view_column_set_resizable (col, TRUE);
		gtk_tree_view_column_set_min_width (col, 70);
		gtk_tree_view_column_set_cell_data_func (col,
							 cell,
							 task_tree_start_data_func,
							 tree, NULL);
		g_object_set_data (G_OBJECT (col),
				   "data-func", task_tree_start_data_func);
		g_object_set_data (G_OBJECT (col),
				   "user-data", tree);
		
		gtk_tree_view_append_column (tree, col);
		break;
		
	case COL_DURATION:
		cell = gtk_cell_renderer_text_new ();
		col = gtk_tree_view_column_new_with_attributes (title,
								cell,
								NULL);
		gtk_tree_view_column_set_resizable (col, TRUE);
		gtk_tree_view_column_set_cell_data_func (col,
							 cell,
							 task_tree_duration_data_func,
							 tree,
							 NULL);
		g_object_set_data (G_OBJECT (col),
				   "data-func", task_tree_duration_data_func);
		g_object_set_data (G_OBJECT (col),
				   "user-data", tree);
		g_signal_connect (cell,
				  "edited",
				  G_CALLBACK (task_tree_duration_edited),
				  tree);
		
		gtk_tree_view_append_column (tree, col);
		break;

	case COL_WORK:
		cell = gtk_cell_renderer_text_new ();
		col = gtk_tree_view_column_new_with_attributes (title,
								cell,
								NULL);
		gtk_tree_view_column_set_resizable (col, TRUE);
		gtk_tree_view_column_set_cell_data_func (col,
							 cell,
							 task_tree_work_data_func,
							 tree,
							 NULL);
		g_object_set_data (G_OBJECT (col),
				   "data-func", task_tree_work_data_func);
		g_object_set_data (G_OBJECT (col),
				   "user-data", tree);

		g_signal_connect (cell,
				  "edited",
				  G_CALLBACK (task_tree_work_edited),
				  tree);
		
		gtk_tree_view_append_column (tree, col);
		break;
		
	case COL_SLACK:
		cell = gtk_cell_renderer_text_new ();
		col = gtk_tree_view_column_new_with_attributes (title,
								cell,
								NULL);
		gtk_tree_view_column_set_resizable (col, TRUE);
		gtk_tree_view_column_set_cell_data_func (col,
							 cell,
							 task_tree_slack_data_func,
							 tree,
							 NULL);
		g_object_set_data (G_OBJECT (col),
				   "data-func", task_tree_slack_data_func);
		g_object_set_data (G_OBJECT (col),
				   "user-data", tree);
		
		gtk_tree_view_append_column (tree, col);
		break;
		
	case COL_FINISH:
		cell = planner_cell_renderer_date_new (FALSE);
		/*g_signal_connect (cell,
		  "edited",
		  G_CALLBACK (task_tree_start_edited),
		  tree);*/
		g_signal_connect (cell,
				  "show-popup",
				  G_CALLBACK (task_tree_start_show_popup),
				  tree);
		
		col = gtk_tree_view_column_new_with_attributes (title,
								cell,
								NULL);
		gtk_tree_view_column_set_resizable (col, TRUE);
		gtk_tree_view_column_set_min_width (col, 70);
		gtk_tree_view_column_set_cell_data_func (col,
							 cell,
							 task_tree_finish_data_func,
							 tree, NULL);
		g_object_set_data (G_OBJECT (col),
				   "data-func", task_tree_finish_data_func);
		g_object_set_data (G_OBJECT (col),
				   "user-data", tree);

		gtk_tree_view_append_column (tree, col);
		break;

	case COL_COST:
		cell = gtk_cell_renderer_text_new ();
		col = gtk_tree_view_column_new_with_attributes (title,
								cell,
								NULL);
		gtk_tree_view_column_set_resizable (col, TRUE);
		gtk_tree_view_column_set_cell_data_func (col,
							 cell,
							 task_tree_cost_data_func,
							 tree,
							 NULL);
		g_object_set_data (G_OBJECT (col),
				   "data-func", task_tree_cost_data_func);
		g_object_set_data (G_OBJECT (col),
				   "user-data", tree);

		gtk_tree_view_append_column (tree, col);
		break;

	default:
		g_assert_not_reached ();
	}
}

GtkWidget *
planner_task_tree_new (PlannerWindow     *main_window,
		       PlannerGanttModel *model, 
		       gboolean           custom_properties,
		       gint               first_column,
		       ...)
{
	MrpProject          *project;
	PlannerTaskTree     *tree;
	PlannerTaskTreePriv *priv;
	va_list              args;
	gpointer             str;
	gint                 col;
	
	tree = g_object_new (PLANNER_TYPE_TASK_TREE, NULL);

	project = planner_window_get_project (main_window);
	
	priv = tree->priv;

	priv->custom_properties = custom_properties;
	priv->main_window = main_window;
	priv->project = project;

	task_tree_setup_tree_view (GTK_TREE_VIEW (tree), project, model);

	va_start (args, first_column);

	col = first_column;
	while (col != -1) {
		str = va_arg (args, gpointer);

		task_tree_add_column (GTK_TREE_VIEW (tree), col, str);
		
		col = va_arg (args, gint);
	}

	va_end (args);
		
	return GTK_WIDGET (tree);
}

/*
 * Commands
 */

void
planner_task_tree_insert_subtask (PlannerTaskTree *tree)
{
	GtkTreeView       *tree_view;
	PlannerGanttModel *model;
	GtkTreePath       *path;
	MrpTask           *parent;
	GList             *list;
	gint               work;
	gint               depth;
	gint               position;
	PlannerCmd        *cmd;
	GtkTreeIter        iter;

	list = planner_task_tree_get_selected_tasks (tree);
	if (list == NULL) {
		return;
	}

	parent = list->data;

	model = PLANNER_GANTT_MODEL (gtk_tree_view_get_model (GTK_TREE_VIEW (tree)));

	path = planner_gantt_model_get_path_from_task (model, parent);

	if (gtk_tree_model_get_iter (GTK_TREE_MODEL (model), &iter, path)) {
		position = gtk_tree_model_iter_n_children (GTK_TREE_MODEL (model), &iter);
	} else {
		position = 0;
	}
	
	gtk_tree_path_append_index (path, position);
	
	work = mrp_calendar_day_get_total_work (
		mrp_project_get_calendar (tree->priv->project),
		mrp_day_get_work ());

	depth = gtk_tree_path_get_depth (path);
	position = gtk_tree_path_get_indices (path)[depth - 1];

	if (depth > 1) {
		gtk_tree_path_up (path);
		parent = task_tree_get_task_from_path (tree, path);
	} else {
		parent = NULL;
	}

	cmd = planner_task_cmd_insert (tree->priv->main_window, 
				       parent, position, work, work, NULL);

	/* cmd = planner_task_cmd_insert (tree, path, work, work); */

	if (!GTK_WIDGET_HAS_FOCUS (tree)) {
		gtk_widget_grab_focus (GTK_WIDGET (tree));
	}

	tree_view = GTK_TREE_VIEW (tree);
	
	model = PLANNER_GANTT_MODEL (gtk_tree_view_get_model (tree_view));
	
	gtk_tree_view_set_cursor (tree_view,
				  path,
				  gtk_tree_view_get_column (tree_view, 0),
				  TRUE);
	
	planner_task_tree_set_anchor (tree, path);
	
	g_list_free (list);
}

void
planner_task_tree_insert_task (PlannerTaskTree *tree)
{
	PlannerTaskTreePriv *priv;
	GtkTreeView         *tree_view;
	PlannerGanttModel   *model;
	GtkTreePath         *path;
	MrpTask             *parent;
	GList               *list;
	gint                 work;
	gint                 position;
	gint                 depth;
	PlannerCmd          *cmd;

	priv = tree->priv;
	
	list = planner_task_tree_get_selected_tasks (tree);
	if (list == NULL) {
		parent = NULL;
		position = -1;
	} else {
		parent = mrp_task_get_parent (list->data);
		position = mrp_task_get_position (list->data) + 1;

		if (mrp_task_get_parent (parent) == NULL) {
			parent = NULL;
		}
	}

	if (parent) {
		model = PLANNER_GANTT_MODEL (gtk_tree_view_get_model (GTK_TREE_VIEW (tree)));

		path = planner_gantt_model_get_path_from_task (model, parent);
		gtk_tree_path_append_index (path, position);
	} else {
		path = gtk_tree_path_new ();
		if (position != -1) {
			gtk_tree_path_append_index (path, position);
		} else {
			gtk_tree_path_append_index (path, 0);
		}
	}

	/*planner_cmd_manager_begin_transaction (planner_window_get_cmd_manager (priv->main_window),
	  _("Insert Task"));*/
		
	work = mrp_calendar_day_get_total_work (
		mrp_project_get_calendar (priv->project),
		mrp_day_get_work ());
	
	/* cmd = planner_task_cmd_insert (tree, path, work, work); */
	
	depth = gtk_tree_path_get_depth (path);
	position = gtk_tree_path_get_indices (path)[depth - 1];

	if (depth > 1) {
		gtk_tree_path_up (path);
		parent = task_tree_get_task_from_path (tree, path);
	} else {
		parent = NULL;
	}

	cmd = planner_task_cmd_insert (tree->priv->main_window, 
				       parent, position, work, work, NULL);
	
	if (!GTK_WIDGET_HAS_FOCUS (tree)) {
		gtk_widget_grab_focus (GTK_WIDGET (tree));
	}

	tree_view = GTK_TREE_VIEW (tree);
	
	gtk_tree_view_set_cursor (tree_view,
				  path,
				  gtk_tree_view_get_column (tree_view, 0),
				  TRUE);

	planner_task_tree_set_anchor (tree, path);
	
	g_list_free (list);
}

void
planner_task_tree_remove_task (PlannerTaskTree *tree)
{
	GList             *list, *l;
	TaskCmdRemove     *cmd;
	PlannerGanttModel *model;
	
	list = planner_task_tree_get_selected_tasks (tree);
	if (list == NULL) {
		return;
	}

	model = PLANNER_GANTT_MODEL (gtk_tree_view_get_model (GTK_TREE_VIEW (tree)));

	for (l = list; l; l = l->next) {
		MrpTask     *task = l->data;
		GtkTreePath *path;

		path = planner_gantt_model_get_path_from_task (model, task);

		/* Children are removed with the parent. */
		if (path != NULL) {
			cmd = (TaskCmdRemove*) task_cmd_remove (tree, path, task);
		}
		gtk_tree_path_free (path);
		/* mrp_project_remove_task (tree->priv->project, l->data); */
	}
	
	g_list_free (list);
	
	planner_task_tree_set_anchor (tree, NULL);
}

void
planner_task_tree_edit_task (PlannerTaskTree *tree, PlannerTaskDialogPage page)
{
	PlannerTaskTreePriv *priv;
	MrpTask             *task;
	GList               *list, *l;
	GtkWidget           *dialog;
	gint                 i;
	gint                 result;
	gboolean             proceed;

	g_return_if_fail (PLANNER_IS_TASK_TREE (tree));
	
	priv = tree->priv;
	
	list = planner_task_tree_get_selected_tasks (tree);
	if (list == NULL) {
		return;
	}

	i = g_list_length (list);

	proceed = TRUE;

	if (i >= WARN_TASK_DIALOGS ) {
		/* FIXME: Use ngettext when we've left the stone ages
		 * (i.e. GNOME 2.0). Also this whole thing with several dialogs
		 * is just a workaround for now, should replace it with a
		 * multitask editing dialog for setting a property to the same
		 * value for all selected tasks. We also need to improve the way
		 * data is input generally in the views.
		 */
		dialog = gtk_message_dialog_new (NULL,
						 GTK_DIALOG_DESTROY_WITH_PARENT,
						 GTK_MESSAGE_QUESTION,
						 GTK_BUTTONS_YES_NO,
						 _("You are about to open an edit dialog each for %i tasks. "
						   "Are you sure that you want to do that?"),
						 i);
		
		result = gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
		
		switch (result) {
		case GTK_RESPONSE_YES:
			proceed= TRUE;
			break;
		default:
			proceed = FALSE;
			break;
		}
	}

	if (proceed) {
		for (l = list, i = 0; l && i < MAX_TASK_DIALOGS; l = l->next, i++) {
			task = l->data;
			dialog = planner_task_dialog_new (priv->main_window, task, page);
			gtk_widget_show (dialog);
		}
	}

	g_list_free (list);
}

static void
task_tree_insert_tasks_dialog_destroy_cb (GtkWidget *dialog,
					  GObject   *window)
{
	g_object_set_data (window, "input-tasks-dialog", NULL);
}

void
planner_task_tree_insert_tasks (PlannerTaskTree   *tree)
{
	PlannerTaskTreePriv *priv;
	GtkWidget           *widget;

	g_return_if_fail (PLANNER_IS_TASK_TREE (tree));
	
	priv = tree->priv;

	/* We only want one of these dialogs per main window. */
	widget = g_object_get_data (G_OBJECT (priv->main_window), "input-tasks-dialog");
	if (widget) {
		gtk_window_present (GTK_WINDOW (widget));
		return;
	}

	widget = planner_task_input_dialog_new (priv->main_window);
	gtk_window_set_transient_for (GTK_WINDOW (widget),
				      GTK_WINDOW (priv->main_window));
	gtk_widget_show (widget);

	g_object_set_data (G_OBJECT (priv->main_window), "input-tasks-dialog", widget);
	
	g_signal_connect_object (widget,
				 "destroy",
				 G_CALLBACK (task_tree_insert_tasks_dialog_destroy_cb),
				 priv->main_window,
				 0);
}

void
planner_task_tree_select_all (PlannerTaskTree *tree)
{
	GtkTreeSelection *selection;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree));

	gtk_tree_selection_select_all (selection);
}

void
planner_task_tree_unlink_task (PlannerTaskTree *tree)
{
	MrpTask     *task;
	GList       *list, *l;
	GList       *relations, *r;
	MrpRelation *relation;

	list = planner_task_tree_get_selected_tasks (tree);
	if (list == NULL) {
		return;
	}

	for (l = list; l; l = l->next) {
		task = l->data;

		relations = g_list_copy (mrp_task_get_predecessor_relations (task));
		for (r = relations; r; r = r->next) {
			relation = r->data;
			
			/* mrp_task_remove_predecessor (
			   task, mrp_relation_get_predecessor (relation)); */

			planner_task_cmd_unlink (tree->priv->main_window, relation); 
		}

		g_list_free (relations);
		
		relations = g_list_copy (mrp_task_get_successor_relations (task));
		for (r = relations; r; r = r->next) {
			relation = r->data;

			/* mrp_task_remove_predecessor (
			   mrp_relation_get_successor (relation), task); */

			planner_task_cmd_unlink (tree->priv->main_window, relation);
		}

		g_list_free (relations);
	}
	
	g_list_free (list);
}

void
planner_task_tree_link_tasks (PlannerTaskTree *tree,
			      MrpRelationType  relationship)
{
	MrpTask   *task;
	MrpTask   *target_task;
	GList     *list, *l;
	GtkWidget *dialog;

	list = planner_task_tree_get_selected_tasks (tree);
	if (list == NULL) {
		return;
	}

	list = g_list_reverse (list);
	
	target_task = list->data;
	for (l = list->next; l; l = l->next) {
		PlannerCmd          *cmd;
		GError              *error = NULL;

		task = l->data;

		cmd = planner_task_cmd_link (tree->priv->main_window, task, target_task, 
					     relationship, 0, &error);

		/* cmd = mrp_task_add_predecessor (target_task,
						task,
						relationship,
						0,
						&error)); */
		if (!cmd) {
			dialog = gtk_message_dialog_new (NULL,
							 GTK_DIALOG_DESTROY_WITH_PARENT,
							 GTK_MESSAGE_ERROR,
							 GTK_BUTTONS_OK,
							 "%s", error->message);
			gtk_dialog_run (GTK_DIALOG (dialog));
			gtk_widget_destroy (dialog);
			g_error_free (error);
		}
		
		target_task = task;
	}
	
	g_list_free (list);
}

void
planner_task_tree_indent_task (PlannerTaskTree *tree)
{
	PlannerGanttModel *model;
	MrpTask           *task;
	MrpTask           *new_parent;
	MrpTask           *first_task_parent;
	MrpProject        *project;
	GList             *list, *l;
	GList             *indent_tasks = NULL;
	GtkTreePath       *path;
	GtkWidget         *dialog;
	GtkTreeSelection  *selection;

	project = tree->priv->project;

	model = PLANNER_GANTT_MODEL (gtk_tree_view_get_model (GTK_TREE_VIEW (tree)));
	
	list = planner_task_tree_get_selected_tasks (tree);
	if (list == NULL) {
		return;
	}
	
	task = list->data;
	
	new_parent = planner_gantt_model_get_indent_task_target (model, task);
	if (new_parent == NULL) {
		g_list_free (list);
		return;
	}

	first_task_parent = mrp_task_get_parent (task);

	/* Get a list of tasks that have the same parent as the first one. */
	for (l = list; l; l = l->next) {
		task = l->data;
		
		if (mrp_task_get_parent (task) == first_task_parent) {
			indent_tasks = g_list_prepend (indent_tasks, task);
		}
	}
	g_list_free (list);

	indent_tasks = g_list_reverse (indent_tasks);

	for (l = indent_tasks; l; l = l->next) {
		TaskCmdMove *cmd;
		GError      *error = NULL;
		
		task = l->data;

		cmd = (TaskCmdMove *) 
			task_cmd_move (tree, task, NULL, new_parent, FALSE, &error); 

		/* cmd = mrp_project_move_task (project,
						 task,
						 NULL,
						 new_parent,
						 FALSE,
					     &error); */

		if (!cmd) {
			dialog = gtk_message_dialog_new (GTK_WINDOW (tree->priv->main_window),
							 GTK_DIALOG_DESTROY_WITH_PARENT,
							 GTK_MESSAGE_ERROR,
							 GTK_BUTTONS_OK,
							 "%s",
							 error->message);
			
			gtk_dialog_run (GTK_DIALOG (dialog));
			gtk_widget_destroy (dialog);
			g_error_free (error);
		}
	}

	path = planner_gantt_model_get_path_from_task (PLANNER_GANTT_MODEL (model), 
						       indent_tasks->data);

	task_tree_block_selection_changed (tree);
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree));
	gtk_tree_selection_select_path (selection, path);
	task_tree_unblock_selection_changed (tree);

	gtk_tree_path_free (path);

	g_list_free (indent_tasks);
}

void
planner_task_tree_unindent_task (PlannerTaskTree *tree)
{
	PlannerGanttModel *model;
	MrpTask           *task;
	MrpTask           *new_parent;
	MrpTask           *first_task_parent;
	MrpProject        *project;
	GList             *list, *l;
	GList             *unindent_tasks = NULL;
	GtkTreePath       *path;
	GtkTreeSelection  *selection;

	project = tree->priv->project;

	model = PLANNER_GANTT_MODEL (gtk_tree_view_get_model (GTK_TREE_VIEW (tree)));
	
	list = planner_task_tree_get_selected_tasks (tree);
	if (list == NULL) {
		return;
	}

	task = list->data;

	new_parent = mrp_task_get_parent (task);
	if (new_parent != NULL) {
		new_parent = mrp_task_get_parent (new_parent);
	}
	if (new_parent == NULL) {
		/* No grandparent to unindent to. */ 
		g_list_free (list);
		return;
	}
	
	first_task_parent = mrp_task_get_parent (task);

	/* Get a list of tasks that have the same parent as the first one. */
	for (l = list; l; l = l->next) {
		task = l->data;
		
		if (mrp_task_get_parent (task) == first_task_parent) {
			unindent_tasks = g_list_prepend (unindent_tasks, task);
		}
	}
	g_list_free (list);

	unindent_tasks = g_list_reverse (unindent_tasks);

	for (l = unindent_tasks; l; l = l->next) {
		task = l->data;

		task_cmd_move (tree, task, NULL, new_parent, FALSE, NULL);

		/* mrp_project_move_task (project,
				       task,
				       NULL,
				       new_parent,
				       FALSE,
				       NULL); */
	}

	path = planner_gantt_model_get_path_from_task (PLANNER_GANTT_MODEL (model), 
						       unindent_tasks->data);

	task_tree_block_selection_changed (tree);
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree));
	gtk_tree_selection_select_path (selection, path);
	task_tree_unblock_selection_changed (tree);

	gtk_tree_path_free (path);

	g_list_free (unindent_tasks);
}

void 
planner_task_tree_move_task_up (PlannerTaskTree *tree)
{
	GtkTreeSelection  *selection;
	PlannerGanttModel *model;
	GtkTreePath	  *path;
	MrpProject  	  *project;
	MrpTask	    	  *task, *parent, *sibling;
	GList	    	  *list, *l, *m;
	guint	    	   position;
	gboolean	   proceed, skip;
	gint		   count;
	MrpTask           *anchor_task;

	project = tree->priv->project;

	list = planner_task_tree_get_selected_tasks (tree);
	if (list == NULL) {
		/* Nothing selected */
		return;
	} 

	task_tree_block_selection_changed (tree);
	
	model = PLANNER_GANTT_MODEL (gtk_tree_view_get_model (GTK_TREE_VIEW (tree)));

	path = planner_task_tree_get_anchor (tree);
	if (path) {
		anchor_task = planner_gantt_model_get_task_from_path (model, path);
	} else {
		anchor_task = NULL;
	}

 	proceed = TRUE;
 	count = 0 ;
 
	for (l = list; l; l = l->next) {
 		count++;

 		task = l->data;
 		position = mrp_task_get_position (task);
 		parent = mrp_task_get_parent (task);
  	
		/* We now check if the parent is selected as well me. If it is
		 * then we skip checks on our position and skip moving because
		 * its just not relevant.
		 */

		/* FIXME: This checking isn't enough, we need to check if any
		 * ancestor of the task is selected. The following won't pick up
		 * unusual selections e.g. if just 1.1 and 1.1.1.1 selected. To
		 * do that we need to recurse this selection list.
		 */
		 
 		skip = FALSE;
 		for (m = list; m; m = m->next) {
 			if (m->data == parent ) {
 				skip = TRUE;
				break;
 			}
 		}
  		
 		if (position == 0 && count == 1) {
			/* We stop everything if at top of list and first task
			 * else just stop moving this one task.
			 */
 			proceed = FALSE;
 		}
 		if (!skip && position != 0 && proceed) {
			/* Move task from position to position - 1. */
 			sibling = mrp_task_get_nth_child (parent, position - 1);
			task_cmd_move (tree, task, sibling, parent, TRUE, NULL);
 			/* mrp_project_move_task (project, task, sibling, 
			   parent, TRUE, NULL);*/
 		}
	}

	/* Reselect all the previous selected tasks. */
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree));
	for (l = list; l; l = l->next) {
		task = l->data;

		path = planner_gantt_model_get_path_from_task (model, task);
		gtk_tree_selection_select_path (selection, path);
		gtk_tree_path_free (path);
	}

	/* Restore anchor to the path of the original anchor task. */
	if (anchor_task) {
		path = planner_gantt_model_get_path_from_task (model, anchor_task);
		planner_task_tree_set_anchor (tree, path);
	}
	
	g_list_free (list);

	task_tree_unblock_selection_changed (tree);
}

void 
planner_task_tree_move_task_down (PlannerTaskTree *tree)
{
	GtkTreeSelection  *selection;
	PlannerGanttModel *model;
	GtkTreePath	  *path;
	MrpProject 	  *project;
	MrpTask	   	  *task, *parent, *sibling;
	GList		  *list, *l, *m;
	guint		   position;
	gboolean	   proceed, skip;
	gint		   count;
	MrpTask           *anchor_task;
	MrpTask           *root;

	project = tree->priv->project;

	list = planner_task_tree_get_selected_tasks (tree);
	if (list == NULL) {
		/* Nothing selected */
		return;
	}

	task_tree_block_selection_changed (tree);

	model = PLANNER_GANTT_MODEL (gtk_tree_view_get_model (GTK_TREE_VIEW (tree)));

	path = planner_task_tree_get_anchor (tree);
	if (path) {
		anchor_task = planner_gantt_model_get_task_from_path (model, path);
	} else {
		anchor_task = NULL;
	}

	root = mrp_project_get_root_task (project);
	
	list = g_list_reverse (list);

	proceed = TRUE;
	count = 0 ;

	for (l = list; l; l = l->next) {
		count++;

		task = l->data;
		position = mrp_task_get_position (task);
		parent = mrp_task_get_parent (task);

		/* We now check if parent is selected also. If so then we skip
		 * checks on our position as its not relevant.
		 */
		
		/* FIXME: This checking isn't enough, we need to check if any
		 * ancestor of the task is selected. The following won't pick up
		 * unusual selections e.g. if just 1.1 and 1.1.1.1 selected. To
		 * do that we need to recurse this selection list.
		 */
		skip = FALSE;
		for (m = list; m; m = m->next) {
			if (m->data == parent) {
				skip = TRUE;
				break;
			}
		}

		if (parent == root && position == mrp_task_get_n_children (root) - 1) {
			/* We stop if at bottom of project and our parent is
			 * root task.
			 */
			proceed = FALSE;
		}
		else if (!skip && position == mrp_task_get_n_children (parent) - 1) {
			/* If the parent task was selected then we don't care if
			 * we are at the bottom of our particular position. Note
			 * NOT all possible selection cases are catered for.
			 */ 
			proceed = FALSE;
		}
		
		if (!skip && proceed) {
			/* Move task from position to position + 1. */
			sibling = mrp_task_get_nth_child (parent, position + 1);

			/* Moving task from 'position' to 'position + 1' */
			task_cmd_move (tree, task, sibling, parent, FALSE, NULL);
			/* mrp_project_move_task (project, task, sibling, 
			   parent, FALSE, NULL); */
		}
	}

	/* Reselect all the previous selected tasks. */
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree));
	for (l = list; l; l = l->next) {
		task = l->data;
		
		path = planner_gantt_model_get_path_from_task (model, task);
		gtk_tree_selection_select_path (selection, path);
		gtk_tree_path_free (path);
	}

	/* Restore anchor to the path of the original anchor task. */
	if (anchor_task) {
		path = planner_gantt_model_get_path_from_task (model, anchor_task);
		planner_task_tree_set_anchor (tree, path);
	}
	
	task_tree_unblock_selection_changed (tree);
	g_list_free (list);
}

void
planner_task_tree_reset_constraint (PlannerTaskTree *tree)
{
	MrpTask *task;
	GList   *list, *l;

	list = planner_task_tree_get_selected_tasks (tree);

	for (l = list; l; l = l->next) {
		task = l->data;
		/* mrp_task_reset_constraint (task); */
		task_cmd_reset_constraint (tree, task);
		
	}
	
	g_list_free (list);
}

void
planner_task_tree_reset_all_constraints (PlannerTaskTree *tree)
{
	MrpProject *project;
	MrpTask    *task;
	GList      *list, *l;

	project = tree->priv->project;
		
	list = mrp_project_get_all_tasks (project);
	for (l = list; l; l = l->next) {
		task = l->data;
		/* mrp_task_reset_constraint (task); */
		task_cmd_reset_constraint (tree, task);
	}
	
	g_list_free (list);
}

static  void
task_tree_get_selected_func (GtkTreeModel *model,
			     GtkTreePath  *path,
			     GtkTreeIter  *iter,
			     gpointer      data)
{
	GList   **list = data;
	MrpTask  *task;

	gtk_tree_model_get (model,
			    iter,
			    COL_TASK, &task,
			    -1);
	
	*list = g_list_prepend (*list, task);
}

GList *
planner_task_tree_get_selected_tasks (PlannerTaskTree *tree)	
{
	GtkTreeSelection *selection;
	GList            *list;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree));
	
	list = NULL;
	gtk_tree_selection_selected_foreach (selection,
					     task_tree_get_selected_func,
					     &list);

	list = g_list_reverse (list);
	
	return list;
}

/* Returns TRUE if one or more of the tasks in the list have links. */
gboolean
planner_task_tree_has_relation (GList *list)
{
	GList   *l;
	MrpTask *task;

	for (l = list; l; l = l->next) {
		task = l->data;

		if (mrp_task_has_relation (task)) {
			return TRUE;
		}
	}

	return FALSE;
}


/* The comments here are for i18n, they get extracted to the po files. */
static Units units[] = {
	{ N_("mon"),     UNIT_MONTH },  /* month unit variant accepted in input */
	{ N_("month"),   UNIT_MONTH },  /* month unit variant accepted in input */
	{ N_("months"),  UNIT_MONTH },  /* month unit variant accepted in input */
	{ N_("w"),       UNIT_WEEK },   /* week unit variant accepted in input */
	{ N_("week"),    UNIT_WEEK },   /* week unit variant accepted in input */
	{ N_("weeks"),   UNIT_WEEK },   /* week unit variant accepted in input */
	{ N_("d"),       UNIT_DAY },    /* day unit variant accepted in input */
	{ N_("day"),     UNIT_DAY },    /* day unit variant accepted in input */
	{ N_("days"),    UNIT_DAY },    /* day unit variant accepted in input */
	{ N_("h"),       UNIT_HOUR },   /* hour unit variant accepted in input */
	{ N_("hour"),    UNIT_HOUR },   /* hour unit variant accepted in input */
	{ N_("hours"),   UNIT_HOUR },   /* hour unit variant accepted in input */
	{ N_("min"),     UNIT_MINUTE }, /* minute unit variant accepted in input */
	{ N_("minute"),  UNIT_MINUTE }, /* minute unit variant accepted in input */
	{ N_("minutes"), UNIT_MINUTE }  /* minute unit variant accepted in input */
};

static Unit
task_tree_get_unit_from_string (const gchar *str)
{
	static Units    *translated_units;
	static gboolean  inited = FALSE;
	Unit             unit = UNIT_NONE;
	gint             i, len;
	gchar           *tmp, *tmp2;

	if (!inited) {
		len = G_N_ELEMENTS (units);

		translated_units = g_new0 (Units, len);
		
		for (i = 0; i < len; i++) {
			tmp = _(units[i].name);

			tmp2 = g_utf8_casefold (tmp, -1);
			/* Not sure this is necessary... */
			tmp = g_utf8_normalize (tmp2, -1, G_NORMALIZE_DEFAULT);

			translated_units[i].name = tmp;
			translated_units[i].unit = units[i].unit;
		}
		
		inited = TRUE;
	}

	
	len = G_N_ELEMENTS (units);
	for (i = 0; i < len; i++) {
		if (!strncmp (str, translated_units[i].name,
			      strlen (translated_units[i].name))) {
			unit = translated_units[i].unit;
		}
	}

	if (unit != UNIT_NONE) {
		return unit;
	}

	/* Try untranslated names as a fallback. */
	for (i = 0; i < len; i++) {
		if (!strncmp (str, units[i].name, strlen (units[i].name))) {
			unit = units[i].unit;
		}
	}

	return unit;
}

static gint
task_tree_multiply_with_unit (gdouble value,
			      Unit    unit,
			      gint    seconds_per_month,
			      gint    seconds_per_week,
			      gint    seconds_per_day)
{
	switch (unit) {
	case UNIT_MONTH:
		value *= seconds_per_month;
		break;
	case UNIT_WEEK:
		value *= seconds_per_week;
		break;
	case UNIT_DAY:
		value *= seconds_per_day;
		break;
	case UNIT_HOUR:
		value *= 60*60;
		break;
	case UNIT_MINUTE:
		value *= 60;
		break;
	case UNIT_NONE:
		return 0;
	}	
	
	return floor (value + 0.5);
}

static gint
task_tree_parse_time_string (PlannerTaskTree  *tree,
			     const gchar *input)
{
	gchar           *tmp;
	gchar           *str;
	gchar           *freeme;
	gchar           *end_ptr;
	gdouble          dbl;
	Unit             unit;
	gint             total;
	gint             seconds_per_month;
	gint             seconds_per_week;
	gint             seconds_per_day;

	seconds_per_day = mrp_calendar_day_get_total_work (
		mrp_project_get_calendar (tree->priv->project),
		mrp_day_get_work ());

	/* Hardcode these for now. */
	seconds_per_week = seconds_per_day * 5;
	seconds_per_month = seconds_per_day * 30;

	tmp = g_utf8_casefold (input, -1);
	/* Not sure this is necessary... */
	str = g_utf8_normalize (tmp, -1, G_NORMALIZE_DEFAULT);
	g_free (tmp);

	freeme = str;

	if (!str) {
		return 0;
	}
	
	total = 0;
	while (*str) {
		while (*str && g_unichar_isalpha (g_utf8_get_char (str))) {   
			str = g_utf8_next_char (str);
		}

		if (*str == 0) {
			break;
		}

		dbl = g_strtod (str, &end_ptr);

		if (end_ptr == str) {
			break;
		}
		
		if (end_ptr) {
			unit = task_tree_get_unit_from_string (end_ptr);

			/* If no unit was specified and it was the first number
			 * in the input, treat it as Day.
			 */
			if (unit == UNIT_NONE && str == freeme) {
				unit = UNIT_DAY;
			}

			total += task_tree_multiply_with_unit (dbl,
							       unit,
							       seconds_per_month,
							       seconds_per_week,
							       seconds_per_day);
		}

		if (*end_ptr == 0) {
			break;
		}
		
		str = end_ptr + 1;
	}

	g_free (freeme);
	
	return total;
}

static MrpProject *
task_tree_get_project (PlannerTaskTree *tree)
{
	return planner_window_get_project (tree->priv->main_window);
}

static MrpTask *
task_tree_get_task_from_path (PlannerTaskTree *tree,
			      GtkTreePath     *path)
{
	PlannerGanttModel   *model;
	MrpTask             *task;
	
	model = PLANNER_GANTT_MODEL (gtk_tree_view_get_model (GTK_TREE_VIEW (tree)));
	task = planner_gantt_model_get_task_from_path (model, path);

	return task;
}

void
planner_task_tree_set_highlight_critical (PlannerTaskTree *tree,
					  gboolean         highlight)
{
	g_return_if_fail (PLANNER_IS_TASK_TREE (tree));

	if (tree->priv->highlight_critical == highlight) {
		return;
	}
	
	tree->priv->highlight_critical = highlight;

	gtk_widget_queue_draw (GTK_WIDGET (tree));
}

gboolean
planner_task_tree_get_highlight_critical (PlannerTaskTree *tree)
{
	g_return_val_if_fail (PLANNER_IS_TASK_TREE (tree), FALSE);
	
	return tree->priv->highlight_critical;
}

void
planner_task_tree_set_anchor (PlannerTaskTree *tree, GtkTreePath *anchor)
{
	g_return_if_fail (PLANNER_IS_TASK_TREE (tree));
	
	if (tree->priv->anchor) {
		gtk_tree_path_free (tree->priv->anchor);
	}
	
	tree->priv->anchor = anchor;
}

GtkTreePath* 
planner_task_tree_get_anchor (PlannerTaskTree *tree)
{
	g_return_val_if_fail (PLANNER_IS_TASK_TREE (tree), FALSE);

	return tree->priv->anchor;
}

PlannerWindow* 
planner_task_tree_get_window (PlannerTaskTree       *tree)
{
	g_return_val_if_fail (PLANNER_IS_TASK_TREE (tree), NULL);

	return tree->priv->main_window;
}
