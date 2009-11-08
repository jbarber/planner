/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2003-2004 Imendio AB
 * Copyright (C) 2003 Benjamin BAYART <benjamin@sitadelle.com>
 * Copyright (C) 2003 Xavier Ordoquy <xordoquy@wanadoo.fr>
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

#ifndef __PLANNER_USAGE_TREE__
#define __PLANNER_USAGE_TREE__

#include <gtk/gtk.h>
#include <libplanner/mrp-project.h>
#include "planner-usage-model.h"
#include "planner-window.h"
#include "planner-task-dialog.h"

#define PLANNER_TYPE_USAGE_TREE               (planner_usage_tree_get_type ())
#define PLANNER_USAGE_TREE(obj)               (GTK_CHECK_CAST ((obj), PLANNER_TYPE_USAGE_TREE, PlannerUsageTree))
#define PLANNER_USAGE_TREE_CLASS(klass)       (GTK_CHECK_CLASS_CAST ((klass), PLANNER_TYPE_USAGE_TREE, PlannerUsageTreeClass))
#define PLANNER_IS_USAGE_TREE(obj)            (GTK_CHECK_TYPE ((obj), PLANNER_TYPE_USAGE_TREE))
#define PLANNER_IS_USAGE_TREE_CLASS(klass)    (GTK_CHECK_CLASS_TYPE ((obj), PLANNER_TYPE_USAGE_TREE))
#define PLANNER_USAGE_TREE_GET_CLASS(obj)     (GTK_CHECK_GET_CLASS ((obj), PLANNER_TYPE_USAGE_TREE, PlannerUsageTreeClass))

typedef struct _PlannerUsageTree PlannerUsageTree;
typedef struct _PlannerUsageTreeClass PlannerUsageTreeClass;
typedef struct _PlannerUsageTreePriv PlannerUsageTreePriv;

struct _PlannerUsageTree {
        GtkTreeView            parent;
        PlannerUsageTreePriv *priv;
};

struct _PlannerUsageTreeClass {
        GtkTreeViewClass parent_class;
};

GType      planner_usage_tree_get_type           (void) G_GNUC_CONST;
GtkWidget *planner_usage_tree_new                (PlannerWindow      *window,
						  PlannerUsageModel *model);
void       planner_usage_tree_set_model          (PlannerUsageTree  *tree,
						  PlannerUsageModel *model);
void       planner_usage_tree_edit_task          (PlannerUsageTree  *tree);
void       planner_usage_tree_edit_resource      (PlannerUsageTree  *tree);
GList *    planner_usage_tree_get_selected_items (PlannerUsageTree  *tree);
void       planner_usage_tree_expand_all         (PlannerUsageTree  *tree);
void       planner_usage_tree_collapse_all       (PlannerUsageTree  *tree);


#endif /* __PLANNER_USAGE_TREE__ */
