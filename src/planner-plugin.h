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

#ifndef __PLANNER_PLUGIN_H__
#define __PLANNER_PLUGIN_H__

#include <gmodule.h>
#include <gtk/gtkwidget.h>
#include "planner-window.h"

#define PLANNER_TYPE_PLUGIN		   (planner_plugin_get_type ())
#define PLANNER_PLUGIN(obj)		   (G_TYPE_CHECK_INSTANCE_CAST ((obj), PLANNER_TYPE_PLUGIN, PlannerPlugin))
#define PLANNER_PLUGIN_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass), PLANNER_TYPE_PLUGIN, PlannerPluginClass))
#define PLANNER_IS_PLUGIN(obj)	   (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PLANNER_TYPE_PLUGIN))
#define PLANNER_IS_PLUGIN_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), PLANNER_TYPE_PLUGIN))
#define PLANNER_PLUGIN_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), PLANNER_TYPE_PLUGIN, PlannerPluginClass))

typedef struct _PlannerPlugin       PlannerPlugin;
typedef struct _PlannerPluginClass  PlannerPluginClass;
typedef struct _PlannerPluginPriv   PlannerPluginPriv;

struct _PlannerPlugin {
	GObject            parent;

	GModule           *handle;
	PlannerWindow      *main_window;

	PlannerPluginPriv      *priv;
	
	/* Methods. */
	void         (*init)       (PlannerPlugin       *plugin,
				    PlannerWindow *main_window);
	void         (*exit)       (PlannerPlugin       *plugin);
};

struct _PlannerPluginClass {
	GObjectClass parent_class;
};

GType        planner_plugin_get_type          (void) G_GNUC_CONST;
void         planner_plugin_init              (PlannerPlugin     *plugin,
					  PlannerWindow *main_window);
void         planner_plugin_exit              (PlannerPlugin     *plugin);

#endif /* __PLANNER_PLUGIN_H__ */

