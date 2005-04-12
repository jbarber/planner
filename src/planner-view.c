/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2003, 2005 Imendio AB
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
#include <time.h>
#include <glib.h>
#include "planner-view.h"
#include "planner-conf.h"

static void mv_init       (PlannerView      *view);
static void mv_class_init (PlannerViewClass *class);


static GObjectClass *parent_class;


GType
planner_view_get_type (void)
{
	static GType type = 0;

	if (!type) {
		static const GTypeInfo info = {
			sizeof (PlannerViewClass),
			NULL,		/* base_init */
			NULL,		/* base_finalize */
			(GClassInitFunc) mv_class_init,
			NULL,		/* class_finalize */
			NULL,		/* class_data */
			sizeof (PlannerView),
			0,
			(GInstanceInitFunc) mv_init
		};

		type = g_type_register_static (G_TYPE_OBJECT,
					       "PlannerView", &info, 0);
	}

	return type;
}

static void
mv_finalize (GObject *object)
{
	(* parent_class->finalize) (object);
}

static void
mv_class_init (PlannerViewClass *klass)
{
	GObjectClass *object_class;

	object_class = (GObjectClass *) klass;

	parent_class = g_type_class_peek_parent (klass);
	
	object_class->finalize = mv_finalize;
}

static void
mv_init (PlannerView *view)
{

}

const gchar *
planner_view_get_label (PlannerView *view)
{
	g_return_val_if_fail (PLANNER_IS_VIEW (view), NULL);

	if (view->get_label) {
		return view->get_label (view);
	}

	return NULL;
}

const gchar *
planner_view_get_menu_label (PlannerView *view)
{
	g_return_val_if_fail (PLANNER_IS_VIEW (view), NULL);

	if (view->get_menu_label) {
		return view->get_menu_label (view);
	}

	return NULL;
}

const gchar *
planner_view_get_icon (PlannerView *view)
{
	g_return_val_if_fail (PLANNER_IS_VIEW (view), NULL);

	if (view->get_icon) {
		return view->get_icon (view);
	}

	return NULL;
}

const gchar *
planner_view_get_name (PlannerView *view)
{
	g_return_val_if_fail (PLANNER_IS_VIEW (view), NULL);

	if (view->get_name) {
		return view->get_name (view);
	}

	return NULL;
}

GtkWidget *
planner_view_get_widget (PlannerView *view)
{
	g_return_val_if_fail (PLANNER_IS_VIEW (view), NULL);

	if (view->get_widget) {
		return view->get_widget (view);
	}

	return NULL;
}

void
planner_view_init (PlannerView   *view,
		   PlannerWindow *main_window)
{
	g_return_if_fail (PLANNER_IS_VIEW (view));

	view->main_window = main_window;

	if (view->init) {
		view->init (view, main_window);
	}
}

void
planner_view_activate (PlannerView *view)
{
	g_return_if_fail (PLANNER_IS_VIEW (view));

	view->activated = TRUE;
	
	if (view->activate) {
		view->activate (view);
	}
}

void
planner_view_deactivate (PlannerView *view)
{
	g_return_if_fail (PLANNER_IS_VIEW (view));

	view->activated = FALSE;

	if (view->deactivate) {
		view->deactivate (view);
	}
}

void
planner_view_print_init (PlannerView     *view,
		    PlannerPrintJob *job)
{
	g_return_if_fail (PLANNER_IS_VIEW (view));
	g_return_if_fail (PLANNER_IS_PRINT_JOB (job));

	if (view->print_init) {
		view->print_init (view, job);
	}
}

gint
planner_view_print_get_n_pages (PlannerView *view)
{
	g_return_val_if_fail (PLANNER_IS_VIEW (view), 0);
	
	if (view->print_get_n_pages) {
		return view->print_get_n_pages (view);
	}
	
	return 0;
}

void
planner_view_print (PlannerView *view)
{
	g_return_if_fail (PLANNER_IS_VIEW (view));

	if (view->print) {
		view->print (view);
	}
}

void
planner_view_print_cleanup (PlannerView *view)
{
	g_return_if_fail (PLANNER_IS_VIEW (view));

	if (view->print_cleanup) {
		view->print_cleanup (view);
	}
}

static gint
column_sort_func (gconstpointer a,
		  gconstpointer b)
{
	gint   order_a, order_b;

	order_a = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (a), "order"));
	order_b = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (b), "order"));

	if (order_a < order_b) {
		return -1;
	}
	else if (order_a > order_b) {
		return 1;
	} else {
		return 0;
	}
}

void
planner_view_column_load_helper (PlannerView *view,
				 GtkTreeView *tree)
{
	GList             *columns, *l;
	GtkTreeViewColumn *column;
	const gchar       *id;
	gchar             *key;
	gboolean           visible, order;
	gint               width;

	columns = gtk_tree_view_get_columns (tree);
	for (l = columns; l; l = l->next) {
		column = l->data;

		if (g_object_get_data (G_OBJECT (column), "custom")) {
			continue;
		}

		id = g_object_get_data (G_OBJECT (column), "id");
		if (!id) {
			continue;
		}

		key = g_strdup_printf ("/%s/columns/%s/visible",
				       planner_view_get_name (view), id);
		visible = planner_conf_get_bool (key, NULL);
		gtk_tree_view_column_set_visible (column, visible);
		g_free (key);

		key = g_strdup_printf ("/%s/columns/%s/order",
				       planner_view_get_name (view), id);
		order = planner_conf_get_int (key, NULL);
		g_object_set_data (G_OBJECT (column), "order", GINT_TO_POINTER (order));
		g_free (key);

		key = g_strdup_printf ("/%s/columns/%s/width",
				       planner_view_get_name (view),
				       id);
		width = planner_conf_get_int (key, NULL);
		if (width > 0) {
			gtk_tree_view_column_set_fixed_width (column, width);
		}
		g_free (key);
	}

	/* Sort the list of columns in the right order. */
	columns = g_list_sort (columns, column_sort_func);

	for (l = columns; l; l = l->next) {
		column = l->data;

		gtk_tree_view_move_column_after (tree,
						 column,
						 l->prev ? l->prev->data : NULL);
	}
		
	g_list_free (columns);
}

void
planner_view_column_save_helper (PlannerView *view,
				 GtkTreeView *tree)
{
	GList             *columns, *l;
	GtkTreeViewColumn *column;
	const gchar       *id;
	gchar             *key;
	gint               i;
	
	columns = gtk_tree_view_get_columns (tree);
	for (i = 0, l = columns; l; i++, l = l->next) {
		column = l->data;

		if (g_object_get_data (G_OBJECT (column), "custom")) {
			continue;
		}

		id = g_object_get_data (G_OBJECT (column), "id");
		if (!id) {
			continue;
		}

		key = g_strdup_printf ("/%s/columns/%s/visible",
				       planner_view_get_name (view), id);
		planner_conf_set_bool (key,
				       gtk_tree_view_column_get_visible (column),
				       NULL);
		g_free (key);

		key = g_strdup_printf ("/%s/columns/%s/order",
				       planner_view_get_name (view), id);
		planner_conf_set_int (key, i, NULL);
		g_free (key);
		
		key = g_strdup_printf ("/%s/columns/%s/width",
				       planner_view_get_name (view), id);
		planner_conf_set_int (key,
				      gtk_tree_view_column_get_width (column),
				      NULL);
		g_free (key);
	}

	g_list_free (columns);
}
