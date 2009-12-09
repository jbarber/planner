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
#include <string.h>
#include <glib.h>
#include <glib/gi18n.h>
#include "libplanner/mrp-paths.h"
#include "planner-plugin.h"
#include "planner-plugin-loader.h"

static PlannerPlugin *
mpl_load (const gchar *file)
{
	PlannerPlugin *plugin;
	GModule       *handle;

	handle = g_module_open (file, 0);

	if (handle == NULL) {
		g_warning (_("Could not open plugin file '%s'\n"),
			   g_module_error ());

		return NULL;
	}

	plugin = g_object_new (PLANNER_TYPE_PLUGIN, NULL);
	plugin->handle = handle;

	g_module_symbol (plugin->handle, "plugin_init", (gpointer)&plugin->init);
	g_module_symbol (plugin->handle, "plugin_exit", (gpointer)&plugin->exit);

	return plugin;
}

static GList *
mpl_load_dir (const gchar *path, PlannerWindow *window)
{
	GDir*        dir;
	const gchar *name;
	PlannerPlugin    *plugin;
	GList       *list = NULL;

	dir = g_dir_open (path, 0, NULL);
	if (dir == NULL) {
		return NULL;
	}

	while ((name = g_dir_read_name (dir)) != NULL) {
		if (g_str_has_suffix (name, G_MODULE_SUFFIX)) {
			gchar *plugin_name;

			plugin_name = g_build_filename (path,
							name,
							NULL);
			plugin = mpl_load (plugin_name);
			if (plugin) {
				list = g_list_append (list, plugin);

				planner_plugin_setup (plugin, window);
			}

			g_free (plugin_name);
		}
	}

	g_dir_close (dir);

	return list;
}

GList *
planner_plugin_loader_load (PlannerWindow *window)
{
	gchar  *path;
	GList  *list;

	path = mrp_paths_get_plugin_dir (NULL);
	list = mpl_load_dir (path, window);
	g_free (path);

	return list;
}

void
planner_plugin_loader_unload (GList *plugins)
{
	g_list_foreach (plugins, (GFunc) g_object_unref, NULL);
}
