/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2001-2002 CodeFactory AB
 * Copyright (C) 2001-2002 Richard Hult <richard@imendio.com>
 * Copyright (C) 2001-2002 Mikael Hallendal <micke@imendio.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __MRP_TASK_MANAGER_H__
#define __MRP_TASK_MANAGER_H__

#include <glib-object.h>

#define MRP_TYPE_TASK_MANAGER         (mrp_task_manager_get_type ())
#define MRP_TASK_MANAGER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), MRP_TYPE_TASK_MANAGER, MrpTaskManager))
#define MRP_TASK_MANAGER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), MRP_TYPE_TASK_MANAGER, MrpTaskManagerClass))
#define MRP_IS_TASK_MANAGER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), MRP_TYPE_TASK_MANAGER))
#define MRP_IS_TASK_MANAGER_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), MRP_TYPE_TASK_MANAGER))
#define MRP_TASK_MANAGER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), MRP_TYPE_TASK_MANAGER, MrpTaskManagerClass))

typedef struct _MrpTaskManager MrpTaskManager;
typedef struct _MrpTaskManagerPriv MrpTaskManagerPriv;

#include <libplanner/mrp-project.h>
#include <libplanner/mrp-task.h>

struct _MrpTaskManager {
	GObject             parent;
	MrpTaskManagerPriv *priv;
};

typedef struct {
	GObjectClass parent_class;
} MrpTaskManagerClass;


GType           mrp_task_manager_get_type             (void) G_GNUC_CONST;
MrpTaskManager *mrp_task_manager_new                  (MrpProject           *project);
GList          *mrp_task_manager_get_all_tasks        (MrpTaskManager       *manager);
void            mrp_task_manager_insert_task          (MrpTaskManager       *manager,
						       MrpTask              *parent,
						       gint                  position,
						       MrpTask              *task);
void            mrp_task_manager_remove_task          (MrpTaskManager       *manager,
						       MrpTask              *task);
gboolean        mrp_task_manager_move_task            (MrpTaskManager       *manager,
						       MrpTask              *task,
						       MrpTask              *sibling,
						       MrpTask              *parent,
						       gboolean              before,
						       GError              **error);
void            mrp_task_manager_set_root             (MrpTaskManager       *manager,
						       MrpTask              *task);
MrpTask        *mrp_task_manager_get_root             (MrpTaskManager       *manager);
void            mrp_task_manager_traverse             (MrpTaskManager       *manager,
						       MrpTask              *root,
						       MrpTaskTraverseFunc   func,
						       gpointer              user_data);
void            mrp_task_manager_set_block_scheduling (MrpTaskManager       *manager,
						       gboolean              block);
void            mrp_task_manager_rebuild              (MrpTaskManager       *manager);
void            mrp_task_manager_recalc               (MrpTaskManager       *manager,
						       gboolean              force);
gint            mrp_task_manager_calculate_task_work  (MrpTaskManager       *manager,
						       MrpTask              *task,
						       mrptime               start,
						       mrptime               finish);
void            mrp_task_manager_dump_task_tree       (MrpTaskManager       *manager);
void            mrp_task_manager_dump_task_list       (MrpTaskManager       *manager);



#endif /* __MRP_TASK_MANAGER_H__ */
