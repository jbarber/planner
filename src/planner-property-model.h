/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2002 CodeFactory AB
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

#ifndef __PLANNER_PROPERTY_MODEL_H__
#define __PLANNER_PROPERTY_MODEL_H__

#include <gtk/gtktreemodel.h>
#include <libplanner/mrp-project.h>

enum {
	COL_NAME,
	COL_LABEL,
	COL_TYPE,
	COL_VALUE,
	COL_PROPERTY
};

typedef struct {
	GType		 owner_type;
	GtkListStore 	*store;
} MrpPropertyStore;

GtkTreeModel *planner_property_model_new (MrpProject       *project,
					  GType             owner_type,
					  MrpPropertyStore *shop);


#endif /* __PLANNER_PROPERTY_MODEL_H__ */









