/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2005 Imendio AB
 * Copyright (C) 2005 Jani Tiainen
 * Copyright (C) 2008 Maurice van der Pot
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

/*
 * Path generation
 * Gnome specific versions
 */

#include <config.h>
#include "mrp-paths.h"

static const gchar *default_plugin_dir        = PLUGINDIR;
static const gchar *default_storagemodule_dir = STORAGEMODULEDIR;
static const gchar *default_file_modules_dir  = FILEMODULESDIR;
static const gchar *default_data_dir          = DATADIR;

static const gchar *locale_dir                = GNOMELOCALEDIR;

gchar *
mrp_paths_get_glade_dir (const gchar *filename)
{
	const gchar *env_data_dir = g_getenv ("PLANNER_DATADIR");
	return g_build_filename (env_data_dir ? env_data_dir
					      : default_data_dir,
				 "glade",
				 filename,
				 NULL);
}

gchar *
mrp_paths_get_image_dir (const gchar *filename)
{
	const gchar *env_data_dir = g_getenv ("PLANNER_DATADIR");
	return g_build_filename (env_data_dir ? env_data_dir
					      : default_data_dir,
				 "images",
				 filename,
				 NULL);
}

gchar *
mrp_paths_get_plugin_dir (const gchar *filename)
{
	const gchar *env_plugin_dir = g_getenv ("PLANNER_PLUGINDIR");
	return g_build_filename (env_plugin_dir ? env_plugin_dir
						: default_plugin_dir,
				 filename,
				 NULL);
}

gchar *
mrp_paths_get_dtd_dir (const gchar *filename)
{
	const gchar *env_data_dir = g_getenv ("PLANNER_DATADIR");
	return g_build_filename (env_data_dir ? env_data_dir
					      : default_data_dir,
				 "dtd",
				 filename,
				 NULL);
}

gchar *
mrp_paths_get_stylesheet_dir (const gchar *filename)
{
	const gchar *env_data_dir = g_getenv ("PLANNER_DATADIR");
	return g_build_filename (env_data_dir ? env_data_dir
					      : default_data_dir,
				 "stylesheets",
				 filename,
				 NULL);
}

gchar *
mrp_paths_get_storagemodule_dir (const gchar *filename)
{
	const gchar *env_storagemodule_dir = g_getenv ("PLANNER_STORAGEMODULEDIR");
	return g_build_filename (env_storagemodule_dir
					? env_storagemodule_dir
					: default_storagemodule_dir,
				 filename,
				 NULL);
}

gchar *
mrp_paths_get_file_modules_dir (const gchar *filename)
{
	const gchar *env_file_modules_dir = g_getenv ("PLANNER_FILEMODULESDIR");
	return g_build_filename (env_file_modules_dir
					? env_file_modules_dir
					: default_file_modules_dir,
				 filename,
				 NULL);
}

gchar *
mrp_paths_get_ui_dir (const gchar *filename)
{
	const gchar *env_data_dir = g_getenv ("PLANNER_DATADIR");
	return g_build_filename (env_data_dir ? env_data_dir
					      : default_data_dir,
				 "ui",
				 filename,
				 NULL);
}

gchar *
mrp_paths_get_sql_dir ()
{
	const gchar *env_data_dir = g_getenv ("PLANNER_DATADIR");
	return g_build_filename (env_data_dir ? env_data_dir
					      : default_data_dir,
				 "sql",
				 NULL);
}

gchar *
mrp_paths_get_locale_dir ()
{
	return g_strdup (locale_dir);
}
