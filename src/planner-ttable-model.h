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

#ifndef __PLANNER_TTABLE_MODEL_H__
#define __PLANNER_TTABLE_MODEL_H__

#include <libplanner/mrp-project.h>
#include <libplanner/mrp-task.h>
#include <libplanner/mrp-resource.h>
#include <gtk/gtktreemodel.h>

#define PLANNER_TYPE_TTABLE_MODEL             (planner_ttable_model_get_type ())
#define PLANNER_TTABLE_MODEL(obj)             (GTK_CHECK_CAST ((obj), PLANNER_TYPE_TTABLE_MODEL, PlannerTtableModel))
#define PLANNER_TTABLE_MODEL_CLASS(klass)     (GTK_CHECK_CLASS_CAST ((klass), PLANNER_TYPE_TTABLE_MODEL, PlannerTtableModelClass))
#define PLANNER_IS_TTABLE_MODEL(obj)          (GTK_CHECK_TYPE ((obj), PLANNER_TYPE_TTABLE_MODEL))
#define PLANNER_IS_TTABLE_MODEL_CLASS(klass)  (GTK_CHECK_CLASS_TYPE ((obj), PLANNER_TYPE_TTABLE_MODEL))

typedef struct _PlannerTtableModel PlannerTtableModel;
typedef struct _PlannerTtableModelClass PlannerTtableModelClass;
typedef struct _PlannerTtableModelPriv PlannerTtableModelPriv;

struct _PlannerTtableModel {
        GObject                 parent;
        gint                    stamp;
        PlannerTtableModelPriv *priv;
};

struct _PlannerTtableModelClass {
        GObjectClass parent_class;
};

enum {
        COL_RESNAME,
        COL_TASKNAME,
        COL_RESOURCE,
        COL_ASSIGNMENT,
        NUM_COLS
};

GType               planner_ttable_model_get_type               (void) G_GNUC_CONST;
PlannerTtableModel *planner_ttable_model_new                    (MrpProject         *project);
GtkTreePath *       planner_ttable_model_get_path_from_resource (PlannerTtableModel *model,
								 MrpResource        *resource);
MrpProject *        planner_ttable_model_get_project            (PlannerTtableModel *model);
MrpAssignment *     planner_ttable_model_get_assignment         (PlannerTtableModel *model,
								 GtkTreeIter        *iter);
MrpResource *       planner_ttable_model_get_resource           (PlannerTtableModel *model,
								 GtkTreeIter        *iter);
gboolean            planner_ttable_model_is_resource            (PlannerTtableModel *model,
								 GtkTreeIter        *iter);
gboolean            planner_ttable_model_is_assignment          (PlannerTtableModel *model,
								 GtkTreeIter        *iter);
MrpAssignment *     planner_ttable_model_path_get_assignment    (PlannerTtableModel *model,
								 GtkTreePath        *path);
MrpResource *       planner_ttable_model_path_get_resource      (PlannerTtableModel *model,
								 GtkTreePath        *path);
gboolean            planner_ttable_model_path_is_resource       (PlannerTtableModel *model,
								 GtkTreePath        *path);
gboolean            planner_ttable_model_path_is_assignment     (PlannerTtableModel *model,
								 GtkTreePath        *Path);


#endif /* __PLANNER_TTABLE_MODEL_H__ */
