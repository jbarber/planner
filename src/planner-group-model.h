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

#ifndef __MG_GROUP_MODEL_H__
#define __MG_GROUP_MODEL_H__

#include <glib-object.h>
#include <gtk/gtktreemodel.h>
#include <gtk/gtktreesortable.h>
#include <libplanner/mrp-project.h>
#include <libplanner/mrp-group.h>
#include "planner-list-model.h"

#define MG_TYPE_GROUP_MODEL	          (planner_group_model_get_type ())
#define MG_GROUP_MODEL(obj)	          (G_TYPE_CHECK_INSTANCE_CAST ((obj), MG_TYPE_GROUP_MODEL, MgGroupModel))
#define MG_GROUP_MODEL_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass), MG_TYPE_GROUP_MODEL, MgGroupModelClass))
#define MG_IS_GROUP_MODEL(obj)	          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MG_TYPE_GROUP_MODEL))
#define MG_IS_GROUP_MODEL_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass), MG_TYPE_GROUP_MODEL))
#define MG_GROUP_MODEL_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj), MG_TYPE_GROUP_MODEL, MgGroupModelClass))

typedef struct _MgGroupModel       MgGroupModel;
typedef struct _MgGroupModelClass  MgGroupModelClass;
typedef struct _MgGroupModelPriv   MgGroupModelPriv;

struct _MgGroupModel
{
        MgListModel       parent;

        MgGroupModelPriv *priv;
};

struct _MgGroupModelClass
{
	MgListModelClass  parent_class;
};

enum {
        GROUP_COL_NAME,
        GROUP_COL_GROUP_DEFAULT,
        GROUP_COL_MANAGER_NAME,
        GROUP_COL_MANAGER_PHONE,
        GROUP_COL_MANAGER_EMAIL,
        NUMBER_OF_GROUP_COLS
};


GType           planner_group_model_get_type        (void);
MgGroupModel *  planner_group_model_new             (MrpProject      *project);

#endif /* __MG_GROUP_MODEL_H__ */
