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

#ifndef __PLANNER_USAGE_CHART_H__
#define __PLANNER_USAGE_CHART_H__

#include <gtk/gtk.h>
#include <libgnomecanvas/gnome-canvas.h>
#include "planner-usage-tree.h"

#define PLANNER_TYPE_USAGE_CHART             (planner_usage_chart_get_type ())
#define PLANNER_USAGE_CHART(obj)             (GTK_CHECK_CAST ((obj), PLANNER_TYPE_USAGE_CHART, PlannerUsageChart))
#define PLANNER_USAGE_CHART_CLASS(klass)     (GTK_CHECK_CLASS_CAST ((klass), PLANNER_TYPE_USAGE_CHART, PlannerUsageChartClass))
#define PLANNER_IS_USAGE_CHART(obj)          (GTK_CHECK_TYPE ((obj), PLANNER_TYPE_USAGE_CHART))
#define PLANNER_IS_USAGE_CHART_CLASS(klass)  (GTK_CHECK_CLASS_TYPE ((obj), PLANNER_TYPE_USAGE_CHART))
#define PLANNER_USAGE_CHART_GET_CLASS(obj)   (GTK_CHECK_GET_CLASS ((obj), PLANNER_TYPE_USAGE_CHART, PlannerUsageChartClass))

typedef struct _PlannerUsageChart PlannerUsageChart;
typedef struct _PlannerUsageChartClass PlannerUsageChartClass;
typedef struct _PlannerUsageChartPriv PlannerUsageChartPriv;

struct _PlannerUsageChart {
        GtkVBox                 parent_class;
        PlannerUsageChartPriv *priv;
};

struct _PlannerUsageChartClass {
        GtkVBoxClass           parent_class;

        void (*set_scroll_adjustments) (PlannerUsageChart *chart,
                                        GtkAdjustment      *hadj,
                                        GtkAdjustment      *vadj);
};

GType         planner_usage_chart_get_type         (void) G_GNUC_CONST;
GtkWidget *   planner_usage_chart_new              (void);
GtkWidget *   planner_usage_chart_new_with_model   (GtkTreeModel       *model);
GtkTreeModel *planner_usage_chart_get_model        (PlannerUsageChart *chart);
void          planner_usage_chart_set_model        (PlannerUsageChart *chart,
						     GtkTreeModel       *model);
PlannerUsageTree *
              planner_usage_chart_get_view         (PlannerUsageChart *chart);
void          planner_usage_chart_set_view         (PlannerUsageChart *chart,
						    PlannerUsageTree  *view);
void          planner_usage_chart_expand_row       (PlannerUsageChart *chart,
						     GtkTreePath        *path);
void          planner_usage_chart_collapse_row     (PlannerUsageChart *chart,
						     GtkTreePath        *path);
void          planner_usage_chart_expand_all       (PlannerUsageChart *chart);
void          planner_usage_chart_collapse_all     (PlannerUsageChart *chart);
void          planner_usage_chart_zoom_in          (PlannerUsageChart *chart);
void          planner_usage_chart_zoom_out         (PlannerUsageChart *chart);
void          planner_usage_chart_can_zoom         (PlannerUsageChart *chart,
						     gboolean           *in,
						     gboolean           *out);
void          planner_usage_chart_zoom_to_fit      (PlannerUsageChart *chart);
gdouble       planner_usage_chart_get_zoom         (PlannerUsageChart *chart);
void          planner_usage_chart_status_updated   (PlannerUsageChart *chart,
						     gchar              *message);

void planner_usage_chart_setup_root_task (PlannerUsageChart *chart);

#endif /* __PLANNER_USAGE_CHART_H__ */
