/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2003      Imendio HB
 * Copyright (C) 2001-2002 CodeFactory AB
 * Copyright (C) 2001-2002 Richard Hult <richard@imendio.com>
 * Copyright (C) 2001-2002 Mikael Hallendal <micke@imendio.com>
 * Copyright (C) 2001-2002 Alvaro del Castillo <acs@barrapunto.com>
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
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <glade/glade.h>
#include <gtk/gtk.h>
#include <libgnome/gnome-i18n.h>
#include <libplanner/mrp-project.h>
#include <libplanner/mrp-time.h>
#include "planner-calendar-selector.h"
#include "planner-format.h"
#include "planner-project-properties.h"

typedef struct {
	PlannerWindow *main_window;
	MrpProject    *project;
	GtkWidget     *dialog;
	GtkWidget     *name_entry;
	GtkWidget     *org_entry;
	GtkWidget     *manager_entry;
	GtkWidget     *start_entry;
	GtkWidget     *phase_option_menu;
	GtkWidget     *calendar_label;

	GtkTreeView   *properties_tree;
	GtkWidget     *add_property_button;
	GtkWidget     *remove_property_button;
} DialogData;

#define DIALOG_GET_DATA(d) g_object_get_data ((GObject*)d, "data")

static void     mpp_select_calendar_clicked_cb        (GtkWidget           *button,
						       GtkWidget           *dialog);
static void     mpp_name_set_from_widget              (GtkWidget           *dialog);
static gboolean mpp_name_focus_out_event_cb           (GtkWidget           *widget,
						       GdkEvent            *event,
						       GtkWidget           *dialog);
static void     mpp_project_name_notify_cb            (MrpProject          *project,
						       GParamSpec          *pspec,
						       GtkWidget           *dialog);
static void     mpp_org_set_from_widget               (GtkWidget           *dialog);
static gboolean mpp_org_focus_out_event_cb            (GtkWidget           *widget,
						       GdkEvent            *event,
						       GtkWidget           *dialog);
static void     mpp_project_org_notify_cb             (MrpProject          *project,
						       GParamSpec          *pspec,
						       GtkWidget           *dialog);
static void     mpp_project_manager_notify_cb         (MrpProject          *project,
						       GParamSpec          *pspec,
						       GtkWidget           *dialog);
static void     mpp_manager_set_from_widget           (GtkWidget           *dialog);
static gboolean mpp_manager_focus_out_event_cb        (GtkWidget           *widget,
						       GdkEvent            *event,
						       GtkWidget           *dialog);
static void     mpp_project_start_notify_cb           (MrpProject          *project,
						       GParamSpec          *pspec,
						       GtkWidget           *dialog);
static void     mpp_start_set_from_widget             (GtkWidget           *dialog);
static gboolean mpp_start_focus_out_event_cb          (GtkWidget           *widget,
						       GdkEvent            *event,
						       GtkWidget           *dialog);
static void     mpp_project_calendar_notify_cb        (MrpProject          *project,
						       GParamSpec          *pspec,
						       GtkWidget           *dialog);
static void     mpp_project_calendar_notify_cb        (MrpProject          *project,
						       GParamSpec          *pspec,
						       GtkWidget           *dialog);
static void     mpp_phase_option_menu_changed_cb      (GtkOptionMenu       *option_menu,
						       GtkWidget           *dialog);
static void     mpp_project_phases_notify_cb          (MrpProject          *project,
						       GParamSpec          *pspec,
						       GtkWidget           *dialog);
static void     mpp_phase_set_from_widget             (GtkWidget *dialog);

static void     mpp_project_phase_notify_cb          (MrpProject          *project,
						      GParamSpec          *pspec,
						      GtkWidget           *dialog);
static void     mpp_setup_phases                      (DialogData          *data);
static void     mpp_set_phase                         (DialogData          *data,
						       const gchar         *phase);
static void     mpp_setup_properties_list             (GtkWidget           *dialog);
static void     mpp_property_name_data_func           (GtkTreeViewColumn   *tree_column,
						       GtkCellRenderer     *cell,
						       GtkTreeModel        *tree_model,
						       GtkTreeIter         *iter,
						       GtkWidget           *dialog);
static void     mpp_property_value_data_func          (GtkTreeViewColumn   *tree_column,
						       GtkCellRenderer     *cell,
						       GtkTreeModel        *tree_model,
						       GtkTreeIter         *iter,
						       GtkWidget           *dialog);
static void     mpp_property_added                    (MrpProject          *project,
						       GType                object_type,
						       MrpProperty         *property,
						       GtkWidget           *dialog);
static void     mpp_property_removed                  (MrpProject          *project,
						       MrpProperty         *property,
						       GtkWidget           *dialog);
static void     mpp_add_property_button_clicked_cb    (GtkButton           *button,
						       GtkWidget           *dialog);
static void     mpp_remove_property_button_clicked_cb (GtkButton           *button,
						       GtkWidget           *dialog);
static void     mpp_property_value_edited             (GtkCellRendererText *cell,
						       gchar               *path_string,
						       gchar               *new_text,
						       GtkWidget           *dialog);



enum {
	COL_PROPERTY,
	NUM_OF_COLS
};


/*
 * Commands
 */

typedef enum {
	PROP_STRING,
	PROP_DATE,
	PROP_CALENDAR
} PropType;

typedef struct {
	PlannerCmd   base;

	MrpProject  *project;

	PropType     type;
	gchar       *property;  
	
	/* String */
	gchar       *str;
	gchar       *str_old;

	/* Date */
	mrptime      t;
	mrptime      t_old;
	
	/* This won't work... the calendar might be removed and re-added... so
	 * the pointer will not be the same.
	 */ 
	MrpCalendar *calendar;
	MrpCalendar *calendar_old;
} PropertyCmdEdit;

static void
property_cmd_edit_do (PlannerCmd *cmd_base)
{
	PropertyCmdEdit *cmd;

	cmd = (PropertyCmdEdit*) cmd_base;

	switch (cmd->type) {
	case PROP_STRING:
		g_object_set (cmd->project, cmd->property,
			      cmd->str, NULL);
		break;
		
	case PROP_DATE:
		g_object_set (cmd->project, cmd->property,
			      cmd->t, NULL);
		break;
		
	case PROP_CALENDAR:
		g_object_set (cmd->project, cmd->property,
			      cmd->calendar, NULL);
		break;
	}
}

static void
property_cmd_edit_undo (PlannerCmd *cmd_base)
{
	PropertyCmdEdit *cmd;

	cmd = (PropertyCmdEdit*) cmd_base;

	switch (cmd->type) {
	case PROP_STRING:
		g_object_set (cmd->project, cmd->property,
			      cmd->str_old, NULL);
		break;
		
	case PROP_DATE:
		g_object_set (cmd->project, cmd->property,
			      cmd->t_old, NULL);
		break;
		
	case PROP_CALENDAR:
		g_object_set (cmd->project, cmd->property,
			      cmd->calendar_old, NULL);
		break;
	}
}

static PlannerCmd *
property_cmd_edit (DialogData  *data,
		   const gchar *label,
		   PropType     type,
		   const gchar *property,
		   const gchar *str_value,
		   time_t       time_value,
		   MrpCalendar *calendar_value)
{
	PlannerCmd      *cmd_base;
	PropertyCmdEdit *cmd;

	cmd = g_new0 (PropertyCmdEdit, 1);

	switch (type) {
	case PROP_STRING:
		g_object_get (data->project, property, &cmd->str_old, NULL);
		if (str_value == NULL && cmd->str_old == NULL) {
			goto no_change;
		}
		
		if (str_value && cmd->str_old &&
		    strcmp (str_value, cmd->str_old) == 0) {
			goto no_change;
		}

		cmd->str = g_strdup (str_value);
		break;
		
	case PROP_DATE:
		g_object_get (data->project, property, &cmd->t_old, NULL);
		if (time_value == cmd->t_old) {
			goto no_change;
		}
		
		cmd->t = time_value;
		break;
		
	case PROP_CALENDAR:
		g_object_get (data->project, property, &cmd->calendar_old, NULL);
		if (calendar_value == cmd->calendar_old) {
			goto no_change;
		}

		cmd->calendar = calendar_value;
		break;
	}
	
	cmd_base = (PlannerCmd*) cmd;

	cmd_base->label = g_strdup (label);
	cmd_base->do_func = property_cmd_edit_do;
	cmd_base->undo_func = property_cmd_edit_undo;
	cmd_base->free_func = NULL; /* FIXME */

	cmd->project = data->project;
	cmd->type = type;
	cmd->property = g_strdup (property);

	planner_window_cmd_manager_insert_and_do (data->main_window, cmd_base);

	return cmd_base;

 no_change:
	g_free (cmd->str_old);
	g_free (cmd);

	return NULL;
}

static void
mpp_select_calendar_clicked_cb (GtkWidget *button,
				GtkWidget *dialog)
{
	DialogData  *data;
	GtkWidget   *selector;
	gint         response;
	MrpCalendar *calendar;
	
	data = DIALOG_GET_DATA (dialog);

	selector = planner_calendar_selector_new (
		data->main_window,
		_("Select a calendar to use for this project:"));

	response = gtk_dialog_run (GTK_DIALOG (selector));
	switch (response) {
	case GTK_RESPONSE_OK:
		calendar = planner_calendar_selector_get_calendar (selector);
		g_object_set (data->project, "calendar", calendar, NULL);
		break;
		
	case GTK_RESPONSE_DELETE_EVENT:
	case GTK_RESPONSE_CANCEL:
		break;

	default:
		break;
	}

	gtk_widget_destroy (selector);
}

static void
mpp_connect_to_project (MrpProject *project, GtkWidget *dialog)
{
	g_signal_connect_object (project, "notify::name",
				 G_CALLBACK (mpp_project_name_notify_cb),
				 dialog,
				 0);

	g_signal_connect_object (project, "notify::project-start",
				 G_CALLBACK (mpp_project_start_notify_cb),
				 dialog,
				 0);

	g_signal_connect_object (project, "notify::organization",
				 G_CALLBACK (mpp_project_org_notify_cb),
				 dialog,
				 0);

	g_signal_connect_object (project, "notify::manager",
				 G_CALLBACK (mpp_project_manager_notify_cb),
				 dialog,
				 0);

	g_signal_connect_object (project, "notify::calendar",
				 G_CALLBACK (mpp_project_calendar_notify_cb),
				 dialog,
				 0);

	g_signal_connect_object (project, "notify::phases",
				 G_CALLBACK (mpp_project_phases_notify_cb),
				 dialog,
				 0);

	g_signal_connect_object (project,
				 "notify::phase",
				 G_CALLBACK (mpp_project_phase_notify_cb),
				 dialog,
				 0);

	g_signal_connect_object (project,
				 "property_added",
				 G_CALLBACK (mpp_property_added),
				 dialog,
				 0);
	g_signal_connect_object (project,
				 "property_removed",
				 G_CALLBACK (mpp_property_removed),
				 dialog,
				 0);
}

static void  
mpp_project_name_notify_cb  (MrpProject *project,  
			     GParamSpec *pspec, 
			     GtkWidget  *dialog)
{
	DialogData *data;
	gchar      *name;

	data = DIALOG_GET_DATA (dialog);

	g_object_get (project, "name", &name, NULL);
	gtk_entry_set_text (GTK_ENTRY (data->name_entry), name);
	g_free (name);
}

static void
mpp_name_set_from_widget (GtkWidget *dialog)
{
	DialogData  *data;
	const gchar *str;

	data = DIALOG_GET_DATA (dialog);
	str = gtk_entry_get_text (GTK_ENTRY (data->name_entry));
	
	property_cmd_edit (data, _("Edit Project Name"), PROP_STRING, "name", str, 0, NULL);
}

static gboolean
mpp_name_focus_out_event_cb (GtkWidget *widget,
			     GdkEvent  *event,
			     GtkWidget *dialog)
{
	mpp_name_set_from_widget (dialog);
	
	return FALSE;
}

static void  
mpp_project_org_notify_cb (MrpProject *project,  
				    GParamSpec *pspec, 
				    GtkWidget *dialog)
{
	DialogData *data;
	gchar      *org;

	g_object_get (project, "organization", &org, NULL);
	data = DIALOG_GET_DATA (dialog);
	gtk_entry_set_text (GTK_ENTRY (data->org_entry), org);
	g_free (org);	
}

static void
mpp_org_set_from_widget (GtkWidget *dialog)
{
	DialogData  *data;
	const gchar *str;
	
	data = DIALOG_GET_DATA (dialog);
	str = gtk_entry_get_text (GTK_ENTRY (data->org_entry));
	
	property_cmd_edit (data, _("Edit Organization"), PROP_STRING, "organization", str, 0, NULL);
}

static gboolean
mpp_org_focus_out_event_cb (GtkWidget *widget,
			    GdkEvent  *event,
			    GtkWidget *dialog)
{
	mpp_org_set_from_widget (dialog);
	
	return FALSE;
}

static void  
mpp_project_manager_notify_cb (MrpProject *project,  
			       GParamSpec *pspec, 
			       GtkWidget  *dialog)
{	
	DialogData *data;
	gchar      *manager;

	g_object_get (project, "manager", &manager, NULL);
	data = DIALOG_GET_DATA (dialog);
	gtk_entry_set_text (GTK_ENTRY (data->manager_entry), manager);
	g_free (manager);
}

static void
mpp_manager_set_from_widget (GtkWidget *dialog)
{
	DialogData  *data;
	const gchar *str;
	
	data = DIALOG_GET_DATA (dialog);
	str = gtk_entry_get_text (GTK_ENTRY (data->manager_entry));
	
	property_cmd_edit (data, _("Edit Manager"), PROP_STRING, "manager", str, 0, NULL);
}

static gboolean
mpp_manager_focus_out_event_cb (GtkWidget *widget,
				GdkEvent  *event,
				GtkWidget *dialog)
{
	mpp_manager_set_from_widget (dialog);
	
	return FALSE;
}

static void
mpp_set_start (GtkWidget *dialog, mrptime start)
{
	DialogData  *data;
	gchar        buffer[256];
	struct tm   *tm;
	const gchar *format = "%x"; /* keep in variable get rid of warning */

	data = DIALOG_GET_DATA (dialog);

	tm = localtime (&start);

	if (!strftime (buffer, sizeof (buffer), format, tm)) {
		strcpy (buffer, "???");
	}

	gtk_entry_set_text (GTK_ENTRY (data->start_entry), buffer);	
}

static void
mpp_project_start_notify_cb (MrpProject *project,
			     GParamSpec *pspec, 
			     GtkWidget  *dialog)
{
	DialogData *data;
	mrptime     start;

	data = DIALOG_GET_DATA (dialog);

	start = mrp_project_get_project_start (data->project);
	mpp_set_start (dialog, start);
}

static void
mpp_start_set_from_widget (GtkWidget *dialog)
{
	DialogData  *data;
	const gchar *str;
	mrptime      start;
	GDate       *date;
	struct tm    tm;

	data = DIALOG_GET_DATA (dialog);

	str = gtk_entry_get_text (GTK_ENTRY (data->start_entry));

	date = g_date_new ();
	g_date_set_parse (date, str);

	if (!g_date_valid (date)) {
		return;
	}

	g_date_to_struct_tm (date, &tm);
	g_date_free (date);
	
	start = mrp_time_from_tm (&tm);

	property_cmd_edit (data, _("Edit Project Start"), PROP_DATE, "project-start", NULL, start, NULL);
}

static gboolean
mpp_start_focus_out_event_cb (GtkWidget *widget,
			      GdkEvent  *event,
			      GtkWidget *dialog)
{
	mpp_start_set_from_widget (dialog);
	
	return FALSE;
}

static void  
mpp_project_calendar_notify_cb (MrpProject *project,  
				GParamSpec *pspec, 
				GtkWidget  *dialog)
{	
	DialogData  *data;
	MrpCalendar *calendar;

	data = DIALOG_GET_DATA (dialog);

	calendar = mrp_project_get_calendar (project);
	
	gtk_label_set_text (GTK_LABEL (data->calendar_label),
			    mrp_calendar_get_name (calendar));
}

static void
mpp_phase_option_menu_changed_cb (GtkOptionMenu *option_menu,
				  GtkWidget     *dialog)
{
	mpp_phase_set_from_widget (dialog);
}

static void
mpp_project_phases_notify_cb (MrpProject  *project,  
			      GParamSpec  *pspec,
			      GtkWidget   *dialog)
{
	DialogData *data = DIALOG_GET_DATA (dialog);
	
	g_signal_handlers_block_by_func (data->phase_option_menu,
					 mpp_phase_option_menu_changed_cb,
					 dialog);
	
	mpp_setup_phases (data);

	g_signal_handlers_unblock_by_func (data->phase_option_menu,
					   mpp_phase_option_menu_changed_cb,
					   dialog);
}

static void
mpp_phase_set_from_widget (GtkWidget *dialog)
{
	DialogData  *data;
	GtkWidget    *menu;
	GtkWidget    *item;
	const gchar *phase;

	data = DIALOG_GET_DATA (dialog);

       	menu = gtk_option_menu_get_menu (GTK_OPTION_MENU (data->phase_option_menu));
	item = gtk_menu_get_active (GTK_MENU (menu));

	phase = g_object_get_data (G_OBJECT (item), "data");
	
	property_cmd_edit (data, _("Edit Project Phase"), PROP_STRING, "phase", phase, 0, NULL);
}

static void
mpp_project_phase_notify_cb (MrpProject  *project,  
			     GParamSpec  *pspec,
			     GtkWidget   *dialog)
{
	DialogData *data;
	gchar      *phase;

	data = DIALOG_GET_DATA (dialog);

	g_object_get (data->project, "phase", &phase, NULL);
	mpp_set_phase (data, phase);
	g_free (phase);
}

static void
mpp_setup_phases (DialogData *data)
{
	GtkOptionMenu *option_menu;
	GList         *phases, *l;
	GtkWidget     *menu;
	GtkWidget     *menu_item;
	gchar         *phase;
	
	phases = NULL;

	g_object_get (data->project, "phases", &phases, NULL);
	
	option_menu = GTK_OPTION_MENU (data->phase_option_menu);
	menu = gtk_option_menu_get_menu (option_menu);

	if (menu) {
		gtk_widget_destroy (menu);
	}

	menu = gtk_menu_new ();

	menu_item = gtk_menu_item_new_with_label (_("None"));
	gtk_widget_show (menu_item);
	gtk_menu_append (GTK_MENU (menu), menu_item);

	for (l = phases; l; l = l->next) {
		menu_item = gtk_menu_item_new_with_label (l->data);
		gtk_widget_show (menu_item);
		gtk_menu_append (GTK_MENU (menu), menu_item);
		
		g_object_set_data_full (G_OBJECT (menu_item),
					"data",
					g_strdup (l->data),
					g_free);
	}

	gtk_widget_show (menu);
	gtk_option_menu_set_menu (option_menu, menu);

	mrp_string_list_free (phases);

	g_object_get (data->project, "phase", &phase, NULL);
	mpp_set_phase (data, phase);
	g_free (phase);
}

static void
mpp_set_phase (DialogData  *data,
	       const gchar *phase)
{
	GtkOptionMenu *option_menu;
	GtkWidget     *menu;
	GtkWidget     *item;
	GList         *children, *l;
	gint           i;
	gchar         *name;

	option_menu = GTK_OPTION_MENU (data->phase_option_menu);

	if (phase) {
		menu = gtk_option_menu_get_menu (option_menu);
		
		children = GTK_MENU_SHELL (menu)->children;
		for (i = 0, l = children; l; i++, l = l->next) {
			item = l->data;
			
			name = g_object_get_data (G_OBJECT (item), "data");
			
			if (name && !strcmp (phase, name)) {
				gtk_option_menu_set_history (option_menu, i);
				return;
			}
		}
	}
	
	/* If we didn't match, set "None". */
	gtk_option_menu_set_history (option_menu, 0);
}

static void
mpp_setup_properties_list (GtkWidget *dialog)
{
	DialogData *data = DIALOG_GET_DATA (dialog);
	GList             *properties, *l;
	GtkTreeModel      *model;
	GtkTreeViewColumn *col;
	GtkCellRenderer   *cell;
	
	model = GTK_TREE_MODEL (
		gtk_list_store_new (NUM_OF_COLS, G_TYPE_POINTER));
	gtk_tree_view_set_model (data->properties_tree, model);
	
	properties = mrp_project_get_properties_from_type (data->project,
							   MRP_TYPE_PROJECT);
	
	for (l = properties; l; l = l->next) {
		MrpProperty *property = MRP_PROPERTY (l->data);
		GtkTreeIter  iter;
		
		if (!mrp_property_get_user_defined (property)) {
			continue;
		}
		
		gtk_list_store_append (GTK_LIST_STORE (model),
				       &iter);
		gtk_list_store_set (GTK_LIST_STORE (model),
				    &iter,
				    COL_PROPERTY, mrp_property_ref (property),
				    -1);
	}

	cell = gtk_cell_renderer_text_new ();
	g_object_set (cell, "editable", FALSE, NULL);
	
	col = gtk_tree_view_column_new_with_attributes (_("Property"), 
							cell, NULL);
	gtk_tree_view_column_set_cell_data_func (col, cell, 
						 (GtkTreeCellDataFunc) mpp_property_name_data_func,
						 dialog, NULL);

	gtk_tree_view_append_column (data->properties_tree, col);
	
	cell = gtk_cell_renderer_text_new ();
	g_object_set (cell, "editable", TRUE, NULL);
	
	col = gtk_tree_view_column_new_with_attributes (_("Value"),
							cell, NULL);

	gtk_tree_view_column_set_cell_data_func (col, cell, 
						 (GtkTreeCellDataFunc) mpp_property_value_data_func,
						 dialog, NULL);
	
	g_signal_connect (cell,
			  "edited",
			  G_CALLBACK (mpp_property_value_edited),
			  dialog);
	
	gtk_tree_view_append_column (data->properties_tree, col);
}

static void
mpp_property_name_data_func (GtkTreeViewColumn *tree_column,
			     GtkCellRenderer   *cell,
			     GtkTreeModel      *tree_model,
			     GtkTreeIter       *iter,
			     GtkWidget         *dialog)
{
	MrpProperty *property;
	
	gtk_tree_model_get (tree_model, iter, COL_PROPERTY, &property, -1);

	g_object_set (cell,
		      "text", mrp_property_get_name (property),
		      NULL);
}

static void
mpp_property_value_data_func (GtkTreeViewColumn *tree_column,
			      GtkCellRenderer   *cell,
			      GtkTreeModel      *tree_model,
			      GtkTreeIter       *iter,
			      GtkWidget         *dialog)
{
	DialogData      *data = DIALOG_GET_DATA (dialog);
	MrpProperty     *property;
	gchar           *svalue;
	MrpPropertyType  type;
	gint             ivalue;
	gfloat           fvalue;
/* 	mrptime          tvalue; */
	gint             work;
	
	gtk_tree_model_get (tree_model, iter, COL_PROPERTY, &property, -1);

	type = mrp_property_get_property_type (property);

	switch (type) {
	case MRP_PROPERTY_TYPE_STRING:
		mrp_object_get (data->project,
				mrp_property_get_name (property), &svalue,
				NULL);
		
		if (svalue == NULL) {
			svalue = g_strdup ("");
		}		

		break;
	case MRP_PROPERTY_TYPE_INT:
		mrp_object_get (data->project,
				mrp_property_get_name (property), &ivalue,
				NULL);
		svalue = g_strdup_printf ("%d", ivalue);
		break;

	case MRP_PROPERTY_TYPE_FLOAT:
		mrp_object_get (data->project,
				mrp_property_get_name (property), &fvalue,
				NULL);

		svalue = planner_format_float (fvalue, 4, FALSE);
		break;

	case MRP_PROPERTY_TYPE_DATE:
		svalue = g_strdup ("");
		
		
/* 		mrp_object_get (data->project, */
/* 				mrp_property_get_name (property), &tvalue, */
/* 				NULL);  */
/* 		svalue = planner_format_date (tvalue); */
		break;
		
	case MRP_PROPERTY_TYPE_DURATION:
		mrp_object_get (data->project,
				mrp_property_get_name (property), &ivalue,
				NULL); 

/*		work = mrp_calendar_day_get_total_work (
			mrp_project_get_calendar (tree->priv->project),
			mrp_day_get_work ());
*/
		work = 8*60*60;

		svalue = planner_format_duration (ivalue, work / (60*60));
		break;
		
	case MRP_PROPERTY_TYPE_COST:
		mrp_object_get (data->project,
				mrp_property_get_name (property), &fvalue,
				NULL); 

		svalue = planner_format_float (fvalue, 2, FALSE);
		break;
				
	default:
		g_warning ("Type not implemented.");
		break;
	}

	g_object_set (cell, "text", svalue, NULL);
	g_free (svalue);
}

static void
mpp_property_added (MrpProject  *project, 
		    GType        object_type,
		    MrpProperty *property,
		    GtkWidget   *dialog)
{
	DialogData   *data = DIALOG_GET_DATA (dialog);
	GtkTreeModel *model;
	GtkTreeIter   iter;
	
	g_return_if_fail (GTK_IS_DIALOG (dialog));
	
	model = gtk_tree_view_get_model (data->properties_tree);

	if (object_type != MRP_TYPE_PROJECT ||
	    !mrp_property_get_user_defined (property)) {
 		return;
	}
/*	
	if (gtk_tree_view_get_model (data->properties_tree) != model) {
		return;
	}
*/
	gtk_list_store_append (GTK_LIST_STORE (model), &iter);
	gtk_list_store_set (GTK_LIST_STORE (model),
			    &iter,
			    COL_PROPERTY, mrp_property_ref (property),
			    -1);
}

typedef struct {
	MrpProperty *property;
	GtkTreePath *found_path;
	GtkTreeIter *found_iter;
} PropertyFindData;

static gboolean
mpp_property_find (GtkTreeModel     *model,
		   GtkTreePath      *path,
		   GtkTreeIter      *iter,
		   PropertyFindData *data)
{
	MrpProperty *property;
	
	gtk_tree_model_get (model, iter,
			    COL_PROPERTY, &property,
			    -1);
	
	if (strcmp (mrp_property_get_name (data->property), 
		    mrp_property_get_name (property)) == 0) {
		data->found_path = gtk_tree_path_copy (path);
		data->found_iter = gtk_tree_iter_copy (iter);
		return TRUE;
	}

	return FALSE;
}

static void
mpp_property_removed (MrpProject  *project, 
		      MrpProperty *property,
		      GtkWidget   *dialog)
{
	DialogData       *data = DIALOG_GET_DATA (dialog);
	GtkTreeModel     *model;
	PropertyFindData *find_data;
	
	model = gtk_tree_view_get_model (data->properties_tree);
	
	find_data = g_new0 (PropertyFindData, 1);
	find_data->property = mrp_property_ref (property);
	find_data->found_path = NULL;
	find_data->found_iter = NULL;
	
	gtk_tree_model_foreach (model,
				(GtkTreeModelForeachFunc) mpp_property_find,
				find_data);

	mrp_property_unref (property);

	if (find_data->found_path) {
		gtk_list_store_remove (GTK_LIST_STORE (model),
				       find_data->found_iter);
		mrp_property_unref (property);
	
		gtk_tree_iter_free (find_data->found_iter);
		gtk_tree_path_free (find_data->found_path);
	}
	
	g_free (find_data);
}

static gboolean
mpp_property_dialog_label_changed_cb (GtkWidget *label_entry,
				      GdkEvent  *event,
				      GtkWidget *name_entry)
{
	const gchar *name;
	const gchar *label;

	name = gtk_entry_get_text (GTK_ENTRY (name_entry));

	if (name == NULL || name[0] == 0) {
		label = gtk_entry_get_text (GTK_ENTRY (label_entry));
		gtk_entry_set_text (GTK_ENTRY (name_entry), label);
	}

	return FALSE;
}

static void
mpp_property_dialog_setup_option_menu (GtkWidget     *option_menu,
				       GCallback      func,
				       gpointer       user_data,
				       gconstpointer  str1, ...)
{
	GtkWidget     *menu;
	GtkWidget     *menu_item;
	gint           i;
	va_list        args;
	gconstpointer  str;
	gint           type;

       	menu = gtk_option_menu_get_menu (GTK_OPTION_MENU (option_menu));
	if (menu) {
		gtk_widget_destroy (menu);
	}
	
	menu = gtk_menu_new ();

	va_start (args, str1);
	for (str = str1, i = 0; str != NULL; str = va_arg (args, gpointer), i++) {
		menu_item = gtk_menu_item_new_with_label (str);
		gtk_widget_show (menu_item);
		gtk_menu_append (GTK_MENU (menu), menu_item);

		type = va_arg (args, gint);
		
		g_object_set_data (G_OBJECT (menu_item),
				   "data",
				   GINT_TO_POINTER (type));
		if (func) {
			g_signal_connect (menu_item,
					  "activate",
					  func,
					  user_data);
		}
	}
	va_end (args);

	gtk_widget_show (menu);
	gtk_option_menu_set_menu (GTK_OPTION_MENU (option_menu), menu);
}

static gint
mpp_property_dialog_get_selected (GtkWidget *option_menu)
{
	GtkWidget *menu;
	GtkWidget *item;
	gint       ret;
	
	menu = gtk_option_menu_get_menu (GTK_OPTION_MENU (option_menu));
		
	item = gtk_menu_get_active (GTK_MENU (menu));

	ret = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (item), "data"));

	return ret;
}	

static void
mpp_property_dialog_type_selected_cb (GtkWidget *widget, GtkWidget *dialog)
{
	gint type;
	
	type = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget), "data"));

	g_object_set_data (G_OBJECT (dialog), "type", GINT_TO_POINTER (type));
}

static void
mpp_add_property_button_clicked_cb (GtkButton *button, GtkWidget *dialog)
{
	DialogData      *data = DIALOG_GET_DATA (dialog);
	MrpProperty     *property;
	MrpPropertyType  type;
	const gchar     *label;
	const gchar     *name;
	const gchar     *description;
	GladeXML        *glade;
	GtkWidget       *label_entry;
	GtkWidget       *name_entry;
	GtkWidget       *add_dialog;
	GtkWidget       *w;
	gint             response;
	gboolean         finished = FALSE;
	
	glade = glade_xml_new (GLADEDIR "/new-property.glade",
			       NULL,
			       NULL);
		
	add_dialog = glade_xml_get_widget (glade, "add_dialog");

	label_entry = glade_xml_get_widget (glade, "label_entry");
	name_entry = glade_xml_get_widget (glade, "name_entry");
	
	g_signal_connect (label_entry,
			  "focus_out_event",
			  G_CALLBACK (mpp_property_dialog_label_changed_cb),
			  name_entry);

	mpp_property_dialog_setup_option_menu (
		glade_xml_get_widget (glade, "type_menu"),
		G_CALLBACK (mpp_property_dialog_type_selected_cb),
		add_dialog,
		mrp_property_type_as_string (MRP_PROPERTY_TYPE_STRING),
		MRP_PROPERTY_TYPE_STRING,
		mrp_property_type_as_string (MRP_PROPERTY_TYPE_INT),
		MRP_PROPERTY_TYPE_INT,
		mrp_property_type_as_string (MRP_PROPERTY_TYPE_FLOAT),
		MRP_PROPERTY_TYPE_FLOAT,
/*  		mrp_property_type_as_string (MRP_PROPERTY_TYPE_DATE), */
/*  		MRP_PROPERTY_TYPE_DATE, */
/*  		mrp_property_type_as_string (MRP_PROPERTY_TYPE_DURATION), */
/*  		MRP_PROPERTY_TYPE_DURATION, */
/* 		mrp_property_type_as_string (MRP_PROPERTY_TYPE_COST), */
/*  		MRP_PROPERTY_TYPE_COST, */
		NULL);

	while (!finished) {
		response = gtk_dialog_run (GTK_DIALOG (add_dialog));

		switch (response) {
		case GTK_RESPONSE_OK:
			label = gtk_entry_get_text (GTK_ENTRY (label_entry));
			if (label == NULL || label[0] == 0) {
				finished = FALSE;
				break;
			}
			
			name = gtk_entry_get_text (GTK_ENTRY (name_entry));
			if (name == NULL || name[0] == 0) {
				finished = FALSE;
				break;
			}
			
			if (!isalpha(name[0])) {
				GtkWidget *dialog;
				
				dialog = gtk_message_dialog_new (GTK_WINDOW (add_dialog),
								 GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
								 GTK_MESSAGE_WARNING,
								 GTK_BUTTONS_OK,
								 _("The name of the custom property needs to start with a letter."));
				gtk_dialog_run (GTK_DIALOG (dialog));
				gtk_widget_destroy (dialog);
				
				finished = FALSE;
				break;
			}
		
			w = glade_xml_get_widget (glade, "description_entry");
			description = gtk_entry_get_text (GTK_ENTRY (w));
			
			w = glade_xml_get_widget (glade, "type_menu");
			
			type = mpp_property_dialog_get_selected (w);

			if (type != MRP_PROPERTY_TYPE_NONE) {
				property = mrp_property_new (name, 
							     type,
							     label,
							     description,
							     TRUE);

				mrp_project_add_property (data->project, 
							  MRP_TYPE_PROJECT,
							  property,
							  TRUE);	
			}

			finished = TRUE;
			break;
			
		case GTK_RESPONSE_DELETE_EVENT:
		case GTK_RESPONSE_CANCEL:
			finished = TRUE;
			break;
			
		default:
			break;
		}
	}
	
 	gtk_widget_destroy (add_dialog);
}

static void
mpp_remove_property_button_clicked_cb (GtkButton *button, GtkWidget *dialog)
{
	DialogData       *data = DIALOG_GET_DATA (dialog);
	GtkTreeModel     *model;
	GtkTreeSelection *selection;
	GtkTreeIter       iter;
	MrpProperty      *property;
	GtkWidget        *remove_dialog;
	gint              response;
	
	model = gtk_tree_view_get_model (data->properties_tree);
	
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (data->properties_tree));

	if (!gtk_tree_selection_get_selected (selection, NULL, &iter)) {
		return;
	}

	gtk_tree_model_get (model, &iter,
			    COL_PROPERTY, &property,
			    -1);

	remove_dialog = gtk_message_dialog_new (GTK_WINDOW (dialog),
						GTK_DIALOG_MODAL |
						GTK_DIALOG_DESTROY_WITH_PARENT,
						GTK_MESSAGE_QUESTION,
						GTK_BUTTONS_YES_NO,
						_("Do you really want to remove the property '%s' from "
						  "the project?"),
						mrp_property_get_name (property));
	
	response = gtk_dialog_run (GTK_DIALOG (remove_dialog));

	switch (response) {
	case GTK_RESPONSE_YES:
		mrp_project_remove_property (data->project,
					     MRP_TYPE_PROJECT,
					     mrp_property_get_name (property));
		break;

	case GTK_RESPONSE_DELETE_EVENT:
	case GTK_RESPONSE_CANCEL:
		break;

	default:
		break;
	}

	gtk_widget_destroy (remove_dialog);
}

static void
mpp_property_value_edited (GtkCellRendererText *cell,
			   gchar               *path_string,
			   gchar               *new_text,
			   GtkWidget           *dialog)
{
	DialogData      *data = DIALOG_GET_DATA (dialog);
	GtkTreePath     *path;
	GtkTreeIter      iter;
	GtkTreeModel    *model;
	MrpProperty     *property;
	MrpProject      *project;
	MrpPropertyType  type;
	gfloat           fvalue;
	
	model = gtk_tree_view_get_model (data->properties_tree);
	project = data->project;
	
	path = gtk_tree_path_new_from_string (path_string);
	gtk_tree_model_get_iter (model, &iter, path);

	gtk_tree_model_get (model, &iter,
			    COL_PROPERTY, &property,
			    -1);
	
	type = mrp_property_get_property_type (property);
	switch (type) {
	case MRP_PROPERTY_TYPE_STRING:
		mrp_object_set (MRP_OBJECT (project),
				mrp_property_get_name (property), 
				new_text,
				NULL);
		break;
	case MRP_PROPERTY_TYPE_INT:
		mrp_object_set (MRP_OBJECT (project),
				mrp_property_get_name (property), 
				atoi (new_text),
				NULL);
		break;
	case MRP_PROPERTY_TYPE_FLOAT:
		fvalue = g_ascii_strtod (new_text, NULL);
		mrp_object_set (MRP_OBJECT (project),
				mrp_property_get_name (property), 
				fvalue,
				NULL);
		break;

	case MRP_PROPERTY_TYPE_DURATION:
		/* FIXME: support reading units etc... */
		mrp_object_set (MRP_OBJECT (project),
				mrp_property_get_name (property), 
				atoi (new_text) *8*60*60,
				NULL);
		break;
		

	case MRP_PROPERTY_TYPE_DATE:
/* 		date = PLANNER_CELL_RENDERER_DATE (cell); */
/* 		mrp_object_set (MRP_OBJECT (project), */
/* 				mrp_property_get_name (property),  */
/* 				&(date->time), */
/* 				NULL); */
		break;
	case MRP_PROPERTY_TYPE_COST:
		fvalue = g_ascii_strtod (new_text, NULL);
		mrp_object_set (MRP_OBJECT (project),
				mrp_property_get_name (property), 
				fvalue,
				NULL);
		break;
	case MRP_PROPERTY_TYPE_STRING_LIST:
		/* FIXME: Should string-list still be around? */
		break;
	default:
		g_assert_not_reached ();
		break;
	}

	gtk_tree_path_free (path);
}

static void
mpp_parent_destroy_cb (GtkWidget *parent,
		       GtkWidget *dialog)
{
	gtk_widget_destroy (dialog);
}

static void
mpp_dialog_destroy_cb (GtkWidget *dialog,
		       gpointer   user_data)
{
	mpp_name_set_from_widget (dialog);
	mpp_org_set_from_widget (dialog);
	mpp_manager_set_from_widget (dialog);
	mpp_start_set_from_widget (dialog);
	mpp_phase_set_from_widget (dialog);

	/* FIXME: calendar */
}
	
GtkWidget *
planner_project_properties_new (PlannerWindow *window)
{
	GladeXML    *glade;
	GtkWidget   *dialog;
	GtkWidget   *w;
	DialogData  *data;
	mrptime      start;
	gchar       *name, *org, *manager;
	MrpCalendar *calendar;
	
	g_return_val_if_fail (PLANNER_IS_MAIN_WINDOW (window), NULL);
	
	glade = glade_xml_new (GLADEDIR "/project-properties.glade",
			       "project_properties",
			       GETTEXT_PACKAGE);
	if (!glade) {
		g_warning (_("Could not create properties dialog."));
		return NULL;
	}

	dialog = glade_xml_get_widget (glade, "project_properties");
	
	data = g_new0 (DialogData, 1);

	data->main_window = window;
	data->project = planner_window_get_project (window);
	data->dialog = dialog;

	g_signal_connect_object (glade_xml_get_widget (glade, "close_button"),
				 "clicked",
				 G_CALLBACK (mpp_parent_destroy_cb),
				 dialog,
				 0);
	
	g_signal_connect_object (window,
				 "destroy",
				 G_CALLBACK (mpp_parent_destroy_cb),
				 dialog,
				 0);

	g_signal_connect (dialog,
			  "destroy",
			  G_CALLBACK (mpp_dialog_destroy_cb),
			  NULL);
	
	data->name_entry = glade_xml_get_widget (glade, "entry_name");
	data->org_entry = glade_xml_get_widget (glade, "entry_org");
	data->manager_entry = glade_xml_get_widget (glade, "entry_manager");
	data->start_entry = glade_xml_get_widget (glade, "entry_start");
	data->phase_option_menu = glade_xml_get_widget (glade, "optionmenu_phase");
	data->calendar_label = glade_xml_get_widget (glade, "label_calendar");
	data->properties_tree = GTK_TREE_VIEW (
		glade_xml_get_widget (glade, "properties_tree"));
	
	data->add_property_button = glade_xml_get_widget (glade,
							  "add_property_button");
	data->remove_property_button = glade_xml_get_widget (glade,
							     "remove_property_button");

	start = mrp_project_get_project_start (data->project);

	g_object_set_data_full (G_OBJECT (dialog), "data", data, g_free);
	g_object_set_data (G_OBJECT (dialog), "project", data->project);
	
	g_object_get (data->project,
		      "name", &name,
		      "organization", &org,
		      "manager", &manager,
		      NULL);

	mpp_set_start (dialog, start);
	g_signal_connect (data->start_entry,
			  "focus_out_event",
			  G_CALLBACK (mpp_start_focus_out_event_cb),
			  dialog);
	
	gtk_entry_set_text (GTK_ENTRY (data->name_entry), name);
	g_signal_connect (data->name_entry,
			  "focus_out_event",
			  G_CALLBACK (mpp_name_focus_out_event_cb),
			  dialog);

	gtk_entry_set_text (GTK_ENTRY (data->org_entry), org);
	g_signal_connect (data->org_entry,
			  "focus_out_event",
			  G_CALLBACK (mpp_org_focus_out_event_cb),
			  dialog);
	
	gtk_entry_set_text (GTK_ENTRY (data->manager_entry), manager);
	g_signal_connect (data->manager_entry,
			  "focus_out_event",
			  G_CALLBACK (mpp_manager_focus_out_event_cb),
			  dialog);

	/* Project phase. */
	mpp_setup_phases (data);

	g_signal_connect (data->phase_option_menu,
			  "changed",
			  G_CALLBACK (mpp_phase_option_menu_changed_cb),
			  dialog);
	g_signal_connect (data->add_property_button,
			  "clicked",
			  G_CALLBACK (mpp_add_property_button_clicked_cb),
			  dialog);
	g_signal_connect (data->remove_property_button,
			  "clicked",
			  G_CALLBACK (mpp_remove_property_button_clicked_cb),
			  dialog);

	mpp_setup_properties_list (dialog);

	/* Project calendar. */
	calendar = mrp_project_get_calendar (data->project);

	gtk_label_set_text (GTK_LABEL (data->calendar_label),
			    mrp_calendar_get_name (calendar));

	w = glade_xml_get_widget (glade, "button_calendar");
	g_signal_connect (w,
			  "clicked",
			  G_CALLBACK (mpp_select_calendar_clicked_cb),
			  dialog);
	
	g_free (name);
	g_free (manager);
	g_free (org);

	mpp_connect_to_project (data->project, dialog);

	g_object_unref (glade);

	return dialog;
}

