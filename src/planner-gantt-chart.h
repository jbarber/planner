/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2001-2002 CodeFactory AB
 * Copyright (C) 2001-2002 Richard Hult <richard@imendio.com>
 * Copyright (C) 2001-2002 Mikael Hallendal <micke@imendio.com>
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

#ifndef __PLANNER_GANTT_CHART_H__
#define __PLANNER_GANTT_CHART_H__

#include <gtk/gtkwidget.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtktreemodel.h>
#include <libgnomecanvas/gnome-canvas.h>
#include "planner-task-tree.h"

#define PLANNER_TYPE_GANTT_CHART		(planner_gantt_chart_get_type ())
#define PLANNER_GANTT_CHART(obj)		(GTK_CHECK_CAST ((obj), PLANNER_TYPE_GANTT_CHART, PlannerGanttChart))
#define PLANNER_GANTT_CHART_CLASS(klass)	(GTK_CHECK_CLASS_CAST ((klass), PLANNER_TYPE_GANTT_CHART, PlannerGanttChartClass))
#define PLANNER_IS_GANTT_CHART(obj)		(GTK_CHECK_TYPE ((obj), PLANNER_TYPE_GANTT_CHART))
#define PLANNER_IS_GANTT_CHART_CLASS(klass)	(GTK_CHECK_CLASS_TYPE ((obj), PLANNER_TYPE_GANTT_CHART))
#define PLANNER_GANTT_CHART_GET_CLASS(obj)	(GTK_CHECK_GET_CLASS ((obj), PLANNER_TYPE_GANTT_CHART, PlannerGanttChartClass))

typedef struct _PlannerGanttChart           PlannerGanttChart;
typedef struct _PlannerGanttChartClass      PlannerGanttChartClass;
typedef struct _PlannerGanttChartPriv       PlannerGanttChartPriv;

struct _PlannerGanttChart
{
	GtkVBox           parent;
	PlannerGanttChartPriv *priv;
};

struct _PlannerGanttChartClass
{
	GtkVBoxClass parent_class;

	void  (* set_scroll_adjustments) (PlannerGanttChart  *chart,
					  GtkAdjustment *hadjustment,
					  GtkAdjustment *vadjustment);
};


GType            planner_gantt_chart_get_type         (void);

GtkWidget       *planner_gantt_chart_new              (void);

GtkWidget       *planner_gantt_chart_new_with_model   (GtkTreeModel  *model);

PlannerTaskTree *planner_gantt_chart_get_view         (PlannerGanttChart *chart);

void planner_gantt_chart_set_view                     (PlannerGanttChart *chart,
						       PlannerTaskTree *view);

GtkTreeModel    *planner_gantt_chart_get_model        (PlannerGanttChart  *tree_view);

void             planner_gantt_chart_set_model        (PlannerGanttChart  *tree_view,
						       GtkTreeModel  *model);

void             planner_gantt_chart_expand_row       (PlannerGanttChart  *chart,
						       GtkTreePath   *path);

void             planner_gantt_chart_collapse_row     (PlannerGanttChart  *chart,
						       GtkTreePath   *path);

void             planner_gantt_chart_scroll_to        (PlannerGanttChart  *chart,
						       time_t         t);

void             planner_gantt_chart_zoom_in          (PlannerGanttChart  *chart);

void             planner_gantt_chart_zoom_out         (PlannerGanttChart  *chart);

void             planner_gantt_chart_zoom_to_fit      (PlannerGanttChart  *chart);

gdouble          planner_gantt_chart_get_zoom         (PlannerGanttChart  *chart);

void             planner_gantt_chart_can_zoom         (PlannerGanttChart  *chart,
						       gboolean      *in,
						       gboolean      *out);

void             planner_gantt_chart_status_updated   (PlannerGanttChart  *chart,
						       const gchar   *message);

void             planner_gantt_chart_resource_clicked (PlannerGanttChart  *chart,
						       MrpResource   *resource);

void
planner_gantt_chart_set_highlight_critical_tasks      (PlannerGanttChart  *chart,
						       gboolean       state);

gboolean
planner_gantt_chart_get_highlight_critical_tasks      (PlannerGanttChart  *chart);


#endif /* __PLANNER_GANTT_CHART_H__ */
