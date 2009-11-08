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

#ifndef __PLANNER_TASK_DATE_WIDGET_H__
#define __PLANNER_TASK_DATE_WIDGET_H__

#include <gtk/gtk.h>

#define PLANNER_TYPE_TASK_DATE_WIDGET            (planner_task_date_widget_get_type ())
#define PLANNER_TASK_DATE_WIDGET(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PLANNER_TYPE_TASK_DATE_WIDGET, PlannerTaskDateWidget))
#define PLANNER_TASK_DATE_WIDGET_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), PLANNER_TYPE_TASK_DATE_WIDGET, PlannerTaskDateWidgetClass))
#define PLANNER_IS_TASK_DATE_WIDGET(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PLANNER_TYPE_TASK_DATE_WIDGET))
#define PLANNER_IS_TASK_DATE_WIDGET_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), PLANNER_TYPE_TASK_DATE_WIDGET))
#define PLANNER_TASK_DATE_WIDGET_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), PLANNER_TYPE_TASK_DATE_WIDGET, PlannerTaskDateWidgetClass))


typedef struct _PlannerTaskDateWidget      PlannerTaskDateWidget;
typedef struct _PlannerTaskDateWidgetClass PlannerTaskDateWidgetClass;

struct _PlannerTaskDateWidget {
	GtkFrame  parent;
};

struct _PlannerTaskDateWidgetClass {
	GtkFrameClass parent_class;
};

GType             planner_task_date_widget_get_type            (void) G_GNUC_CONST;
GtkWidget *       planner_task_date_widget_new                 (void);
void              planner_task_date_widget_set_date            (PlannerTaskDateWidget *widget,
								mrptime                t);
mrptime           planner_task_date_widget_get_date            (PlannerTaskDateWidget *widget);
void              planner_task_date_widget_set_constraint_type (PlannerTaskDateWidget *widget,
								MrpConstraintType      type);
MrpConstraintType planner_task_date_widget_get_constraint_type (PlannerTaskDateWidget *widget);

#endif /* __PLANNER_TASK_DATE_WIDGET_H__ */
