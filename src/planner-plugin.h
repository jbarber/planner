/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2002 CodeFactory AB
 * Copyright (C) 2002 Richard Hult <richard@imendio.com>
 * Copyright (C) 2002 Mikael Hallendal <micke@imendio.com>
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

#ifndef __MG_PLUGIN_H__
#define __MG_PLUGIN_H__

#include <gmodule.h>
#include <gtk/gtkwidget.h>
#include "planner-main-window.h"

#define MG_TYPE_PLUGIN		   (planner_plugin_get_type ())
#define MG_PLUGIN(obj)		   (G_TYPE_CHECK_INSTANCE_CAST ((obj), MG_TYPE_PLUGIN, MgPlugin))
#define MG_PLUGIN_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass), MG_TYPE_PLUGIN, MgPluginClass))
#define MG_IS_PLUGIN(obj)	   (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MG_TYPE_PLUGIN))
#define MG_IS_PLUGIN_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), MG_TYPE_PLUGIN))
#define MG_PLUGIN_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), MG_TYPE_PLUGIN, MgPluginClass))

typedef struct _MgPlugin       MgPlugin;
typedef struct _MgPluginClass  MgPluginClass;
typedef struct _MgPluginPriv   MgPluginPriv;

struct _MgPlugin {
	GObject            parent;

	GModule           *handle;
	MgMainWindow      *main_window;

	MgPluginPriv      *priv;
	
	/* Methods. */
	void         (*init)       (MgPlugin       *plugin,
				    MgMainWindow *main_window);
	void         (*exit)       (MgPlugin       *plugin);
};

struct _MgPluginClass {
	GObjectClass parent_class;
};

GType        planner_plugin_get_type          (void);
void         planner_plugin_init              (MgPlugin     *plugin,
					  MgMainWindow *main_window);
void         planner_plugin_exit              (MgPlugin     *plugin);

#endif /* __MG_PLUGIN_H__ */

