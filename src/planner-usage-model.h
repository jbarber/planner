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

#ifndef __PLANNER_USAGE_MODEL_H__
#define __PLANNER_USAGE_MODEL_H__

#include <gtk/gtk.h>
#include <libplanner/mrp-project.h>
#include <libplanner/mrp-task.h>
#include <libplanner/mrp-resource.h>

#define PLANNER_TYPE_USAGE_MODEL             (planner_usage_model_get_type ())
#define PLANNER_USAGE_MODEL(obj)             (GTK_CHECK_CAST ((obj), PLANNER_TYPE_USAGE_MODEL, PlannerUsageModel))
#define PLANNER_USAGE_MODEL_CLASS(klass)     (GTK_CHECK_CLASS_CAST ((klass), PLANNER_TYPE_USAGE_MODEL, PlannerUsageModelClass))
#define PLANNER_IS_USAGE_MODEL(obj)          (GTK_CHECK_TYPE ((obj), PLANNER_TYPE_USAGE_MODEL))
#define PLANNER_IS_USAGE_MODEL_CLASS(klass)  (GTK_CHECK_CLASS_TYPE ((obj), PLANNER_TYPE_USAGE_MODEL))

typedef struct _PlannerUsageModel PlannerUsageModel;
typedef struct _PlannerUsageModelClass PlannerUsageModelClass;
typedef struct _PlannerUsageModelPriv PlannerUsageModelPriv;

struct _PlannerUsageModel {
        GObject                parent;
        gint                   stamp;
        PlannerUsageModelPriv *priv;
};

struct _PlannerUsageModelClass {
        GObjectClass parent_class;
};

enum {
        COL_RESNAME,
        COL_TASKNAME,
        COL_RESOURCE,
        COL_ASSIGNMENT,
        NUM_COLS
};

GType               planner_usage_model_get_type               (void) G_GNUC_CONST;
PlannerUsageModel *planner_usage_model_new                    (MrpProject         *project);
GtkTreePath *       planner_usage_model_get_path_from_resource (PlannerUsageModel *model,
								 MrpResource        *resource);
GtkTreePath *       planner_usage_model_get_path_from_assignment
                                                               (PlannerUsageModel *model,
								MrpAssignment     *assignment);
MrpProject *        planner_usage_model_get_project            (PlannerUsageModel *model);
MrpAssignment *     planner_usage_model_get_assignment         (PlannerUsageModel *model,
								 GtkTreeIter        *iter);
MrpResource *       planner_usage_model_get_resource           (PlannerUsageModel *model,
								 GtkTreeIter        *iter);
gboolean            planner_usage_model_is_resource            (PlannerUsageModel *model,
								 GtkTreeIter        *iter);
gboolean            planner_usage_model_is_assignment          (PlannerUsageModel *model,
								 GtkTreeIter        *iter);
MrpAssignment *     planner_usage_model_path_get_assignment    (PlannerUsageModel *model,
								 GtkTreePath        *path);
MrpResource *       planner_usage_model_path_get_resource      (PlannerUsageModel *model,
								 GtkTreePath        *path);
gboolean            planner_usage_model_path_is_resource       (PlannerUsageModel *model,
								 GtkTreePath        *path);
gboolean            planner_usage_model_path_is_assignment     (PlannerUsageModel *model,
								 GtkTreePath        *Path);


#endif /* __PLANNER_USAGE_MODEL_H__ */
