/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2004      Imendio HB
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

#ifndef __MRP_PRIVATE_H__
#define __MRP_PRIVATE_H__

#include <libplanner/mrp-calendar.h>
#include <libplanner/mrp-task-manager.h>
#include <libplanner/mrp-project.h>
#include <libplanner/mrp-storage-module.h>
#include <libplanner/mrp-task.h>
#include <libplanner/mrp-file-module.h>

typedef struct {
	GList *prev;  /* <MrpTask> The node depends on those. */
	GList *next;  /* <MrpTask> Those depends on this node. */
} MrpTaskGraphNode;


/* MrpProject functions. */
void            imrp_project_set_resources         (MrpProject        *project,
						    GList             *resources);
void            imrp_project_set_groups            (MrpProject        *project,
						    GList             *groups);
MrpTaskManager *imrp_project_get_task_manager      (MrpProject        *project);
void            imrp_task_add_assignment           (MrpTask           *task,
						    MrpAssignment     *assignment);
void            imrp_resource_add_assignment       (MrpResource       *resource,
						    MrpAssignment     *assignment);

guint           imrp_task_get_unique_id            (MrpProject        *project);

/* Task functions. */
gboolean          mrp_task_manager_check_predecessor (MrpTaskManager  *manager,
						      MrpTask         *task,
						      MrpTask         *predecessor,
						      GError         **error);
gboolean          mrp_task_manager_check_move        (MrpTaskManager  *manager,
						      MrpTask         *task,
						      MrpTask         *parent,
						      GError         **error);
void              imrp_task_insert_child             (MrpTask         *parent,
						      gint             position,
						      MrpTask         *child);
void              imrp_task_remove_subtree           (MrpTask         *task);
void              imrp_task_detach                   (MrpTask         *task);
void              imrp_task_reattach                 (MrpTask         *task,
						      MrpTask         *sibling,
						      MrpTask         *parent,
						      gboolean         before);
void              imrp_task_reattach_pos             (MrpTask         *task,
						      gint             pos,
						      MrpTask         *parent);
void              imrp_task_set_visited              (MrpTask         *task,
						      gboolean         visited);
gboolean          imrp_task_get_visited              (MrpTask         *task);
void              imrp_task_set_start                (MrpTask         *task,
						      mrptime          start);
void              imrp_task_set_work_start           (MrpTask         *task,
						      mrptime          start);
void              imrp_task_set_finish               (MrpTask         *task,
						      mrptime          finish);
void              imrp_task_set_latest_start         (MrpTask         *task,
						      mrptime          time);
void              imrp_task_set_latest_finish        (MrpTask         *task,
						      mrptime          time);
void              imrp_task_set_duration             (MrpTask         *task,
						      gint             duration);
void              imrp_task_set_work                 (MrpTask         *task,
						      gint             work);
MrpTaskGraphNode *imrp_task_get_graph_node           (MrpTask         *task);
MrpConstraint     impr_task_get_constraint           (MrpTask         *task);
void              impr_task_set_constraint           (MrpTask         *task,
						      MrpConstraint    constraint);
gint              imrp_task_get_depth                (MrpTask         *task);
GNode *           imrp_task_get_node                 (MrpTask         *task);
GList *           imrp_task_peek_predecessors        (MrpTask         *task);
GList *           imrp_task_peek_successors          (MrpTask         *task);
MrpTaskType       imrp_task_get_type                 (MrpTask         *task);
MrpTaskSched      imrp_task_get_sched                (MrpTask         *task);


/* MrpTime funcitons. */
void            imrp_time_init                     (void);

/* MrpStorageModule functions. */
void     imrp_storage_module_set_project  (MrpStorageModule *module,
					   MrpProject       *project);
gboolean imrp_project_add_calendar_day    (MrpProject       *project,
					   MrpDay           *day);
GList *  imrp_project_get_calendar_days   (MrpProject       *project);
void     imrp_project_remove_calendar_day (MrpProject       *project,
					   MrpDay           *day);


/* Calendar functions. */
void imrp_project_signal_calendar_tree_changed (MrpProject  *project);
void imrp_day_setup_defaults                   (void);
void imrp_calendar_replace_day                 (MrpCalendar *calendar,
						MrpDay      *orig_day,
						MrpDay      *new_day);


/* Signals. */
void imrp_project_set_needs_saving (MrpProject *project,
				    gboolean    needs_saving);
void imrp_object_removed           (MrpObject  *object);
void imrp_project_task_inserted    (MrpProject *project,
				    MrpTask    *task);
void imrp_project_task_moved       (MrpProject *project,
				    MrpTask    *task);


/* Property related stuff */
void imrp_project_property_changed (MrpProject  *project,
				    MrpProperty *property);
void imrp_property_set_project     (MrpProperty *property,
				    MrpProject  *project);


/* MrpApplication functions. */
GList *  imrp_application_get_all_file_readers (MrpApplication *app);
GList *  imrp_application_get_all_file_writers (MrpApplication *app);
void     imrp_application_register_reader      (MrpApplication *app,
						MrpFileReader  *reader);
void     imrp_application_register_writer      (MrpApplication *app,
						MrpFileWriter  *writer);
gboolean imrp_application_id_set_data          (gpointer        data,
						guint           data_id);
gboolean imrp_application_id_remove_data       (guint           object_id);




#endif /* __MRP_PRIVATE_H__ */
