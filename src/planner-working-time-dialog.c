/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2002-2003 CodeFactory AB
 * Copyright (C) 2002-2003 Richard Hult <richard@imendio.com>
 * Copyright (C) 2002 Mikael Hallendal <micke@imendio.com>
 * Copyright (C) 2004 Alvaro del Castillo <acs@barrapunto.com>
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
#include "planner-working-time-dialog.h"

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

	MrpCalendar   *calendar;

	GtkWidget     *dialog;
	GtkWidget     *tree_view;

	GtkWidget     *apply_button;
	
	GtkWidget     *from_entry[5];
	GtkWidget     *to_entry[5];
} DialogData;

typedef struct {
	DialogData    *data;
	MrpDay        *day;
	gboolean       found;
	GtkTreeIter    found_iter;
} FindDayData;

#define DIALOG_GET_DATA(d) g_object_get_data ((GObject*)d, "data")


static void          working_time_dialog_response_cb          (GtkWidget        *dialog,
							       gint              response,
							       DialogData       *data);
static void          working_time_dialog_build_list           (DialogData       *data);
static GtkTreeModel *working_time_dialog_create_model         (DialogData       *data);
static void          working_time_dialog_type_added_cb        (MrpProject       *project,
							       MrpDay           *day,
							       GtkWidget        *dialog);
static void          working_time_dialog_type_removed_cb      (MrpProject       *project,
							       MrpDay           *day,
							       GtkWidget        *dialog);
static gboolean      working_time_dialog_find_day             (DialogData       *data,
							       MrpDay           *day,
							       GtkTreeIter      *iter);
static MrpDay *      working_time_dialog_get_selected_day     (DialogData       *data);
static void          working_time_dialog_selection_changed_cb (GtkTreeSelection *selection,
							       DialogData       *data);
static void          working_time_dialog_apply                (DialogData       *data);

static void          working_time_dialog_entries_changed_cb   (GtkEntry         *entry,
							       DialogData       *data);


typedef struct {
	PlannerCmd   base;

	MrpDay      *day;
	MrpCalendar *calendar;
	GList       *ivals;
	GList       *old_ivals;
} WorkingTimeCmdEdit;

static gboolean
working_time_cmd_edit_do (PlannerCmd *cmd_base)
{
	WorkingTimeCmdEdit *cmd = (WorkingTimeCmdEdit*) cmd_base;

	mrp_calendar_day_set_intervals (cmd->calendar, cmd->day, cmd->ivals);

	return TRUE;
}

static void
working_time_cmd_edit_undo (PlannerCmd *cmd_base)
{
	WorkingTimeCmdEdit *cmd = (WorkingTimeCmdEdit*) cmd_base;

	mrp_calendar_day_set_intervals (cmd->calendar, cmd->day, cmd->old_ivals);
}

static void
working_time_cmd_edit_free (PlannerCmd *cmd_base)
{
	WorkingTimeCmdEdit *cmd = (WorkingTimeCmdEdit*) cmd_base;

	g_list_foreach (cmd->ivals, (GFunc) mrp_interval_unref, NULL);
	g_list_free (cmd->ivals);
	cmd->ivals = NULL;

	g_list_foreach (cmd->old_ivals, (GFunc) mrp_interval_unref, NULL);
	g_list_free (cmd->old_ivals);
	cmd->old_ivals = NULL;

	mrp_day_unref (cmd->day);
	g_object_unref (cmd->calendar);
}

static PlannerCmd *
working_time_cmd_edit (PlannerWindow   *main_window,
		       MrpCalendar     *calendar,
		       MrpDay          *day,
		       GList           *ivals)
{
	PlannerCmd          *cmd_base;
	WorkingTimeCmdEdit  *cmd;

	cmd_base = planner_cmd_new (WorkingTimeCmdEdit,
				    _("Edit working time"),
				    working_time_cmd_edit_do,
				    working_time_cmd_edit_undo,
				    working_time_cmd_edit_free);
	
	cmd = (WorkingTimeCmdEdit *) cmd_base;

	cmd->calendar = g_object_ref (calendar);
	cmd->day = mrp_day_ref (day);

	cmd->ivals = g_list_copy (ivals);
	g_list_foreach (ivals, (GFunc) mrp_interval_ref, NULL);

	cmd->old_ivals = g_list_copy (mrp_calendar_day_get_intervals 
				      (cmd->calendar, cmd->day, TRUE));
	g_list_foreach (cmd->old_ivals, (GFunc) mrp_interval_ref, NULL);
			
	planner_cmd_manager_insert_and_do (planner_window_get_cmd_manager 
					   (main_window),
					   cmd_base);
	return cmd_base;
}


static void
working_time_dialog_response_cb (GtkWidget  *dialog,
				 gint        response,
				 DialogData *data)
{
	switch (response) {
	case RESPONSE_APPLY:
		working_time_dialog_apply (data);
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
working_time_dialog_parent_destroy_cb (GtkWidget *window, GtkWidget *dialog)
{
	gtk_widget_destroy (dialog);
}

GtkWidget *
planner_working_time_dialog_new (PlannerWindow *window,
			    MrpCalendar  *calendar)
{
	DialogData        *data;
	GladeXML          *glade;
	GtkWidget         *dialog;
	gint               i;
	GtkTreeModel      *model;
	GtkCellRenderer   *cell;
	GtkTreeViewColumn *col;
	GtkTreeSelection  *selection;
	
	g_return_val_if_fail (PLANNER_IS_MAIN_WINDOW (window), NULL);
	
	glade = glade_xml_new (GLADEDIR "/calendar-dialog.glade",
			       "working_time_dialog",
			       GETTEXT_PACKAGE);
	if (!glade) {
		g_warning ("Could not create working_time dialog.");
		return NULL;
	}

	dialog = glade_xml_get_widget (glade, "working_time_dialog");
	
	data = g_new0 (DialogData, 1);

	data->main_window = window;
	data->project = planner_window_get_project (window);
	data->calendar = calendar;
	data->dialog = dialog;
	data->apply_button = glade_xml_get_widget (glade, "apply_button");

	g_signal_connect_object (window,
				 "destroy",
				 G_CALLBACK (working_time_dialog_parent_destroy_cb),
				 dialog,
				 0);
	
	g_signal_connect_object (data->project,
				 "day_added",
				 G_CALLBACK (working_time_dialog_type_added_cb),
				 data->dialog,
				 0);

	g_signal_connect_object (data->project,
				 "day_removed",
				 G_CALLBACK (working_time_dialog_type_removed_cb),
				 data->dialog,
				 0);
	
	data->tree_view = glade_xml_get_widget (glade, "treeview");

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (data->tree_view));
	
	g_signal_connect (selection,
			  "changed",
			  G_CALLBACK (working_time_dialog_selection_changed_cb),
			  data);
	
	/* Get the 5 from/to entries. */
	for (i = 0; i < 5; i++) {
		gchar *tmp;

		tmp = g_strdup_printf ("from%d_entry", i + 1);
		data->from_entry[i] = glade_xml_get_widget (glade, tmp);
		g_free (tmp);

		tmp = g_strdup_printf ("to%d_entry", i + 1);
		data->to_entry[i] = glade_xml_get_widget (glade, tmp);
		g_free (tmp);

		g_signal_connect (data->from_entry[i],
				  "changed",
				  G_CALLBACK (working_time_dialog_entries_changed_cb),
				  data);

		g_signal_connect (data->to_entry[i],
				  "changed",
				  G_CALLBACK (working_time_dialog_entries_changed_cb),
				  data);
	}

	g_object_set_data_full (G_OBJECT (dialog),
				"data", data,
				g_free);
	
	model = working_time_dialog_create_model (data);
	gtk_tree_view_set_model (GTK_TREE_VIEW (data->tree_view), model);

	working_time_dialog_build_list (data);
	
	cell = gtk_cell_renderer_text_new ();
	col = gtk_tree_view_column_new_with_attributes (
		NULL,
		cell,
		"text", COL_NAME,
		NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (data->tree_view), col);
	
	g_signal_connect (dialog,
			  "response",
			  G_CALLBACK (working_time_dialog_response_cb),
			  data);

	return dialog;
}

static void
working_time_dialog_build_list (DialogData *data)
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
working_time_dialog_create_model (DialogData *data)
{
	GtkListStore *store;
	
	store = gtk_list_store_new (NUM_COLS,
				    G_TYPE_STRING,
				    G_TYPE_INT,
				    G_TYPE_POINTER);

	return GTK_TREE_MODEL (store);
}

static void
working_time_dialog_type_added_cb (MrpProject *project,
				   MrpDay     *day,
				   GtkWidget  *dialog)
{
	DialogData *data = DIALOG_GET_DATA (dialog);

	/* Just rebuild the list of day types. */
	working_time_dialog_build_list (data);
}

static void
working_time_dialog_type_removed_cb (MrpProject *project,
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
	if (working_time_dialog_find_day (data, day, &iter)) {
		gtk_list_store_remove (store, &iter);
	}
}

static gboolean
working_time_dialog_find_day_foreach (GtkTreeModel *model,
				      GtkTreePath  *path,
				      GtkTreeIter  *iter,
				      FindDayData  *data)
{
	MrpDay *day;

	gtk_tree_model_get (model,
			    iter,
			    COL_DAY, &day,
			    -1);

	if (day == data->day) {
		data->found = TRUE;
		data->found_iter = *iter;
		return TRUE;
	}
	
	return FALSE;
}

static gboolean
working_time_dialog_find_day (DialogData  *data,
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
				(GtkTreeModelForeachFunc) working_time_dialog_find_day_foreach,
				&find_data);

	if (find_data.found) {
		*iter = find_data.found_iter;
		return TRUE;
	}

	return FALSE;
}

static MrpDay *
working_time_dialog_get_selected_day (DialogData *data)
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

static void
working_time_dialog_update_times (DialogData *data)
{
	MrpDay      *day;
	GList       *ivals, *l;
	MrpInterval *ival;
	mrptime      start, end;
	gint         i;
	gchar       *str;
	
	day = working_time_dialog_get_selected_day (data);

	ivals = mrp_calendar_day_get_intervals (data->calendar, day, TRUE);

	for (i = 0; i < 5; i++) {
		gtk_entry_set_text (GTK_ENTRY (data->from_entry[i]), "");
		gtk_entry_set_text (GTK_ENTRY (data->to_entry[i]), "");
	}
	
	for (l = ivals, i = 0; l && i < 5; l = l->next, i++) {
		ival = l->data;
		
		mrp_interval_get_absolute (ival, 0, &start, &end);
		
		str = mrp_time_format ("%R", start);
		gtk_entry_set_text (GTK_ENTRY (data->from_entry[i]), str);
		g_free (str);
		
		str = mrp_time_format ("%R", end);
		gtk_entry_set_text (GTK_ENTRY (data->to_entry[i]), str);
		g_free (str);
	}

	/* Only have the button sensitive after something is changed. */
	gtk_widget_set_sensitive (data->apply_button, FALSE);
}

static void
working_time_dialog_selection_changed_cb (GtkTreeSelection *selection,
					  DialogData       *data)
{
	working_time_dialog_update_times (data);
}

static void
working_time_dialog_apply (DialogData *data)
{
	MrpDay      *day;
	GList       *ivals;
	MrpInterval *ival;
	gint         i;
	const gchar *str;
	glong        hour, min;
	mrptime      start, end;

	day = working_time_dialog_get_selected_day (data);

	/* FIXME: use locale information to get the time separator. See #412. */
	
	ivals = NULL;
	for (i = 0; i < 5; i++) {
		str = gtk_entry_get_text (GTK_ENTRY (data->from_entry[i]));

		if (!str || !str[0]) {
			continue;
		}

		min = 0;
		if (sscanf (str, "%ld:%ld", &hour, &min) != 2) {
			if (sscanf (str, "%ld.%ld", &hour, &min) != 2) {
				if (sscanf (str, "%ld", &hour) != 1) {
					continue;
				}
			}
		}
		start = hour * 60*60 + min * 60;

		str = gtk_entry_get_text (GTK_ENTRY (data->to_entry[i]));

		if (!str || !str[0]) {
			continue;
		}

		min = 0;
		if (sscanf (str, "%ld:%ld", &hour, &min) != 2) {
			if (sscanf (str, "%ld.%ld", &hour, &min) != 2) {
				if (sscanf (str, "%ld", &hour) != 1) {
					continue;
				}
			}
		}

		end = hour * 60*60 + min * 60;

		if (end > start) {
			if (end == 60*60*24) {
				/* Interpret 24:00 as the last second of the
				 * day. Not sure that it's right, but it
				 * prevents us from storing invalid intervals (0
				 * to 0).
				 */
				end--;
			}
			
			ival = mrp_interval_new (start, end);
			ivals = g_list_append (ivals, ival);
		}
	}

	/* mrp_calendar_day_set_intervals (data->calendar, day, ivals); */
	working_time_cmd_edit (data->main_window, data->calendar, day, ivals);		     

	g_list_foreach (ivals, (GFunc) mrp_interval_unref, NULL);
	g_list_free (ivals);

	working_time_dialog_update_times (data);
}

static void
working_time_dialog_entries_changed_cb (GtkEntry   *entry,
					DialogData *data)
{
	gtk_widget_set_sensitive (data->apply_button, TRUE);
}
