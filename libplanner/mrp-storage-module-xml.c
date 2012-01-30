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
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include <config.h>
#include <gmodule.h>

#include "mrp-storage-module-xml.h"

static void   mpsm_xml_init        (MrPStorageModuleXML      *entry);
static void   mpsm_xml_class_init  (MrPStorageModuleXMLClass *class);
static void   mpsm_xml_ps_init     (MrPProjectStorageIface   *iface);
static void   mpsm_xml_ts_init     (MrPTaskStorageIface      *iface);
static void   mpsm_xml_rs_init     (MrPResourceStorageIface  *iface);

static MrPStorageModuleClass *parent_class;
GType mrp_storage_module_xml_type = 0;


void
mrp_storage_module_xml_register_type (GTypeModule *module)
{
	static const GTypeInfo object_info = {
		sizeof (MrPStorageModuleXMLClass),
		(GBaseInitFunc) NULL,
		(GBaseFinalizeFunc) NULL,
		(GClassInitFunc) mpsm_xml_class_init,
		NULL,           /* class_finalize */
		NULL,           /* class_data */
		sizeof (MrPStorageModuleXML),
		0,              /* n_preallocs */
		(GInstanceInitFunc) mpsm_xml_init,
	};

	static const GInterfaceInfo project_storage_info = {
		(GInterfaceInitFunc) mpsm_xml_ps_init,        /* interface_init */
		NULL,                                         /* interface_finalize */
		NULL                                          /* interface_data */
	};

	static const GInterfaceInfo task_storage_info = {
		(GInterfaceInitFunc) mpsm_xml_ts_init,
		NULL,
		NULL
	};

        static const GInterfaceInfo resource_storage_info = {
                (GInterfaceInitFunc) mpsm_xml_rs_init,
                NULL,
                NULL
        };

	mrp_storage_module_xml_type = g_type_module_register_type (
		module,
		MRP_TYPE_STORAGE_MODULE,
		"MrPStorageModuleXML",
		&object_info, 0);

	g_type_add_interface_static (mrp_storage_module_xml_type,
				     MRP_TYPE_PROJECT_STORAGE,
				     &project_storage_info);

	g_type_add_interface_static (mrp_storage_module_xml_type,
				     MRP_TYPE_TASK_STORAGE,
				     &task_storage_info);

	g_type_add_interface_static (mrp_storage_module_xml_type,
				     MRP_TYPE_RESOURCE_STORAGE,
				     &resource_storage_info);
}

static void
mpsm_xml_init (MrPStorageModuleXML *xml)
{
	MrPStorageModule *module;

	module = MRP_STORAGE_MODULE (xml);

	MRP_STORAGE_MODULE_SET_FLAGS (module, MRP_STORAGE_READWRITE);

	xml->summary = g_new0 (MrPProjectSummary, 1);
	g_datalist_init (&xml->summary->properties);
}

static void
mpsm_xml_class_init (MrPStorageModuleXMLClass *klass)
{
	GObjectClass          *object_class = G_OBJECT_CLASS (klass);
	MrPStorageModuleClass *mrp_storage_module_class = MRP_STORAGE_MODULE_CLASS (klass);

	parent_class = MRP_STORAGE_MODULE_CLASS (g_type_class_peek_parent (klass));
}

G_MODULE_EXPORT void
module_init (GTypeModule *module)
{
	mrp_storage_module_xml_register_type (module);
}

G_MODULE_EXPORT MrPStorageModule *
module_new (void)
{
	return MRP_STORAGE_MODULE (g_object_new (MRP_TYPE_STORAGE_MODULE_XML, NULL));
}

G_MODULE_EXPORT void
module_exit (void)
{
}

/*
 * Project storage interface.
 */

static MrPProjectSummary *
mpsm_xml_ps_get_summary (MrPProjectStorage *storage)
{
	MrPStorageModuleXML *module = MRP_STORAGE_MODULE_XML (storage);

	return module->summary;
}

static void
mpsm_xml_ps_init (MrPProjectStorageIface *iface)
{
	iface->get_summary = mpsm_xml_ps_get_summary;
}

/*
 * Task storage interface.
 */

static GNode *
mpsm_xml_ts_get_task_tree (MrPTaskStorage *storage)
{
	return NULL;
}

static void
mpsm_xml_ts_init (MrPTaskStorageIface *iface)
{
	iface->get_task_tree = mpsm_xml_ts_get_task_tree;
}

/*
 * Resource storage interface.
 */

static GList *
mpsm_xml_rs_get_resource_list (MrPResourceStorage *storage)
{
        return NULL;
}

static void
mpsm_xml_rs_init (MrPResourceStorageIface *iface)
{
        iface->get_resource_list = mpsm_xml_rs_get_resource_list;
}

