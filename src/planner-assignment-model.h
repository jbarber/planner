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

#ifndef __MG_ASSIGNMENT_MODEL_H__
#define __MG_ASSIGNMENT_MODEL_H__

#include <libplanner/mrp-project.h>
#include "planner-list-model.h"

#define MG_TYPE_ASSIGNMENT_MODEL	          (planner_assignment_model_get_type ())
#define MG_ASSIGNMENT_MODEL(obj)	          (G_TYPE_CHECK_INSTANCE_CAST ((obj), MG_TYPE_ASSIGNMENT_MODEL, MgAssignmentModel))
#define MG_ASSIGNMENT_MODEL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), MG_TYPE_ASSIGNMENT_MODEL, MgAssignmentModelClass))
#define MG_IS_ASSIGNMENT_MODEL(obj)	  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MG_TYPE_ASSIGNMENT_MODEL))
#define MG_IS_ASSIGNMENT_MODEL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MG_TYPE_ASSIGNMENT_MODEL))
#define MG_ASSIGNMENT_MODEL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), MG_TYPE_ASSIGNMENT_MODEL, MgAssignmentModelClass))

typedef struct _MgAssignmentModel       MgAssignmentModel;
typedef struct _MgAssignmentModelClass  MgAssignmentModelClass;
typedef struct _MgAssignmentModelPriv   MgAssignmentModelPriv;

struct _MgAssignmentModel
{
        MgListModel          parent;

        MgAssignmentModelPriv *priv;
};

struct _MgAssignmentModelClass
{
	MgListModelClass parent_class;
};

enum {
        RESOURCE_ASSIGNMENT_COL_NAME,
        RESOURCE_ASSIGNMENT_COL_UNITS,
        RESOURCE_ASSIGNMENT_COL_COST_STD,
        RESOURCE_ASSIGNMENT_COL_COST_OVT,
        RESOURCE_ASSIGNMENT_COL_ASSIGNED,
        RESOURCE_ASSIGNMENT_COL_ASSIGNED_UNITS,
        RESOURCE_ASSIGNMENT_COL_ASSIGNED_UNITS_EDITABLE,
        NUMBER_OF_RESOURCE_ASSIGNMENT_COLS
};

GType              planner_assignment_model_get_type     (void);
MgAssignmentModel *planner_assignment_model_new          (MrpTask    *task);

#endif /* __MG_ASSIGNMENT_MODEL_H__ */
