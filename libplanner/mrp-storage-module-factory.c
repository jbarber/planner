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

#include <config.h>
#include <gmodule.h>
#include "mrp-paths.h"
#include "mrp-storage-module-factory.h"
#include "mrp-storage-module.h"

static void     storage_module_factory_init       (MrpStorageModuleFactory      *factory);
static void     storage_module_factory_class_init (MrpStorageModuleFactoryClass *class);

static gboolean storage_module_factory_load       (GTypeModule                  *module);
static void     storage_module_factory_unload     (GTypeModule                  *module);


static GHashTable       *module_hash = NULL;
static GTypeModuleClass *parent_class;


GType
mrp_storage_module_factory_get_type (void)
{
	static GType object_type = 0;

	if (!object_type) {
		static const GTypeInfo object_info = {
			sizeof (MrpStorageModuleFactoryClass),
			NULL,		/* base_init */
			NULL,		/* base_finalize */
			(GClassInitFunc) storage_module_factory_class_init,
			NULL,		/* class_finalize */
			NULL,		/* class_data */
			sizeof (MrpStorageModuleFactory),
			0,              /* n_preallocs */
			(GInstanceInitFunc) storage_module_factory_init,
		};

		object_type =
			g_type_register_static (G_TYPE_TYPE_MODULE,
						"MrpStorageModuleFactory",
						&object_info, 0);
	}

	return object_type;
}

static void
storage_module_factory_init (MrpStorageModuleFactory *factory)
{
}

static void
storage_module_factory_class_init (MrpStorageModuleFactoryClass *klass)
{
	/*GObjectClass *object_class = G_OBJECT_CLASS (klass);*/
	GTypeModuleClass *module_class = G_TYPE_MODULE_CLASS (klass);

	parent_class = G_TYPE_MODULE_CLASS (g_type_class_peek_parent (klass));

	module_class->load = storage_module_factory_load;
	module_class->unload = storage_module_factory_unload;
}

static gboolean
storage_module_factory_load (GTypeModule *module)
{
	MrpStorageModuleFactory *factory = MRP_STORAGE_MODULE_FACTORY (module);

	/*g_print ("Load '%s'\n", factory->name);*/

	factory->library = g_module_open (factory->name, 0);
	if (!factory->library) {
		g_warning ("%s", g_module_error ());
		return FALSE;
	}

	/* These must be implemented by all storage modules. */
	if (!g_module_symbol (factory->library, "module_init", (gpointer)&factory->init) ||
	    !g_module_symbol (factory->library, "module_new", (gpointer)&factory->new) ||
	    !g_module_symbol (factory->library, "module_exit", (gpointer)&factory->exit)) {
		g_warning ("%s", g_module_error ());
		g_module_close (factory->library);

		return FALSE;
	}

	factory->init (module);

	return TRUE;
}

static void
storage_module_factory_unload (GTypeModule *module)
{
	MrpStorageModuleFactory *factory = MRP_STORAGE_MODULE_FACTORY (module);

	/*g_print ("Unload '%s'\n", factory->name);*/

	/* FIXME: should pass the module here somehow. */
	/*factory->exit (NULL); */

	g_module_close (factory->library);
	factory->library = NULL;

	factory->init = NULL;
	factory->exit = NULL;
}

MrpStorageModuleFactory *
mrp_storage_module_factory_get (const gchar *name)
{
	MrpStorageModuleFactory *factory;
	gchar                   *fullname, *libname;
	gchar                   *path;

	fullname = g_strconcat ("storage-", name, NULL);

	path = mrp_paths_get_storagemodule_dir (NULL);
	libname = g_module_build_path (path, fullname);
	g_free (path);

	if (!module_hash) {
		module_hash = g_hash_table_new (g_str_hash, g_str_equal);
	}

	factory = g_hash_table_lookup (module_hash, libname);

	if (!factory) {
		factory = g_object_new (MRP_TYPE_STORAGE_MODULE_FACTORY, NULL);
		g_type_module_set_name (G_TYPE_MODULE (factory), libname);
		factory->name = libname;

		g_hash_table_insert (module_hash, factory->name, factory);
	}

	g_free (fullname);

	if (!g_type_module_use (G_TYPE_MODULE (factory))) {
		return NULL;
	}

	return factory;
}

MrpStorageModule *
mrp_storage_module_factory_create_module (MrpStorageModuleFactory *factory)
{
	MrpStorageModule *module;

	module = factory->new ();

	return module;
}


