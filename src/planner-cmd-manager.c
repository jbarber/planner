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
#include <libgnome/gnome-i18n.h>
#include "planner-cmd-manager.h"
#include "planner-marshal.h"

struct _PlannerCmdManagerPriv {
	gint   limit;
	
	GList *list;
	GList *current;

	/* FIXME: add transactions */
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
static void cmd_manager_free_func  (PlannerCmd             *cmd,
				    gpointer                data);

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
		planner_marshal_VOID__BOOLEAN_STRING,
		G_TYPE_NONE, 2, G_TYPE_BOOLEAN, G_TYPE_STRING);

	signals[REDO_STATE_CHANGED] = g_signal_new (
		"redo_state_changed",
		G_TYPE_FROM_CLASS (klass),
		G_SIGNAL_RUN_LAST,
		0,
		NULL, NULL,
		planner_marshal_VOID__BOOLEAN_STRING,
		G_TYPE_NONE, 2, G_TYPE_BOOLEAN, G_TYPE_STRING);
}

static void
cmd_manager_init (PlannerCmdManager *manager)
{
	PlannerCmdManagerPriv *priv;
	
	priv = g_new0 (PlannerCmdManagerPriv, 1);
	manager->priv = priv;

	/* FIXME: What default to use? */
	priv->limit = 100;
}

static void
cmd_manager_finalize (GObject *object)
{
	PlannerCmdManager     *manager = PLANNER_CMD_MANAGER (object);
	PlannerCmdManagerPriv *priv = manager->priv;;
	
	g_list_foreach (priv->list, (GFunc) cmd_manager_free_func, NULL);
	
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

static PlannerCmd *
get_undo_cmd (PlannerCmdManager *manager, gboolean next)
{
	PlannerCmdManagerPriv *priv = manager->priv;
	PlannerCmd            *cmd;
	
	if (priv->current) {
		cmd = priv->current->data;

		if (next) {
			priv->current = priv->current->next;
		}
		
		return cmd;
	}

	return NULL;
}

static PlannerCmd *
get_redo_cmd (PlannerCmdManager *manager, gboolean next)
{
	PlannerCmdManagerPriv *priv = manager->priv;
	GList                 *l;
	PlannerCmd            *cmd;
	
	if (!priv->current && priv->list) {
		l = g_list_last (priv->list);
	}
	else if (!priv->current || !priv->current->prev) {
		l = NULL;
	} else {
		l = priv->current->prev;
	}

	if (!l) {
		return NULL;
	}

	if (next) {
		priv->current = l;
	}

	cmd = l->data;

	return cmd;
}

static void
state_changed (PlannerCmdManager *manager)
{
	gchar      *label;
	PlannerCmd *cmd;
	
	/* Undo */
	cmd = get_undo_cmd (manager, FALSE);
	if (cmd) {
		label = g_strdup_printf (_("_Undo '%s'"), cmd->label);
	} else {
		label = g_strdup (_("_Undo"));
	}
	
	g_signal_emit (manager, signals[UNDO_STATE_CHANGED], 0, cmd != NULL, label);

	g_free (label);
	
	/* Redo */
	cmd = get_redo_cmd (manager, FALSE);
	if (cmd) {
		label = g_strdup_printf (_("_Redo '%s'"), cmd->label);
	} else {
		label = g_strdup (_("_Redo"));
	}
	
	g_signal_emit (manager, signals[REDO_STATE_CHANGED], 0, cmd != NULL, label);

	g_free (label);
}

static void
cmd_manager_free_func (PlannerCmd *cmd,
		       gpointer    data)
{
	if (cmd->free_func) {
		cmd->free_func (cmd);
	}
}

void
planner_cmd_manager_insert_and_do (PlannerCmdManager *manager, PlannerCmd *cmd)
{
	PlannerCmdManagerPriv *priv;
	GList                 *current;

	g_return_if_fail (PLANNER_IS_CMD_MANAGER (manager));
	g_return_if_fail (cmd != NULL);

	priv = manager->priv;

	if (priv->current != priv->list) {
		/* Need to wipe the cmd history added after the current
		 * element.
		 */

		current = priv->current;

		if (current && current->prev) {
			current->prev->next = NULL;
			current->prev = NULL;
		}

		g_list_foreach (priv->list, (GFunc) cmd_manager_free_func, NULL);
		g_list_free (priv->list);

		priv->list = current;
	}
	
	priv->list = g_list_prepend (priv->list, cmd);
	priv->current = priv->list;

	cmd->do_func (cmd);

	cmd_manager_dump (manager);

	state_changed (manager);
}

gboolean
planner_cmd_manager_undo (PlannerCmdManager *manager)
{
	PlannerCmdManagerPriv *priv;
	PlannerCmd            *cmd;

	g_return_val_if_fail (PLANNER_IS_CMD_MANAGER (manager), FALSE);

	priv = manager->priv;

	cmd = get_undo_cmd (manager, TRUE);

	if (!cmd) {
		return FALSE;
	}

	cmd->undo_func (cmd);

	cmd_manager_dump (manager);

	state_changed (manager);

	return TRUE;
}

gboolean
planner_cmd_manager_redo (PlannerCmdManager *manager)
{
	PlannerCmdManagerPriv *priv;
	PlannerCmd            *cmd;

	g_return_val_if_fail (PLANNER_IS_CMD_MANAGER (manager), FALSE);

	priv = manager->priv;

	cmd = get_redo_cmd (manager, TRUE);
	if (!cmd) {
		return FALSE;
	}

	cmd->do_func (cmd);

	cmd_manager_dump (manager);

	state_changed (manager);

	return TRUE;
}

PlannerCmdManager *
planner_cmd_manager_new (void)
{
	PlannerCmdManager *manager;

	manager = g_object_new (PLANNER_TYPE_CMD_MANAGER, NULL);
	
	return manager;
}

