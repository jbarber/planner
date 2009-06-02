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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
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
		glade_dir = g_win32_get_package_installation_subdirectory (NULL, NULL, GLADEDIR);
	}

	return g_build_filename (glade_dir, filename, NULL);
}

gchar *
mrp_paths_get_image_dir (const gchar *filename)
{
	if (!image_dir) {
		image_dir = g_win32_get_package_installation_subdirectory (NULL, NULL, IMAGEDIR);
	}

	return g_build_filename (image_dir, filename, NULL);
}

gchar *
mrp_paths_get_plugin_dir (const gchar *filename)
{
	if (!plugin_dir) {
		plugin_dir = g_win32_get_package_installation_subdirectory (NULL, NULL, PLUGINDIR);
	}

	return g_build_filename (plugin_dir, filename, NULL);
}

gchar *
mrp_paths_get_dtd_dir (const gchar *filename)
{
	if (!dtd_dir) {
		dtd_dir = g_win32_get_package_installation_subdirectory (NULL, NULL, DTDDIR);
	}

	return g_build_filename (dtd_dir, filename, NULL);
}

gchar *
mrp_paths_get_stylesheet_dir (const gchar *filename)
{
	if (!stylesheet_dir) {
		stylesheet_dir = g_win32_get_package_installation_subdirectory (NULL, NULL, STYLESHEETDIR);
	}

	return g_build_filename (stylesheet_dir, filename, NULL);
}

gchar *
mrp_paths_get_ui_dir (const gchar *filename)
{
	if (!ui_dir) {
		ui_dir = g_win32_get_package_installation_subdirectory (NULL, NULL, UIDIR);
	}

	return g_build_filename (ui_dir, filename, NULL);
}

gchar *
mrp_paths_get_storagemodule_dir (const gchar *filename)
{
	if (!storagemodule_dir) {
		storagemodule_dir = g_win32_get_package_installation_subdirectory (NULL, NULL, STORAGEMODULEDIR);
	}

	return g_build_filename (storagemodule_dir, filename, NULL);
}

gchar *
mrp_paths_get_file_modules_dir (const gchar *filename)
{
	if (!file_modules_dir) {
		file_modules_dir = g_win32_get_package_installation_subdirectory (NULL, NULL, FILEMODULEDIR);
	}

	return g_build_filename (file_modules_dir, filename, NULL);
}

gchar *
mrp_paths_get_sql_dir (void)
{
	if (!sql_dir) {
		sql_dir = g_win32_get_package_installation_subdirectory (NULL, NULL, SQLDIR);
	}

	return sql_dir;
}

gchar *
mrp_paths_get_locale_dir (void)
{
	if (!locale_dir) {
		locale_dir = g_win32_get_package_installation_subdirectory (NULL, NULL, GNOMELOCALEDIR);
	}

	return locale_dir;
}
