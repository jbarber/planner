/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2002-2003 CodeFactory AB
 * Copyright (C) 2002-2003 Richard Hult <richard@imendio.com>
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
#include <libgnome/gnome-i18n.h>
#include <glade/glade.h>
#include <gtk/gtk.h>
#include <libplanner/mrp-project.h>
#include "planner-phase-dialog.h"

#define RESPONSE_ADD    1
#define RESPONSE_REMOVE 2
#define RESPONSE_CLOSE  GTK_RESPONSE_CLOSE

enum {
	COL_PHASE,
	NUM_COLS
};

typedef struct {
	PlannerWindow  *main_window;
	MrpProject    *project;

	GtkWidget     *dialog;
	GtkWidget     *tree_view;
	GtkWidget     *remove_button;

	/* The "new phase dialog. */
	GtkWidget     *new_ok_button;
} DialogData;

typedef struct {
	DialogData    *data;
	gchar         *phase;
	gboolean       found;
	GtkTreeIter    found_iter;
} FindData;


#define DIALOG_GET_DATA(d) g_object_get_data ((GObject*)d, "data")


static void     phase_dialog_notify_phases          (MrpProject       *project,  
						     GParamSpec       *pspec,
						     GtkWidget        *dialog);
static void     phase_dialog_response_cb            (GtkWidget        *dialog,
						     gint              response,
						     DialogData       *data);
static void     phase_dialog_setup_tree_view        (DialogData       *data);
static void     phase_dialog_rebuild_model          (DialogData       *data,
						     GList            *list);
static gchar *  phase_dialog_get_selected_phase     (DialogData       *data);
#if 0
static void     phase_dialog_set_selected_phase     (DialogData       *data,
						     const gchar      *phase);
static gboolean phase_dialog_find_foreach           (GtkTreeModel     *model,
						     GtkTreePath      *path,
						     GtkTreeIter      *iter,
						     FindData         *data);
static gboolean phase_dialog_find                   (DialogData       *data,
						     const gchar      *phase,
						     GtkTreeIter      *iter);
#endif
static void     phase_dialog_selection_changed_cb   (GtkTreeSelection *selection,
						     DialogData       *data);
static void     phase_dialog_remove_phase           (DialogData       *data,
						     const gchar      *phase);
static void     phase_dialog_new_dialog_run         (DialogData       *data);


static void
phase_dialog_notify_phases (MrpProject  *project,  
			    GParamSpec  *pspec,
			    GtkWidget   *dialog)
{
	DialogData *data = DIALOG_GET_DATA (dialog);
	GList      *list;
	
	g_return_if_fail (MRP_IS_PROJECT (project));

	g_object_get (project, "phases", &list, NULL);
	phase_dialog_rebuild_model (data, list);
	mrp_string_list_free (list);
}

static void
phase_dialog_response_cb (GtkWidget  *dialog,
			  gint        response,
			  DialogData *data)
{
	gchar *phase;
	
	switch (response) {
	case RESPONSE_REMOVE:
		phase = phase_dialog_get_selected_phase (data);
		phase_dialog_remove_phase (data, phase);
		g_free (phase);
		break;

	case RESPONSE_ADD:
		phase_dialog_new_dialog_run (data);
		break;
		
	case RESPONSE_CLOSE:
		gtk_widget_destroy (data->dialog);
		break;

	case GTK_RESPONSE_DELETE_EVENT:
		break;

	default:
		g_assert_not_reached ();
		break;
	}
}

static void
phase_dialog_parent_destroy_cb (GtkWidget *window, GtkWidget *dialog)
{
	gtk_widget_destroy (dialog);
}

GtkWidget *
planner_phase_dialog_new (PlannerWindow *window)
{
	DialogData       *data;
	GladeXML         *glade;
	GtkWidget        *dialog;
	GtkTreeSelection *selection;	

	g_return_val_if_fail (PLANNER_IS_MAIN_WINDOW (window), NULL);
	
	glade = glade_xml_new (GLADEDIR "/project-properties.glade",
			       "phase_dialog",
			       GETTEXT_PACKAGE);
	if (!glade) {
		g_warning ("Could not create phase dialog.");
		return NULL;
	}

	dialog = glade_xml_get_widget (glade, "phase_dialog");
	
	data = g_new0 (DialogData, 1);

	data->main_window = window;
	data->project = planner_window_get_project (window);
	data->dialog = dialog;

	g_signal_connect_object (window,
				 "destroy",
				 G_CALLBACK (phase_dialog_parent_destroy_cb),
				 dialog,
				 0);
	
	data->remove_button = glade_xml_get_widget (glade, "remove_button");

	data->tree_view = glade_xml_get_widget (glade, "phase_treeview");

	phase_dialog_setup_tree_view (data);
	
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (data->tree_view));
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_BROWSE);

	g_signal_connect (selection,
			  "changed",
			  G_CALLBACK (phase_dialog_selection_changed_cb),
			  data);
	
	g_signal_connect (dialog,
			  "response",
			  G_CALLBACK (phase_dialog_response_cb),
			  data);

	g_object_set_data_full (G_OBJECT (dialog),
				"data", data,
				g_free);

	return dialog;
}

static void
phase_dialog_setup_tree_view (DialogData *data)
{
	GtkListStore      *store;
	GtkCellRenderer   *cell;
	GtkTreeViewColumn *col;
	GList             *list;
	
	store = gtk_list_store_new (NUM_COLS,
				    G_TYPE_STRING);
	
	g_signal_connect_object (data->project,
				 "notify::phases",
				 G_CALLBACK (phase_dialog_notify_phases),
				 data->dialog,
				 0);
	
	gtk_tree_view_set_model (GTK_TREE_VIEW (data->tree_view),
				 GTK_TREE_MODEL (store));

	cell = gtk_cell_renderer_text_new ();
	col = gtk_tree_view_column_new_with_attributes (
		NULL,
		cell,
		"text", COL_PHASE,
		NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (data->tree_view), col);

	g_object_get (data->project, "phases", &list, NULL);
	phase_dialog_rebuild_model (data, list);
	mrp_string_list_free (list);
	
	gtk_tree_view_expand_all (GTK_TREE_VIEW (data->tree_view));
}

static void
phase_dialog_rebuild_model (DialogData  *data,
			    GList       *list)
{
	GtkTreeView  *tree_view;
	GtkListStore *store;
	GtkTreeIter   iter;
	GList        *l;

	tree_view = GTK_TREE_VIEW (data->tree_view);
	store = GTK_LIST_STORE (gtk_tree_view_get_model (tree_view));

	gtk_list_store_clear (store);
	
	if (!list) {
		return;
	}
	
	for (l = list; l; l = l->next) {
		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter,
				    COL_PHASE, l->data,
				    -1);
	}
}

static gchar *
phase_dialog_get_selected_phase (DialogData *data)
{
	GtkTreeView      *tree_view;
	GtkTreeSelection *selection;
	GtkTreeModel     *model;
	GtkTreeIter       iter;
	gchar            *phase;

	tree_view = GTK_TREE_VIEW (data->tree_view);
	
	selection = gtk_tree_view_get_selection (tree_view);
	if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
		gtk_tree_model_get (model, &iter,
				    COL_PHASE, &phase,
				    -1);

		return phase;
	}

	return NULL;
}

#if 0
static void
phase_dialog_set_selected_phase (DialogData  *data,
				 const gchar *phase)
{
	GtkTreeIter   iter;
	GtkTreePath  *path;
	GtkTreeModel *model;
	
	if (phase_dialog_find (data, phase, &iter)) {
		model = gtk_tree_view_get_model (GTK_TREE_VIEW (data->tree_view));
		
		path = gtk_tree_model_get_path (model, &iter);
		
		gtk_tree_view_set_cursor (GTK_TREE_VIEW (data->tree_view),
					  path, NULL, FALSE);

		gtk_tree_path_free (path);
	}
}

static gboolean
phase_dialog_find_foreach (GtkTreeModel *model,
			   GtkTreePath  *path,
			   GtkTreeIter  *iter,
			   FindData     *data)
{
	gchar *phase;

	gtk_tree_model_get (model,
			    iter,
			    COL_PHASE, &phase,
			    -1);

	if (phase == data->phase) {
		data->found = TRUE;
		data->found_iter = *iter;
		return TRUE;
	}
	
	return FALSE;
}

static gboolean
phase_dialog_find (DialogData  *data,
		   const gchar *phase,
		   GtkTreeIter *iter)
{
	GtkTreeModel *model;
	FindData      find_data;

	find_data.found = FALSE;
	find_data.phase = (gchar *) phase;
	find_data.data = data;

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (data->tree_view));

	gtk_tree_model_foreach (model,
				(GtkTreeModelForeachFunc) phase_dialog_find_foreach,
				&find_data);

	if (find_data.found) {
		*iter = find_data.found_iter;
		return TRUE;
	}

	return FALSE;
}
#endif

static void
phase_dialog_selection_changed_cb (GtkTreeSelection *selection,
				   DialogData       *data)
{
	gchar *phase;

	phase = phase_dialog_get_selected_phase (data);

	gtk_widget_set_sensitive (data->remove_button, phase != NULL);

	g_free (phase);
}

static void
phase_dialog_remove_phase (DialogData  *data,
			   const gchar *phase)
{
	GList        *list, *l;
	GtkTreeView  *tree_view;
	GtkListStore *store;
	gboolean      found = FALSE;

	tree_view = GTK_TREE_VIEW (data->tree_view);
	store = GTK_LIST_STORE (gtk_tree_view_get_model (tree_view));

	/* Get all phases from the project, remove the selected one, re-set the
	 * list.
	 */
	g_object_get (data->project, "phases", &list, NULL);

	for (l = list; l; l = l->next) {
		if (!strcmp (phase, l->data)) {
			g_free (l->data);
			list = g_list_remove_link (list, l);
			found = TRUE;
			break;
		}
	}

	if (found) {
		g_object_set (data->project, "phases", list, NULL);
	}

	mrp_string_list_free (list);
}

/* Handle the little "new phase" dialog. */
static void
phase_dialog_new_name_changed_cb (GtkEntry   *entry,
				  DialogData *data)
{
	const gchar *name;
	gboolean     sensitive;

	name = gtk_entry_get_text (entry);
	
	sensitive = name && name[0];
	gtk_widget_set_sensitive (data->new_ok_button, sensitive);
}

static void
phase_dialog_new_dialog_run (DialogData *data)
{
	GladeXML    *glade;
	GtkWidget   *dialog;
	GtkWidget   *entry;
	const gchar *name;
	GList       *list;
	
	glade = glade_xml_new (GLADEDIR "/project-properties.glade",
			       "new_phase_dialog",
			       GETTEXT_PACKAGE);

	dialog = glade_xml_get_widget (glade, "new_phase_dialog");

	data->new_ok_button = glade_xml_get_widget (glade, "ok_button");

	entry = glade_xml_get_widget (glade, "name_entry");
	g_signal_connect (entry,
			  "changed",
			  G_CALLBACK (phase_dialog_new_name_changed_cb),
			  data);

	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_OK) {
		name = gtk_entry_get_text (GTK_ENTRY (entry));

		/* Get all phases from the project, add the new one, re-set the
		 * list.
		 */
		g_object_get (data->project, "phases", &list, NULL);
		list = g_list_append (list, g_strdup (name)); 
		g_object_set (data->project, "phases", list, NULL);
		mrp_string_list_free (list);
	}

	g_object_unref (glade);
	gtk_widget_destroy (dialog);
}

