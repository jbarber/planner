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
#include <signal.h>
#include <string.h>
#include <popt.h>
#include <gtk/gtkwidget.h>
#include <gtk/gtkmain.h>
#include <glib/gi18n.h>
#include <libgnome/gnome-program.h>
#include <libgnomeui/gnome-ui-init.h>
#include <libgnomeui/gnome-window-icon.h>
#include "planner-application.h"
#include "planner-window.h"

static PlannerApplication *application;


int
main (int argc, char **argv)
{
        GtkWidget          *main_window;
        GnomeProgram       *program;
	gchar              *geometry;
	poptContext         popt_context;
	const gchar       **args;
	gint                i;
	struct poptOption   options[] = {
		{ "geometry", 'g', POPT_ARG_STRING, &geometry, 0,
		  N_("Create the initial window with the given geometry."), N_("GEOMETRY") },
		{ NULL, '\0', 0, NULL, 0, NULL, NULL }
	};

	bindtextdomain (GETTEXT_PACKAGE, GNOMELOCALEDIR);  
        bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

	geometry = NULL;

	program = gnome_program_init (PACKAGE, VERSION,
				      LIBGNOMEUI_MODULE,
				      argc, argv,
				      GNOME_PROGRAM_STANDARD_PROPERTIES,
				      GNOME_PARAM_POPT_TABLE, options,
				      GNOME_PARAM_HUMAN_READABLE_NAME, "Planner",
				      NULL);

	g_object_get (program,
		      GNOME_PARAM_POPT_CONTEXT,
		      &popt_context,
		      NULL);

	/* Check for argument consistency. */
	args = poptGetArgs (popt_context);
	if (geometry != NULL && args != NULL && args[0] != NULL && args[1] != NULL) {
		g_warning (_("planner: --geometry cannot be used with more than one file."));
		return (1);
	}

	if (g_getenv ("MRP_G_FATAL_WARNINGS") != NULL) {
		g_log_set_always_fatal (G_LOG_LEVEL_MASK);
	}
	
	application = planner_application_new ();

	main_window = planner_application_new_window (application);
	if (geometry != NULL) {
		gtk_window_parse_geometry (GTK_WINDOW (main_window), geometry);
	}
		
	gtk_widget_show_all (main_window);

	if (args != NULL) {
		i = 0;
		while (args[i]) {
			if (g_str_has_prefix (args[i], "file:")) {
				planner_window_open_in_existing_or_new (
					PLANNER_WINDOW (main_window), args[i], FALSE);
			} else {
				gchar *uri;

				uri = g_filename_to_uri (args[i], NULL, NULL);
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
	
	poptFreeContext (popt_context);

        return 0;
}
