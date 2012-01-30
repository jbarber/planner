/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2005 Jani Tiainen
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
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

/*
 * Path generation
 * Win32 specific versions
 */

#include "mrp-paths.h"

static gchar *glade_dir 	= NULL;
static gchar *image_dir 	= NULL;
static gchar *plugin_dir 	= NULL;
static gchar *dtd_dir 		= NULL;
static gchar *stylesheet_dir 	= NULL;
static gchar *ui_dir		= NULL;
static gchar *storagemodule_dir = NULL;
static gchar *file_modules_dir 	= NULL;
static gchar *sql_dir           = NULL;
static gchar *locale_dir        = NULL;

#define STORAGEMODULEDIR "lib/planner/storage-modules"
#define FILEMODULEDIR    "lib/planner/file-modules"
#define PLUGINDIR        "lib/planner/plugins"
#define DTDDIR           "share/planner/dtd"
#define STYLESHEETDIR    "share/planner/stylesheets"
#define GLADEDIR         "share/planner/glade"
#define IMAGEDIR         "share/pixmaps/planner"
#define UIDIR            "share/planner/ui"
#define SQLDIR           "share/planner/sql"
#define GNOMELOCALEDIR   "share/locale"

gchar *
mrp_paths_get_glade_dir (const gchar *filename)
{
	if (!glade_dir) {
		gchar *module_dir;
		module_dir = g_win32_get_package_installation_directory_of_module (NULL);
		glade_dir = g_build_filename (module_dir, GLADEDIR);
		g_free (module_dir);
	}

	return g_build_filename (glade_dir, filename, NULL);
}

gchar *
mrp_paths_get_image_dir (const gchar *filename)
{
	if (!image_dir) {
		gchar *module_dir;
		module_dir = g_win32_get_package_installation_directory_of_module (NULL);
		image_dir = g_build_filename (module_dir, IMAGEDIR);
		g_free (module_dir);
	}

	return g_build_filename (image_dir, filename, NULL);
}

gchar *
mrp_paths_get_plugin_dir (const gchar *filename)
{
	if (!plugin_dir) {
		gchar *module_dir;
		module_dir = g_win32_get_package_installation_directory_of_module (NULL);
		plugin_dir = g_build_filename (module_dir, PLUGINDIR);
		g_free (module_dir);
	}

	return g_build_filename (plugin_dir, filename, NULL);
}

gchar *
mrp_paths_get_dtd_dir (const gchar *filename)
{
	if (!dtd_dir) {
		gchar *module_dir;
		module_dir = g_win32_get_package_installation_directory_of_module (NULL);
		dtd_dir = g_build_filename (module_dir, DTDDIR);
		g_free (module_dir);
	}

	return g_build_filename (dtd_dir, filename, NULL);
}

gchar *
mrp_paths_get_stylesheet_dir (const gchar *filename)
{
	if (!stylesheet_dir) {
		gchar *module_dir;
		module_dir = g_win32_get_package_installation_directory_of_module (NULL);
		stylesheet_dir = g_build_filename (module_dir, STYLESHEETDIR);
		g_free (module_dir);
	}

	return g_build_filename (stylesheet_dir, filename, NULL);
}

gchar *
mrp_paths_get_ui_dir (const gchar *filename)
{
	if (!ui_dir) {
		gchar *module_dir;
		module_dir = g_win32_get_package_installation_directory_of_module (NULL);
		ui_dir = g_build_filename (module_dir, UIDIR);
		g_free (module_dir);
	}

	return g_build_filename (ui_dir, filename, NULL);
}

gchar *
mrp_paths_get_storagemodule_dir (const gchar *filename)
{
	if (!storagemodule_dir) {
		gchar *module_dir;
		module_dir = g_win32_get_package_installation_directory_of_module (NULL);
		storagemodule_dir = g_build_filename (module_dir, STORAGEMODULEDIR);
		g_free (module_dir);
	}

	return g_build_filename (storagemodule_dir, filename, NULL);
}

gchar *
mrp_paths_get_file_modules_dir (const gchar *filename)
{
	if (!file_modules_dir) {
		gchar *module_dir;
		module_dir = g_win32_get_package_installation_directory_of_module (NULL);
		file_modules_dir = g_build_filename (module_dir, FILEMODULEDIR);
		g_free (module_dir);
	}

	return g_build_filename (file_modules_dir, filename, NULL);
}

gchar *
mrp_paths_get_sql_dir (void)
{
	if (!sql_dir) {
		gchar *module_dir;
		module_dir = g_win32_get_package_installation_directory_of_module (NULL);
		sql_dir = g_build_filename (module_dir, SQLDIR);
		g_free (module_dir);
	}

	return sql_dir;
}

gchar *
mrp_paths_get_locale_dir (void)
{
	if (!locale_dir) {
		gchar *module_dir;
		module_dir = g_win32_get_package_installation_directory_of_module (NULL);
		locale_dir = g_build_filename (module_dir, GNOMELOCALEDIR);
		g_free (module_dir);
	}

	return locale_dir;
}
