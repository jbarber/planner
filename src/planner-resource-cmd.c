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

#include <config.h>
#include <libgnome/gnome-i18n.h>
#include "planner-resource-cmd.h"

typedef struct {
	PlannerCmd   base;

	MrpProject  *project;
	const gchar *name;
	MrpResource *resource;
	MrpGroup    *group;
} ResourceCmdInsert;

static gboolean
resource_cmd_insert_do (PlannerCmd *cmd_base)
{
	ResourceCmdInsert *cmd;

	cmd = (ResourceCmdInsert*) cmd_base;

	mrp_project_add_resource (cmd->project, cmd->resource);

	return TRUE;
}

static void
resource_cmd_insert_undo (PlannerCmd *cmd_base)
{
	ResourceCmdInsert *cmd;
	
	cmd = (ResourceCmdInsert*) cmd_base;

	mrp_project_remove_resource (cmd->project,
				     cmd->resource);
}

static void
resource_cmd_insert_free (PlannerCmd  *cmd_base)
{
	ResourceCmdInsert *cmd;
	cmd = (ResourceCmdInsert*) cmd_base;

	g_object_unref (cmd->resource);
}


PlannerCmd *
planner_resource_cmd_insert (PlannerWindow *main_window,
			     MrpResource   *resource)
{
	PlannerCmd          *cmd_base;
	ResourceCmdInsert   *cmd;

	cmd_base = planner_cmd_new (ResourceCmdInsert,
				    _("Insert resource"),
				    resource_cmd_insert_do,
				    resource_cmd_insert_undo,
				    resource_cmd_insert_free);

	cmd = (ResourceCmdInsert *) cmd_base;

	if (resource == NULL) {
		cmd->resource = g_object_new (MRP_TYPE_RESOURCE, NULL);
	} else {
		cmd->resource = g_object_ref (resource);	
	}

	cmd->project = planner_window_get_project (main_window);

	planner_cmd_manager_insert_and_do (planner_window_get_cmd_manager (main_window),
					   cmd_base);

	return cmd_base;
}
