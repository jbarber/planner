/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2005 Imendio AB
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
#include <gtk/gtk.h>
#include <glade/glade.h>
#include "libplanner/mrp-paths.h"
#include "planner-column-dialog.h"

typedef struct {
	PlannerWindow *main_window;

	GtkTreeView   *edited_tree;

	GtkWidget     *dialog;
	GtkWidget     *visible_tree;
	GtkWidget     *hidden_tree;
	GtkWidget     *up_button;
	GtkWidget     *down_button;
	GtkWidget     *show_button;
	GtkWidget     *hide_button;
} DialogData;

static void               column_dialog_update_sensitivity  (DialogData        *data);
static GtkTreeViewColumn *column_dialog_get_selected_column (DialogData        *data,
							     GtkTreeView       *tree);
static void               column_dialog_set_selected_column (DialogData        *data,
							     GtkTreeViewColumn *column);
static void               column_dialog_setup_tree          (DialogData        *data,
							     GtkTreeView       *tree);
static void               column_dialog_fill_trees          (DialogData        *data);


static void
column_dialog_update_sensitivity (DialogData *data)
{
	gboolean     hidden_sensitive, visible_sensitive;
	gboolean     up_sensitive, down_sensitive;
	gboolean     hide_sensitive;
	GtkTreePath *path;

	path = NULL;

	hidden_sensitive = FALSE;
	visible_sensitive = FALSE;
	up_sensitive = FALSE;
	down_sensitive = FALSE;
	hide_sensitive = FALSE;

	if (GTK_WIDGET_HAS_FOCUS (data->hidden_tree)) {
		hidden_sensitive = TRUE;
		gtk_tree_view_get_cursor (GTK_TREE_VIEW (data->hidden_tree),
					  &path,
					  NULL);
	}
	else if (GTK_WIDGET_HAS_FOCUS (data->visible_tree)) {
		visible_sensitive = TRUE;
		gtk_tree_view_get_cursor (GTK_TREE_VIEW (data->visible_tree),
					  &path,
					  NULL);

		if (path) {
			GtkTreeModel *model;
			gint         *indices;
			gint          num_rows;

			indices = gtk_tree_path_get_indices (path);
			if (indices[0] > 0) {
				up_sensitive = TRUE;
			}

			model = gtk_tree_view_get_model (GTK_TREE_VIEW (data->visible_tree));
			num_rows = gtk_tree_model_iter_n_children (model, NULL);

			if (indices[0] < num_rows - 1) {
				down_sensitive = TRUE;
			}

			if (num_rows > 1) {
				hide_sensitive = TRUE;
			}
		}
	}

	hidden_sensitive &= path != NULL;
	visible_sensitive &= path != NULL;

	if (path) {
		gtk_tree_path_free (path);
	}

	gtk_widget_set_sensitive (data->up_button, visible_sensitive && up_sensitive);
	gtk_widget_set_sensitive (data->down_button, visible_sensitive && down_sensitive);
	gtk_widget_set_sensitive (data->hide_button, visible_sensitive && hide_sensitive);

	gtk_widget_set_sensitive (data->show_button, hidden_sensitive);
}

static GtkTreeViewColumn *
column_dialog_get_selected_column (DialogData  *data,
				   GtkTreeView *tree)
{
	GtkTreeSelection    *selection;
	GtkTreeIter          iter;
	GtkTreeModel        *model;
	GtkTreeViewColumn   *column;

	selection = gtk_tree_view_get_selection (tree);
	if (!gtk_tree_selection_get_selected (selection, &model, &iter)) {
		return NULL;
	}

	gtk_tree_model_get (model, &iter,
			    0, &column,
			    -1);

	return column;
}

static void
column_dialog_set_selected_column (DialogData        *data,
				   GtkTreeViewColumn *column)
{
	GtkTreeView      *tree;
	GtkTreeModel     *model;
	GtkTreeIter       iter;
	gboolean          found;
	GtkTreeSelection *selection;

	if (gtk_tree_view_column_get_visible (column)) {
		tree = GTK_TREE_VIEW (data->visible_tree);
	} else {
		tree = GTK_TREE_VIEW (data->hidden_tree);
	}

	model = gtk_tree_view_get_model (tree);
	found = FALSE;

	if (gtk_tree_model_get_iter_first (model, &iter)) {
		GtkTreeViewColumn *tmp;

		do {
			gtk_tree_model_get (model, &iter,
					    0, &tmp,
					    -1);

			if (tmp == column) {
				found = TRUE;
				break;
			}
		} while (gtk_tree_model_iter_next (model, &iter));
	}

	if (found) {
		selection = gtk_tree_view_get_selection (tree);
		gtk_tree_selection_select_iter (selection, &iter);
	}
}

static GtkTreeViewColumn *
column_dialog_get_next_visible_column (DialogData *data, GtkTreeViewColumn *column)
{
	GList             *columns, *l;
	GtkTreeViewColumn *next = NULL;

	columns = gtk_tree_view_get_columns (data->edited_tree);

	for (l = columns; l; l = l->next) {
		if (l->data == column) {
			l = l->next;
			break;
		}
	}

	while (l && !gtk_tree_view_column_get_visible (l->data)) {
		l = l->next;
	}

	if (l) {
		next = l->data;
	}

	g_list_free (columns);

	return next;
}

static GtkTreeViewColumn *
column_dialog_get_prev_visible_column (DialogData *data, GtkTreeViewColumn *column)
{
	GList             *columns, *l;
	GtkTreeViewColumn *prev = NULL;

	columns = gtk_tree_view_get_columns (data->edited_tree);

	for (l = columns; l; l = l->next) {
		if (l->data == column) {
			l = l->prev;
			break;
		}
	}

	while (l && !gtk_tree_view_column_get_visible (l->data)) {
		l = l->prev;
	}

	if (l) {
		prev = l->data;
	}

	g_list_free (columns);

	return prev;
}

static void
column_dialog_down_button_clicked_cb (GtkWidget  *button,
				      DialogData *data)
{
	GtkTreeViewColumn *column;
	GtkTreeViewColumn *base_column;

	column = column_dialog_get_selected_column (
		data, GTK_TREE_VIEW (data->visible_tree));

	if (!column) {
		return;
	}

	base_column = column_dialog_get_next_visible_column (data, column);
	if (base_column) {
		gtk_tree_view_move_column_after (data->edited_tree,
						 column,
						 base_column);
	}

	/* Note: We should probalby not be lazy here and reorder the rows
	 * instead.
	 */
	column_dialog_fill_trees (data);
	column_dialog_set_selected_column (data, column);
	gtk_widget_grab_focus (GTK_WIDGET (data->visible_tree));
}

static void
column_dialog_up_button_clicked_cb (GtkWidget  *button,
				    DialogData *data)
{
	GtkTreeViewColumn *column;
	GtkTreeViewColumn *base_column;

	column = column_dialog_get_selected_column (
		data, GTK_TREE_VIEW (data->visible_tree));

	if (!column) {
		return;
	}

	base_column = column_dialog_get_prev_visible_column (data, column);
	if (base_column) {
		base_column = column_dialog_get_prev_visible_column (data, base_column);
	}

	gtk_tree_view_move_column_after (data->edited_tree,
					 column,
					 base_column);

	/* Note: We should probalby not be lazy here and reorder the rows
	 * instead.
	 */
	column_dialog_fill_trees (data);
	column_dialog_set_selected_column (data, column);
	gtk_widget_grab_focus (GTK_WIDGET (data->visible_tree));
}

static void
column_dialog_hide_button_clicked_cb (GtkWidget  *button,
				      DialogData *data)
{
	GtkTreeViewColumn *column;

	column = column_dialog_get_selected_column (
		data, GTK_TREE_VIEW (data->visible_tree));

	if (!column) {
		return;
	}

	gtk_tree_view_column_set_visible (column, FALSE);

	/* This is only emitted when columns are reordered, so we do it here. */
	g_signal_emit_by_name (data->edited_tree, "columns-changed");

	/* Note: We should probalby not be lazy here and reorder the rows
	 * instead.
	 */
	column_dialog_fill_trees (data);
	column_dialog_set_selected_column (data, column);
	gtk_widget_grab_focus (GTK_WIDGET (data->hidden_tree));
}

static void
column_dialog_show_button_clicked_cb (GtkWidget  *button,
				      DialogData *data)
{
	GtkTreeViewColumn *column;

	column = column_dialog_get_selected_column (
		data, GTK_TREE_VIEW (data->hidden_tree));

	if (!column) {
		return;
	}

	gtk_tree_view_column_set_visible (column, TRUE);

	/* This is only emitted when columns are reordered, so we do it here. */
	g_signal_emit_by_name (data->edited_tree, "columns-changed");

	/* Note: We should probalby not be lazy here and reorder the rows
	 * instead.
	 */
	column_dialog_fill_trees (data);
	column_dialog_set_selected_column (data, column);
	gtk_widget_grab_focus (GTK_WIDGET (data->visible_tree));
}

static void
column_dialog_cursor_changed_cb (GtkTreeView *tree,
				 DialogData  *data)
{
	column_dialog_update_sensitivity (data);
}

static gboolean
column_dialog_focus_in_event_cb (GtkTreeView *tree,
				 GdkEvent    *event,
				 DialogData  *data)
{
	column_dialog_update_sensitivity (data);

	return FALSE;
}

static void
column_dialog_label_cell_data_func (GtkTreeViewColumn *tree_column,
				    GtkCellRenderer   *cell,
				    GtkTreeModel      *tree_model,
				    GtkTreeIter       *iter,
				    gpointer           user_data)
{
	GtkTreeViewColumn *column;
	gchar             *str, *label;

	gtk_tree_model_get (tree_model, iter,
			    0, &column,
			    -1);

	/* A small hack here, we strstrip since the gantt view has a \n in the
	 * title.
	 */
	str = g_strdup (gtk_tree_view_column_get_title (column));
	label = g_strstrip (str);

	g_object_set (cell,
		      "text", label,
		      NULL);

	g_free (str);
}

static void
column_dialog_setup_tree (DialogData *data, GtkTreeView *tree)
{
	GtkTreeSelection  *selection;
	GtkListStore      *store;
	GtkCellRenderer   *cell;
	GtkTreeViewColumn *col;

	selection = gtk_tree_view_get_selection (tree);
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_BROWSE);

	g_signal_connect (tree,
			  "cursor-changed",
			  G_CALLBACK (column_dialog_cursor_changed_cb),
			  data);

	store = gtk_list_store_new (1, G_TYPE_POINTER);
	gtk_tree_view_set_model (tree, GTK_TREE_MODEL (store));
	g_object_unref (store);

	cell = gtk_cell_renderer_text_new ();
	col = gtk_tree_view_column_new_with_attributes (NULL, cell, NULL);
	gtk_tree_view_column_set_cell_data_func (col,
						 cell,
						 column_dialog_label_cell_data_func,
						 data,
						 NULL);
	gtk_tree_view_append_column (tree, col);
}

static void
column_dialog_fill_trees (DialogData *data)
{
	GtkListStore *hidden_store;
	GtkListStore *visible_store;
	GList        *columns, *l;
	GtkTreeIter   iter;

	hidden_store = GTK_LIST_STORE (
		gtk_tree_view_get_model (GTK_TREE_VIEW (data->hidden_tree)));
	visible_store = GTK_LIST_STORE (
		gtk_tree_view_get_model (GTK_TREE_VIEW (data->visible_tree)));

	gtk_list_store_clear (hidden_store);
	gtk_list_store_clear (visible_store);

	columns = gtk_tree_view_get_columns (data->edited_tree);

	for (l = columns; l; l = l->next) {
		GtkTreeViewColumn *column;

		column = l->data;

		if (g_object_get_data (G_OBJECT (column), "custom")) {
			continue;
		}

		if (gtk_tree_view_column_get_visible (column)) {
			gtk_list_store_append (visible_store, &iter);
			gtk_list_store_set (visible_store,
					    &iter,
					    0, column,
					    -1);
		} else {
			gtk_list_store_append (hidden_store, &iter);
			gtk_list_store_set (hidden_store,
					    &iter,
					    0, column,
					    -1);
		}
	}

	g_list_free (columns);
}

static void
column_dialog_close_button_clicked_cb (GtkWidget  *button,
				       DialogData *data)
{
	gtk_widget_destroy (data->dialog);
}

void
planner_column_dialog_show (PlannerWindow *window,
			    const gchar   *title,
			    GtkTreeView   *edited_tree)
{
	DialogData *data;
	GladeXML   *glade;
	GtkWidget  *close_button;
	gchar      *filename;

	filename = mrp_paths_get_glade_dir ("column-dialog.glade");
	glade = glade_xml_new (filename, NULL, NULL);
	g_free (filename);

	if (!glade) {
		g_warning ("Could not create column dialog.");
		return;
	}

	data = g_new0 (DialogData, 1);

	data->main_window = window;
	data->edited_tree = edited_tree;

	data->dialog = glade_xml_get_widget (glade, "column_dialog");

	gtk_window_set_title (GTK_WINDOW (data->dialog), title);
	gtk_window_set_transient_for (GTK_WINDOW (data->dialog),
				      GTK_WINDOW (window));
	gtk_window_set_destroy_with_parent (GTK_WINDOW (data->dialog), TRUE);

	data->visible_tree = glade_xml_get_widget (glade, "visible_tree");
	data->hidden_tree = glade_xml_get_widget (glade, "hidden_tree");
	data->up_button = glade_xml_get_widget (glade, "up_button");
	data->down_button = glade_xml_get_widget (glade, "down_button");
	data->show_button = glade_xml_get_widget (glade, "show_button");
	data->hide_button = glade_xml_get_widget (glade, "hide_button");

	column_dialog_setup_tree (data, GTK_TREE_VIEW (data->hidden_tree));
	column_dialog_setup_tree (data, GTK_TREE_VIEW (data->visible_tree));

	column_dialog_fill_trees (data);

	g_signal_connect (data->hidden_tree,
			  "focus-in-event",
			  G_CALLBACK (column_dialog_focus_in_event_cb),
			  data);
	g_signal_connect (data->visible_tree,
			  "focus-in-event",
			  G_CALLBACK (column_dialog_focus_in_event_cb),
			  data);

	g_signal_connect (data->up_button,
			  "clicked",
			  G_CALLBACK (column_dialog_up_button_clicked_cb),
			  data);
	g_signal_connect (data->down_button,
			  "clicked",
			  G_CALLBACK (column_dialog_down_button_clicked_cb),
			  data);
	g_signal_connect (data->hide_button,
			  "clicked",
			  G_CALLBACK (column_dialog_hide_button_clicked_cb),
			  data);
	g_signal_connect (data->show_button,
			  "clicked",
			  G_CALLBACK (column_dialog_show_button_clicked_cb),
			  data);

	close_button = glade_xml_get_widget (glade, "close_button");
	g_signal_connect (close_button,
			  "clicked",
			  G_CALLBACK (column_dialog_close_button_clicked_cb),
			  data);

	g_object_set_data_full (G_OBJECT (data->dialog),
				"data", data,
				g_free);

	column_dialog_update_sensitivity (data);

	gtk_widget_show (data->dialog);

	g_object_unref (glade);
}
