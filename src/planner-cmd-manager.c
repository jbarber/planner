/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2003 Imendio HB
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
#include <glib.h>
#include "planner-cmd-manager.h"
#include "planner-marshal.h"

struct _PlannerCmdManagerPriv {
	GList *list;
	GList *current;
};

/* Signals */
enum {
	UNDO_STATE_CHANGED,
	REDO_STATE_CHANGED,
	LAST_SIGNAL
};

static GObjectClass *parent_class = NULL;
static guint signals[LAST_SIGNAL];

static void cmd_manager_class_init (PlannerCmdManagerClass *klass);
static void cmd_manager_init       (PlannerCmdManager      *manager);
static void cmd_manager_finalize   (GObject                *object);


GType
planner_cmd_manager_get_type (void)
{
	static GType type = 0;
	
	if (!type) {
		static const GTypeInfo info = {
			sizeof (PlannerCmdManagerClass),
			NULL,		/* base_init */
			NULL,		/* base_finalize */
			(GClassInitFunc) cmd_manager_class_init,
			NULL,		/* class_finalize */
			NULL,		/* class_data */
			sizeof (PlannerCmdManager),
			0,              /* n_preallocs */
			(GInstanceInitFunc) cmd_manager_init
		};

		type = g_type_register_static (G_TYPE_OBJECT,
					       "PlannerCmdManager",
					       &info, 0);
	}
	
	return type;
}

static void
cmd_manager_class_init (PlannerCmdManagerClass *klass)
{
	GObjectClass *o_class;
	
	parent_class = g_type_class_peek_parent (klass);

	o_class = (GObjectClass *) klass;
	
	/* GObject functions */
	o_class->finalize = cmd_manager_finalize;
	
	/* Signals */
	signals[UNDO_STATE_CHANGED] = g_signal_new (
		"undo_state_changed",
		G_TYPE_FROM_CLASS (klass),
		G_SIGNAL_RUN_LAST,
		0,
		NULL, NULL,
		planner_marshal_VOID__BOOLEAN,
		G_TYPE_NONE, 1, G_TYPE_BOOLEAN);

	signals[REDO_STATE_CHANGED] = g_signal_new (
		"redo_state_changed",
		G_TYPE_FROM_CLASS (klass),
		G_SIGNAL_RUN_LAST,
		0,
		NULL, NULL,
		planner_marshal_VOID__BOOLEAN,
		G_TYPE_NONE, 1, G_TYPE_BOOLEAN);
}

static void
cmd_manager_init (PlannerCmdManager *manager)
{
	manager->priv = g_new0 (PlannerCmdManagerPriv, 1);
}

static void
cmd_manager_finalize (GObject *object)
{
	PlannerCmdManager *manager = PLANNER_CMD_MANAGER (object);

	/* FIXME: Free the list with cmds. */
	
	g_free (manager->priv);
	
	if (G_OBJECT_CLASS (parent_class)->finalize) {
		(* G_OBJECT_CLASS (parent_class)->finalize) (object);
	}
}

static void
cmd_manager_dump (PlannerCmdManager *manager)
{
	PlannerCmdManagerPriv *priv;
	GList                 *l;
	PlannerCmd            *cmd;

	priv = manager->priv;
	
	if (!g_getenv ("PLANNER_DEBUG_UNDO")) {
		return;
	}
	
	g_print ("--------\n");
	
	for (l = priv->list; l; l = l->next) {
		if (l == priv->current) {
			g_print ("*");
		} else {
			g_print (" ");
		}			

		cmd = l->data;
		
		g_print (" %s\n", cmd->label);
	}

	g_print ("\n");
}

void
planner_cmd_manager_insert_and_do (PlannerCmdManager *manager, PlannerCmd *cmd)
{
	PlannerCmdManagerPriv *priv;
	GList                 *current;
	gboolean               undo, redo;

	g_return_if_fail (PLANNER_IS_CMD_MANAGER (manager));
	g_return_if_fail (cmd != NULL);

	priv = manager->priv;

	undo = (priv->current != NULL);
	redo = (priv->current != priv->list);

	if (priv->current != priv->list) {
		/* Need to wipe the cmd history added after the current
		 * element.
		 */

		current = priv->current;

		if (current && current->prev) {
			current->prev->next = NULL;
			current->prev = NULL;
		}

		/* FIXME: g_list_foreach (next, (GFunc) cmd_free); */
		g_list_free (priv->list);

		priv->list = current;
	}
	
	priv->list = g_list_prepend (priv->list, cmd);
	priv->current = priv->list;

	cmd->do_func (cmd);

	cmd_manager_dump (manager);

	if (!undo) {
		g_signal_emit (manager, signals[UNDO_STATE_CHANGED], 0, TRUE);
	}
	if (redo) {
		g_signal_emit (manager, signals[REDO_STATE_CHANGED], 0, FALSE);
	}
}

gboolean
planner_cmd_manager_undo (PlannerCmdManager *manager)
{
	PlannerCmdManagerPriv *priv;
	PlannerCmd            *cmd;
	gboolean               undo_before, redo_before;
	gboolean               undo_after, redo_after;

	g_return_val_if_fail (PLANNER_IS_CMD_MANAGER (manager), FALSE);

	priv = manager->priv;

	if (!priv->current) {
		return FALSE;
	}

	undo_before = (priv->current != NULL);
	redo_before = (priv->current != priv->list);
	
	cmd = priv->current->data;

	cmd->undo_func (cmd);

	priv->current = priv->current->next;

	cmd_manager_dump (manager);

	undo_after = (priv->current != NULL);
	redo_after = (priv->current != priv->list);

	if (undo_before != undo_after) {
		g_signal_emit (manager, signals[UNDO_STATE_CHANGED], 0, undo_after);
	}
	if (redo_before != redo_after) {
		g_signal_emit (manager, signals[REDO_STATE_CHANGED], 0, redo_after);
	}
	
	return TRUE;
}

gboolean
planner_cmd_manager_redo (PlannerCmdManager *manager)
{
	PlannerCmdManagerPriv *priv;
	PlannerCmd            *cmd;
	gboolean               undo_before, redo_before;
	gboolean               undo_after, redo_after;

	g_return_val_if_fail (PLANNER_IS_CMD_MANAGER (manager), FALSE);

	priv = manager->priv;

	undo_before = (priv->current != NULL);
	redo_before = (priv->current != priv->list);
	
	if (!priv->current && priv->list) {
		priv->current = g_list_last (priv->list);
	}
	else if (!priv->current || !priv->current->prev) {
		return FALSE;
	} else {
		priv->current = priv->current->prev;
	}

	cmd = priv->current->data;

	cmd->do_func (cmd);

	cmd_manager_dump (manager);

	undo_after = (priv->current != NULL);
	redo_after = (priv->current != priv->list);

	if (undo_before != undo_after) {
		g_signal_emit (manager, signals[UNDO_STATE_CHANGED], 0, undo_after);
	}
	if (redo_before != redo_after) {
		g_signal_emit (manager, signals[REDO_STATE_CHANGED], 0, redo_after);
	}

	return TRUE;
}

PlannerCmdManager *
planner_cmd_manager_new (void)
{
	PlannerCmdManager *manager;

	manager = g_object_new (PLANNER_TYPE_CMD_MANAGER, NULL);
	
	return manager;
}

