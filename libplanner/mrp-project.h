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

#ifndef __MRP_PROJECT_H__
#define __MRP_PROJECT_H__

#include <glib-object.h>
#include <libplanner/mrp-application.h>
#include <libplanner/mrp-error.h>
#include <libplanner/mrp-group.h>
#include <libplanner/mrp-object.h>

#define MRP_TYPE_PROJECT         (mrp_project_get_type ())
#define MRP_PROJECT(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), MRP_TYPE_PROJECT, MrpProject))
#define MRP_PROJECT_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), MRP_TYPE_PROJECT, MrpProjectClass))
#define MRP_IS_PROJECT(o)	 (G_TYPE_CHECK_INSTANCE_TYPE ((o), MRP_TYPE_PROJECT))
#define MRP_IS_PROJECT_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), MRP_TYPE_PROJECT))
#define MRP_PROJECT_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), MRP_TYPE_PROJECT, MrpProjectClass))

typedef struct _MrpProject     MrpProject;
typedef struct _MrpProjectPriv MrpProjectPriv;

#include <libplanner/mrp-resource.h>
#include <libplanner/mrp-calendar.h>

typedef gboolean (*MrpTaskTraverseFunc) (MrpTask*, gpointer);

struct _MrpProject {
	MrpObject       parent;
	MrpProjectPriv *priv;
};

typedef struct {
	MrpObjectClass  parent_class;
} MrpProjectClass;

GType            mrp_project_get_type                 (void);
MrpProject      *mrp_project_new                      (MrpApplication       *app);
gboolean         mrp_project_is_empty                 (MrpProject           *project);
gboolean         mrp_project_needs_saving             (MrpProject           *project);
mrptime          mrp_project_get_project_start        (MrpProject           *project);
void             mrp_project_set_project_start        (MrpProject           *project,
						       mrptime               start);
gboolean         mrp_project_load                     (MrpProject           *project,
						       const gchar          *uri,
						       GError              **error);
gboolean         mrp_project_save                     (MrpProject           *project,
						       gboolean              force,
						       GError              **error);
gboolean         mrp_project_save_as                  (MrpProject           *project,
						       const gchar          *uri,
						       gboolean              force,
						       GError              **error);
gboolean         mrp_project_export                   (MrpProject           *project,
						       const gchar          *uri,
						       const gchar          *identifier,
						       gboolean              force,
						       GError              **error);
gboolean         mrp_project_save_to_xml              (MrpProject           *project,
						       gchar               **str,
						       GError              **error);
gboolean         mrp_project_load_from_xml            (MrpProject           *project,
						       const gchar          *str,
						       GError              **error);
void             mrp_project_close                    (MrpProject           *project);
const gchar     *mrp_project_get_uri                  (MrpProject           *project);
MrpResource *    mrp_project_get_resource_by_name     (MrpProject           *project,
						       const gchar          *name);
GList           *mrp_project_get_resources            (MrpProject           *project);
void             mrp_project_add_resource             (MrpProject           *project,
						       MrpResource          *resource);
void             mrp_project_remove_resource          (MrpProject           *project,
						       MrpResource          *resource);
MrpGroup        *mrp_project_get_group_by_name        (MrpProject           *project,
						       const gchar          *name);
GList           *mrp_project_get_groups               (MrpProject           *project);
void             mrp_project_add_group                (MrpProject           *project,
						       MrpGroup             *group);
void             mrp_project_remove_group             (MrpProject           *project,
						       MrpGroup             *group);
MrpTask         *mrp_project_get_task_by_name         (MrpProject           *project,
						       const gchar          *name);
GList           *mrp_project_get_all_tasks            (MrpProject           *project);
void             mrp_project_insert_task              (MrpProject           *project,
						       MrpTask              *parent,
						       gint                  position,
						       MrpTask              *task);
void             mrp_project_remove_task              (MrpProject           *project,
						       MrpTask              *task);
gboolean         mrp_project_move_task                (MrpProject           *project,
						       MrpTask              *task,
						       MrpTask              *sibling,
						       MrpTask              *parent,
						       gboolean              before,
						       GError              **error);
MrpTask         *mrp_project_get_root_task            (MrpProject           *project);
void             mrp_project_task_traverse            (MrpProject           *project,
						       MrpTask              *root,
						       MrpTaskTraverseFunc   func,
						       gpointer              user_data);
void             mrp_project_reschedule               (MrpProject           *project);
gint             mrp_project_calculate_task_work      (MrpProject           *project,
						       MrpTask              *task,
						       mrptime               start,
						       mrptime               finish);
GList *          mrp_project_get_properties_from_type (MrpProject           *project,
						       GType                 object_type);
void             mrp_project_add_property             (MrpProject           *project,
						       GType                 object_type,
						       MrpProperty          *property,
						       gboolean              user_defined);
void             mrp_project_remove_property          (MrpProject           *project,
						       GType                 object_type,
						       const gchar          *name);
gboolean         mrp_project_has_property             (MrpProject           *project,
						       GType                 owner_type,
						       const gchar          *name);
MrpProperty *    mrp_project_get_property             (MrpProject           *project,
						       const gchar          *name,
						       GType                 object_type);
MrpCalendar *    mrp_project_get_root_calendar        (MrpProject           *project);
MrpCalendar *    mrp_project_get_calendar             (MrpProject           *project);
MrpDay *         mrp_project_get_calendar_day_by_id   (MrpProject           *project,
						       gint                  id);


#endif /* __MRP_PROJECT_H__ */
