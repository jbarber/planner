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

#ifndef __MG_LIST_MODEL_H__
#define __MG_LIST_MODEL_H__

#include <glib-object.h>
#include <gtk/gtktreemodel.h>
#include <gtk/gtktreesortable.h>
#include <libplanner/mrp-object.h>

#define MG_TYPE_LIST_MODEL	          (planner_list_model_get_type ())
#define MG_LIST_MODEL(obj)	          (G_TYPE_CHECK_INSTANCE_CAST ((obj), MG_TYPE_LIST_MODEL, MgListModel))
#define MG_LIST_MODEL_CLASS(klass)        (G_TYPE_CHECK_CLASS_CAST ((klass), MG_TYPE_LIST_MODEL, MgListModelClass))
#define MG_IS_LIST_MODEL(obj)	          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MG_TYPE_LIST_MODEL))
#define MG_IS_LIST_MODEL_CLASS(klass)     (G_TYPE_CHECK_CLASS_TYPE ((klass), MG_TYPE_LIST_MODEL))
#define MG_LIST_MODEL_GET_CLASS(obj)      (G_TYPE_INSTANCE_GET_CLASS ((obj), MG_TYPE_LIST_MODEL, MgListModelClass))

typedef struct _MgListModel       MgListModel;
typedef struct _MgListModelClass  MgListModelClass;
typedef struct _MgListModelPriv   MgListModelPriv;

struct _MgListModel
{
        GObject          parent;
	
        MgListModelPriv *priv;
};

struct _MgListModelClass
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

GType            planner_list_model_get_type      (void);
void             planner_list_model_append        (MgListModel      *model, 
					      MrpObject        *object);
void             planner_list_model_remove        (MgListModel      *model, 
					      MrpObject        *object);
void             planner_list_model_update        (MgListModel      *model,
					      MrpObject        *object);
GtkTreePath *    planner_list_model_get_path      (MgListModel      *model,
					      MrpObject        *object);
MrpObject *      planner_list_model_get_object    (MgListModel      *model,
					      GtkTreeIter      *iter);
void             planner_list_model_set_data      (MgListModel      *model,
					      GList            *data);
GList *          planner_list_model_get_data      (MgListModel      *model);

#endif /* __MG_LIST_MODEL_H__ */
