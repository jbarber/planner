/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 * Copyright (C) 2003-2004 Imendio HB
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

#ifndef __PLANNER_TTABLE_TREE__
#define __PLANNER_TTABLE_TREE__

#include <gtk/gtktreeview.h>
#include <libplanner/mrp-project.h>
#include "planner-ttable-model.h"
#include "planner-window.h"

#define PLANNER_TYPE_TTABLE_TREE               (planner_ttable_tree_get_type ())
#define PLANNER_TTABLE_TREE(obj)               (GTK_CHECK_CAST ((obj), PLANNER_TYPE_TTABLE_TREE, PlannerTtableTree))
#define PLANNER_TTABLE_TREE_CLASS(klass)       (GTK_CHECK_CLASS_CAST ((klass), PLANNER_TYPE_TTABLE_TREE, PlannerTtableTreeClass))
#define PLANNER_IS_TTABLE_TREE(obj)            (GTK_CHECK_TYPE ((obj), PLANNER_TYPE_TTABLE_TREE))
#define PLANNER_IS_TTABLE_TREE_CLASS(klass)    (GTK_CHECK_CLASS_TYPE ((obj), PLANNER_TYPE_TTABLE_TREE))
#define PLANNER_TTABLE_TREE_GET_CLASS(obj)     (GTK_CHECK_GET_CLASS ((obj), PLANNER_TYPE_TTABLE_TREE, PlannerTtableTreeClass))

typedef struct _PlannerTtableTree PlannerTtableTree;
typedef struct _PlannerTtableTreeClass PlannerTtableTreeClass;
typedef struct _PlannerTtableTreePriv PlannerTtableTreePriv;

struct _PlannerTtableTree {
        GtkTreeView            parent;
        PlannerTtableTreePriv *priv;
};

struct _PlannerTtableTreeClass {
        GtkTreeViewClass parent_class;
};

GType      planner_ttable_tree_get_type           (void) G_GNUC_CONST;
GtkWidget *planner_ttable_tree_new                (PlannerWindow      *window,
						   PlannerTtableModel *model);
void       planner_ttable_tree_set_model          (PlannerTtableTree  *tree,
						   PlannerTtableModel *model);
void       planner_ttable_tree_edit_task          (PlannerTtableTree  *tree);
void       planner_ttable_tree_edit_resource      (PlannerTtableTree  *tree);
GList *    planner_ttable_tree_get_selected_items (PlannerTtableTree  *tree);
void       planner_ttable_tree_expand_all         (PlannerTtableTree  *tree);
void       planner_ttable_tree_collapse_all       (PlannerTtableTree  *tree);


#endif /* __PLANNER_TTABLE_TREE__ */
