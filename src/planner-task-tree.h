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

#ifndef __MG_TASK_TREE_H__
#define __MG_TASK_TREE_H__

#include <gtk/gtktreeview.h>
#include <libplanner/mrp-project.h>
#include "planner-gantt-model.h"
#include "planner-main-window.h"

#define MG_TYPE_TASK_TREE		(planner_task_tree_get_type ())
#define MG_TASK_TREE(obj)		(GTK_CHECK_CAST ((obj), MG_TYPE_TASK_TREE, MgTaskTree))
#define MG_TASK_TREE_CLASS(klass)	(GTK_CHECK_CLASS_CAST ((klass), MG_TYPE_TASK_TREE, MgTaskTreeClass))
#define MG_IS_TASK_TREE(obj)		(GTK_CHECK_TYPE ((obj), MG_TYPE_TASK_TREE))
#define MG_IS_TASK_TREE_CLASS(klass)	(GTK_CHECK_CLASS_TYPE ((obj), MG_TYPE_TASK_TREE))
#define MG_TASK_TREE_GET_CLASS(obj)	(GTK_CHECK_GET_CLASS ((obj), MG_TYPE_TASK_TREE, MgTaskTreeClass))

typedef struct _MgTaskTree           MgTaskTree;
typedef struct _MgTaskTreeClass      MgTaskTreeClass;
typedef struct _MgTaskTreePriv       MgTaskTreePriv;

struct _MgTaskTree
{
	GtkTreeView       parent;
	MgTaskTreePriv   *priv;
};

struct _MgTaskTreeClass
{
	GtkTreeViewClass  parent_class;
};


GType      planner_task_tree_get_type              (void);

GtkWidget *planner_task_tree_new                   (MgMainWindow *main_window,
					       MgGanttModel *model,
					       gboolean      custom_properties,
					       gpointer      first_column,
					       ...);

void       planner_task_tree_set_model             (MgTaskTree   *tree,
					       MgGanttModel *model);

void       planner_task_tree_insert_subtask        (MgTaskTree   *tree);

void       planner_task_tree_insert_task           (MgTaskTree   *tree);

void       planner_task_tree_remove_task           (MgTaskTree   *tree);

void       planner_task_tree_edit_task             (MgTaskTree   *tree);

void       planner_task_tree_insert_tasks          (MgTaskTree   *tree);

void       planner_task_tree_select_all            (MgTaskTree   *tree);

void       planner_task_tree_unlink_task           (MgTaskTree   *tree);

void       planner_task_tree_indent_task           (MgTaskTree   *tree);

void       planner_task_tree_unindent_task         (MgTaskTree   *tree);

void       planner_task_tree_reset_constraint      (MgTaskTree   *tree);

void       planner_task_tree_reset_all_constraints (MgTaskTree   *tree);

void       planner_task_tree_move_task_up          (MgTaskTree   *tree);

void       planner_task_tree_move_task_down        (MgTaskTree   *tree);

GList *    planner_task_tree_get_selected_tasks    (MgTaskTree   *tree);

gboolean   planner_task_tree_has_relation          (GList        *list);

#endif /* __MG_TASK_TREE_H__ */
