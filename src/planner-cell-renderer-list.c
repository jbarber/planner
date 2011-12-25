/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2001-2002 CodeFactory AB
 * Copyright (C) 2001-2002 Richard Hult <richard@imendio.com>
 * Copyright (C) 2001-2002 Mikael Hallendal <micke@imendio.com>
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
#include <stdlib.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include "planner-marshal.h"
#include "planner-cell-renderer-list.h"
#include "planner-popup-entry.h"


static void     mcrl_init                 (PlannerCellRendererList      *list);
static void     mcrl_class_init           (PlannerCellRendererListClass *class);
static void     mcrl_show_popup           (PlannerCellRendererPopup     *cell,
					   const gchar             *path,
					   gint                     x1,
					   gint                     y1,
					   gint                     x2,
					   gint                     y2);
static void     mcrl_hide_popup           (PlannerCellRendererPopup     *cell);
static void     mcrl_selection_changed_cb (GtkTreeSelection        *selection,
					   PlannerCellRendererList      *cell);

static void     mcrl_select_index         (PlannerCellRendererList      *cell,
					   gint                     index);
static gboolean mcrl_button_press_event   (GtkWidget               *widget,
					   GdkEventButton          *event,
					   PlannerCellRendererList      *list);


static PlannerCellRendererPopupClass *parent_class;


GType
planner_cell_renderer_list_get_type (void)
{
	static GType type = 0;

	if (!type) {
		static const GTypeInfo info = {
			sizeof (PlannerCellRendererListClass),
			NULL,		/* base_init */
			NULL,		/* base_finalize */
			(GClassInitFunc) mcrl_class_init,
			NULL,		/* class_finalize */
			NULL,		/* class_data */
			sizeof (PlannerCellRendererList),
			0,              /* n_preallocs */
			(GInstanceInitFunc) mcrl_init,
		};

		type = g_type_register_static (PLANNER_TYPE_CELL_RENDERER_POPUP,
					       "PlannerCellRendererList",
					       &info,
					       0);
	}

	return type;
}

static void
mcrl_init (PlannerCellRendererList *cell)
{
	PlannerCellRendererPopup *popup_cell;
	GtkWidget           *frame;
	GtkCellRenderer     *textcell;
	GtkTreeSelection    *selection;

	popup_cell = PLANNER_CELL_RENDERER_POPUP (cell);

	frame = gtk_frame_new (NULL);
	gtk_container_add (GTK_CONTAINER (popup_cell->popup_window), frame);
	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);
	gtk_widget_show (frame);

	cell->tree_view = gtk_tree_view_new ();
	gtk_container_add (GTK_CONTAINER (frame), cell->tree_view);

	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (cell->tree_view), FALSE);

	textcell = gtk_cell_renderer_text_new ();

	gtk_tree_view_insert_column_with_attributes (
		GTK_TREE_VIEW (cell->tree_view),
		0,
		NULL,
		textcell,
		"text", 0,
		NULL);

	g_signal_connect (cell->tree_view, "button-release-event",
			  G_CALLBACK (mcrl_button_press_event),
			  cell);

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (cell->tree_view));
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_BROWSE);
	g_signal_connect (selection, "changed",
			  G_CALLBACK (mcrl_selection_changed_cb),
			  cell);

	gtk_widget_show (cell->tree_view);

	popup_cell->focus_window = cell->tree_view;
}

static void
mcrl_class_init (PlannerCellRendererListClass *class)
{
	PlannerCellRendererPopupClass *popup_class;

	popup_class = PLANNER_CELL_RENDERER_POPUP_CLASS (class);

	parent_class = PLANNER_CELL_RENDERER_POPUP_CLASS (
		g_type_class_peek_parent (class));

	popup_class->show_popup = mcrl_show_popup;
	popup_class->hide_popup = mcrl_hide_popup;
}

static void
mcrl_show_popup (PlannerCellRendererPopup *cell,
		 const gchar         *path,
		 gint                 x1,
		 gint                 y1,
		 gint                 x2,
		 gint                 y2)
{
	PlannerCellRendererList *list_cell;
	GList              *l;
	GtkTreeModel       *model;
	GtkTreeIter         iter;

	list_cell = PLANNER_CELL_RENDERER_LIST (cell);

	if (list_cell->list == NULL) {
		return;
	}

	model = GTK_TREE_MODEL (gtk_list_store_new (1, G_TYPE_STRING));

	for (l = list_cell->list; l; l = l->next) {
		gtk_list_store_append (GTK_LIST_STORE (model), &iter);
		gtk_list_store_set (GTK_LIST_STORE (model),
				    &iter,
				    0, l->data,
				    -1);
	}

	gtk_tree_view_set_model (GTK_TREE_VIEW (list_cell->tree_view), model);
	g_object_unref (model);

	mcrl_select_index (list_cell, list_cell->selected_index);

	gtk_widget_set_size_request (list_cell->tree_view, x2 - x1, -1);

	if (parent_class->show_popup) {
		parent_class->show_popup (cell,
					  path,
					  x1, y1,
					  x2, y2);
	}
}

static void
mcrl_hide_popup (PlannerCellRendererPopup *cell)
{
	PlannerCellRendererList *list_cell;
	GList              *l;

	list_cell = PLANNER_CELL_RENDERER_LIST (cell);

	if (parent_class->hide_popup) {
		parent_class->hide_popup (cell);
	}

	for (l = list_cell->list; l; l = l->next) {
		g_free (l->data);
	}

	g_list_free (list_cell->list);
	list_cell->list = NULL;
}

GtkCellRenderer *
planner_cell_renderer_list_new (void)
{
	GObject *cell;

	cell = g_object_new (PLANNER_TYPE_CELL_RENDERER_LIST, NULL);

	return GTK_CELL_RENDERER (cell);
}

static void
mcrl_selection_changed_cb (GtkTreeSelection   *selection,
			   PlannerCellRendererList *cell)
{
	gboolean      selected;
	GtkTreeModel *model;
	GtkTreeIter   iter;
	GtkTreePath  *path;
	gint         *indices;

	selected = gtk_tree_selection_get_selected (selection,
						    &model,
						    &iter);

	if (!selected) {
		return;
	}

	path = gtk_tree_model_get_path (model, &iter);
	indices = gtk_tree_path_get_indices (path);

	cell->selected_index = indices[0];

	gtk_tree_path_free (path);
}

static void
mcrl_select_index (PlannerCellRendererList *cell,
		   gint                index)
{
	GtkTreeView      *tree_view;
	GtkTreePath      *path;

	tree_view = GTK_TREE_VIEW (cell->tree_view);

	path = gtk_tree_path_new ();
	gtk_tree_path_append_index (path, index);

	gtk_tree_view_set_cursor (tree_view,
				  path,
				  NULL,
				  FALSE);

	gtk_tree_path_free (path);
}

static gboolean
mcrl_button_press_event (GtkWidget          *widget,
			 GdkEventButton     *event,
			 PlannerCellRendererList *list)
{
	if (event->button == 1) {
		planner_cell_renderer_popup_hide (PLANNER_CELL_RENDERER_POPUP (list));
		return TRUE;
	}

	return FALSE;
}
