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
 * Path generation functions
 */

#ifndef __MRP_PATHS_H__
#define __MRP_PATHS_H__

#include <glib.h>

gchar *mrp_paths_get_plugin_dir        (const gchar *filename);
gchar *mrp_paths_get_storagemodule_dir (const gchar *filename);
gchar *mrp_paths_get_file_modules_dir  (const gchar *filename);
gchar *mrp_paths_get_glade_dir         (const gchar *filename);
gchar *mrp_paths_get_image_dir         (const gchar *filename);
gchar *mrp_paths_get_dtd_dir           (const gchar *filename);
gchar *mrp_paths_get_stylesheet_dir    (const gchar *filename);
gchar *mrp_paths_get_ui_dir            (const gchar *filename);
gchar *mrp_paths_get_sql_dir           (void);
gchar *mrp_paths_get_locale_dir        (void);

#endif /* __MRP_PATHS_H__ */
