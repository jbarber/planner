/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2001-2002 CodeFactory AB
 * Copyright (C) 2001-2002 Richard Hult <richard@imendio.com>
 * Copyright (C) 2001-2002 Mikael Hallendal <micke@imendio.com>
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

#ifndef __MRP_STORAGE_MODULE_XML_H__
#define __MRP_STORAGE_MODULE_XML_H__

#include <glib-object.h>

#include "mrp-storage-module.h"
#include "mrp-project-storage.h"
#include "mrp-resource-storage.h"
#include "mrp-task-storage.h"

extern GType mrp_storage_module_xml_type;

#define MRP_TYPE_STORAGE_MODULE_XML		mrp_storage_module_xml_type
#define MRP_STORAGE_MODULE_XML(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), MRP_TYPE_STORAGE_MODULE_XML, MrPStorageModuleXML))
#define MRP_STORAGE_MODULE_XML_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), MRP_TYPE_STORAGE_MODULE_XML, MrPStorageModuleXMLClass))
#define MRP_IS_STORAGE_MODULE_XML(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), MRP_TYPE_STORAGE_MODULE_XML))
#define MRP_IS_STORAGE_MODULE_XML_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((obj), MRP_TYPE_STORAGE_MODULE_XML))
#define MRP_STORAGE_MODULE_XML_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), MRP_TYPE_STORAGE_MODULE_XML, MrPStorageModuleXMLClass))

typedef struct _MrPStorageModuleXML      MrPStorageModuleXML;
typedef struct _MrPStorageModuleXMLClass MrPStorageModuleXMLClass;

struct _MrPStorageModuleXML
{
	MrPStorageModule   parent;

	MrPProjectSummary *summary;
};

struct _MrPStorageModuleXMLClass
{
	MrPStorageModuleClass parent_class;
};

#endif /* __MRP_STORAGE_MODULE_XML_H__ */
