/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* Evolution calendar - Planner file backend
 *
 * Copyright 2004 (C) Alvaro del Castillo <acs@barrapunto.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef E_CAL_BACKEND_PLANNER_H
#define E_CAL_BACKEND_PLANNER_H

#include <libedata-cal/e-cal-backend-sync.h>
#include <libplanner/mrp-application.h>

G_BEGIN_DECLS



#define E_TYPE_CAL_BACKEND_PLANNER            (e_cal_backend_planner_get_type ())
#define E_CAL_BACKEND_PLANNER(obj)            (G_TYPE_CHECK_INSTANCE_CAST \
					       ((obj), E_TYPE_CAL_BACKEND_PLANNER, \
						ECalBackendPlanner))
#define E_CAL_BACKEND_PLANNER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST \
					       ((klass), \
						E_TYPE_CAL_BACKEND_PLANNER, \
						ECalBackendPlannerClass))
#define E_IS_CAL_BACKEND_PLANNER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE \
					       ((obj), E_TYPE_CAL_BACKEND_PLANNER))
#define E_IS_CAL_BACKEND_PLANNER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE \
					       ((klass), E_TYPE_CAL_BACKEND_PLANNER))

typedef struct _ECalBackendPlanner        ECalBackendPlanner;
typedef struct _ECalBackendPlannerClass   ECalBackendPlannerClass;

typedef struct _ECalBackendPlannerPrivate ECalBackendPlannerPrivate;

struct _ECalBackendPlanner {
	ECalBackendSync backend;

	/* Private data */
	ECalBackendPlannerPrivate *priv;
};

struct _ECalBackendPlannerClass {
	ECalBackendSyncClass  parent_class;
	MrpApplication       *mrp_app;
};

GType       e_cal_backend_planner_get_type (void);

G_END_DECLS

#endif
