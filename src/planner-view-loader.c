/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2003-2004 Imendio AB
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
#include <glib/gi18n.h>
#include "planner-view.h"
#include "planner-view-loader.h"

static PlannerView *
mvl_load (const gchar *file)
{
	PlannerView *view;

	view = g_object_new (PLANNER_TYPE_VIEW, NULL);
	
	view->handle = g_module_open (file, 0);
	
	if (view->handle == NULL) {
		g_warning (_("Could not open view plugin file '%s'\n"),
			   g_module_error ());
		
		return NULL;
	}
	
	g_module_symbol (view->handle, "init", (gpointer*)&view->init);
	g_module_symbol (view->handle, "activate", (gpointer*)&view->activate);
	g_module_symbol (view->handle, "deactivate", (gpointer*)&view->deactivate);
	g_module_symbol (view->handle, "get_label", (gpointer*)&view->get_label);
	g_module_symbol (view->handle, "get_menu_label", (gpointer*)&view->get_menu_label);
	g_module_symbol (view->handle, "get_icon", (gpointer*)&view->get_icon);
	g_module_symbol (view->handle, "get_name", (gpointer*)&view->get_name);
	g_module_symbol (view->handle, "get_widget", (gpointer*)&view->get_widget);
	g_module_symbol (view->handle, "print_init", (gpointer*)&view->print_init);
	g_module_symbol (view->handle, "print", (gpointer*)&view->print);
	g_module_symbol (view->handle, "print_get_n_pages", (gpointer*)&view->print_get_n_pages);
	g_module_symbol (view->handle, "print_cleanup", (gpointer*)&view->print_cleanup);

	return view;
}

/* Note: this is a hackish solution to the fact that we don't get the views in a
 * consistent order.
 */
static gint
get_order (PlannerView *view)
{
	if (strcmp (planner_view_get_name (view), "gantt_view") == 0) {
		return 1;
	}
	else if (strcmp (planner_view_get_name (view), "task_view") == 0) {
		return 2;
	}
	else if (strcmp (planner_view_get_name (view), "resource_view") == 0) {
		return 3;
	}
	else if (strcmp (planner_view_get_name (view), "resource_usage_view") == 0) {
		return 4;
	}
	else {
		return 5;
	}
}

static gint
sort_compare_func (gconstpointer a,
		   gconstpointer b)
{
	gint order_a, order_b;

	order_a = get_order ((PlannerView *) a);
	order_b = get_order ((PlannerView *) b);

	if (order_a < order_b) {
		return -1;
	}
	else if (order_a > order_b) {
		return 1;
	} else {
		return 0;
	}
}

static GList *
mvl_load_dir (const gchar *path, PlannerWindow *window)
{
	GDir*        dir;
	const gchar *name;
	PlannerView *view;
	GList       *list = NULL;

	dir = g_dir_open (path, 0, NULL);
	if (dir == NULL) {		
		return NULL;
	}

	while ((name = g_dir_read_name (dir)) != NULL) {
		if (g_str_has_suffix (name, G_MODULE_SUFFIX)) {
			gchar *plugin;
			
			plugin = g_build_filename (path,
						   name,
						   NULL);
			view = mvl_load (plugin);
			if (view) {
				list = g_list_append (list, view);
				planner_view_init (view, window);
			}
			
			g_free (plugin);
		}
	}

	g_dir_close (dir);

	return g_list_sort (list, sort_compare_func);
}

GList *
planner_view_loader_load (PlannerWindow *window)
{
	return mvl_load_dir (VIEWDIR, window);
}
