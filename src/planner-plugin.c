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
#include "planner-window.h"
#include "planner-plugin.h"
#include "valgrind.h"

static void mv_init       (PlannerPlugin      *plugin);
static void mv_class_init (PlannerPluginClass *class);
static void mv_finalize   (GObject            *object);

static GObjectClass *parent_class;

GType
planner_plugin_get_type (void)
{
	static GType type = 0;

	if (!type) {
		static const GTypeInfo info = {
			sizeof (PlannerPluginClass),
			NULL,		/* base_init */
			NULL,		/* base_finalize */
			(GClassInitFunc) mv_class_init,
			NULL,		/* class_finalize */
			NULL,		/* class_data */
			sizeof (PlannerPlugin),
			0,
			(GInstanceInitFunc) mv_init
		};

		type = g_type_register_static (G_TYPE_OBJECT,
					       "PlannerPlugin", &info, 0);
	}

	return type;
}

static void
mv_class_init (PlannerPluginClass *klass)
{
	GObjectClass *o_class;

	parent_class = g_type_class_peek_parent (klass);

	o_class = (GObjectClass *) klass;

	o_class->finalize = mv_finalize;

}

static void
mv_init (PlannerPlugin *plugin)
{
	plugin->init = NULL;
	plugin->exit = NULL;
}

static void
mv_finalize (GObject *object)
{
	PlannerPlugin *plugin = PLANNER_PLUGIN (object);

	if (plugin->exit) {
		plugin->exit (plugin);
	}

	/* Don't unload modules when running in valgrind to make sure
	 * stack traces won't have missing symbols.
	 */
	if (!RUNNING_ON_VALGRIND) {
		g_module_close (plugin->handle);
	}
}

void
planner_plugin_setup (PlannerPlugin *plugin,
		      PlannerWindow *main_window)
{
	g_return_if_fail (PLANNER_IS_PLUGIN (plugin));

	plugin->main_window = main_window;

	if (plugin->init) {
		plugin->init (plugin);
	}
}

