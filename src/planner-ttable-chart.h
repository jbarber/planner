/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2003-2004 Imendio HB
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

#ifndef __PLANNER_TTABLE_CHART_H__
#define __PLANNER_TTABLE_CHART_H__

#include <gtk/gtkwidget.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtktreemodel.h>
#include <libgnomecanvas/gnome-canvas.h>

#define PLANNER_TYPE_TTABLE_CHART             (planner_ttable_chart_get_type ())
#define PLANNER_TTABLE_CHART(obj)             (GTK_CHECK_CAST ((obj), PLANNER_TYPE_TTABLE_CHART, PlannerTtableChart))
#define PLANNER_TTABLE_CHART_CLASS(klass)     (GTK_CHECK_CLASS_CAST ((klass), PLANNER_TYPE_TTABLE_CHART, PlannerTtableChartClass))
#define PLANNER_IS_TTABLE_CHART(obj)          (GTK_CHECK_TYPE ((obj), PLANNER_TYPE_TTABLE_CHART))
#define PLANNER_IS_TTABLE_CHART_CLASS(klass)  (GTK_CHECK_CLASS_TYPE ((obj), PLANNER_TYPE_TTABLE_CHART))
#define PLANNER_TTABLE_CHART_GET_CLASS(obj)   (GTK_CHECK_GET_CLASS ((obj), PLANNER_TYPE_TTABLE_CHART, PlannerTtableChartClass))

typedef struct _PlannerTtableChart PlannerTtableChart;
typedef struct _PlannerTtableChartClass PlannerTtableChartClass;
typedef struct _PlannerTtableChartPriv PlannerTtableChartPriv;

struct _PlannerTtableChart {
        GtkVBox                 parent_class;
        PlannerTtableChartPriv *priv;
};

struct _PlannerTtableChartClass {
        GtkVBoxClass           parent_class;

        void (*set_scroll_adjustments) (PlannerTtableChart *chart,
                                        GtkAdjustment      *hadj,
                                        GtkAdjustment      *vadj);
};

GType         planner_ttable_chart_get_type         (void) G_GNUC_CONST;
GtkWidget *   planner_ttable_chart_new              (void);
GtkWidget *   planner_ttable_chart_new_with_model   (GtkTreeModel       *model);
GtkTreeModel *planner_ttable_chart_get_model        (PlannerTtableChart *chart);
void          planner_ttable_chart_set_model        (PlannerTtableChart *chart,
						     GtkTreeModel       *model);
void          planner_ttable_chart_expand_row       (PlannerTtableChart *chart,
						     GtkTreePath        *path);
void          planner_ttable_chart_collapse_row     (PlannerTtableChart *chart,
						     GtkTreePath        *path);
void          planner_ttable_chart_expand_all       (PlannerTtableChart *chart);
void          planner_ttable_chart_collapse_all     (PlannerTtableChart *chart);
void          planner_ttable_chart_zoom_in          (PlannerTtableChart *chart);
void          planner_ttable_chart_zoom_out         (PlannerTtableChart *chart);
void          planner_ttable_chart_can_zoom         (PlannerTtableChart *chart,
						     gboolean           *in,
						     gboolean           *out);
void          planner_ttable_chart_zoom_to_fit      (PlannerTtableChart *chart);
gdouble       planner_ttable_chart_get_zoom         (PlannerTtableChart *chart);
void          planner_ttable_chart_status_updated   (PlannerTtableChart *chart,
						     gchar              *message);

void planner_ttable_chart_setup_root_task (PlannerTtableChart *chart);

#endif /* __PLANNER_TTABLE_CHART_H__ */
