/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2002 CodeFactory AB
 * Copyright (C) 2002 Richard Hult <richard@imendio.com>
 * Copyright (C) 2002 Mikael Hallendal <micke@imendio.com>
 * Copyright (C) 2002-2004 Alvaro del Castillo <acs@barrapunto.com>
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
#include <libgnome/gnome-i18n.h>
#include <glade/glade.h>
#include <gtk/gtk.h>
#include "planner-marshal.h"
#include "planner-group-model.h"
#include "planner-group-dialog.h"

/* GtkCellRenderer types */
enum {
	TREE_VIEW_TEXT,
	TREE_VIEW_ACTIVE
};

typedef struct {
	PlannerCmd   base;

	MrpProject  *project;
	MrpGroup    *group;
} GroupCmdInsert;

typedef struct {
	PlannerCmd   base;

	MrpProject  *project;
	MrpGroup    *group;
	GList       *group_resources;
	gboolean     is_default;
} GroupCmdRemove;

typedef struct {
	PlannerCmd   base;

	MrpGroup    *group;
	const gchar *property;  
	GValue      *value;
	GValue      *old_value;
} GroupCmdEditProperty;


typedef struct {
	PlannerCmd   base;

	MrpProject  *project;
	MrpGroup    *group;
	MrpGroup    *old_group;
} GroupCmdDefault;

typedef struct {
	MrpProject  *project;
	PlannerView *view;
	GtkTreeView *tree_view;
	GtkWidget   *remove_button;
} DialogData;

typedef struct {
	MrpGroup    *group;
	GtkTreeIter *found_iter;
} FindGroupData;

static void 
group_dialog_free_find_group_data (FindGroupData *data)
{
	if (data->found_iter) {
		gtk_tree_iter_free (data->found_iter);
	}
	
	g_free (data);
}

static gboolean
group_dialog_foreach_find_group_func (GtkTreeModel     *model,
				      GtkTreePath      *path,
				      GtkTreeIter      *iter,
				      FindGroupData    *data)
{
	MrpGroup *group;
	
	gtk_tree_model_get (model, iter,
			    GROUP_COL, &group,
			    -1);
	
	if (group == data->group) {
		data->found_iter = gtk_tree_iter_copy (iter);
		return TRUE;
	}
	
	return FALSE;
}

static FindGroupData *
group_dialog_find_group (GtkTreeView *tree_view, MrpGroup *group)
{
	FindGroupData *data;
	GtkTreeModel  *model;
	
	data = g_new0 (FindGroupData, 1);
	data->group = group;
	data->found_iter = NULL;

	model = gtk_tree_view_get_model (tree_view);

	gtk_tree_model_foreach (model,
				(GtkTreeModelForeachFunc) group_dialog_foreach_find_group_func,
				data);

	if (data->found_iter) {
		return data;
	}
	
	g_free (data);
	return NULL;
}


static GtkWidget * 
group_dialog_create                       (PlannerView          *view);

static void  group_dialog_setup_tree_view (GtkWidget            *dialog);

static void  group_dialog_insert_group_cb (GtkWidget            *button, 
					   GtkWidget            *dialog);

static void  group_dialog_remove_group_cb (GtkWidget            *button, 
					   GtkWidget            *dialog);

static void  group_dialog_close_editor_cb (GtkWidget            *button, 
					   GtkWidget            *dialog);

static void  group_dialog_cell_toggled    (GtkCellRendererText  *cell, 
					   gchar                *path_str,
					   GtkWindow            *dialog);

static void  group_dialog_cell_edited     (GtkCellRendererText  *cell,
					   gchar                *path_str,
					   gchar                *new_text,
					   GtkWindow            *dialog);

static void  group_dialog_add_column      (GtkWidget            *dialog,
					   int                   column,
					   char                 *title, 
					   guint                 type,
					   gint                  min_width);

static void  group_dialog_add_columns     (GtkWidget            *dialog);

static void 
group_dialog_selection_changed_cb         (GtkTreeSelection     *selection,
					   GtkWidget            *dialog);
static GList *
group_dialog_selection_get_list           (GtkWidget            *dialog);

static GtkWidget *
group_dialog_create (PlannerView *view)
{
	DialogData *data;
	GladeXML   *gui;
	GtkWidget  *dialog;
	GtkWidget  *button;
	MrpProject *project;

	data = g_new0 (DialogData, 1);

	data->view = g_object_ref (view);
	project = planner_window_get_project (data->view->main_window);

	data->project = g_object_ref (project);
	
	gui = glade_xml_new (
		GLADEDIR"/group-dialog.glade", NULL , NULL);
		
	dialog = glade_xml_get_widget (gui, "dialog_group_editor"); 
	
	data->tree_view = GTK_TREE_VIEW (
		glade_xml_get_widget (gui, 
				      "group_edit_treeview"));
	
	button = glade_xml_get_widget (gui, "add_group");
	g_signal_connect (button, "clicked",
			  G_CALLBACK (group_dialog_insert_group_cb), 
			  dialog);
	
	data->remove_button = glade_xml_get_widget (gui, "remove_group");
	g_signal_connect (data->remove_button,
			  "clicked",
			  G_CALLBACK (group_dialog_remove_group_cb), 
			  dialog);

	button = glade_xml_get_widget (gui, "close_editor");
	g_signal_connect (button,
			  "clicked",
			  G_CALLBACK (group_dialog_close_editor_cb), 
			  dialog);
	
	g_object_set_data (G_OBJECT (dialog), "data", data);

	group_dialog_setup_tree_view (dialog);

	return dialog;
}

static void
group_dialog_setup_tree_view (GtkWidget *dialog)
{
	DialogData       *data;
	GtkTreeModel     *model;
	GtkTreeModel     *sorted_model;
	GtkTreeSelection *selection;
	
	g_return_if_fail (GTK_IS_DIALOG (dialog));

	data = g_object_get_data (G_OBJECT (dialog), "data");

	model = GTK_TREE_MODEL (planner_group_model_new (data->project));
	sorted_model = gtk_tree_model_sort_new_with_model (model);
	
	gtk_tree_view_set_model (data->tree_view, sorted_model);
	selection = gtk_tree_view_get_selection (data->tree_view);
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_MULTIPLE);

	g_signal_connect (selection,
			  "changed",
			  G_CALLBACK (group_dialog_selection_changed_cb),
			  dialog);

	group_dialog_add_columns (dialog);
	
	g_object_unref (model);
	g_object_unref (sorted_model);
}

static gboolean
group_cmd_insert_do (PlannerCmd *cmd_base)
{
	GroupCmdInsert *cmd;

	cmd = (GroupCmdInsert*) cmd_base;

	g_assert (MRP_IS_GROUP (cmd->group));
		
	mrp_project_add_group (cmd->project, cmd->group);

	return TRUE;
}

static void
group_cmd_insert_undo (PlannerCmd *cmd_base)
{
	GroupCmdInsert *cmd;
	
	cmd = (GroupCmdInsert*) cmd_base;

	mrp_project_remove_group (cmd->project,
				  cmd->group);
	
}


static void
group_cmd_insert_free (PlannerCmd *cmd_base)
{
	GroupCmdInsert  *cmd;

	cmd = (GroupCmdInsert*) cmd_base;	

	cmd->group = NULL;
	cmd->project = NULL;
}

static PlannerCmd *
group_cmd_insert (PlannerView *view)
{
	PlannerCmd      *cmd_base;
	GroupCmdInsert  *cmd;

	cmd_base = planner_cmd_new (GroupCmdInsert,
				    _("Insert group"),
				    group_cmd_insert_do,
				    group_cmd_insert_undo,
				    group_cmd_insert_free);

	cmd = (GroupCmdInsert *) cmd_base;

	cmd->project = planner_window_get_project (view->main_window);

	cmd->group = g_object_new (MRP_TYPE_GROUP, NULL);	

	planner_cmd_manager_insert_and_do (planner_window_get_cmd_manager (view->main_window),
					   cmd_base);

	return cmd_base;
}

static void
group_dialog_insert_group_cb (GtkWidget *button, GtkWidget *dialog)
{
	DialogData     *data;
	FindGroupData  *find_data;
	GtkTreeModel   *model;
	GtkTreePath    *path;
	GroupCmdInsert *cmd;
	
	g_return_if_fail (GTK_IS_DIALOG (dialog));

	data = g_object_get_data (G_OBJECT (dialog), "data");
	
	cmd = (GroupCmdInsert*) group_cmd_insert (data->view);

	if (!GTK_WIDGET_HAS_FOCUS (data->tree_view)) {
		gtk_widget_grab_focus (GTK_WIDGET (data->tree_view));
	}

	find_data = group_dialog_find_group (data->tree_view, cmd->group);
	if (find_data) {
		model = gtk_tree_view_get_model (data->tree_view);
		path = gtk_tree_model_get_path (model, find_data->found_iter);
		
		gtk_tree_view_set_cursor (data->tree_view,
					  path,
					  gtk_tree_view_get_column (data->tree_view, 0),
					  TRUE);	
		gtk_tree_path_free (path);

		group_dialog_free_find_group_data (find_data);
	}	
}

static gboolean
group_cmd_remove_do (PlannerCmd *cmd_base)
{
	GroupCmdRemove *cmd;
	GList          *resources, *l;
	MrpGroup       *default_group;

	cmd = (GroupCmdRemove*) cmd_base;

	resources = mrp_project_get_resources (cmd->project);

	for (l = resources; l; l = l->next) {
		MrpGroup *group;
		
		mrp_object_get (MRP_OBJECT (l->data), "group", &group, NULL);

		if (cmd->group == group)
			cmd->group_resources = g_list_prepend (cmd->group_resources, l->data);
	}

	mrp_object_get (cmd->project, "default-group", &default_group, NULL);
	if (default_group == cmd->group)
		cmd->is_default = TRUE;
	
	mrp_project_remove_group (cmd->project, cmd->group);

	return TRUE;
}

static void
group_cmd_remove_undo (PlannerCmd *cmd_base)
{
	GroupCmdRemove *cmd;
	GList          *l;
	
	cmd = (GroupCmdRemove*) cmd_base;

	/* We need to recover the group deleted */
	g_assert (MRP_IS_GROUP (cmd->group));
		
	mrp_project_add_group (cmd->project, cmd->group);

	/* Now we need to recover all the links of the project
	   with the group: resources link */

	for (l = cmd->group_resources; l; l = l->next) {
		mrp_object_set (MRP_OBJECT (l->data), "group", cmd->group, NULL);
	}

	if (cmd->is_default) 
		mrp_object_set (cmd->project, "default-group", cmd->group, NULL);
}

static void
group_cmd_remove_free (PlannerCmd *cmd_base)
{
	GroupCmdRemove *cmd;

	cmd = (GroupCmdRemove*) cmd_base;

	g_list_free (cmd->group_resources);

	cmd->project = NULL;
	g_object_unref (cmd->group);
}

static PlannerCmd *
group_cmd_remove (PlannerView *view, MrpGroup *group)
{
	PlannerCmd      *cmd_base;
	GroupCmdRemove  *cmd;

	cmd_base = planner_cmd_new (GroupCmdRemove,
				    _("Remove group"),
				    group_cmd_remove_do,
				    group_cmd_remove_undo,
				    group_cmd_remove_free);

	cmd = (GroupCmdRemove *) cmd_base;

	cmd->project = planner_window_get_project (view->main_window);
	cmd->group = group;

	planner_cmd_manager_insert_and_do (planner_window_get_cmd_manager (view->main_window),
					   cmd_base);

	return cmd_base;
}

static void
group_dialog_remove_group_cb (GtkWidget *widget, GtkWidget *dialog)
{
	DialogData *data;
	GList             *list, *node;
	GroupCmdRemove *cmd;
	
	g_return_if_fail (GTK_IS_DIALOG (dialog));

	data = g_object_get_data (G_OBJECT (dialog), "data");

	list = group_dialog_selection_get_list (dialog);

	for (node = list; node; node = node->next) {
		cmd = (GroupCmdRemove*) group_cmd_remove (data->view, MRP_GROUP (node->data));
	}
	
	g_list_free (list);
}

static void
group_dialog_close_editor_cb (GtkWidget *button, GtkWidget *dialog)
{
	DialogData *data;

	g_return_if_fail (GTK_IS_DIALOG (dialog));

	/* We have to destroy the model data */
	data = g_object_get_data (G_OBJECT (dialog), "data");

	g_object_unref (data->project);
	g_free (data);

	gtk_widget_destroy (dialog);
}

static gboolean
group_cmd_default_do (PlannerCmd *cmd_base)
{
	GroupCmdDefault *cmd;

	cmd = (GroupCmdDefault*) cmd_base;

	mrp_object_set (cmd->project, "default-group", cmd->group, NULL);

	return TRUE;
}

static void
group_cmd_default_undo (PlannerCmd *cmd_base)
{
	GroupCmdDefault *cmd;
	
	cmd = (GroupCmdDefault*) cmd_base;
	mrp_object_set (cmd->project, "default-group", cmd->old_group, NULL);
}

static void
group_cmd_default_free (PlannerCmd *cmd_base)
{
	GroupCmdDefault *cmd;
	
	cmd = (GroupCmdDefault*) cmd_base;

	g_object_unref (cmd->group);
	g_object_unref (cmd->old_group);
	g_free (cmd_base->label);

	g_free (cmd);
}

static PlannerCmd *
group_cmd_default (PlannerView *view,
		   MrpGroup    *group)
{
	PlannerCmd       *cmd_base;
	GroupCmdDefault  *cmd;

	cmd_base = planner_cmd_new (GroupCmdDefault,
				    _("Default group"),
				    group_cmd_default_do,
				    group_cmd_default_undo,
				    group_cmd_default_free);

	cmd = (GroupCmdDefault *) cmd_base;

	cmd->project = planner_window_get_project (view->main_window);

	cmd->group = g_object_ref (group);
	mrp_object_get (cmd->project, "default-group", &cmd->old_group, NULL);

	planner_cmd_manager_insert_and_do (planner_window_get_cmd_manager (view->main_window),
					   cmd_base);

	return cmd_base;
}


static void
group_dialog_cell_toggled (GtkCellRendererText *cell, 
			   gchar               *path_str,
			   GtkWindow           *dialog)
{
	DialogData       *data;
	GtkTreeModel     *model;
	GtkTreePath      *path;
	GtkTreeIter       iter;
	GtkTreeModelSort *sorted_model;
	GtkTreeIter       sorted_iter;
	gint              column;
	gboolean          is_default;
	MrpGroup         *group;
	
	data = g_object_get_data (G_OBJECT (dialog), "data");

	sorted_model = GTK_TREE_MODEL_SORT (gtk_tree_view_get_model (data->tree_view));
	
	model = gtk_tree_model_sort_get_model (sorted_model);
	
	path   = gtk_tree_path_new_from_string (path_str);
	column = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (cell),
						     "column"));

	gtk_tree_model_get_iter (GTK_TREE_MODEL (sorted_model),
				 &sorted_iter, path);

	gtk_tree_model_sort_convert_iter_to_child_iter (sorted_model, 
							&iter,
							&sorted_iter);

	switch (column) {	
	case GROUP_COL_GROUP_DEFAULT:
		gtk_tree_model_get (model, &iter, column, &is_default, -1);

		group = MRP_GROUP (planner_list_model_get_object (
					   PLANNER_LIST_MODEL (model), &iter));
		if (!is_default) {
			group_cmd_default (data->view, group);
		}
		break;

	default:
		g_assert_not_reached ();
	}

	gtk_tree_path_free (path);		
}


static gboolean
group_cmd_edit_property_do (PlannerCmd *cmd_base)
{
	GroupCmdEditProperty *cmd;

	cmd = (GroupCmdEditProperty *) cmd_base;	
	
	g_object_set_property (G_OBJECT (cmd->group),
			       cmd->property,
			       cmd->value);

	return TRUE;
}

static void
group_cmd_edit_property_undo (PlannerCmd *cmd_base)
{
	GroupCmdEditProperty *cmd;
	
	cmd = (GroupCmdEditProperty *) cmd_base;

	g_object_set_property (G_OBJECT (cmd->group),
			       cmd->property,
			       cmd->old_value);
}

static void
group_cmd_edit_property_free (PlannerCmd *cmd_base)
{
	GroupCmdEditProperty *cmd;
	
	cmd = (GroupCmdEditProperty *) cmd_base;
	
	cmd->group = NULL;
	g_value_unset (cmd->value);
	g_value_unset (cmd->old_value);
}

static PlannerCmd *
group_cmd_edit_property (PlannerView  *view, 
			 MrpGroup     *group,
			 const gchar  *property,
			 GValue       *value)
{
	PlannerCmd            *cmd_base;
	GroupCmdEditProperty  *cmd;

	cmd_base = planner_cmd_new (GroupCmdEditProperty,
				    _("Edit group property"),
				    group_cmd_edit_property_do,
				    group_cmd_edit_property_undo,
				    group_cmd_edit_property_free);

	cmd = (GroupCmdEditProperty *) cmd_base;

	cmd->property = property;
	cmd->group = group;

	cmd->value = g_new0 (GValue, 1);
	g_value_init (cmd->value, G_VALUE_TYPE (value));
	g_value_copy (value, cmd->value);

	cmd->old_value = g_new0 (GValue, 1);
	g_value_init (cmd->old_value, G_VALUE_TYPE (value));

	g_object_get_property (G_OBJECT (cmd->group),
			       cmd->property,
			       cmd->old_value);

	/* FIXME: if old and new value are the same, do nothing 
	   How we can compare values?
	 */

	planner_cmd_manager_insert_and_do (planner_window_get_cmd_manager (view->main_window),
					   cmd_base);

	return cmd_base;
}

static void
group_dialog_cell_edited (GtkCellRendererText *cell, 
			  gchar               *path_str,
			  gchar               *new_text,
			  GtkWindow           *dialog)
{
	DialogData       *data;
	GtkTreeModel     *model;
	GtkTreePath      *path;
	GtkTreeIter       iter;
	GtkTreeModelSort *sorted_model;
	GtkTreeIter       sorted_iter;
	GValue            value = { 0 };
	gint              column;
	MrpGroup         *group;
	PlannerCmd       *cmd;
	gchar            *property = "";

	data  = g_object_get_data (G_OBJECT (dialog), "data");

	sorted_model = GTK_TREE_MODEL_SORT (gtk_tree_view_get_model (data->tree_view));
	
	model = gtk_tree_model_sort_get_model (sorted_model);
	
	path   = gtk_tree_path_new_from_string (path_str);
	column = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (cell), "column"));

	gtk_tree_model_get_iter (GTK_TREE_MODEL (sorted_model),
				 &sorted_iter, path);
	
	gtk_tree_model_sort_convert_iter_to_child_iter (sorted_model, 
							&iter,
							&sorted_iter);

	group = MRP_GROUP (planner_list_model_get_object (
				   PLANNER_LIST_MODEL (model), &iter));
	
	switch (column) {
	case GROUP_COL_NAME:
		property = "name";
		g_value_init (&value, G_TYPE_STRING);
		g_value_set_string (&value, new_text);

		break;
	case GROUP_COL_MANAGER_NAME:
		property = "manager_name";
		g_value_init (&value, G_TYPE_STRING);
		g_value_set_string (&value, new_text);

		break;
	case GROUP_COL_MANAGER_PHONE:
		property = "manager_phone";
		g_value_init (&value, G_TYPE_STRING);
		g_value_set_string (&value, new_text);

		break;
	case GROUP_COL_MANAGER_EMAIL:
		property = "manager_email";
		g_value_init (&value, G_TYPE_STRING);
		g_value_set_string (&value, new_text);

		break;
	default:
		g_assert_not_reached ();
	}

	cmd = group_cmd_edit_property (data->view,
				       group, 
				       property,
				       &value);

	gtk_tree_path_free (path);
}

static void
group_dialog_add_column (GtkWidget *dialog,
			 int        column, 
			 char      *title,
			 guint      type,
			 gint       min_width) 
{
	DialogData        *data;
	GtkCellRenderer   *cell = NULL;
	GtkTreeModel      *model;
	GtkTreeViewColumn *column_data;
	gchar             *tree_type = NULL;

	data  = g_object_get_data (G_OBJECT (dialog), "data");
	model = gtk_tree_model_sort_get_model (GTK_TREE_MODEL_SORT (gtk_tree_view_get_model (data->tree_view)));

	switch (type) {
	case TREE_VIEW_TEXT: 
		cell = gtk_cell_renderer_text_new ();
		g_object_set (cell,
			      "editable", TRUE,
			      NULL);

		g_signal_connect (cell,
				  "edited",
				  G_CALLBACK (group_dialog_cell_edited),
				  dialog);

		tree_type = "text";
		break;
	case TREE_VIEW_ACTIVE:
		cell = gtk_cell_renderer_toggle_new ();
		g_object_set (cell,
			      "activatable", TRUE,
			      "radio", TRUE,
			      NULL);

		g_signal_connect (cell,
				  "toggled",
				  G_CALLBACK (group_dialog_cell_toggled),
				  dialog);

		tree_type = "active";
		break;
	default:
		g_assert_not_reached ();
		break;
	}
		
	g_object_set_data (G_OBJECT (cell),
			   "column", GINT_TO_POINTER (column));

	column_data = gtk_tree_view_column_new_with_attributes (title, cell,
								tree_type, 
								column,
								NULL);

	gtk_tree_view_column_set_min_width (column_data, min_width);

	if (type == TREE_VIEW_TEXT) {
		gtk_tree_view_column_set_sort_column_id (column_data, column);
	}

	gtk_tree_view_column_set_resizable (column_data, TRUE);

	gtk_tree_view_append_column (data->tree_view, column_data);
}

static void
group_dialog_add_columns (GtkWidget *dialog)
{
	group_dialog_add_column (dialog,
				 GROUP_COL_NAME, 
				 _("Name"),
				 TREE_VIEW_TEXT,
				 100);

	group_dialog_add_column (dialog,
				 GROUP_COL_MANAGER_NAME, 
				 _("Manager name"),
				 TREE_VIEW_TEXT,
				 50);
	
	group_dialog_add_column (dialog,
				 GROUP_COL_MANAGER_PHONE, 
				 _("Manager phone"),
				 TREE_VIEW_TEXT,
				 50);

	group_dialog_add_column (dialog,
				 GROUP_COL_MANAGER_EMAIL, 
				 _("Manager email"),
				 TREE_VIEW_TEXT,
				 50);

	group_dialog_add_column (dialog,
				 GROUP_COL_GROUP_DEFAULT, 
				 _("Default"),
				 TREE_VIEW_ACTIVE,
				 -1);
}

static void
group_dialog_selection_changed_cb (GtkTreeSelection *selection, 
				   GtkWidget        *dialog)
{
	DialogData *data;
	GList      *list;
	gboolean    selected = FALSE;
	
	g_return_if_fail (GTK_IS_TREE_SELECTION (selection));
	g_return_if_fail (GTK_IS_WIDGET (dialog));
	
	data = g_object_get_data (G_OBJECT (dialog), "data");

	list = group_dialog_selection_get_list (dialog);
	
	if (list) {
		selected = TRUE;
		g_list_free (list);
	} 

	gtk_widget_set_sensitive (data->remove_button, selected);
}

static void
group_dialog_get_selected_func (GtkTreeModel *sorted_model,
				GtkTreePath  *path,
				GtkTreeIter  *sorted_iter,
				gpointer      data)
{
	GList            **list = data;
	MrpObject         *object;
	GtkTreeIter        iter;
	PlannerListModel  *model;

	model = PLANNER_LIST_MODEL (gtk_tree_model_sort_get_model (
				       GTK_TREE_MODEL_SORT (sorted_model)));

 	gtk_tree_model_sort_convert_iter_to_child_iter (
		GTK_TREE_MODEL_SORT (sorted_model),
		&iter, sorted_iter);

	object = planner_list_model_get_object (model, &iter);

	if (object) {
		*list = g_list_prepend (*list, object);
	}
}

static GList *
group_dialog_selection_get_list (GtkWidget *dialog)
{
	DialogData       *data;
	GtkTreeSelection *selection;
	GList            *list;
	
	g_return_val_if_fail (GTK_IS_DIALOG (dialog), NULL);

	data = g_object_get_data (G_OBJECT (dialog), "data");
	
	selection = gtk_tree_view_get_selection (data->tree_view);
	
	list = NULL;
	gtk_tree_selection_selected_foreach (
		selection,
		(GtkTreeSelectionForeachFunc) group_dialog_get_selected_func,
		&list);

	return list;
}

GtkWidget *
planner_group_dialog_new (PlannerView *view)
{
	GtkWidget *dialog;
	
	g_return_val_if_fail (PLANNER_IS_VIEW (view), NULL);

	dialog = group_dialog_create (view);
	
        return dialog;
}
