/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2002 CodeFactory AB
 * Copyright (C) 2002 Richard Hult <richard@imendio.com>
 * Copyright (C) 2002 Mikael Hallendal <micke@imendio.com>
 * Copyright (C) 2004 Imendio HB
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
#include <libplanner/mrp-day.h>
#include <libplanner/mrp-object.h>
#include <libplanner/mrp-project.h>
#include "planner-calendar.h"
#include "planner-working-time-dialog.h"
#include "planner-default-week-dialog.h"
#include "planner-calendar-dialog.h"


#define RESPONSE_ADD          1
#define RESPONSE_REMOVE       2
#define RESPONSE_WORKING_TIME 3
#define RESPONSE_DEFAULT_WEEK 4
#define RESPONSE_CLOSE        GTK_RESPONSE_CLOSE

enum {
	COL_CALENDAR,
	COL_NAME,
	NUM_COLS
};

typedef struct {
	PlannerWindow *main_window;
	MrpProject    *project;

	GtkWidget     *dialog;
	GtkWidget     *tree_view;
	GtkWidget     *remove_button;
	GtkWidget     *apply_button;
	GtkWidget     *default_week_button;
	GtkWidget     *working_time_button;

	GtkWidget     *name_label;
	
	GtkWidget     *calendar;
	GtkWidget     *option_menu;

	GtkWidget     *base_radiobutton;
	GtkWidget     *type_radiobutton;
	GtkWidget     *custom_radiobutton;

	GtkWidget     *from_entry[5];
	GtkWidget     *to_entry[5];
	
	/* Data for the little modal "new calendar" dialog. */
	GtkWidget     *new_ok_button;
	GtkWidget     *new_copy_radiobutton;
	GtkWidget     *new_derive_radiobutton;
	GtkWidget     *new_empty_radiobutton;

	/* The other dialogs that we launch from this one. */
	GtkWidget     *default_week_dialog;

	/* Keep this just so we can disconnect easily. */
	MrpCalendar   *connected_calendar;
} DialogData;


#define DIALOG_GET_DATA(d) g_object_get_data ((GObject*)d, "data")


static void          cal_dialog_setup_option_menu      (DialogData       *data);
static void          cal_dialog_option_menu_set_day    (GtkWidget        *option_menu,
							MrpDay           *day);
static MrpDay *      cal_dialog_option_menu_get_day    (GtkWidget        *option_menu);
static void          cal_dialog_response_cb            (GtkWidget        *dialog,
							gint              response,
							DialogData       *data);
static GtkTreeModel *cal_dialog_create_model           (MrpProject       *project,
							GtkTreeView      *tree_view);
static void          cal_dialog_setup_tree_view        (GtkTreeView      *tree_view,
							MrpProject       *project);
static MrpCalendar * cal_dialog_get_selected_calendar  (GtkTreeView      *tree_view);
static void          cal_dialog_calendar_changed_cb    (MrpCalendar      *calendar,
							DialogData       *data);
static void          cal_dialog_selection_changed_cb   (GtkTreeSelection *selection,
							DialogData       *data);
static void          cal_dialog_apply_clicked_cb       (GtkWidget        *button,
							DialogData       *data);
static void          cal_dialog_update_calendar_widgets (DialogData      *data);
static void          cal_dialog_month_changed_cb       (PlannerCalendar       *calendar,
							DialogData       *data);
static void          cal_dialog_date_selected_cb       (PlannerCalendar       *calendar,
							DialogData       *data);
static void          cal_dialog_option_menu_changed_cb (GtkWidget        *option_menu,
							DialogData       *data);

static void          cal_dialog_day_types_toggled_cb   (GtkWidget        *widget,
							DialogData       *data);
static void          cal_dialog_update_day_widgets     (DialogData       *data);
static void          cal_dialog_project_day_added_cb   (MrpProject       *project,
							MrpDay           *day,
							DialogData       *data);
static void          cal_dialog_project_day_removed_cb (MrpProject       *project,
							MrpDay           *day,
							DialogData       *data);
static void          cal_dialog_project_day_changed_cb (MrpProject       *project,
							MrpDay           *day,
							DialogData       *data);
static void          cal_dialog_new_dialog_run         (DialogData       *data);


typedef struct {
	PlannerCmd        base;

	MrpCalendar *calendar;
	MrpDay      *day;
	MrpDay      *old_day;
	mrptime      time;
} CalCmdDayType;

static gboolean
cal_cmd_day_type_do (PlannerCmd *cmd_base)
{
	CalCmdDayType *cmd = (CalCmdDayType*) cmd_base;

	if (g_getenv ("PLANNER_DEBUG_UNDO_CAL")) {
		g_message ("Changing calendar for day ...");
	}

	mrp_calendar_set_days (cmd->calendar, cmd->time, cmd->day, -1);

	return TRUE;
}

static void
cal_cmd_day_type_undo (PlannerCmd *cmd_base)
{
	CalCmdDayType *cmd = (CalCmdDayType*) cmd_base;

	mrp_calendar_set_days (cmd->calendar, cmd->time, cmd->old_day, -1);
}


static void
cal_cmd_day_type_free (PlannerCmd *cmd_base)
{
	CalCmdDayType *cmd = (CalCmdDayType*) cmd_base;

	g_object_unref (cmd->calendar);
	mrp_day_unref  (cmd->day);
	mrp_day_unref  (cmd->old_day);
}

static PlannerCmd *
planner_cal_cmd_day_type (PlannerWindow  *main_window,
			  MrpCalendar    *calendar,
			  MrpDay         *day,
			  mrptime         time)
{
	PlannerCmd    *cmd_base;
	CalCmdDayType *cmd;

	cmd_base = planner_cmd_new (CalCmdDayType,
				    _("Change day type"),
				    cal_cmd_day_type_do,
				    cal_cmd_day_type_undo,
				    cal_cmd_day_type_free);

	cmd = (CalCmdDayType*) cmd_base;

	cmd->calendar = g_object_ref (calendar);
	cmd->day = mrp_day_ref (day);
	cmd->old_day = mrp_day_ref (mrp_calendar_get_day (calendar, time, FALSE));
	cmd->time = time;
			
	planner_cmd_manager_insert_and_do (planner_window_get_cmd_manager 
					   (main_window),
					   cmd_base);
	return cmd_base;
}

typedef struct {
	PlannerCmd   base;

	MrpProject  *project;
	MrpCalendar *calendar;
	MrpCalendar *parent;
	/* Resources that use this calendar */
	GList       *resources;
	/* Calendar that inherits from this calendar */
	GList       *children;
} CalCmdRemove;

static gboolean
cal_cmd_remove_do (PlannerCmd *cmd_base)
{
	GList *all_resources, *r;

	CalCmdRemove *cmd = (CalCmdRemove*) cmd_base;

	if (g_getenv ("PLANNER_DEBUG_UNDO_CAL")) {
		g_message ("Removing a calendar ...");
	}

	all_resources = mrp_project_get_resources (cmd->project);
	for (r = all_resources; r; r = r->next) {
		MrpResource *resource = r->data;
		MrpCalendar *tmp_cal;
		
		tmp_cal = mrp_resource_get_calendar (resource);
		if (tmp_cal == cmd->calendar) {			
			cmd->resources = g_list_append (cmd->resources, 
							g_object_ref (resource));
		} 
	}

	cmd->children = g_list_copy (mrp_calendar_get_children (cmd->calendar));
	if (cmd->children) {
		g_list_foreach (cmd->children, (GFunc) g_object_ref, NULL);
	}
		
	mrp_calendar_remove (cmd->calendar);

	return TRUE;
}

/* Reassign resources and calendar childs */
static void
cal_cmd_remove_undo (PlannerCmd *cmd_base)
{
	GList        *r;
	GList        *c;
	CalCmdRemove *cmd = (CalCmdRemove*) cmd_base;

	mrp_calendar_add (cmd->calendar, cmd->parent);

	if (cmd->resources != NULL) {
		for (r = cmd->resources; r; r = r->next) {
			mrp_resource_set_calendar (r->data, cmd->calendar);
		}
		g_list_foreach (cmd->resources, (GFunc) g_object_unref, NULL);
		g_list_free (cmd->resources);
		cmd->resources = NULL;
	}

	if (cmd->children != NULL) {
		for (c = cmd->children; c; c = c->next) {
			mrp_calendar_reparent (cmd->calendar, c->data);
		}
		g_list_foreach (cmd->children, (GFunc) g_object_unref, NULL);
		g_list_free (cmd->children);
		cmd->children = NULL;
	}
}


static void
cal_cmd_remove_free (PlannerCmd *cmd_base)
{
	CalCmdRemove *cmd = (CalCmdRemove*) cmd_base;

	g_object_unref (cmd->calendar);
	g_object_unref (cmd->project);

	if (cmd->resources != NULL) {
		g_list_foreach (cmd->resources, (GFunc) g_object_unref, NULL);
		g_list_free (cmd->resources);
		cmd->resources = NULL;
	}
	if (cmd->children != NULL) {
		g_list_foreach (cmd->children, (GFunc) g_object_unref, NULL);
		g_list_free (cmd->children);
		cmd->children = NULL;
	}
}

static PlannerCmd *
planner_cal_cmd_remove (PlannerWindow  *main_window,
			MrpProject     *project,
			MrpCalendar    *calendar)
{
	PlannerCmd   *cmd_base;
	CalCmdRemove *cmd;

	cmd_base = planner_cmd_new (CalCmdRemove,
				    _("Remove calendar"),
				    cal_cmd_remove_do,
				    cal_cmd_remove_undo,
				    cal_cmd_remove_free);

	cmd = (CalCmdRemove*) cmd_base;

	cmd->project = g_object_ref (project);
	cmd->calendar = g_object_ref (calendar);
	cmd->parent = g_object_ref (mrp_calendar_get_parent (cmd->calendar));

	planner_cmd_manager_insert_and_do (planner_window_get_cmd_manager 
					   (main_window),
					   cmd_base);
	return cmd_base;
}

typedef struct {
	PlannerCmd   base;

	gchar       *name;
	MrpProject  *project;
	MrpCalendar *calendar;
	MrpCalendar *parent;
	MrpCalendar *copy;
} CalCmdAdd;

static gboolean
cal_cmd_add_do (PlannerCmd *cmd_base)
{
	CalCmdAdd *cmd = (CalCmdAdd*) cmd_base;

	if (g_getenv ("PLANNER_DEBUG_UNDO_CAL")) {
		g_message ("Adding a new calendar ...");
	}

	if (cmd->calendar == NULL) {
		if (cmd->parent != NULL && cmd->copy == NULL) {
			cmd->calendar = mrp_calendar_derive (cmd->name, cmd->parent);
			g_object_unref (cmd->parent);
			cmd->parent = NULL;
		} 
		else if (cmd->parent == NULL && cmd->copy != NULL) {
			cmd->calendar = mrp_calendar_copy (cmd->name, cmd->copy);
			g_object_unref (cmd->copy);
			cmd->copy = NULL;
		} 
		else if (cmd->parent == NULL && cmd->copy == NULL) {
			cmd->calendar = mrp_calendar_new (cmd->name, cmd->project);
			cmd->parent = g_object_ref (mrp_calendar_get_parent (cmd->calendar));
		} else {
			g_warning ("Incorrect use adding new calendar");
		}
		cmd->parent = g_object_ref (mrp_calendar_get_parent (cmd->calendar));
	} else {
		mrp_calendar_add (cmd->calendar, cmd->parent);
	}

	return TRUE;
}

static void
cal_cmd_add_undo (PlannerCmd *cmd_base)
{
	CalCmdAdd *cmd = (CalCmdAdd*) cmd_base;

	mrp_calendar_remove (cmd->calendar);
}


static void
cal_cmd_add_free (PlannerCmd *cmd_base)
{
	CalCmdAdd *cmd = (CalCmdAdd*) cmd_base;

	g_object_unref (cmd->calendar);
	g_object_unref (cmd->parent);
	g_object_unref (cmd->project);

	g_free (cmd->name);
}

static PlannerCmd *
planner_cal_cmd_add (PlannerWindow  *main_window,
		     const gchar    *name,
		     MrpCalendar    *parent,
		     MrpCalendar    *copy)
{
	PlannerCmd *cmd_base;
	CalCmdAdd  *cmd;

	cmd_base = planner_cmd_new (CalCmdAdd,
				    _("Add new calendar"),
				    cal_cmd_add_do,
				    cal_cmd_add_undo,
				    cal_cmd_add_free);

	cmd = (CalCmdAdd*) cmd_base;

	cmd->project = g_object_ref (planner_window_get_project (main_window));
	cmd->name = g_strdup (name);
	if (parent != NULL) {
		cmd->parent = g_object_ref (parent);
	}
	if (copy != NULL) {
		cmd->copy = g_object_ref (copy);
	}
			
	planner_cmd_manager_insert_and_do (planner_window_get_cmd_manager 
					   (main_window),
					   cmd_base);
	return cmd_base;
}

static void
cal_dialog_setup_option_menu (DialogData *data)
{
	GtkOptionMenu *option_menu;
	GList         *types, *l;
	GtkWidget     *menu;
	GtkWidget     *menu_item;
	const gchar   *name;
	MrpDay        *day;

	types = mrp_day_get_all (data->project);

	types = g_list_prepend (types, mrp_day_get_nonwork ());
	types = g_list_prepend (types, mrp_day_get_work ());

	option_menu = GTK_OPTION_MENU (data->option_menu);
	menu = gtk_option_menu_get_menu (option_menu);

	if (menu) {
		gtk_widget_destroy (menu);
	}

	menu = gtk_menu_new ();

	for (l = types; l; l = l->next) {
		day = l->data;

		name = mrp_day_get_name (day);

		menu_item = gtk_menu_item_new_with_label (name);
		gtk_widget_show (menu_item);
		gtk_menu_append (GTK_MENU (menu), menu_item);

		g_object_set_data (G_OBJECT (menu_item),
				   "data",
				   day);
	}

	gtk_widget_show (menu);
	gtk_option_menu_set_menu (option_menu, menu);

	g_list_free (types);
}

static void
cal_dialog_option_menu_set_day (GtkWidget *option_menu,
				MrpDay    *day)
{
	GtkWidget *menu;
	GtkWidget *item;
	GList     *children, *l;
	gint       i;

       	menu = gtk_option_menu_get_menu (GTK_OPTION_MENU (option_menu));

	children = GTK_MENU_SHELL (menu)->children;
	for (i = 0, l = children; l; i++, l = l->next) {
		item = l->data;

		if (day == g_object_get_data (G_OBJECT (item), "data")) {
			gtk_option_menu_set_history (GTK_OPTION_MENU (option_menu), i);
			break;
		}
	}
}

static MrpDay *
cal_dialog_option_menu_get_day (GtkWidget *option_menu)
{
	GtkWidget *menu;
	GtkWidget *item;
	
	menu = gtk_option_menu_get_menu (GTK_OPTION_MENU (option_menu));
	item = gtk_menu_get_active (GTK_MENU (menu));

	return g_object_get_data (G_OBJECT (item), "data");
}

static void
cal_dialog_response_cb (GtkWidget  *dialog,
			gint        response,
			DialogData *data)
{
	MrpCalendar *calendar;
	GtkWidget   *window;
	
	calendar = cal_dialog_get_selected_calendar (GTK_TREE_VIEW (data->tree_view));
	
	switch (response) {
	case RESPONSE_REMOVE:
		/* mrp_calendar_remove (calendar); */
		planner_cal_cmd_remove (data->main_window, data->project, calendar);
		break;

	case RESPONSE_ADD:
		cal_dialog_new_dialog_run (data);
		break;
		
	case RESPONSE_WORKING_TIME:
		window = planner_working_time_dialog_new (data->main_window, calendar);

		/* FIXME: Not very nice to have it modal, but I can't think of a
		 * better way right now. Could have a hashtable with calendars
		 * and make sure we only show one dialog per calendar, or only
		 * show one dialog and change the active calendar...
		 */
		gtk_window_set_transient_for (GTK_WINDOW (window),
					      GTK_WINDOW (data->dialog));
		gtk_window_set_modal (GTK_WINDOW (window), TRUE);
		gtk_widget_show (window);
		break;
		
	case RESPONSE_DEFAULT_WEEK:
		window = planner_default_week_dialog_new (data->main_window, calendar);

		/* FIXME: Not very nice to have it modal, but I can't think of a
		 * better way right now.
		 */
		gtk_window_set_transient_for (GTK_WINDOW (window),
					      GTK_WINDOW (data->dialog));
		gtk_window_set_modal (GTK_WINDOW (window), TRUE);
		gtk_widget_show (window);
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
cal_dialog_destroy_cb (GtkWidget *dialog, DialogData *data)
{
	g_signal_handlers_disconnect_by_func (data->project,
					      cal_dialog_project_day_added_cb,
					      data);
	g_signal_handlers_disconnect_by_func (data->project,
					      cal_dialog_project_day_removed_cb,
					      data);
	g_signal_handlers_disconnect_by_func (data->project,
					      cal_dialog_project_day_changed_cb,
					      data);

	if (data->connected_calendar) {
		g_signal_handlers_disconnect_by_func (data->connected_calendar,
						      cal_dialog_calendar_changed_cb,
						      data);
	}
}

static void
cal_dialog_parent_destroy_cb (GtkWidget *window, GtkWidget *dialog)
{
	gtk_widget_destroy (dialog);
}
	
GtkWidget *
planner_calendar_dialog_new (PlannerWindow *window)
{
	DialogData       *data;
	GladeXML         *glade;
	GtkWidget        *dialog;
	GtkWidget        *w;
	GtkTreeSelection *selection;
	gint              i;
	
	g_return_val_if_fail (PLANNER_IS_MAIN_WINDOW (window), NULL);
	
	glade = glade_xml_new (GLADEDIR "/calendar-dialog.glade",
			       "calendar_dialog",
			       GETTEXT_PACKAGE);
	if (!glade) {
		g_warning ("Could not create calendar dialog.");
		return NULL;
	}

	dialog = glade_xml_get_widget (glade, "calendar_dialog");
	
	data = g_new0 (DialogData, 1);

	data->project = planner_window_get_project (window);
	data->main_window = window;
	data->dialog = dialog;

	g_signal_connect_object (window,
				 "destroy",
				 G_CALLBACK (cal_dialog_parent_destroy_cb),
				 dialog,
				 0);
	
	data->tree_view = glade_xml_get_widget (glade, "treeview");
	data->calendar = planner_calendar_new ();
	gtk_widget_show (data->calendar);

	planner_calendar_display_options (PLANNER_CALENDAR (data->calendar),
					  PLANNER_CALENDAR_SHOW_HEADING |
					  PLANNER_CALENDAR_SHOW_DAY_NAMES |
					  PLANNER_CALENDAR_SHOW_WEEK_NUMBERS |
					  PLANNER_CALENDAR_WEEK_START_MONDAY);
	g_signal_connect (data->calendar,
			  "month_changed",
			  G_CALLBACK (cal_dialog_month_changed_cb),
			  data);	

	g_signal_connect (data->calendar,
			  "day_selected",
			  G_CALLBACK (cal_dialog_date_selected_cb),
			  data);	

	w = glade_xml_get_widget (glade, "calendar_frame");
	gtk_container_add (GTK_CONTAINER (w), data->calendar);

	data->remove_button = glade_xml_get_widget (glade, "remove_button");
	data->apply_button = glade_xml_get_widget (glade, "apply_button");

	g_signal_connect  (data->apply_button,
			   "clicked",
			   G_CALLBACK (cal_dialog_apply_clicked_cb),
			   data);
	
	data->default_week_button = glade_xml_get_widget (glade, "default_week_button");
	data->working_time_button = glade_xml_get_widget (glade, "working_time_button");

	/* Get the 5 from/to entries. */
	for (i = 0; i < 5; i++) {
		gchar *tmp;
		
		tmp = g_strdup_printf ("from%d_entry", i + 1);
		data->from_entry[i] = glade_xml_get_widget (glade, tmp);
		g_free (tmp);
		
		tmp = g_strdup_printf ("to%d_entry", i + 1);
		data->to_entry[i] = glade_xml_get_widget (glade, tmp);
		g_free (tmp);
	}
	
	data->option_menu = glade_xml_get_widget (glade, "optionmenu");
	cal_dialog_setup_option_menu (data);
	g_signal_connect  (data->option_menu,
			   "changed",
			   G_CALLBACK (cal_dialog_option_menu_changed_cb),
			   data);

	data->base_radiobutton = glade_xml_get_widget (glade, "base_radiobutton");
	g_signal_connect (data->base_radiobutton,
			  "toggled",
			  G_CALLBACK (cal_dialog_day_types_toggled_cb),
			  data);

	data->type_radiobutton = glade_xml_get_widget (glade, "type_radiobutton");
	g_signal_connect (data->type_radiobutton,
			  "toggled",
			  G_CALLBACK (cal_dialog_day_types_toggled_cb),
			  data);
		
	data->custom_radiobutton = glade_xml_get_widget (glade, "custom_radiobutton");
	g_signal_connect (data->custom_radiobutton,
			  "toggled",
			  G_CALLBACK (cal_dialog_day_types_toggled_cb),
			  data);
		
	g_object_set_data_full (G_OBJECT (dialog),
				"data", data,
				g_free);

	cal_dialog_setup_tree_view (GTK_TREE_VIEW (data->tree_view), data->project);

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (data->tree_view));
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_BROWSE);

	g_signal_connect (selection,
			  "changed",
			  G_CALLBACK (cal_dialog_selection_changed_cb),
			  data);

	g_signal_connect (data->project,
			  "day_added",
			  G_CALLBACK (cal_dialog_project_day_added_cb),
			  data);
	
	g_signal_connect (data->project,
			  "day_removed",
			  G_CALLBACK (cal_dialog_project_day_removed_cb),
			  data);
	
	g_signal_connect (data->project,
			  "day_changed",
			  G_CALLBACK (cal_dialog_project_day_changed_cb),
			  data);
	
	/* Set the sensitivity of the option menu and entries. */
	cal_dialog_update_day_widgets (data);

	g_signal_connect (dialog,
			  "response",
			  G_CALLBACK (cal_dialog_response_cb),
			  data);

	g_signal_connect (dialog,
			  "destroy",
			  G_CALLBACK (cal_dialog_destroy_cb),
			  data);

	return dialog;
}

static MrpCalendar *
cal_dialog_get_selected_calendar (GtkTreeView *tree_view)
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
cal_dialog_calendar_changed_cb (MrpCalendar *calendar,
				DialogData  *data)
{
	cal_dialog_update_calendar_widgets (data);
	cal_dialog_update_day_widgets (data);
}

static gboolean
foreach_count_calendars (GtkTreeModel *model,
			 GtkTreePath *path,
			 GtkTreeIter *iter,
			 gpointer data)
{
	gint *i = (gint *) data;

	(*i)++;

	return FALSE;
}

static void
cal_dialog_selection_changed_cb (GtkTreeSelection *selection,
				 DialogData       *data)
{
	MrpCalendar *calendar;
	gboolean     sensitive = FALSE;
	gint         num_calendars = 0;

	calendar = cal_dialog_get_selected_calendar (GTK_TREE_VIEW (data->tree_view));

	gtk_tree_model_foreach (gtk_tree_view_get_model (GTK_TREE_VIEW (data->tree_view)),
				foreach_count_calendars,
				&num_calendars);

	/* We don't allow the last calendar to be removed. Might come up with a
	 * better solution later, but this will have to do for now.
	 */
	if (calendar && num_calendars > 1) {
		sensitive = TRUE;
	}
	
	gtk_widget_set_sensitive (data->remove_button, sensitive);

	cal_dialog_update_calendar_widgets (data);
	cal_dialog_update_day_widgets (data);
	
	if (data->connected_calendar) {
		g_signal_handlers_disconnect_by_func (data->connected_calendar,
						      cal_dialog_calendar_changed_cb,
						      data);
		data->connected_calendar = NULL;
	}
	
	if (calendar) {
		data->connected_calendar = calendar;

		g_signal_connect (calendar,
				  "calendar-changed",
				  G_CALLBACK (cal_dialog_calendar_changed_cb),
				  data);
	}
}

static void
cal_dialog_apply_clicked_cb (GtkWidget  *button,
			     DialogData *data)
{
	MrpCalendar *calendar;
	guint        y, m, d;
	mrptime      t;
	MrpDay      *day;

	calendar = cal_dialog_get_selected_calendar (GTK_TREE_VIEW (data->tree_view));

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->base_radiobutton))) {
		/* Derived. */
		day = mrp_day_get_use_base ();
	}
	else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->type_radiobutton))) {
		/* Day type. */
		day = cal_dialog_option_menu_get_day (data->option_menu);
	} else {
		/* Custom. Not implemented yet. */
		day = NULL;
		return;
	}
	
	planner_calendar_get_date (PLANNER_CALENDAR (data->calendar), &y, &m, &d);
	t = mrp_time_compose (y, m + 1, d, 0, 0, 0);
	/* mrp_calendar_set_days (calendar, t, day, -1); */
	planner_cal_cmd_day_type (data->main_window, calendar, day, t);
}

static void
cal_dialog_setup_tree_view (GtkTreeView *tree_view,
			    MrpProject  *project)
{
	GtkTreeModel      *model;
	GtkCellRenderer   *cell;
	GtkTreeViewColumn *col;
	
	model = cal_dialog_create_model (project, tree_view);

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
cal_dialog_update_calendar_widgets (DialogData *data)
{
	MrpCalendar *calendar;
	PlannerCalendar  *calendar_widget;
	guint        y, m, d;
	mrptime      t;
	MrpDay      *day;
	
	calendar = cal_dialog_get_selected_calendar (GTK_TREE_VIEW (data->tree_view));
	if (!calendar) {
		gtk_widget_set_sensitive (data->calendar, FALSE);
		gtk_widget_set_sensitive (data->default_week_button, FALSE);
		gtk_widget_set_sensitive (data->working_time_button, FALSE);
		return;
	}

	calendar_widget = PLANNER_CALENDAR (data->calendar);

	gtk_widget_set_sensitive (data->calendar, TRUE);
	gtk_widget_set_sensitive (data->default_week_button, TRUE);
	gtk_widget_set_sensitive (data->working_time_button, TRUE);

	planner_calendar_get_date (calendar_widget, &y, &m, &d);

	for (d = 1; d <= 31; d++) {
		t = mrp_time_compose (y, m + 1, d, 0, 0, 0);
		
		day = mrp_calendar_get_day (calendar, t, TRUE);

		if (day == mrp_day_get_nonwork ()) {
			planner_calendar_mark_day (calendar_widget,
						   d,
						   PLANNER_CALENDAR_MARK_SHADE);
		}
		else if (day == mrp_day_get_work ()) {
			planner_calendar_mark_day (calendar_widget,
						   d,
						   PLANNER_CALENDAR_MARK_NONE);
		}
	}
}

static void
cal_dialog_month_changed_cb (PlannerCalendar *calendar_widget,
			     DialogData *data)
{
	cal_dialog_update_calendar_widgets (data);
	cal_dialog_update_day_widgets (data);
}

static void
cal_dialog_date_selected_cb (PlannerCalendar *calendar_widget,
			     DialogData *data)
{
	cal_dialog_update_day_widgets (data);
}

static void
cal_dialog_option_menu_changed_cb (GtkWidget  *option_menu,
				   DialogData *data)
{
	gtk_widget_set_sensitive (data->apply_button, TRUE);
}
	
static void
cal_dialog_day_types_toggled_cb (GtkWidget  *widget,
				 DialogData *data)
{
	gboolean sensitive;
	gint     i;

	sensitive = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->type_radiobutton));
	gtk_widget_set_sensitive (data->option_menu, sensitive);

	gtk_widget_set_sensitive (data->apply_button, TRUE);

	/* The custom entries. */
	sensitive = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->custom_radiobutton));
	for (i = 0; i < 5; i++) {
		gtk_widget_set_sensitive (data->from_entry[i], sensitive);
		gtk_widget_set_sensitive (data->to_entry[i], sensitive);
	}
}

static void
cal_dialog_update_day_widgets (DialogData *data)
{
	MrpDay      *day;
	GList       *ivals, *l;
	MrpCalendar *calendar, *root;
	PlannerCalendar  *calendar_widget;
	guint        y, m, d;
	mrptime      t;
	gint         i;

	/* This should always be insensitive after this call, since we either 1)
	 * don't have a calendar, or 2) this call will restore the widgets to
	 * match the calendar.
	 */
	gtk_widget_set_sensitive (data->apply_button, FALSE);

	calendar = cal_dialog_get_selected_calendar (GTK_TREE_VIEW (data->tree_view));
	if (!calendar) {
		gtk_widget_set_sensitive (data->base_radiobutton, FALSE);
		gtk_widget_set_sensitive (data->type_radiobutton, FALSE);
		gtk_widget_set_sensitive (data->option_menu, FALSE);
		gtk_widget_set_sensitive (data->custom_radiobutton, FALSE);

		return;
	}

	gtk_widget_set_sensitive (data->type_radiobutton, TRUE);

	g_signal_handlers_block_by_func (data->option_menu,
					 cal_dialog_option_menu_changed_cb,
					 data);
	g_signal_handlers_block_by_func (data->base_radiobutton,
					 cal_dialog_day_types_toggled_cb,
					 data);
	g_signal_handlers_block_by_func (data->type_radiobutton,
					 cal_dialog_day_types_toggled_cb,
					 data);
	g_signal_handlers_block_by_func (data->custom_radiobutton,
					 cal_dialog_day_types_toggled_cb,
					 data);
	
	/* Only make "use base" sensitive if the calendar has a parent. */
	root = mrp_project_get_root_calendar (data->project);
	if (root == mrp_calendar_get_parent (calendar)) {
		gtk_widget_set_sensitive (data->base_radiobutton, FALSE);
	} else {
		gtk_widget_set_sensitive (data->base_radiobutton, TRUE);
	}

	calendar_widget = PLANNER_CALENDAR (data->calendar);
	
	planner_calendar_get_date (calendar_widget, &y, &m, &d);
	t = mrp_time_compose (y, m + 1, d, 0, 0, 0);
	
	day = mrp_calendar_get_day (calendar, t, FALSE);

	if (day == mrp_day_get_use_base ()) {
		gtk_widget_set_sensitive (data->option_menu, FALSE);
		
		gtk_toggle_button_set_active (
			GTK_TOGGLE_BUTTON (data->base_radiobutton), TRUE);
	} else {
		gtk_widget_set_sensitive (data->option_menu, TRUE);
		
		cal_dialog_option_menu_set_day (data->option_menu, day);

		gtk_toggle_button_set_active (
			GTK_TOGGLE_BUTTON (data->type_radiobutton), TRUE);
	}

	calendar = cal_dialog_get_selected_calendar (GTK_TREE_VIEW (data->tree_view));

	if (day == mrp_day_get_use_base ()) {
		day = mrp_calendar_get_day (calendar, t, TRUE);
	}
	
	ivals = mrp_calendar_day_get_intervals (calendar, day, TRUE);
	
	for (i = 0; i < 5; i++) {
		gtk_entry_set_text (GTK_ENTRY (data->from_entry[i]), "");
		gtk_entry_set_text (GTK_ENTRY (data->to_entry[i]), "");
	}
	
	for (l = ivals, i = 0; l && i < 5; l = l->next, i++) {
		MrpInterval *ival;
		mrptime      start, end;
		gchar       *str;
		
		ival = l->data;
		
		mrp_interval_get_absolute (ival, 0, &start, &end);
		
		str = mrp_time_format ("%H:%M", start);
		gtk_entry_set_text (GTK_ENTRY (data->from_entry[i]), str);
		g_free (str);
		
		str = mrp_time_format ("%H:%M", end);
		gtk_entry_set_text (GTK_ENTRY (data->to_entry[i]), str);
		g_free (str);
	}

	g_signal_handlers_unblock_by_func (data->option_menu,
					   cal_dialog_option_menu_changed_cb,
					   data);
	g_signal_handlers_unblock_by_func (data->base_radiobutton,
					   cal_dialog_day_types_toggled_cb,
					   data);
	g_signal_handlers_unblock_by_func (data->type_radiobutton,
					   cal_dialog_day_types_toggled_cb,
					   data);
	g_signal_handlers_unblock_by_func (data->custom_radiobutton,
					   cal_dialog_day_types_toggled_cb,
					   data);
}

static void
cal_dialog_project_day_added_cb (MrpProject *project,
				 MrpDay     *day,
				 DialogData *data)
{
	MrpDay *selected_day;

	selected_day = cal_dialog_option_menu_get_day (data->option_menu);
	
	cal_dialog_setup_option_menu (data);

	/* Reselect the same day as before. */
	cal_dialog_option_menu_set_day (data->option_menu, selected_day);
}

static void
cal_dialog_project_day_removed_cb (MrpProject *project,
				   MrpDay     *day,
				   DialogData *data)
{
	GtkOptionMenu *option_menu;
	GtkWidget     *menu;
	GtkWidget     *item;
	GList         *children, *l;

	/* We get the signal before the day is actually removed, so we can't
	 * just re-add all types, we need to find it and remove it.
	 */

	option_menu = GTK_OPTION_MENU (data->option_menu);
	menu = gtk_option_menu_get_menu (option_menu);

	if (!menu) {
		return;
	}

	children = GTK_MENU_SHELL (menu)->children;
	for (l = children; l; l = l->next) {
		item = l->data;

		if (day == g_object_get_data (G_OBJECT (item), "data")) {
			gtk_widget_destroy (item);
			break;
		}
	}
}

static void
cal_dialog_project_day_changed_cb (MrpProject *project,
				   MrpDay     *day,
				   DialogData *data)
{
	g_print ("day changed\n");
}

static void
cal_dialog_build_tree (GtkTreeStore *store,
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
		cal_dialog_build_tree (store, &iter, l->data);
	}
}

static void
cal_dialog_tree_changed (MrpProject  *project,
			 MrpCalendar *root,
			 GtkTreeView *tree_view)
{
	GtkTreeStore *store;
	GList        *children, *l;

	g_return_if_fail (MRP_IS_PROJECT (project));
	g_return_if_fail (MRP_IS_CALENDAR (root));
	g_return_if_fail (GTK_IS_TREE_VIEW (tree_view));
	
	store = GTK_TREE_STORE (gtk_tree_view_get_model (tree_view));
	
	gtk_tree_store_clear (store);
	
	children = mrp_calendar_get_children (root);	
	for (l = children; l; l = l->next) {
		cal_dialog_build_tree (store, NULL, l->data);
	}

	gtk_tree_view_expand_all (tree_view);
}

static GtkTreeModel *
cal_dialog_create_model (MrpProject  *project,
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
		cal_dialog_build_tree (store, NULL, l->data);
	}

	g_signal_connect_object (project,
				 "calendar_tree_changed",
				 G_CALLBACK (cal_dialog_tree_changed),
				 tree_view,
				 0);
	
	return GTK_TREE_MODEL (store);
}

/* Handle the little "new calendar" dialog. */
static void
cal_dialog_new_selection_changed_cb (GtkTreeSelection *selection,
				     DialogData       *data)
{
	GtkTreeIter   iter;
	GtkTreeModel *model;
	MrpCalendar  *calendar = NULL;

	if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
		gtk_tree_model_get (model, &iter,
				    COL_CALENDAR, &calendar,
				    -1);
	}

	if (!calendar) {
		gtk_widget_set_sensitive (data->new_derive_radiobutton, FALSE);
		gtk_widget_set_sensitive (data->new_copy_radiobutton, FALSE);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->new_empty_radiobutton), TRUE);
	} else {
		gtk_widget_set_sensitive (data->new_derive_radiobutton, TRUE);
		gtk_widget_set_sensitive (data->new_copy_radiobutton, TRUE);
	}	
}

static void
cal_dialog_new_name_changed_cb (GtkEntry   *entry,
				DialogData *data)
{
	const gchar *name;
	gboolean     sensitive;

	name = gtk_entry_get_text (entry);
	
	sensitive =  name[0] != 0;
	gtk_widget_set_sensitive (data->new_ok_button, sensitive);
}

static void
cal_dialog_new_dialog_run (DialogData *data)
{
	GladeXML         *glade;
	GtkWidget        *dialog;
	MrpCalendar      *parent;
	GtkTreeSelection *selection;
	GtkWidget        *entry;
	GtkWidget        *tree_view;
	const gchar      *name;
	GtkTreePath      *path;
	
	glade = glade_xml_new (GLADEDIR "/calendar-dialog.glade",
			       "new_calendar_dialog",
			       GETTEXT_PACKAGE);

	dialog = glade_xml_get_widget (glade, "new_calendar_dialog");

	data->new_ok_button = glade_xml_get_widget (glade, "ok_button");

	entry = glade_xml_get_widget (glade, "name_entry");
	g_signal_connect (entry,
			  "changed",
			  G_CALLBACK (cal_dialog_new_name_changed_cb),
			  data);

	data->new_copy_radiobutton = glade_xml_get_widget (glade, "copy_radiobutton");
	data->new_derive_radiobutton = glade_xml_get_widget (glade, "derive_radiobutton");
	data->new_empty_radiobutton = glade_xml_get_widget (glade, "empty_radiobutton");
	
	tree_view = glade_xml_get_widget (glade, "treeview");
	cal_dialog_setup_tree_view (GTK_TREE_VIEW (tree_view), data->project);

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree_view));
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_BROWSE);

	g_signal_connect (selection,
			  "changed",
			  G_CALLBACK (cal_dialog_new_selection_changed_cb),
			  data);

	/* Of some reason, nothing is selected even though we have a calendar
	 * and mode is set to BROWSE. So we select the first one.
	 */
	path = gtk_tree_path_new_first ();
	gtk_tree_selection_select_path (selection, path);
	gtk_tree_path_free (path);
	
	if (!gtk_tree_selection_get_selected (selection, NULL, NULL)) {
		gtk_widget_set_sensitive (data->new_derive_radiobutton, FALSE);
		gtk_widget_set_sensitive (data->new_copy_radiobutton, FALSE);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->new_empty_radiobutton), TRUE);
	}
		
	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_OK) {
		name = gtk_entry_get_text (GTK_ENTRY (entry));

		parent = cal_dialog_get_selected_calendar (GTK_TREE_VIEW (tree_view));

		if (parent && gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->new_copy_radiobutton))) {
			/* calendar = mrp_calendar_copy (name, parent); */
			planner_cal_cmd_add (data->main_window, name, NULL, parent);
		}
		else if (parent && gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->new_derive_radiobutton))) {
			/* calendar = mrp_calendar_derive (name, parent); */
			planner_cal_cmd_add (data->main_window, name, parent, NULL);
		}
		else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->new_empty_radiobutton))) {
			/* calendar = mrp_calendar_new (name, data->project); */
			planner_cal_cmd_add (data->main_window, name, NULL, NULL);
			   
		}
	}

	g_object_unref (glade);
	gtk_widget_destroy (dialog);
}
