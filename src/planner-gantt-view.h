/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2005 Imendio AB
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

#ifndef __PLANNER_GANTT_VIEW_H__
#define __PLANNER_GANTT_VIEW_H__

#include <gtk/gtk.h>
#include "planner-view.h"

#define PLANNER_TYPE_GANTT_VIEW	           (planner_gantt_view_get_type ())
#define PLANNER_GANTT_VIEW(obj)	           (G_TYPE_CHECK_INSTANCE_CAST ((obj), PLANNER_TYPE_GANTT_VIEW, PlannerGanttView))
#define PLANNER_GANTT_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), PLANNER_TYPE_GANTT_VIEW, PlannerGanttViewClass))
#define PLANNER_IS_GANTT_VIEW(obj)	   (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PLANNER_TYPE_GANTT_VIEW))
#define PLANNER_IS_GANTT_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), PLANNER_TYPE_GANTT_VIEW))
#define PLANNER_GANTT_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), PLANNER_TYPE_GANTT_VIEW, PlannerGanttViewClass))

typedef struct _PlannerGanttView       PlannerGanttView;
typedef struct _PlannerGanttViewClass  PlannerGanttViewClass;
typedef struct _PlannerGanttViewPriv   PlannerGanttViewPriv;

struct _PlannerGanttView {
	PlannerView           parent;
	PlannerGanttViewPriv *priv;
};

struct _PlannerGanttViewClass {
	PlannerViewClass parent_class;
};

GType        planner_gantt_view_get_type     (void) G_GNUC_CONST;
PlannerView *planner_gantt_view_new          (void);

#endif /* __PLANNER_GANTT_VIEW_H__ */

