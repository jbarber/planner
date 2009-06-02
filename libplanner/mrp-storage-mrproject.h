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

#ifndef __MRP_STORAGE_MRPROJECT_H__
#define __MRP_STORAGE_MRPROJECT_H__

#include <glib-object.h>
#include "mrp-storage-module.h"
#include "mrp-types.h"
#include "mrp-project.h"

extern GType mrp_storage_mrproject_type;

#define MRP_TYPE_STORAGE_MRPROJECT		mrp_storage_mrproject_type
#define MRP_STORAGE_MRPROJECT(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), MRP_TYPE_STORAGE_MRPROJECT, MrpStorageMrproject))
#define MRP_STORAGE_MRPROJECT_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), MRP_TYPE_STORAGE_MRPROJECT, MrpStorageMrprojectClass))
#define MRP_IS_STORAGE_MRPROJECT(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), MRP_TYPE_STORAGE_MRPROJECT))
#define MRP_IS_STORAGE_MRPROJECT_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((obj), MRP_TYPE_STORAGE_MRPROJECT))
#define MRP_STORAGE_MRPROJECT_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), MRP_TYPE_STORAGE_MRPROJECT, MrpStorageMrprojectClass))

typedef struct _MrpStorageMrproject      MrpStorageMrproject;
typedef struct _MrpStorageMrprojectClass MrpStorageMrprojectClass;

struct _MrpStorageMrproject
{
	MrpStorageModule  parent;

	MrpProject       *project;

	MrpTask    *root_task;
	GHashTable *task_id_hash;
	GList      *delayed_relations;
	GList      *groups;
	GList      *resources;
	GList      *assignments;
	mrptime     project_start;
	MrpGroup   *default_group;
};

struct _MrpStorageMrprojectClass
{
	MrpStorageModuleClass parent_class;
};


void mrp_storage_mrproject_register_type (GTypeModule *module);

#endif /* __MRP_STORAGE_MRPROJECT_H__ */
