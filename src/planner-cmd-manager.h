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

#ifndef __PLANNER_CMD_MANAGER_H__
#define __PLANNER_CMD_MANAGER_H__

#include <glib-object.h>

#define PLANNER_TYPE_CMD_MANAGER            (planner_cmd_manager_get_type ())
#define PLANNER_CMD_MANAGER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PLANNER_TYPE_CMD_MANAGER, PlannerCmdManager))
#define PLANNER_CMD_MANAGER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), PLANNER_TYPE_CMD_MANAGER, PlannerCmdManagerClass))
#define PLANNER_IS_CMD_MANAGER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PLANNER_TYPE_CMD_MANAGER))
#define PLANNER_IS_CMD_MANAGER_CLASS(klass) (G_TYPE_CHECK_TYPE ((klass), PLANNER_TYPE_CMD_MANAGER))
#define PLANNER_CMD_MANAGER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), PLANNER_TYPE_CMD_MANAGER, PlannerCmdManagerClass))

typedef struct _PlannerCmdManager      PlannerCmdManager;
typedef struct _PlannerCmdManagerClass PlannerCmdManagerClass;
typedef struct _PlannerCmdManagerPriv  PlannerCmdManagerPriv;

struct _PlannerCmdManager {
	GObject                parent;
	PlannerCmdManagerPriv *priv;
};

struct _PlannerCmdManagerClass {
	GObjectClass parent_class;
};

typedef struct _PlannerCmd     PlannerCmd;

typedef gboolean (*PlannerCmdDoFunc)   (PlannerCmd *cmd);
typedef void     (*PlannerCmdUndoFunc) (PlannerCmd *cmd);
typedef void     (*PlannerCmdFreeFunc) (PlannerCmd *cmd);

typedef enum {
	PLANNER_CMD_TYPE_NORMAL = 0,
	PLANNER_CMD_TYPE_BEGIN_TRANSACTION,
	PLANNER_CMD_TYPE_END_TRANSACTION
} PlannerCmdType;


struct _PlannerCmd {
	gchar              *label;
	
	PlannerCmdManager  *manager;
	
	PlannerCmdDoFunc    do_func;
	PlannerCmdUndoFunc  undo_func;
	PlannerCmdFreeFunc  free_func;

	PlannerCmdType      type;
};


GType              planner_cmd_manager_get_type          (void) G_GNUC_CONST;
PlannerCmdManager *planner_cmd_manager_new               (void);
gboolean           planner_cmd_manager_insert_and_do     (PlannerCmdManager  *manager,
							  PlannerCmd         *cmd);
gboolean           planner_cmd_manager_undo              (PlannerCmdManager  *manager);
gboolean           planner_cmd_manager_redo              (PlannerCmdManager  *manager);
gboolean           planner_cmd_manager_begin_transaction (PlannerCmdManager  *manager,
							  const gchar        *label);
gboolean           planner_cmd_manager_end_transaction   (PlannerCmdManager  *manager);
PlannerCmd *       planner_cmd_new_size                  (gsize               size,
							  const gchar        *label,
							  PlannerCmdDoFunc    do_func,
							  PlannerCmdUndoFunc  undo_func,
							  PlannerCmdFreeFunc  free_func);

#define planner_cmd_new(t,l,d,u,f) planner_cmd_new_size(sizeof(t),l,d,u,f)


#endif /* __PLANNER_CMD_MANAGER_H__ */



