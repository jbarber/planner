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

#ifndef __MRP_STORAGE_MODULE_H__
#define __MRP_STORAGE_MODULE_H__

#include <glib-object.h>
#include <libplanner/mrp-error.h>
#include <libplanner/mrp-project.h>

#define MRP_TYPE_STORAGE_MODULE			(mrp_storage_module_get_type ())
#define MRP_STORAGE_MODULE(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), MRP_TYPE_STORAGE_MODULE, MrpStorageModule))
#define MRP_STORAGE_MODULE_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), MRP_TYPE_STORAGE_MODULE, MrpStorageModuleClass))
#define MRP_IS_STORAGE_MODULE(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), MRP_TYPE_STORAGE_MODULE))
#define MRP_IS_STORAGE_MODULE_CLASS(klass)	(G_TYPE_CHECK_TYPE ((obj), MRP_TYPE_STORAGE_MODULE))
#define MRP_STORAGE_MODULE_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), MRP_TYPE_STORAGE_MODULE, MrpStorageModuleClass))

typedef struct _MrpStorageModule      MrpStorageModule;
typedef struct _MrpStorageModuleClass MrpStorageModuleClass;

struct _MrpStorageModule
{
	GObject parent;

	/* <private> */

	guint flags;
};

struct _MrpStorageModuleClass
{
	GObjectClass parent_class;

 	gboolean (* save)	(MrpStorageModule  *module,
				 const gchar       *uri,
				 gboolean           force,
				 GError           **error);
 	gboolean (* load)	(MrpStorageModule  *module,
				 const gchar       *uri,
				 GError           **error);
 	gboolean (* to_xml)     (MrpStorageModule  *module,
				 gchar            **str,
				 GError           **error);
 	gboolean (* from_xml)   (MrpStorageModule  *module,
				 const gchar       *str,
				 GError           **error);
	void (* set_project)    (MrpStorageModule  *module,
				 MrpProject        *project);
};

typedef enum {
	MRP_STORAGE_NONE                = 0,
	MRP_STORAGE_READABLE            = 1 << 0,
	MRP_STORAGE_WRITABLE            = 1 << 1,
	MRP_STORAGE_DIRECT_WRITE        = 1 << 2,
	MRP_STORAGE_SUPPORTS_TASK       = 1 << 3,
	MRP_STORAGE_SUPPORTS_RESOURCE   = 1 << 4,
	MRP_STORAGE_SUPPORTS_PRIMARY    = 1 << 5,

	MRP_STORAGE_READWRITE           = (MRP_STORAGE_READABLE |
                                            MRP_STORAGE_WRITABLE),

	MRP_STORAGE_SUPPORTS_ALL        = (MRP_STORAGE_SUPPORTS_TASK |
					   MRP_STORAGE_SUPPORTS_RESOURCE |
					   MRP_STORAGE_SUPPORTS_PRIMARY)
} MrpStorageModuleFlags;

GType    mrp_storage_module_get_type (void) G_GNUC_CONST;
gboolean mrp_storage_module_load     (MrpStorageModule  *module,
				      const gchar       *uri,
				      GError           **error);
gboolean mrp_storage_module_save     (MrpStorageModule  *module,
				      const gchar       *uri,
				      gboolean           force,
				      GError           **error);
gboolean mrp_storage_module_to_xml   (MrpStorageModule  *module,
				      gchar            **str,
				      GError           **error);
gboolean mrp_storage_module_from_xml (MrpStorageModule  *module, 
				      const gchar       *str,
				      GError           **error);


#endif /* __MRP_STORAGE_MODULE_H__ */
