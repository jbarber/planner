/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2004 Alvaro del Castillo <acs@barrapunto.com>
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

#ifndef __PLANNER_TASK_CMD_H__
#define __PLANNER_TASK_CMD_H__

#include "planner-window.h"
#include "planner-task-tree.h"

PlannerCmd * planner_task_cmd_link   (PlannerWindow   *main_window,
				      MrpTask         *before,
				      MrpTask         *after,
				      MrpRelationType  relationship,
				      glong            lag,
				      GError         **error);

PlannerCmd * planner_task_cmd_unlink (PlannerWindow   *main_window,
				      MrpRelation     *relation);

PlannerCmd * planner_task_cmd_insert (PlannerWindow  *main_window,
				      MrpTask        *parent,
				      gint            position,
				      gint            work,
				      gint            duration,
				      MrpTask        *new_task);

#endif /* __PLANNER_TASK_CMD_H__ */
