/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2004 Imendio AB
 * Copyright (C) 2002 CodeFactory AB
 * Copyright (C) 2002 Richard Hult <richard@imendio.com>
 * Copyright (C) 2002 Mikael Hallendal <micke@imendio.com>
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

#include <config.h>
#include <string.h>
#include <glib/gi18n.h>
#include "libplanner/mrp-paths.h"
#include "mrp-file-module.h"

static MrpFileModule *
file_module_load (const gchar *file)
{
	MrpFileModule *module;

	module = mrp_file_module_new ();

	module->handle = g_module_open (file, G_MODULE_BIND_LAZY);

	if (module->handle == NULL) {
		g_warning ("Could not open file module '%s'\n",
			   g_module_error ());

		return NULL;
	}

	g_module_symbol (module->handle, "init", (gpointer)&module->init);

	return module;
}

GList *
mrp_file_module_load_all (MrpApplication *app)
{
	GDir*          dir;
	const gchar   *name;
	MrpFileModule *module;
	gchar         *path;
	GList         *modules = NULL;

	path = mrp_paths_get_file_modules_dir (NULL);

	dir = g_dir_open (path, 0, NULL);

	if (dir == NULL) {
		g_free (path);
		return NULL;
	}

	while ((name = g_dir_read_name (dir)) != NULL) {
		if (g_str_has_suffix (name, G_MODULE_SUFFIX)) {
			gchar *plugin;

			plugin = g_build_filename (path,
						   name,
						   NULL);

			module = file_module_load (plugin);
			if (module) {
				mrp_file_module_init (module, app);
				modules = g_list_prepend (modules, module);
			}

			g_free (plugin);
		}
	}

	g_free (path);
	g_dir_close (dir);

	return modules;
}

MrpFileModule *
mrp_file_module_new (void)
{
        return g_new0 (MrpFileModule, 1);
}

void
mrp_file_module_init (MrpFileModule *plugin, MrpApplication *app)
{
        g_return_if_fail (plugin != NULL);
        g_return_if_fail (MRP_IS_APPLICATION (app));

        plugin->app = app;

        if (plugin->init) {
                plugin->init (plugin, app);
        }
}

gboolean
mrp_file_reader_read_string (MrpFileReader  *reader,
			     const gchar    *str,
			     MrpProject     *project,
			     GError        **error)
{
	if (reader->read_string) {
		return reader->read_string (reader, str, project, error);
	}

	g_set_error (error,
		     MRP_ERROR,
		     MRP_ERROR_FAILED,
		     _("This format does not support reading"));

	return FALSE;
}

const gchar *
mrp_file_writer_get_string (MrpFileWriter *writer)
{
        g_return_val_if_fail (writer != NULL, NULL);

	if (writer->get_mime_type) {
		return writer->get_mime_type (writer);
	}

	return NULL;
}

const gchar *
mrp_file_writer_get_mime_type (MrpFileWriter *writer)
{
        g_return_val_if_fail (writer != NULL, NULL);

	if (writer->get_string) {
		return writer->get_string (writer);
	}

	return NULL;
}

gboolean
mrp_file_writer_write (MrpFileWriter    *writer,
		       MrpProject       *project,
		       const gchar      *uri,
		       gboolean          force,
		       GError          **error)
{
        g_return_val_if_fail (writer != NULL, FALSE);

        if (writer->write) {
		return writer->write (writer, project, uri, force, error);
        }

        return FALSE;
}
