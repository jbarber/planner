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

	gboolean  inside_transaction;
};


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

	/* FIXME: What default to use? Need some limit, or we'll use a lot of
	 * memory.
	 */
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
		label = g_strdup_printf (_("Undo '%s'"), cmd->label);
	} else {
		label = g_strdup (_("Undo"));
	}
	
	g_signal_emit (manager, signals[UNDO_STATE_CHANGED], 0, cmd != NULL, label);

	g_free (label);
	
	/* Redo */
	cmd = get_redo_cmd (manager, FALSE);
	if (cmd) {
		label = g_strdup_printf (_("Redo '%s'"), cmd->label);
	} else {
		label = g_strdup (_("Redo"));
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

	/* FIXME: Add this when the rest of the code is fixed up. */
/*	g_free (cmd->label);
	g_free (cmd);
*/
}

static gboolean
cmd_manager_insert (PlannerCmdManager *manager,
		    PlannerCmd        *cmd,
		    gboolean           run_do)
{
	PlannerCmdManagerPriv *priv;
	GList                 *current;
	gboolean               retval;

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

	/* Trim the history if necessary. */
	if (g_list_length (priv->list) >= priv->limit) {
		GList *l;

		l = g_list_nth (priv->list, priv->limit - 1);
		
		/* Don't crash in case we should have a limit of 1 :) */
		if (l->prev) {
			l->prev->next = NULL;
			l->prev = NULL;
		}

		g_list_foreach (l, (GFunc) cmd_manager_free_func, NULL);
		g_list_free (l);

		/* Limit of 1 again... */
		if (l == priv->list) {
			priv->list = NULL;
		}
	}
	
	priv->list = g_list_prepend (priv->list, cmd);
	priv->current = priv->list;

	cmd->manager = manager;
	
	if (run_do && cmd->do_func) {
		retval = cmd->do_func (cmd);
	} else {
		retval = TRUE;
	}
	
	cmd_manager_dump (manager);

	state_changed (manager);

	return retval;
}

gboolean
planner_cmd_manager_insert_and_do (PlannerCmdManager *manager, PlannerCmd *cmd)
{
	g_return_val_if_fail (PLANNER_IS_CMD_MANAGER (manager), FALSE);
	g_return_val_if_fail (cmd != NULL, FALSE);

	return cmd_manager_insert (manager, cmd, TRUE);
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

	if (cmd->undo_func) {
		cmd->undo_func (cmd);
	}
	
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

	if (cmd->do_func) {
		cmd->do_func (cmd);
	}
	
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


/*
 * Transaction commands
 */

static gboolean
transaction_cmd_do (PlannerCmd *cmd)
{
	PlannerCmd *cmd_sub;
	
	g_print ("Transaction do: %s\n", cmd->label);
	
	while (1) {
		cmd_sub = get_redo_cmd (cmd->manager, TRUE);

		if (!cmd_sub) {
			break;
		}
		
		if (cmd_sub->do_func) {
			cmd_sub->do_func (cmd_sub);
		}
		
		if (cmd_sub->type == PLANNER_CMD_TYPE_END_TRANSACTION) {
			break;
		}

		g_assert (cmd_sub->type == PLANNER_CMD_TYPE_NORMAL);
	}

	/* FIXME: need to make sure we handle transactions that doesn't work. */
	
	return TRUE;
}

static void
transaction_cmd_undo (PlannerCmd *cmd)
{
	PlannerCmd *cmd_sub;

	g_print ("Transaction undo: %s\n", cmd->label);

	while (1) {
		cmd_sub = get_undo_cmd (cmd->manager, TRUE);

		if (!cmd_sub) {
			break;
		}
		
		if (cmd_sub->undo_func) {
			cmd_sub->undo_func (cmd_sub);
		}
		
		if (cmd_sub->type == PLANNER_CMD_TYPE_BEGIN_TRANSACTION) {
			break;
		}

		g_assert (cmd_sub->type == PLANNER_CMD_TYPE_NORMAL);
	}
}

gboolean
planner_cmd_manager_begin_transaction (PlannerCmdManager *manager,
				       const gchar       *label)
{
	PlannerCmdManagerPriv *priv;
	PlannerCmd            *cmd;
	
	g_return_val_if_fail (PLANNER_IS_CMD_MANAGER (manager), FALSE);

	priv = manager->priv;
	
	if (priv->inside_transaction) {
		g_warning ("Already inside transaction.");
		return FALSE;
	}

	priv->inside_transaction = TRUE;

	cmd = g_new0 (PlannerCmd, 1);

	cmd->do_func = transaction_cmd_do;
	cmd->undo_func = transaction_cmd_undo;
	cmd->free_func = (PlannerCmdFreeFunc) g_free;
	
	cmd->label = g_strdup (label);
	cmd->type = PLANNER_CMD_TYPE_BEGIN_TRANSACTION;
		
	cmd_manager_insert (manager, cmd, FALSE);
	
	return TRUE;
}

gboolean
planner_cmd_manager_end_transaction (PlannerCmdManager *manager)
{
	PlannerCmdManagerPriv *priv;
	PlannerCmd            *cmd, *begin_cmd = NULL;
	GList                 *l;

	g_return_val_if_fail (PLANNER_IS_CMD_MANAGER (manager), FALSE);

	priv = manager->priv;
	
	if (!priv->inside_transaction) {
		g_warning ("Don't have transaction to end.");
		return FALSE;
	}

	priv->inside_transaction = FALSE;

	for (l = priv->current; l; l = l->next) {
		begin_cmd = l->data;

		if (begin_cmd->type == PLANNER_CMD_TYPE_BEGIN_TRANSACTION) {
			break;
		}

		begin_cmd = NULL;
	}

	if (!begin_cmd) {
		g_warning ("Can't find beginning of transaction.");
		return FALSE;
	}
	
	cmd = g_new0 (PlannerCmd, 1);
		
	cmd->do_func = transaction_cmd_do;
	cmd->undo_func = transaction_cmd_undo;
	cmd->free_func = (PlannerCmdFreeFunc) g_free;
	
	cmd->label = g_strdup (begin_cmd->label);
	cmd->type = PLANNER_CMD_TYPE_END_TRANSACTION;
	
	cmd_manager_insert (manager, cmd, FALSE);

	return TRUE;
}

PlannerCmd *
planner_cmd_new_size (gsize               size,
		      const gchar        *label,
		      PlannerCmdDoFunc    do_func,
		      PlannerCmdUndoFunc  undo_func,
		      PlannerCmdFreeFunc  free_func)
{
	PlannerCmd *cmd;

	cmd = g_malloc0 (size);

	cmd->label = g_strdup (label);
	cmd->do_func = do_func;
	cmd->undo_func = undo_func;
	cmd->free_func = free_func;

	return cmd;
}

