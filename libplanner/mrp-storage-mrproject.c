/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2001-2003 CodeFactory AB
 * Copyright (C) 2001-2003 Richard Hult <richard@imendio.com>
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

#include <config.h>
#include <gmodule.h>
#include "mrp-error.h"
#include "mrp-storage-module.h"
#include "mrp-private.h"
#include "mrp-storage-mrproject.h"
#include "mrp-parser.h"

static void       mpsm_init        (MrpStorageMrproject       *storage);
static void       mpsm_class_init  (MrpStorageMrprojectClass  *class);
static gboolean   mpsm_load        (MrpStorageModule          *module,
				    const gchar               *uri,
				    GError                   **error);
static gboolean   mpsm_save        (MrpStorageModule          *module,
				    const gchar               *uri,
				    gboolean                   force,
				    GError                   **error);
static gboolean   mpsm_to_xml      (MrpStorageModule          *module,
				    char                     **str,
				    GError                   **error);
static gboolean   mpsm_from_xml    (MrpStorageModule          *module,
				    const gchar               *str,
				    GError                   **error);
static void       mpsm_set_project (MrpStorageModule          *module,
				    MrpProject                *project);
void              module_init      (GTypeModule               *module);
MrpStorageModule *module_new       (void                      *project);
void              module_exit      (void);



static MrpStorageModuleClass *parent_class;
GType mrp_storage_mrproject_type = 0;


void
mrp_storage_mrproject_register_type (GTypeModule *module)
{
	static const GTypeInfo object_info = {
		sizeof (MrpStorageMrprojectClass),
		(GBaseInitFunc) NULL,
		(GBaseFinalizeFunc) NULL,
		(GClassInitFunc) mpsm_class_init,
		NULL,           /* class_finalize */
		NULL,           /* class_data */
		sizeof (MrpStorageMrproject),
		0,              /* n_preallocs */
		(GInstanceInitFunc) mpsm_init,
	};

	mrp_storage_mrproject_type = g_type_module_register_type (
		module,
		MRP_TYPE_STORAGE_MODULE,
		"MrpStorageMrproject",
		&object_info, 0);
}

static void
mpsm_init (MrpStorageMrproject *storage)
{
}

static void
mpsm_class_init (MrpStorageMrprojectClass *klass)
{
	MrpStorageModuleClass *mrp_storage_module_class = MRP_STORAGE_MODULE_CLASS (klass);

	parent_class = MRP_STORAGE_MODULE_CLASS (g_type_class_peek_parent (klass));

	mrp_storage_module_class->load = mpsm_load;
	mrp_storage_module_class->save = mpsm_save;
	mrp_storage_module_class->to_xml = mpsm_to_xml;
	mrp_storage_module_class->from_xml = mpsm_from_xml;
	mrp_storage_module_class->set_project = mpsm_set_project;
}

G_MODULE_EXPORT void
module_init (GTypeModule *module)
{
	mrp_storage_mrproject_register_type (module);
}

G_MODULE_EXPORT MrpStorageModule *
module_new (void *project)
{
	MrpStorageModule *module;

	module = MRP_STORAGE_MODULE (g_object_new (MRP_TYPE_STORAGE_MRPROJECT,
						   NULL));

	return module;
}

G_MODULE_EXPORT void
module_exit (void)
{
}

static void
mpsm_process_delayed_relations (MrpStorageMrproject *sm)
{
	GList           *l;
	MrpTask         *task, *predecessor_task;
	DelayedRelation *relation;

	for (l = sm->delayed_relations; l; l = l->next) {
		relation = l->data;

		task = g_hash_table_lookup (sm->task_id_hash,
					    GINT_TO_POINTER (relation->successor_id));
		g_assert (task != NULL);

		predecessor_task = g_hash_table_lookup (sm->task_id_hash,
							GINT_TO_POINTER (relation->predecessor_id));
		g_assert (predecessor_task != NULL);

		mrp_task_add_predecessor (task,
					  predecessor_task,
					  relation->type,
					  relation->lag,
					  NULL);

		g_free (relation);
	}
}

static gboolean
mpsm_load (MrpStorageModule *module, const gchar *uri, GError **error)
{
	MrpStorageMrproject *sm;
	MrpTaskManager      *task_manager;
	MrpAssignment       *assignment;
	GList               *node;

	g_return_val_if_fail (MRP_IS_STORAGE_MRPROJECT (module), FALSE);

	sm = MRP_STORAGE_MRPROJECT (module);

	/* FIXME: Check that we don't load twice? Free before loading? */

#if 0
	if (!mrp_parser_load (MRP_STORAGE_MRPROJECT (module), uri, error)) {
		return FALSE;
	}
#endif

	task_manager = imrp_project_get_task_manager (sm->project);

	mrp_task_manager_set_root (task_manager,
				   sm->root_task);

	g_object_set (sm->project,
		      "project-start", sm->project_start,
		      "default-group", sm->default_group,
		      NULL);

	mpsm_process_delayed_relations (sm);

	g_hash_table_destroy (sm->task_id_hash);
	g_list_free (sm->delayed_relations);

	imrp_project_set_groups (sm->project, sm->groups);

	for (node = sm->assignments; node; node = node->next) {
		assignment = MRP_ASSIGNMENT (node->data);

		imrp_task_add_assignment (mrp_assignment_get_task (assignment),
					  assignment);
		imrp_resource_add_assignment (mrp_assignment_get_resource (assignment),
					      assignment);
		g_object_unref (assignment);
	}

	return TRUE;
}

static gboolean
mpsm_save (MrpStorageModule  *module,
	   const gchar       *uri,
	   gboolean           force,
	   GError           **error)
{
	g_return_val_if_fail (MRP_IS_STORAGE_MRPROJECT (module), FALSE);

	return mrp_parser_save (MRP_STORAGE_MRPROJECT (module),
				uri,
				force,
				error);
}

static gboolean
mpsm_to_xml (MrpStorageModule  *module,
	     gchar            **str,
	     GError           **error)
{
	g_return_val_if_fail (MRP_IS_STORAGE_MRPROJECT (module), FALSE);

	return mrp_parser_to_xml (MRP_STORAGE_MRPROJECT (module),
				  str,
				  error);
}

static gboolean
mpsm_from_xml (MrpStorageModule  *module,
	       const gchar       *str,
	       GError           **error)
{
	g_return_val_if_fail (MRP_IS_STORAGE_MRPROJECT (module), FALSE);

	return mrp_parser_from_xml (MRP_STORAGE_MRPROJECT (module),
				    str,
				    error);
}

static void
mpsm_set_project (MrpStorageModule *module,
		  MrpProject       *project)
{
	MRP_STORAGE_MRPROJECT (module)->project = project;
}
