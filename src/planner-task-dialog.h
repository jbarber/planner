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

#ifndef __PLANNER_TASK_DIALOG_H__
#define __PLANNER_TASK_DIALOG_H__

#include <gtk/gtkwidget.h>
#include <libplanner/mrp-project.h>
#include "planner-window.h"

typedef enum {
	PLANNER_TASK_DIALOG_PAGE_GENERAL =      0,
	PLANNER_TASK_DIALOG_PAGE_RESOURCES =    1,
	PLANNER_TASK_DIALOG_PAGE_PREDECESSORS = 2,
	PLANNER_TASK_DIALOG_PAGE_NOTES =        3
} PlannerTaskDialogPage;


GtkWidget * planner_task_dialog_new  (PlannerWindow         *window,
				      MrpTask               *task,
				      PlannerTaskDialogPage  page);

#endif /* __PLANNER_TASK_DIALOG_H__ */









