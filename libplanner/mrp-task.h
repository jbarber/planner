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

#ifndef __MRP_TASK_H__
#define __MRP_TASK_H__

#include <libplanner/mrp-object.h>
#include <libplanner/mrp-types.h>
#include <libplanner/mrp-time.h>
#include <libplanner/mrp-assignment.h>

#define MRP_TYPE_TASK			(mrp_task_get_type ())
#define MRP_TASK(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), MRP_TYPE_TASK, MrpTask))
#define MRP_TASK_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), MRP_TYPE_TASK, MrpTaskClass))
#define MRP_IS_TASK(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), MRP_TYPE_TASK))
#define MRP_IS_TASK_CLASS(klass)	(G_TYPE_CHECK_TYPE ((obj), MRP_TYPE_TASK))
#define MRP_TASK_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS ((obj), MRP_TYPE_TASK, MrpTaskClass))

#define MRP_TYPE_CONSTRAINT             (mrp_constraint_get_type ())
#define MRP_TYPE_RELATION               (mrp_relation_get_type ())

typedef struct _MrpTaskClass MrpTaskClass;
typedef struct _MrpTaskPriv  MrpTaskPriv;

#include <libplanner/mrp-relation.h>

struct _MrpTask
{
	MrpObject    parent;
	MrpTaskPriv *priv;
};

struct _MrpTaskClass
{
	MrpObjectClass parent_class;
};

GType            mrp_task_get_type                  (void);
GType            mrp_constraint_get_type            (void);
GType            mrp_relation_get_type              (void);
MrpTask         *mrp_task_new                       (void);
const gchar     *mrp_task_get_name                  (MrpTask          *task);
void             mrp_task_set_name                  (MrpTask          *task,
						     const gchar      *name);
MrpRelation     *mrp_task_add_predecessor           (MrpTask          *task,
						     MrpTask          *predecessor,
						     MrpRelationType   type,
						     glong             lag,
						     GError          **error);
void             mrp_task_remove_predecessor        (MrpTask          *task,
						     MrpTask          *predecessor);
MrpRelation     *mrp_task_get_relation              (MrpTask          *task_a,
						     MrpTask          *task_b);
MrpRelation     *mrp_task_get_predecessor_relation  (MrpTask          *task,
						     MrpTask          *predecessor);
MrpRelation     *mrp_task_get_successor_relation    (MrpTask          *task,
						     MrpTask          *successor);
GList           *mrp_task_get_predecessor_relations (MrpTask          *task);
GList           *mrp_task_get_successor_relations   (MrpTask          *task);
gboolean         mrp_task_has_relation_to           (MrpTask          *task_a,
						     MrpTask          *task_b);
gboolean         mrp_task_has_relation              (MrpTask          *task);
MrpTask         *mrp_task_get_parent                (MrpTask          *task);
MrpTask         *mrp_task_get_first_child           (MrpTask          *task);
MrpTask         *mrp_task_get_next_sibling          (MrpTask          *task);
guint            mrp_task_get_n_children            (MrpTask          *task);
MrpTask         *mrp_task_get_nth_child             (MrpTask          *task,
						     guint             n);
gint             mrp_task_get_position              (MrpTask          *task);
mrptime          mrp_task_get_start                 (MrpTask          *task);
mrptime          mrp_task_get_work_start            (MrpTask          *task);
mrptime          mrp_task_get_finish                (MrpTask          *task);
mrptime          mrp_task_get_latest_start          (MrpTask          *task);
mrptime          mrp_task_get_latest_finish         (MrpTask          *task);
gint             mrp_task_get_duration              (MrpTask          *task);
gint             mrp_task_get_work                  (MrpTask          *task);
GList           *mrp_task_get_assignments           (MrpTask          *task);
MrpAssignment   *mrp_task_get_assignment            (MrpTask          *task,
						     MrpResource      *resource);
void             mrp_task_reset_constraint          (MrpTask          *task);
gfloat           mrp_task_get_cost                  (MrpTask          *task);
GList           *mrp_task_get_assigned_resources    (MrpTask          *task);
gint             mrp_task_compare                   (gconstpointer     a,
						     gconstpointer     b);
MrpTaskType      mrp_task_get_task_type             (MrpTask          *task);
MrpTaskSched     mrp_task_get_sched                 (MrpTask          *task);
gshort           mrp_task_get_percent_complete      (MrpTask          *task);
gboolean         mrp_task_get_critical              (MrpTask          *task);


#endif /* __MRP_TASK_H__ */
