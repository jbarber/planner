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

#ifndef __MRP_STORAGE_MODULE_FACTORY_H__
#define __MRP_STORAGE_MODULE_FACTORY_H__

#include <glib-object.h>
#include <gmodule.h>
#include <libplanner/mrp-storage-module.h>
#include <libplanner/mrp-project.h>


#define MRP_TYPE_STORAGE_MODULE_FACTORY		  (mrp_storage_module_factory_get_type ())
#define MRP_STORAGE_MODULE_FACTORY(obj)		  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MRP_TYPE_STORAGE_MODULE_FACTORY, MrpStorageModuleFactory))
#define MRP_STORAGE_MODULE_FACTORY_CLASS(klass)	  (G_TYPE_CHECK_CLASS_CAST ((klass), MRP_TYPE_STORAGE_MODULE_FACTORY, MrpStorageModuleFactoryClass))
#define MRP_IS_STORAGE_MODULE_FACTORY(obj)	  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MRP_TYPE_STORAGE_MODULE_FACTORY))
#define MRP_IS_STORAGE_MODULE_FACTORY_CLASS(klass) (G_TYPE_CHECK_TYPE ((obj), MRP_TYPE_STORAGE_MODULE_FACTORY))
#define MRP_STORAGE_MODULE_FACTORY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), MRP_TYPE_STORAGE_MODULE_FACTORY, MrpStorageModuleFactoryClass))


typedef struct _MrpStorageModuleFactory      MrpStorageModuleFactory;
typedef struct _MrpStorageModuleFactoryClass MrpStorageModuleFactoryClass;

struct _MrpStorageModuleFactory
{
	GTypeModule   parent;

	GModule       *library;
	gchar         *name;

	/* Initialization and uninitialization. */
	void		  (* init)	(GTypeModule      *module);
	void		  (* exit)	(MrpStorageModule *module);
	MrpStorageModule *(* new)	(void);
};

struct _MrpStorageModuleFactoryClass
{
	GTypeModuleClass parent_class;
};

GType                    mrp_storage_module_factory_get_type	  (void) G_GNUC_CONST;

MrpStorageModuleFactory *mrp_storage_module_factory_get	          (const gchar	    	   *name);

MrpStorageModule        *mrp_storage_module_factory_create_module (MrpStorageModuleFactory *factory);


#endif /* __MRP_STORAGE_MODULE_FACTORY_H__ */

