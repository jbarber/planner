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

#ifndef __PLANNER_PREDECESSOR_MODEL_H__
#define __PLANNER_PREDECESSOR_MODEL_H__

#include <libplanner/mrp-project.h>
#include "planner-list-model.h"

#define PLANNER_TYPE_PREDECESSOR_MODEL	     (planner_predecessor_model_get_type ())
#define PLANNER_PREDECESSOR_MODEL(obj)	     (G_TYPE_CHECK_INSTANCE_CAST ((obj), PLANNER_TYPE_PREDECESSOR_MODEL, PlannerPredecessorModel))
#define PLANNER_PREDECESSOR_MODEL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), PLANNER_TYPE_PREDECESSOR_MODEL, PlannerPredecessorModelClass))
#define PLANNER_IS_PREDECESSOR_MODEL(obj)	     (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PLANNER_TYPE_PREDECESSOR_MODEL))
#define PLANNER_IS_PREDECESSOR_MODEL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), PLANNER_TYPE_PREDECESSOR_MODEL))
#define PLANNER_PREDECESSOR_MODEL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), PLANNER_TYPE_PREDECESSOR_MODEL, PlannerPredecessorModelClass))

typedef struct _PlannerPredecessorModel       PlannerPredecessorModel;
typedef struct _PlannerPredecessorModelClass  PlannerPredecessorModelClass;
typedef struct _PlannerPredecessorModelPriv   PlannerPredecessorModelPriv;

struct _PlannerPredecessorModel
{
        PlannerListModel             parent;

        PlannerPredecessorModelPriv *priv;
};

struct _PlannerPredecessorModelClass
{
	PlannerListModelClass parent_class;
};

enum {
        PREDECESSOR_COL_NAME,
        PREDECESSOR_COL_TYPE,
        PREDECESSOR_COL_LAG,
        NUMBER_OF_PREDECESSOR_COLS
};

GType         planner_predecessor_model_get_type (void) G_GNUC_CONST;
GtkTreeModel *planner_predecessor_model_new      (MrpTask    *task);

#endif /* __PLANNER_PREDECESSOR_MODEL_H__ */
