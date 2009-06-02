/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
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
#include <glib/gi18n.h>
#include <glade/glade.h>
#include <gtk/gtk.h>
#include <libplanner/mrp-day.h>
#include <libplanner/mrp-object.h>
#include <libplanner/mrp-project.h>
#include "libplanner/mrp-paths.h"
#include "planner-calendar.h"
#include "planner-calendar-selector.h"

enum {
	COL_CALENDAR,
	COL_NAME,
	NUM_COLS
};

typedef struct {
	PlannerWindow  *main_window;
	MrpProject    *project;

	GtkWidget     *selector;
	GtkWidget     *tree_view;
	GtkWidget     *ok_button;
	GtkWidget     *cancel_button;

	GtkWidget     *this_label;
} DialogData;

typedef struct {
	DialogData    *data;
	MrpCalendar   *calendar;
	gboolean       found;
	GtkTreeIter    found_iter;
} FindCalendarData;


#define SELECTOR_GET_DATA(d) g_object_get_data ((GObject*)d, "data")

static GtkTreeModel *cal_selector_create_model          (MrpProject       *project,
							 GtkTreeView      *tree_view);
static void          cal_selector_setup_tree_view       (GtkTreeView      *tree_view,
							 MrpProject       *project);
static MrpCalendar * cal_selector_get_selected_calendar (GtkTreeView      *tree_view);
static void          cal_selector_selection_changed_cb  (GtkTreeSelection *selection,
							 DialogData       *data);

GtkWidget *
planner_calendar_selector_new (PlannerWindow *window,
			       const gchar  *caption)
{
	DialogData       *data;
	GladeXML         *glade;
	GtkWidget        *selector;
	GtkWidget        *w;
	GtkTreeSelection *selection;
	gchar		 *filename;

	g_return_val_if_fail (PLANNER_IS_WINDOW (window), NULL);

	filename = mrp_paths_get_glade_dir ("calendar-dialog.glade");
	glade = glade_xml_new (filename,
			       "calendar_selector",
			       GETTEXT_PACKAGE);
	g_free (filename);
	if (!glade) {
		g_warning ("Could not create calendar selector.");
		return NULL;
	}

	selector = glade_xml_get_widget (glade, "calendar_selector");

	data = g_new0 (DialogData, 1);

	data->project = planner_window_get_project (window);
	data->main_window = window;
	data->selector = selector;

	data->tree_view = glade_xml_get_widget (glade, "treeview");

	w = glade_xml_get_widget (glade, "caption_label");
	gtk_label_set_text (GTK_LABEL (w), caption);

	data->ok_button = glade_xml_get_widget (glade, "ok_button");
	data->cancel_button = glade_xml_get_widget (glade, "cancel_button");

	g_object_set_data_full (G_OBJECT (selector),
				"data", data,
				g_free);

	cal_selector_setup_tree_view (GTK_TREE_VIEW (data->tree_view),
				      data->project);

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (data->tree_view));
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_BROWSE);

	g_signal_connect (selection,
			  "changed",
			  G_CALLBACK (cal_selector_selection_changed_cb),
			  data);

	return selector;
}

MrpCalendar *
planner_calendar_selector_get_calendar (GtkWidget *dialog)
{
	DialogData  *data;
	MrpCalendar *calendar;

	g_return_val_if_fail (GTK_IS_WIDGET (dialog), NULL);

	data = SELECTOR_GET_DATA (dialog);

	calendar = cal_selector_get_selected_calendar (GTK_TREE_VIEW (data->tree_view));

	return calendar;
}

static MrpCalendar *
cal_selector_get_selected_calendar (GtkTreeView *tree_view)
{
	GtkTreeSelection *selection;
	GtkTreeModel     *model;
	GtkTreeIter       iter;
	MrpCalendar      *calendar;

	selection = gtk_tree_view_get_selection (tree_view);
	if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
		gtk_tree_model_get (model, &iter,
				    COL_CALENDAR, &calendar,
				    -1);

		return calendar;
	}

	return NULL;
}

static void
cal_selector_selection_changed_cb (GtkTreeSelection *selection,
				   DialogData       *data)
{
	MrpCalendar *calendar;

	calendar = cal_selector_get_selected_calendar (GTK_TREE_VIEW (data->tree_view));

	gtk_widget_set_sensitive (data->ok_button, calendar != NULL);
}

static void
cal_selector_setup_tree_view (GtkTreeView *tree_view,
			      MrpProject  *project)
{
	GtkTreeModel      *model;
	GtkCellRenderer   *cell;
	GtkTreeViewColumn *col;

	model = cal_selector_create_model (project, tree_view);

	gtk_tree_view_set_model (tree_view, model);

	cell = gtk_cell_renderer_text_new ();
	col = gtk_tree_view_column_new_with_attributes (
		NULL,
		cell,
		"text", COL_NAME,
		NULL);
	gtk_tree_view_append_column (tree_view, col);

	gtk_tree_view_expand_all (tree_view);
}

static void
cal_selector_build_tree (GtkTreeStore *store,
			 GtkTreeIter  *parent,
			 MrpCalendar  *calendar)
{
	GtkTreeIter  iter;
	const gchar *name;
	GList       *children, *l;

	name = mrp_calendar_get_name (calendar);

	gtk_tree_store_append (store, &iter, parent);
	gtk_tree_store_set (store,
			    &iter,
			    COL_NAME, name,
			    COL_CALENDAR, calendar,
			    -1);

	children = mrp_calendar_get_children (calendar);
	for (l = children; l; l = l->next) {
		cal_selector_build_tree (store, &iter, l->data);
	}
}

static void
cal_selector_tree_changed (MrpProject  *project,
			   MrpCalendar *root,
			   GtkTreeView *tree_view)
{
	GtkTreeStore *store;
	GList        *children, *l;

	store = GTK_TREE_STORE (gtk_tree_view_get_model (tree_view));

	gtk_tree_store_clear (store);

	children = mrp_calendar_get_children (root);
	for (l = children; l; l = l->next) {
		cal_selector_build_tree (store, NULL, l->data);
	}

	gtk_tree_view_expand_all (tree_view);
}

static GtkTreeModel *
cal_selector_create_model (MrpProject  *project,
			   GtkTreeView *tree_view)
{
	GtkTreeStore *store;
	MrpCalendar  *root;
	GList        *children, *l;

	root = mrp_project_get_root_calendar (project);

	store = gtk_tree_store_new (NUM_COLS,
				    G_TYPE_OBJECT,
				    G_TYPE_STRING);

	children = mrp_calendar_get_children (root);
	for (l = children; l; l = l->next) {
		cal_selector_build_tree (store, NULL, l->data);
	}

	g_signal_connect_object (project,
				 "calendar_tree_changed",
				 G_CALLBACK (cal_selector_tree_changed),
				 tree_view,
				 0);

	return GTK_TREE_MODEL (store);
}
