/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2002 CodeFactory AB
 * Copyright (C) 2002 Richard Hult <richard@imendio.com>
 * Copyright (C) 2002 Mikael Hallendal <micke@imendio.com>
 * Copyright (C) 2002 Alvaro del Castillo <acs@barrapunto.com>
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

#ifndef __PLANNER_LIST_MODEL_H__
#define __PLANNER_LIST_MODEL_H__

#include <glib-object.h>
#include <gtk/gtktreemodel.h>
#include <gtk/gtktreesortable.h>
#include <libplanner/mrp-object.h>

#define PLANNER_TYPE_LIST_MODEL	          (planner_list_model_get_type ())
#define PLANNER_LIST_MODEL(obj)	          (G_TYPE_CHECK_INSTANCE_CAST ((obj), PLANNER_TYPE_LIST_MODEL, PlannerListModel))
#define PLANNER_LIST_MODEL_CLASS(klass)        (G_TYPE_CHECK_CLASS_CAST ((klass), PLANNER_TYPE_LIST_MODEL, PlannerListModelClass))
#define PLANNER_IS_LIST_MODEL(obj)	          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PLANNER_TYPE_LIST_MODEL))
#define PLANNER_IS_LIST_MODEL_CLASS(klass)     (G_TYPE_CHECK_CLASS_TYPE ((klass), PLANNER_TYPE_LIST_MODEL))
#define PLANNER_LIST_MODEL_GET_CLASS(obj)      (G_TYPE_INSTANCE_GET_CLASS ((obj), PLANNER_TYPE_LIST_MODEL, PlannerListModelClass))

typedef struct _PlannerListModel       PlannerListModel;
typedef struct _PlannerListModelClass  PlannerListModelClass;
typedef struct _PlannerListModelPriv   PlannerListModelPriv;

struct _PlannerListModel
{
        GObject          parent;
	
        PlannerListModelPriv *priv;
};

struct _PlannerListModelClass
{
	GObjectClass parent_class;

	gint  (*get_n_columns)   (GtkTreeModel *tree_model);
	GType (*get_column_type) (GtkTreeModel *tree_model,
				  gint          column);
	void  (*get_value)       (GtkTreeModel *tree_model,
				  GtkTreeIter  *iter,
				  gint          column,
				  GValue       *value);
};

GType            planner_list_model_get_type      (void) G_GNUC_CONST;
void             planner_list_model_append        (PlannerListModel      *model, 
					      MrpObject        *object);
void             planner_list_model_remove        (PlannerListModel      *model, 
					      MrpObject        *object);
void             planner_list_model_update        (PlannerListModel      *model,
					      MrpObject        *object);
GtkTreePath *    planner_list_model_get_path      (PlannerListModel      *model,
					      MrpObject        *object);
MrpObject *      planner_list_model_get_object    (PlannerListModel      *model,
					      GtkTreeIter      *iter);
void             planner_list_model_set_data      (PlannerListModel      *model,
					      GList            *data);
GList *          planner_list_model_get_data      (PlannerListModel      *model);

#endif /* __PLANNER_LIST_MODEL_H__ */
