/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2008 Maurice van der Pot <griffon26@kfk4ever.com>
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
#include <string.h>
#include <stdlib.h>
#include "src/planner-cmd-manager.h"
#include "self-check.h"

typedef struct {
	gchar actions[100];
	gint  next;
} ActionHistory;

typedef struct {
	PlannerCmd     base;

	ActionHistory *action_history;
	gboolean       allocated;
	gchar          id;
} TestCmd;

static gboolean
test_cmd_do (PlannerCmd *cmd_base)
{
	TestCmd           *cmd;
	ActionHistory     *history;

	cmd = (TestCmd*) cmd_base;

	history = cmd->action_history;

	CHECK_BOOLEAN_RESULT (cmd->allocated, TRUE);
	CHECK_INTEGER_RESULT (history->actions[history->next], 0);
	history->actions[history->next] = cmd->id;
	history->next++;

	return TRUE;
}

static void
test_cmd_undo (PlannerCmd *cmd_base)
{
	TestCmd *cmd;
	ActionHistory *history;

	cmd = (TestCmd*) cmd_base;

	history = cmd->action_history;

	CHECK_BOOLEAN_RESULT (cmd->allocated, TRUE);
	CHECK_BOOLEAN_RESULT (history->next > 0, TRUE);
	CHECK_BOOLEAN_RESULT (history->actions[history->next - 1] == cmd->id, TRUE);

	history->actions[--history->next] = 0;
}

static void
test_cmd_free (PlannerCmd *cmd_base)
{
	TestCmd *cmd;

	cmd = (TestCmd*) cmd_base;

	CHECK_BOOLEAN_RESULT (cmd->allocated, TRUE);
	cmd->allocated = FALSE;
}


static PlannerCmd *
test_cmd (PlannerCmdManager *cmd_manager,
	  ActionHistory     *action_history,
	  gchar              cmd_id)
{
	PlannerCmd  *cmd_base;
	TestCmd     *cmd;
	gchar        cmd_name[10];

	CHECK_BOOLEAN_RESULT (cmd_id != 0, TRUE);

        g_snprintf (cmd_name, sizeof(cmd_name), "cmd %c", cmd_id);

	cmd_base = planner_cmd_new (TestCmd,
				    cmd_name,
				    test_cmd_do,
				    test_cmd_undo,
				    test_cmd_free);

	cmd = (TestCmd *) cmd_base;

	cmd->action_history = action_history;
	cmd->allocated = TRUE;
	cmd->id = cmd_id;

	planner_cmd_manager_insert_and_do (cmd_manager,
					   cmd_base);

	return cmd_base;
}

gint
main (gint argc, gchar **argv)
{
	ActionHistory action_history;

        g_type_init ();

	PlannerCmdManager *cmd_manager = planner_cmd_manager_new ();

	/* Initialise the action history */
	memset(action_history.actions, 0, sizeof(action_history.actions));
	action_history.next = 0;

	/* Test adding a bunch of normal commands */
	test_cmd(cmd_manager, &action_history, '1');
	test_cmd(cmd_manager, &action_history, '2');
	test_cmd(cmd_manager, &action_history, '3');
	test_cmd(cmd_manager, &action_history, '4');
	CHECK_STRING_RESULT (g_strdup(action_history.actions), "1234");

	/* Test undo and redo with normal commands */
	planner_cmd_manager_undo (cmd_manager);
	CHECK_STRING_RESULT (g_strdup(action_history.actions), "123");

	planner_cmd_manager_redo (cmd_manager);
	CHECK_STRING_RESULT (g_strdup(action_history.actions), "1234");

	planner_cmd_manager_undo (cmd_manager);
	planner_cmd_manager_undo (cmd_manager);
	planner_cmd_manager_undo (cmd_manager);
	CHECK_STRING_RESULT (g_strdup(action_history.actions), "1");

	planner_cmd_manager_redo (cmd_manager);
	planner_cmd_manager_redo (cmd_manager);
	CHECK_STRING_RESULT (g_strdup(action_history.actions), "123");

	/* Test if undo and redo stop at the end of the queue */
	planner_cmd_manager_redo (cmd_manager);
	CHECK_STRING_RESULT (g_strdup(action_history.actions), "1234");
	planner_cmd_manager_redo (cmd_manager);
	planner_cmd_manager_redo (cmd_manager);
	CHECK_STRING_RESULT (g_strdup(action_history.actions), "1234");

	planner_cmd_manager_undo (cmd_manager);
	planner_cmd_manager_undo (cmd_manager);
	planner_cmd_manager_undo (cmd_manager);
	planner_cmd_manager_undo (cmd_manager);
	CHECK_STRING_RESULT (g_strdup(action_history.actions), "");
	planner_cmd_manager_undo (cmd_manager);
	planner_cmd_manager_undo (cmd_manager);
	CHECK_STRING_RESULT (g_strdup(action_history.actions), "");

	/* Test if performing a new action after some undos prevents redoes
	 * from happening */
	planner_cmd_manager_redo (cmd_manager);
	planner_cmd_manager_redo (cmd_manager);
	test_cmd(cmd_manager, &action_history, '3');
	CHECK_STRING_RESULT (g_strdup(action_history.actions), "123");
	planner_cmd_manager_redo (cmd_manager);
	CHECK_STRING_RESULT (g_strdup(action_history.actions), "123");

	/* Test adding some transactions as well as normal commands */
	planner_cmd_manager_begin_transaction (cmd_manager, "trans 1");
	test_cmd(cmd_manager, &action_history, 'a');
	test_cmd(cmd_manager, &action_history, 'b');
	test_cmd(cmd_manager, &action_history, 'c');
	test_cmd(cmd_manager, &action_history, 'd');
	planner_cmd_manager_end_transaction (cmd_manager);
	CHECK_STRING_RESULT (g_strdup(action_history.actions), "123abcd");

	planner_cmd_manager_begin_transaction (cmd_manager, "trans 2");
	test_cmd(cmd_manager, &action_history, 'i');
	planner_cmd_manager_end_transaction (cmd_manager);
	CHECK_STRING_RESULT (g_strdup(action_history.actions), "123abcdi");

	test_cmd(cmd_manager, &action_history, '5');
	CHECK_STRING_RESULT (g_strdup(action_history.actions), "123abcdi5");

	planner_cmd_manager_begin_transaction (cmd_manager, "trans 3");
	test_cmd(cmd_manager, &action_history, 'x');
	test_cmd(cmd_manager, &action_history, 'y');
	planner_cmd_manager_end_transaction (cmd_manager);
	CHECK_STRING_RESULT (g_strdup(action_history.actions), "123abcdi5xy");

	/* Test undo & redo with a mix of transactions and normal commands */
	planner_cmd_manager_undo (cmd_manager);
	CHECK_STRING_RESULT (g_strdup(action_history.actions), "123abcdi5");

	planner_cmd_manager_redo (cmd_manager);
	CHECK_STRING_RESULT (g_strdup(action_history.actions), "123abcdi5xy");

	planner_cmd_manager_undo (cmd_manager);
	planner_cmd_manager_undo (cmd_manager);
	planner_cmd_manager_undo (cmd_manager);
	planner_cmd_manager_undo (cmd_manager);
	CHECK_STRING_RESULT (g_strdup(action_history.actions), "123");

	planner_cmd_manager_redo (cmd_manager);
	planner_cmd_manager_redo (cmd_manager);
	CHECK_STRING_RESULT (g_strdup(action_history.actions), "123abcdi");

	g_object_unref (cmd_manager);

	return EXIT_SUCCESS;
}

