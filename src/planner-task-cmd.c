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

#include <libgnome/gnome-i18n.h>
#include "planner-task-cmd.h"
#include "planner-task-tree.h"

typedef struct {
	PlannerCmd        base;

	MrpProject       *project;
	MrpTask          *before;
	MrpTask          *after;
	MrpRelationType   relationship;
	glong             lag;
	GError           *error;
} TaskCmdLink;

static gboolean
task_cmd_link_do (PlannerCmd *cmd_base)
{
	TaskCmdLink *cmd;
	GError      *error = NULL;
	MrpRelation *relation;
	gboolean     retval;

	cmd = (TaskCmdLink *) cmd_base;

	relation = mrp_task_add_predecessor (cmd->after,
					     cmd->before,
					     cmd->relationship,
					     cmd->lag,
					     &error);
	if (!error) {
		retval = TRUE;
	} else {
		cmd->error = error;
		retval = FALSE;		
	} 

	return retval;
}

static void
task_cmd_link_undo (PlannerCmd *cmd_base)
{
	TaskCmdLink *cmd;
	
	cmd = (TaskCmdLink*) cmd_base;
	
	mrp_task_remove_predecessor (cmd->after, cmd->before);
}

static void
task_cmd_link_free (PlannerCmd *cmd_base)
{
	TaskCmdLink *cmd;

	cmd = (TaskCmdLink *) cmd_base;

	g_object_unref (cmd->project);
	g_object_unref (cmd->before);
	g_object_unref (cmd->after);
}


PlannerCmd *
planner_task_cmd_link (PlannerWindow   *main_window,
		       MrpTask         *before,
		       MrpTask         *after,
		       MrpRelationType  relationship,
		       glong            lag,
		       GError         **error)
{
	PlannerCmd          *cmd_base;
	TaskCmdLink         *cmd;

	cmd_base = planner_cmd_new (TaskCmdLink,
				    _("Link task"),
				    task_cmd_link_do,
				    task_cmd_link_undo,
				    task_cmd_link_free);

	cmd = (TaskCmdLink *) cmd_base;

	cmd->project = g_object_ref (planner_window_get_project (main_window));

	cmd->before = g_object_ref (before);
	cmd->after = g_object_ref (after);
	cmd->relationship = relationship;
	cmd->lag = lag;
			
	planner_cmd_manager_insert_and_do (planner_window_get_cmd_manager 
					   (main_window),
					   cmd_base);

	if (cmd->error) {
		g_propagate_error (error, cmd->error);
		/* FIXME: who clean the cmd memory? */
		return NULL;
	}

	return cmd_base;	
}

static gboolean
task_cmd_unlink_do (PlannerCmd *cmd_base)
{
	TaskCmdLink *cmd;
	
	cmd = (TaskCmdLink*) cmd_base;

	mrp_task_remove_predecessor (cmd->after, cmd->before);

	return TRUE;
}

static void
task_cmd_unlink_undo (PlannerCmd *cmd_base)
{
	TaskCmdLink *cmd;
	GError      *error = NULL;
	MrpRelation *relation;

	cmd = (TaskCmdLink *) cmd_base;

	relation = mrp_task_add_predecessor (cmd->after,
					     cmd->before,
					     cmd->relationship,
					     cmd->lag,
					     &error);
	g_assert (relation);
}

static void
task_cmd_unlink_free (PlannerCmd *cmd_base)
{
	TaskCmdLink *cmd;

	cmd = (TaskCmdLink *) cmd_base;

	g_object_unref (cmd->project);
	g_object_unref (cmd->before);
	g_object_unref (cmd->after);
}

PlannerCmd *
planner_task_cmd_unlink (PlannerWindow   *main_window,
			 MrpRelation     *relation)
{
	PlannerCmd          *cmd_base;
	TaskCmdLink         *cmd;

	cmd_base = planner_cmd_new (TaskCmdLink,
				    _("Unlink task"),
				    task_cmd_unlink_do,
				    task_cmd_unlink_undo,
				    task_cmd_unlink_free);
	
	cmd = (TaskCmdLink *) cmd_base;

	cmd->project = g_object_ref (planner_window_get_project (main_window));

	cmd->before = g_object_ref (mrp_relation_get_predecessor (relation));
	cmd->after = g_object_ref (mrp_relation_get_successor (relation));
	cmd->relationship = mrp_relation_get_relation_type (relation);
	cmd->lag = mrp_relation_get_lag (relation);
			
	planner_cmd_manager_insert_and_do (planner_window_get_cmd_manager 
					   (main_window),
					   cmd_base);
	return cmd_base;
}

typedef struct {
	PlannerCmd   base;

	MrpProject  *project;

	gint         position; 
	gint         work;
	gint         duration;
	
	MrpTask     *task; 	/* The inserted task */
	MrpTask     *parent;
} TaskCmdInsert;


static gboolean
task_cmd_insert_do (PlannerCmd *cmd_base)
{
	TaskCmdInsert *cmd;

	cmd = (TaskCmdInsert *) cmd_base;

	if (cmd->task == NULL) {
		cmd->task = g_object_new (MRP_TYPE_TASK,
					  "work", cmd->work,
					  "duration", cmd->duration,
					  NULL);
	}

	mrp_project_insert_task (cmd->project,
				 cmd->parent,
				 cmd->position,
				 cmd->task);

	return TRUE;
}

static void
task_cmd_insert_undo (PlannerCmd *cmd_base)
{
	TaskCmdInsert *cmd;
	
	cmd = (TaskCmdInsert *) cmd_base;

	mrp_project_remove_task (cmd->project, cmd->task);
}

static void
task_cmd_insert_free (PlannerCmd *cmd_base)
{
	TaskCmdInsert *cmd;

	cmd = (TaskCmdInsert*) cmd_base;

	g_object_unref (cmd->task);
	if (cmd->parent != NULL) {
		g_object_unref (cmd->parent);
	}
	g_object_unref (cmd->project);
	cmd->task = NULL;
}

PlannerCmd *
planner_task_cmd_insert (PlannerWindow  *main_window,
			 MrpTask        *parent,
			 gint            position,
			 gint            work,
			 gint            duration,
			 MrpTask        *new_task)
{
	PlannerCmd     *cmd_base;
	TaskCmdInsert  *cmd;

	cmd_base = planner_cmd_new (TaskCmdInsert,
				    _("Insert task"),
				    task_cmd_insert_do,
				    task_cmd_insert_undo,
				    task_cmd_insert_free);

	cmd = (TaskCmdInsert *) cmd_base;

	if (parent != NULL) {
		cmd->parent = g_object_ref (parent);
	}
	cmd->project = g_object_ref (planner_window_get_project (main_window));

	cmd->position = position;
	cmd->work = work;
	cmd->duration = duration;
	if (new_task != NULL) {
		cmd->task = g_object_ref (new_task);
	}
	
	planner_cmd_manager_insert_and_do (planner_window_get_cmd_manager (main_window),
					   cmd_base);

	return cmd_base;
}
