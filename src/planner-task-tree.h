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

#ifndef __PLANNER_TASK_TREE_H__
#define __PLANNER_TASK_TREE_H__

#include <gtk/gtktreeview.h>
#include <libplanner/mrp-project.h>
#include "planner-gantt-model.h"
#include "planner-window.h"
#include "planner-task-dialog.h"

#define PLANNER_TYPE_TASK_TREE		(planner_task_tree_get_type ())
#define PLANNER_TASK_TREE(obj)		(GTK_CHECK_CAST ((obj), PLANNER_TYPE_TASK_TREE, PlannerTaskTree))
#define PLANNER_TASK_TREE_CLASS(klass)	(GTK_CHECK_CLASS_CAST ((klass), PLANNER_TYPE_TASK_TREE, PlannerTaskTreeClass))
#define PLANNER_IS_TASK_TREE(obj)		(GTK_CHECK_TYPE ((obj), PLANNER_TYPE_TASK_TREE))
#define PLANNER_IS_TASK_TREE_CLASS(klass)	(GTK_CHECK_CLASS_TYPE ((obj), PLANNER_TYPE_TASK_TREE))
#define PLANNER_TASK_TREE_GET_CLASS(obj)	(GTK_CHECK_GET_CLASS ((obj), PLANNER_TYPE_TASK_TREE, PlannerTaskTreeClass))

typedef struct _PlannerTaskTree           PlannerTaskTree;
typedef struct _PlannerTaskTreeClass      PlannerTaskTreeClass;
typedef struct _PlannerTaskTreePriv       PlannerTaskTreePriv;

struct _PlannerTaskTree
{
	GtkTreeView           parent;
	PlannerTaskTreePriv  *priv;
};

struct _PlannerTaskTreeClass
{
	GtkTreeViewClass  parent_class;
};


GType        planner_task_tree_get_type               (void) G_GNUC_CONST;
GtkWidget *  planner_task_tree_new                    (PlannerWindow         *window,
						       PlannerGanttModel     *model,
						       gboolean               custom_properties,
						       gint                   first_column,
						       ...);
void         planner_task_tree_set_model              (PlannerTaskTree       *tree,
						       PlannerGanttModel     *model);
void         planner_task_tree_insert_subtask         (PlannerTaskTree       *tree);
void         planner_task_tree_insert_task            (PlannerTaskTree       *tree);
void         planner_task_tree_remove_task            (PlannerTaskTree       *tree);
void         planner_task_tree_edit_task              (PlannerTaskTree       *tree,
						       PlannerTaskDialogPage  page);
void         planner_task_tree_insert_tasks           (PlannerTaskTree       *tree);
void         planner_task_tree_select_all             (PlannerTaskTree       *tree);
void         planner_task_tree_unlink_task            (PlannerTaskTree       *tree);
void         planner_task_tree_link_tasks             (PlannerTaskTree       *tree,
						       MrpRelationType        relationship);
void         planner_task_tree_indent_task            (PlannerTaskTree       *tree);
void         planner_task_tree_unindent_task          (PlannerTaskTree       *tree);
void         planner_task_tree_reset_constraint       (PlannerTaskTree       *tree);
void         planner_task_tree_reset_all_constraints  (PlannerTaskTree       *tree);
void         planner_task_tree_move_task_up           (PlannerTaskTree       *tree);
void         planner_task_tree_move_task_down         (PlannerTaskTree       *tree);
GList *      planner_task_tree_get_selected_tasks     (PlannerTaskTree       *tree);
gboolean     planner_task_tree_has_relation           (GList                 *list);
void         planner_task_tree_set_highlight_critical (PlannerTaskTree       *tree,
						       gboolean               highlight);
gboolean     planner_task_tree_get_highlight_critical (PlannerTaskTree       *tree);
void         planner_task_tree_set_anchor             (PlannerTaskTree       *tree,
						       GtkTreePath           *anchor);
GtkTreePath* planner_task_tree_get_anchor             (PlannerTaskTree       *tree);

/* Undo support */

PlannerCmd*  planner_task_tree_task_cmd_link          (PlannerTaskTree       *tree,
						       MrpTask               *before,
						       MrpTask               *after,
						       MrpRelationType        relationship,
						       glong                  lag,
						       GError               **error);

/* Temporal functions. We need to approve them. */
PlannerWindow * planner_task_tree_get_window          (PlannerTaskTree       *tree);

#endif /* __PLANNER_TASK_TREE_H__ */
