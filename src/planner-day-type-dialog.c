/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2002 CodeFactory AB
 * Copyright (C) 2002 Richard Hult <richard@imendio.com>
 * Copyright (C) 2002 Mikael Hallendal <micke@imendio.com>
 * Copyright (C) 2004 Imendio HB
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
#include <libgnome/gnome-i18n.h>
#include <glade/glade.h>
#include <gtk/gtk.h>
#include <libplanner/mrp-project.h>
#include "planner-day-type-dialog.h"

#define RESPONSE_ADD    1
#define RESPONSE_REMOVE 2
#define RESPONSE_CLOSE  GTK_RESPONSE_CLOSE
#define RESPONSE_APPLY  GTK_RESPONSE_APPLY

enum {
	COL_NAME,
	COL_ID,
	COL_DAY,
	NUM_COLS
};

typedef struct {
	PlannerWindow *main_window;
	MrpProject    *project;

	GtkWidget     *dialog;
	GtkWidget     *tree_view;

	GtkWidget     *remove_button;
} DialogData;

typedef struct {
	DialogData    *data;
	MrpDay        *day;
	gboolean       found;
	GtkTreeIter    found_iter;
} FindDayData;

typedef struct {
	PlannerCmd   base;

	MrpProject  *project;
	
	gchar       *name;
	MrpDay      *day;
} DayTypeCmdAdd;

typedef struct {
	PlannerCmd   base;

	MrpProject  *project;

	gchar       *name;
	MrpDay      *day;
} DayTypeCmdRemove;

#define DIALOG_GET_DATA(d) g_object_get_data ((GObject*)d, "data")


static void          day_type_dialog_response_cb          (GtkWidget        *dialog,
							   gint              response,
							   DialogData       *data);
static void          day_type_dialog_build_list           (DialogData       *data);
static GtkTreeModel *day_type_dialog_create_model         (DialogData       *data);
static void          day_type_dialog_type_added_cb        (MrpProject       *project,
							   MrpDay           *day,
							   GtkWidget        *dialog);
static void          day_type_dialog_type_removed_cb      (MrpProject       *project,
							   MrpDay           *day,
							   GtkWidget        *dialog);
static gboolean      day_type_dialog_find_day             (DialogData       *data,
							   MrpDay           *day,
							   GtkTreeIter      *iter);
static MrpDay *      day_type_dialog_get_selected_day     (DialogData       *data);
static void          day_type_dialog_selection_changed_cb (GtkTreeSelection *selection,
							   DialogData       *data);
static void          day_type_dialog_new_dialog_run       (DialogData       *data);

static PlannerCmd *  day_type_cmd_add                     (DialogData       *data,
							   const char       *name);
static PlannerCmd *  day_type_cmd_remove                  (DialogData       *data,
							   MrpDay           *day);


static void
day_type_dialog_response_cb (GtkWidget  *dialog,
			     gint        response,
			     DialogData *data)
{
	MrpDay *day;

	switch (response) {
	case RESPONSE_REMOVE:
		day = day_type_dialog_get_selected_day (data);
		day_type_cmd_remove (data, day);
		break;

	case RESPONSE_ADD:
		day_type_dialog_new_dialog_run (data);
		break;
		
	case RESPONSE_CLOSE:
		gtk_widget_destroy (data->dialog);
		break;

	case GTK_RESPONSE_DELETE_EVENT:
		break;

	default:
		g_assert_not_reached ();
	}
}

static void
day_type_dialog_parent_destroy_cb (GtkWidget *window, GtkWidget *dialog)
{
	gtk_widget_destroy (dialog);
}

GtkWidget *
planner_day_type_dialog_new (PlannerWindow *window)
{
	DialogData        *data;
	GladeXML          *glade;
	GtkWidget         *dialog;
	GtkTreeModel      *model;
	GtkCellRenderer   *cell;
	GtkTreeViewColumn *col;
	GtkTreeSelection  *selection;
	
	g_return_val_if_fail (PLANNER_IS_MAIN_WINDOW (window), NULL);
	
	glade = glade_xml_new (GLADEDIR "/calendar-dialog.glade",
			       "day_type_dialog",
			       GETTEXT_PACKAGE);
	if (!glade) {
		g_warning ("Could not create day_type dialog.");
		return NULL;
	}

	dialog = glade_xml_get_widget (glade, "day_type_dialog");
	
	data = g_new0 (DialogData, 1);

	data->main_window = window;
	data->project = planner_window_get_project (window);
	data->dialog = dialog;

	g_signal_connect_object (window,
				 "destroy",
				 G_CALLBACK (day_type_dialog_parent_destroy_cb),
				 dialog,
				 0);
	
	data->tree_view = glade_xml_get_widget (glade, "treeview");
	data->remove_button = glade_xml_get_widget (glade, "remove_button");

	g_signal_connect_object (data->project,
				 "day_added",
				 G_CALLBACK (day_type_dialog_type_added_cb),
				 data->dialog,
				 0);

	g_signal_connect_object (data->project,
				 "day_removed",
				 G_CALLBACK (day_type_dialog_type_removed_cb),
				 data->dialog,
				 0);

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (data->tree_view));
	
	g_signal_connect (selection,
			  "changed",
			  G_CALLBACK (day_type_dialog_selection_changed_cb),
			  data);

	g_object_set_data_full (G_OBJECT (dialog),
				"data", data,
				g_free);
	
	model = day_type_dialog_create_model (data);
	gtk_tree_view_set_model (GTK_TREE_VIEW (data->tree_view), model);

	day_type_dialog_build_list (data);
	
	cell = gtk_cell_renderer_text_new ();
	col = gtk_tree_view_column_new_with_attributes (
		NULL,
		cell,
		"text", COL_NAME,
		NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (data->tree_view), col);
	
	g_signal_connect (dialog,
			  "response",
			  G_CALLBACK (day_type_dialog_response_cb),
			  data);

	return dialog;
}

static void
day_type_dialog_selection_changed_cb (GtkTreeSelection *selection,
				      DialogData       *data)
{
	MrpDay   *day;
	gboolean  sensitive = FALSE;
	
	day = day_type_dialog_get_selected_day (data);
	
	if (day != NULL && day != mrp_day_get_work () &&
	    day != mrp_day_get_nonwork () && day != mrp_day_get_use_base ()) {
		sensitive = TRUE;
	}
	
	gtk_widget_set_sensitive (data->remove_button, sensitive);
}

static void
day_type_dialog_build_list (DialogData *data)
{
	GtkListStore *store;
	GtkTreeIter   iter;
	GList        *days, *l;
	MrpDay       *day;
	const gchar  *name;
	
	store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (data->tree_view)));

	gtk_list_store_clear (store);
	
	day = mrp_day_get_nonwork ();

	name = mrp_day_get_name (day);
	gtk_list_store_append (store, &iter);
	gtk_list_store_set (store,
			    &iter,
			    COL_NAME, name,
			    COL_DAY, day,
			    -1);

	day = mrp_day_get_work ();
	name = mrp_day_get_name (day);
	gtk_list_store_append (store, &iter);
	gtk_list_store_set (store,
			    &iter,
			    COL_NAME, name,
			    COL_DAY, day,
			    -1);

	days = mrp_day_get_all (data->project);
	for (l = days; l; l = l->next) {
		day = l->data;
		
		name = mrp_day_get_name (day);
		
		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store,
				    &iter,
				    COL_NAME, name,
				    COL_DAY, day,
				    -1);
	}
}

static GtkTreeModel *
day_type_dialog_create_model (DialogData *data)
{
	GtkListStore *store;
	
	store = gtk_list_store_new (NUM_COLS,
				    G_TYPE_STRING,
				    G_TYPE_INT,
				    G_TYPE_POINTER);

	return GTK_TREE_MODEL (store);
}

static void
day_type_dialog_type_added_cb (MrpProject *project,
			       MrpDay     *day,
			       GtkWidget  *dialog)
{
	DialogData *data = DIALOG_GET_DATA (dialog);

	/* Just rebuild the list of day types. */
	day_type_dialog_build_list (data);
}

static void
day_type_dialog_type_removed_cb (MrpProject *project,
				 MrpDay     *day,
				 GtkWidget  *dialog)
{
	DialogData   *data = DIALOG_GET_DATA (dialog);
	GtkListStore *store;
	GtkTreeIter   iter;

	store = GTK_LIST_STORE (gtk_tree_view_get_model (
					GTK_TREE_VIEW (data->tree_view)));

	/* We get the signal before the day is actually removed, so we can't
	 * just re-add all types, we need to find it and remove it.
	 */
	if (day_type_dialog_find_day (data, day, &iter)) {
		gtk_list_store_remove (store, &iter);
	}
}

static gboolean
day_type_dialog_find_day_foreach (GtkTreeModel *model,
				  GtkTreePath  *path,
				  GtkTreeIter  *iter,
				  FindDayData  *data)
{
	MrpDay *day;

	gtk_tree_model_get (model,
			    iter,
			    COL_DAY, &day,
			    -1);

	if (mrp_day_get_id (day) == mrp_day_get_id (data->day)) {
		data->found = TRUE;
		data->found_iter = *iter;
		return TRUE;
	}
	
	return FALSE;
}

static gboolean
day_type_dialog_find_day (DialogData  *data,
			  MrpDay      *day,
			  GtkTreeIter *iter)
{
	GtkTreeModel *model;
	FindDayData    find_data;

	find_data.found = FALSE;
	find_data.day = day;
	find_data.data = data;

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (data->tree_view));

	gtk_tree_model_foreach (model,
				(GtkTreeModelForeachFunc) day_type_dialog_find_day_foreach,
				&find_data);

	if (find_data.found) {
		*iter = find_data.found_iter;
		return TRUE;
	}

	return FALSE;
}

static MrpDay *
day_type_dialog_get_selected_day (DialogData *data)
{
	GtkTreeSelection *selection;
	GtkTreeModel     *model;
	GtkTreeIter       iter;
	MrpDay           *day;
	
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (data->tree_view));

	if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
		gtk_tree_model_get (model,
				    &iter,
				    COL_DAY, &day,
				    -1);
		return day;
	}

	return NULL;
}

/* Handle the little "new day type" dialog. */
static void
day_type_dialog_new_name_changed_cb (GtkEntry  *entry,
				     GtkWidget *button)
{
	const gchar *name;
	gboolean     sensitive;

	name = gtk_entry_get_text (entry);
	
	sensitive =  name[0] != 0;
	gtk_widget_set_sensitive (button, sensitive);
}

static void
day_type_dialog_new_dialog_run (DialogData *data)
{
	GladeXML    *glade;
	GtkWidget   *dialog;
	GtkWidget   *entry;
	GtkWidget   *button;
	const gchar *name;

	glade = glade_xml_new (GLADEDIR "/calendar-dialog.glade",
			       "new_day_dialog",
			       GETTEXT_PACKAGE);

	dialog = glade_xml_get_widget (glade, "new_day_dialog");

	button = glade_xml_get_widget (glade, "ok_button");

	entry = glade_xml_get_widget (glade, "name_entry");
	g_signal_connect (entry,
			  "changed",
			  G_CALLBACK (day_type_dialog_new_name_changed_cb),
			  button);

	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_OK) {
		name = gtk_entry_get_text (GTK_ENTRY (entry));

		day_type_cmd_add (data, name);
	}

	g_object_unref (glade);
	gtk_widget_destroy (dialog);
}


/* Commands */

static gboolean
day_type_cmd_add_do (PlannerCmd *cmd_base)
{
	DayTypeCmdAdd *cmd;

	cmd = (DayTypeCmdAdd *) cmd_base;

	cmd->day = mrp_day_add (cmd->project, cmd->name, "");

	return TRUE;
}

static void
day_type_cmd_add_undo (PlannerCmd *cmd_base)
{
	DayTypeCmdAdd *cmd;

	cmd = (DayTypeCmdAdd *) cmd_base;
	
	mrp_day_remove (cmd->project, cmd->day);
	cmd->day = NULL;
}

static void
day_type_cmd_add_free (PlannerCmd *cmd_base)
{
	DayTypeCmdAdd *cmd;

	cmd = (DayTypeCmdAdd *) cmd_base;

	g_free (cmd->name);
}

static PlannerCmd *
day_type_cmd_add (DialogData  *data,
		  const char  *name)
{
	PlannerCmd    *cmd_base;
	DayTypeCmdAdd *cmd;
	gchar         *undo_label;;

	undo_label = g_strdup_printf (_("Add day type \"%s\""), name);
	cmd_base = planner_cmd_new (DayTypeCmdAdd,
				    undo_label,
				    day_type_cmd_add_do,
				    day_type_cmd_add_undo,
				    day_type_cmd_add_free);
	g_free (undo_label);

	cmd = (DayTypeCmdAdd *) cmd_base;

	cmd->project = data->project;

	cmd->name = g_strdup (name);

	planner_cmd_manager_insert_and_do (planner_window_get_cmd_manager (data->main_window),
					   cmd_base);

	return cmd_base;
}

static gboolean
day_type_cmd_remove_do (PlannerCmd *cmd_base)
{
	DayTypeCmdRemove *cmd;

	cmd = (DayTypeCmdRemove *) cmd_base;

	mrp_day_remove (cmd->project, cmd->day);
	cmd->day = NULL;

	return TRUE;
}

static void
day_type_cmd_remove_undo (PlannerCmd *cmd_base)
{
	DayTypeCmdRemove *cmd;

	cmd = (DayTypeCmdRemove *) cmd_base;
	
	cmd->day = mrp_day_add (cmd->project, cmd->name, "");
}

static void
day_type_cmd_remove_free (PlannerCmd *cmd_base)
{
	DayTypeCmdRemove *cmd;

	cmd = (DayTypeCmdRemove *) cmd_base;

	g_free (cmd->name);
}

static PlannerCmd *
day_type_cmd_remove (DialogData *data,
		     MrpDay     *day)
{
	PlannerCmd       *cmd_base;
	DayTypeCmdRemove *cmd;
	gchar            *undo_label;;

	undo_label = g_strdup_printf (_("Remove day type \"%s\""), mrp_day_get_name (day));

	cmd_base = planner_cmd_new (DayTypeCmdRemove,
				    undo_label,
				    day_type_cmd_remove_do,
				    day_type_cmd_remove_undo,
				    day_type_cmd_remove_free);

	g_free (undo_label);

	cmd = (DayTypeCmdRemove *) cmd_base;

	cmd->project = data->project;

	cmd->day = day;
	cmd->name = g_strdup (mrp_day_get_name (day));

	planner_cmd_manager_insert_and_do (planner_window_get_cmd_manager (data->main_window),
					   cmd_base);
	
	return cmd_base;
}
