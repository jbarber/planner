/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* Evolution planner - Create source entries in GConf
 *
 * Copyright (C) Alvaro del Castillo <acs@barrapunto.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 *
 * Based in part on create_account from GW
 */

#include <config.h>
#include <gconf/gconf-client.h>
#include <glib.h>
#include <libgnome/gnome-init.h>
#include <libedataserver/e-source-list.h>

static GConfClient *conf_client;
static GMainLoop   *main_loop;
static char        *planner_file;

static void
add_planner_file (const gchar *conf_key,
		  const gchar *uri)
{
	ESourceList  *source_list;
	ESourceGroup *group;
	ESource      *source;
	char         *group_name;

	source_list = e_source_list_new_for_gconf (conf_client, conf_key);

	group_name = g_strdup_printf ("Projects");
	group = e_source_group_new (group_name, "planner://");
	e_source_list_add_group (source_list, group, -1);

	g_free (group_name);
	group_name = g_strdup_printf ("%s", uri);
	source = e_source_new ("Calendar", group_name);
	e_source_group_add_source (group, source, -1);

	e_source_list_sync (source_list, NULL);

	g_free (group_name);
	g_object_unref (source);
	g_object_unref (group);
	g_object_unref (source_list);
}

static gboolean
idle_cb (gpointer data)
{
	add_planner_file ("/apps/evolution/tasks/sources", planner_file);

	g_main_loop_quit (main_loop);

	return FALSE;
}

int
main (int argc, char *argv[])
{
	gnome_program_init (PACKAGE, VERSION,
			    LIBGNOME_MODULE,
			    argc, argv,
			    NULL);

	if (argc != 2) {
		g_print ("Usage: %s planner_file \n", argv[0]);
		return -1;
	}

	planner_file = argv[1];

	conf_client = gconf_client_get_default ();

	main_loop = g_main_loop_new (NULL, TRUE);
	g_idle_add ((GSourceFunc) idle_cb, NULL);
	g_main_loop_run (main_loop);

	/* terminate */
	g_object_unref (conf_client);
	g_main_loop_unref (main_loop);

	return 0;
}
