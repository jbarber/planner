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

#include <config.h>
#include <glib.h>
#include "planner-main-window.h"
#include "planner-plugin.h"

static void mv_init       (MgPlugin      *plugin);
static void mv_class_init (MgPluginClass *class);


static GObjectClass *parent_class;

GType
planner_plugin_get_type (void)
{
	static GType type = 0;

	if (!type) {
		static const GTypeInfo info = {
			sizeof (MgPluginClass),
			NULL,		/* base_init */
			NULL,		/* base_finalize */
			(GClassInitFunc) mv_class_init,
			NULL,		/* class_finalize */
			NULL,		/* class_data */
			sizeof (MgPlugin),
			0,
			(GInstanceInitFunc) mv_init
		};

		type = g_type_register_static (G_TYPE_OBJECT,
					       "MgPlugin", &info, 0);
	}

	return type;
}

static void
mv_class_init (MgPluginClass *klass)
{
	/*GObjectClass *object_class;

	object_class = (GObjectClass *) klass;*/

	parent_class = g_type_class_peek_parent (klass);
}

static void
mv_init (MgPlugin *plugin)
{

}

void
planner_plugin_init (MgPlugin     *plugin,
		MgMainWindow *main_window)
{
	g_return_if_fail (MG_IS_PLUGIN (plugin));

	plugin->main_window = main_window;

	if (plugin->init) {
		plugin->init (plugin, main_window);
	}
}

void
planner_plugin_exit (MgPlugin *plugin)
{
	g_return_if_fail (MG_IS_PLUGIN (plugin));

	if (plugin->exit) {
		plugin->exit (plugin);
	}

	g_module_close (plugin->handle);
	plugin->handle = NULL;
	plugin->main_window = NULL;
}

