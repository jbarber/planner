/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2005 Imendio AB
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
 * Gnome specific versions
 */

#include <config.h>
#include "mrp-paths.h"

static const gchar *plugin_dir        = PLUGINDIR;
static const gchar *storagemodule_dir = STORAGEMODULEDIR;
static const gchar *file_modules_dir  = FILEMODULEDIR;

static const gchar *glade_dir         = DATADIR "/planner/glade";
static const gchar *image_dir         = DATADIR "/planner/images";
static const gchar *dtd_dir           = DATADIR "/planner/dtd";
static const gchar *stylesheet_dir    = DATADIR "/planner/stylesheets";
static const gchar *ui_dir            = DATADIR "/planner/ui";

gchar *
mrp_paths_get_glade_dir (const gchar *filename)
{
	return g_build_filename (glade_dir, filename, NULL);
}

gchar *
mrp_paths_get_image_dir (const gchar *filename)
{
	return g_build_filename (image_dir, filename, NULL);
}

gchar *
mrp_paths_get_plugin_dir (const gchar *filename)
{	
	return g_build_filename (plugin_dir, filename, NULL);
}

gchar *
mrp_paths_get_dtd_dir (const gchar *filename)
{
	return g_build_filename (dtd_dir, filename, NULL);
}

gchar *
mrp_paths_get_stylesheet_dir (const gchar *filename)
{
	return g_build_filename (stylesheet_dir, filename, NULL);
}

gchar *
mrp_paths_get_storagemodule_dir (const gchar *filename)
{
	return g_build_filename (storagemodule_dir, filename, NULL);
}

gchar *
mrp_paths_get_file_modules_dir (const gchar *filename)
{
	return g_build_filename (file_modules_dir, filename, NULL);
}

gchar *
mrp_paths_get_ui_dir (const gchar *filename)
{
	return g_build_filename (ui_dir, filename, NULL);
}

