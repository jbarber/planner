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
#include <libgnome/gnome-i18n.h>
#include "planner-plugin.h"
#include "planner-plugin-loader.h"

static MgPlugin *
mpl_load (const gchar *file)
{
	MgPlugin *plugin;

	plugin = g_object_new (MG_TYPE_PLUGIN, NULL);
	
	plugin->handle = g_module_open (file, G_MODULE_BIND_LAZY);
	
	if (plugin->handle == NULL) {
		g_warning (_("Could not open plugin file '%s'\n"),
			   g_module_error ());
		
		return NULL;
	}
	
	g_module_symbol (plugin->handle, "plugin_init", (gpointer*)&plugin->init);
	g_module_symbol (plugin->handle, "plugin_exit", (gpointer*)&plugin->exit);

	return plugin;
}

static GList *
mpl_load_dir (const gchar *path, MgMainWindow *window)
{
	GDir*        dir;
	const gchar *name;
	MgPlugin    *plugin;
	GList       *list = NULL;

	dir = g_dir_open (path, 0, NULL);
	if (dir == NULL) {		
		return NULL;
	}
	
	while ((name = g_dir_read_name (dir)) != NULL) {
		if (strncmp (name + strlen (name) - 3, ".so", 3) == 0) {
			gchar *plugin_name =  g_build_path (G_DIR_SEPARATOR_S,
							    path,
							    name,
							    NULL);
			plugin = mpl_load (plugin_name);
			if (plugin) {
				list = g_list_append (list, plugin);

				planner_plugin_init (plugin, window);
			}
			
			g_free (plugin_name);
		}
	}

	g_dir_close (dir);

	return list;
}

GList *
planner_plugin_loader_load (MgMainWindow *window)
{
	return mpl_load_dir (MRP_PLUGINDIR, window);
}
