/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2003 Imendio AB
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
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include "libplanner/mrp-paths.h"
#include "planner-application.h"
#include "planner-window.h"

static PlannerApplication *application;

/* Command line options */
static gchar *geometry = NULL;
static gchar **args_remaining = NULL;

static GOptionEntry options[] = {
		{ "geometry", 'g', 0, G_OPTION_ARG_STRING, &geometry, N_("Create the initial window with the given geometry."), N_("GEOMETRY")},
		{ G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_FILENAME_ARRAY, &args_remaining, NULL, N_("FILES") },
		{ NULL }
	};

int
main (int argc, char **argv)
{
        GtkWidget       *main_window;
	GError          *error = NULL;
	gchar           *filename;
	gchar           *locale_dir;
	gint             i;

	locale_dir = mrp_paths_get_locale_dir ();
	bindtextdomain (GETTEXT_PACKAGE, locale_dir);
	g_free(locale_dir);
        bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

	g_set_application_name ("Planner");

	 /* Initialize GTK+ program */
	if (!gtk_init_with_args (&argc, &argv,
	                         _("- Planner project management"),
	                         options,
	                         GETTEXT_PACKAGE,
	                         &error)) {
		g_printerr (_("%s\nRun '%s --help' to see a full list of available command line options.\n"),
		            error->message, argv[0]);
		g_error_free (error);
		return 1;
	}

	filename = mrp_paths_get_image_dir ("gnome-planner.png");
	gtk_window_set_default_icon_from_file (filename, NULL);
	g_free (filename);

#ifdef WIN32
	gchar *module_dir;
	module_dir = g_win32_get_package_installation_directory_of_module (NULL);
	filename = g_build_filename (module_dir, "share/icons");
	g_free (module_dir);
	gtk_icon_theme_append_search_path (gtk_icon_theme_get_default(),
					   filename);
	g_free (filename);
#endif

	application = planner_application_new ();
	main_window = planner_application_new_window (application);

	if ((geometry) && !gtk_window_parse_geometry (GTK_WINDOW (main_window), geometry))
		g_warning(_("Invalid geometry string \"%s\"\n"), geometry);

	gtk_widget_show_all (main_window);

	if (argc > 1) {
		i = 1;
		while (argv[i]) {
			if (g_str_has_prefix (argv[i], "file:")) {
				planner_window_open_in_existing_or_new (
					PLANNER_WINDOW (main_window), argv[i], FALSE);
			} else {
				gchar *uri;

				if (!g_path_is_absolute (argv[i])) {
					/* Relative path. */
					gchar *cwd, *tmp;

					cwd = g_get_current_dir ();
					tmp = g_build_filename (cwd, argv[i], NULL);
					uri = g_filename_to_uri (tmp, NULL, NULL);
					g_free (tmp);
					g_free (cwd);
				} else {
					uri = g_filename_to_uri (argv[i], NULL, NULL);
				}

				if (uri) {
					planner_window_open_in_existing_or_new (
						PLANNER_WINDOW (main_window), uri, FALSE);
					g_free (uri);
				}
			}

			i++;
		}
	}

        gtk_main ();

	g_object_unref (application);

        return 0;
}
