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
#include "planner-default-week-dialog.h"

#define RESPONSE_ADD    1
#define RESPONSE_REMOVE 2
#define RESPONSE_CLOSE  GTK_RESPONSE_CLOSE
#define RESPONSE_APPLY  GTK_RESPONSE_APPLY

/* FIXME: Move to mrp-time and use locale. */
static struct {
	gint   day;
	gchar *name;
} days[] = {
	{ MRP_CALENDAR_DAY_MON, N_("Monday") },
	{ MRP_CALENDAR_DAY_TUE, N_("Tuesday") },
	{ MRP_CALENDAR_DAY_WED, N_("Wednesday") },
	{ MRP_CALENDAR_DAY_THU, N_("Thursday") },
	{ MRP_CALENDAR_DAY_FRI, N_("Friday") },
	{ MRP_CALENDAR_DAY_SAT, N_("Saturday") },
	{ MRP_CALENDAR_DAY_SUN, N_("Sunday") }
};

enum {
	COL_NAME,
	COL_ID,
	NUM_COLS
};

typedef struct {
	PlannerWindow *main_window;
	MrpProject    *project;

	MrpCalendar   *calendar;

	GtkWidget     *dialog;
	GtkWidget     *weekday_option_menu;
	GtkWidget     *day_option_menu;

	GtkWidget     *from_label[5];
	GtkWidget     *to_label[5];
	GtkWidget     *dash_label[5];
} DialogData;

typedef struct {
	PlannerCmd   base;

	MrpProject  *project;
	MrpCalendar *calendar;

	gint         weekday;

	/* If work/nonwork/use base, we use the day, otherwise the ID. */
	MrpDay      *day;
	gint         day_id;

	/* If work/nonwork/use base, we use the day, otherwise the ID. */
	MrpDay      *old_day;
	gint         old_day_id;
} DefaultWeekCmdEdit;


#define DIALOG_GET_DATA(d) g_object_get_data ((GObject*)d, "data")

static void        default_week_dialog_response_cb               (GtkWidget     *dialog,
								  gint           response,
								  DialogData    *data);
static void        default_week_dialog_update_labels             (DialogData    *data);
static void        default_week_dialog_weekday_selected_cb       (GtkOptionMenu *option_menu,
								  DialogData    *data);
static void        default_week_dialog_day_selected_cb           (GtkOptionMenu *option_menu,
								  DialogData    *data);
static void        default_week_dialog_setup_day_option_menu     (GtkOptionMenu *option_menu,
								  MrpProject    *project,
								  MrpCalendar   *calendar);
static void        default_week_dialog_setup_weekday_option_menu (GtkOptionMenu *option_menu);
static gint        default_week_dialog_get_selected_weekday      (DialogData    *data);
static MrpDay *    default_week_dialog_get_selected_day          (DialogData    *data);
static void        default_week_dialog_set_selected_day          (DialogData    *data,
								  MrpDay        *day);
static PlannerCmd *default_week_cmd_edit                         (DialogData    *data,
								  gint           weekday,
								  MrpDay        *day);


static void
default_week_dialog_response_cb (GtkWidget  *dialog,
				 gint        response,
				 DialogData *data)
{
	MrpDay *day;
	gint    weekday;
	
	switch (response) {
	case RESPONSE_APPLY:
		weekday = default_week_dialog_get_selected_weekday (data);
		day = default_week_dialog_get_selected_day (data);

		default_week_cmd_edit (data, weekday, day);
		
/*		mrp_calendar_set_default_days (data->calendar,
					       weekday, day,
					       -1);
*/
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
default_week_dialog_parent_destroy_cb (GtkWidget *window, GtkWidget *dialog)
{
	gtk_widget_destroy (dialog);
}

GtkWidget *
planner_default_week_dialog_new (PlannerWindow *window,
				 MrpCalendar  *calendar)
{
	DialogData *data;
	GladeXML   *glade;
	GtkWidget  *dialog;
	GtkWidget  *w;
	gint        i;
	
	g_return_val_if_fail (PLANNER_IS_MAIN_WINDOW (window), NULL);
	
	glade = glade_xml_new (GLADEDIR "/calendar-dialog.glade",
			       "default_week_dialog",
			       GETTEXT_PACKAGE);
	if (!glade) {
		g_warning ("Could not create default_week dialog.");
		return NULL;
	}

	dialog = glade_xml_get_widget (glade, "default_week_dialog");
	
	data = g_new0 (DialogData, 1);

	data->main_window = window;
	data->calendar = calendar;
	data->project = planner_window_get_project (window);
	data->dialog = dialog;

	g_signal_connect_object (window,
				 "destroy",
				 G_CALLBACK (default_week_dialog_parent_destroy_cb),
				 dialog,
				 0);
	
	/* Get the from/to labels. */
	for (i = 0; i < 5; i++) {
		gchar *tmp;
		
		tmp = g_strdup_printf ("from%d_label", i + 1);
		data->from_label[i] = glade_xml_get_widget (glade, tmp);
		g_free (tmp);
		
		tmp = g_strdup_printf ("to%d_label", i + 1);
		data->to_label[i] = glade_xml_get_widget (glade, tmp);
		g_free (tmp);

		tmp = g_strdup_printf ("dash%d_label", i + 1);
		data->dash_label[i] = glade_xml_get_widget (glade, tmp);
		g_free (tmp);
	}

	w = glade_xml_get_widget (glade, "name_label");
	gtk_label_set_text (GTK_LABEL (w), mrp_calendar_get_name (calendar));
	
	data->weekday_option_menu = glade_xml_get_widget (glade, "weekday_optionmenu");
	data->day_option_menu = glade_xml_get_widget (glade, "day_optionmenu");

	default_week_dialog_setup_day_option_menu (GTK_OPTION_MENU (data->day_option_menu),
						   data->project,
						   calendar);

	g_signal_connect (data->day_option_menu,
			  "changed",
			  G_CALLBACK (default_week_dialog_day_selected_cb),
			  data);

	g_signal_connect (data->weekday_option_menu,
			  "changed",
			  G_CALLBACK (default_week_dialog_weekday_selected_cb),
			  data);

	default_week_dialog_setup_weekday_option_menu (GTK_OPTION_MENU (data->weekday_option_menu));

	g_signal_connect (dialog,
			  "response",
			  G_CALLBACK (default_week_dialog_response_cb),
			  data);

	g_object_set_data_full (G_OBJECT (dialog),
				"data", data,
				g_free);

	default_week_dialog_update_labels (data);
	
	return dialog;
}

static void
default_week_dialog_update_labels (DialogData *data)
{
	gint         i;
	MrpDay      *day;
	GList       *ivals, *l;
	MrpInterval *ival;
	gchar       *str;
	mrptime      start, end;

	day = default_week_dialog_get_selected_day (data);

	/* Special case "use base", since we can't get intervals for that
	 * type.
	 */
	if (day == mrp_day_get_use_base ()) {
		MrpCalendar *parent;
		gint         weekday;
				
		parent = mrp_calendar_get_parent (data->calendar);
		if (parent) {
			weekday = default_week_dialog_get_selected_weekday (data);
			
			day = mrp_calendar_get_default_day (parent, weekday);
			
			ivals = mrp_calendar_day_get_intervals (parent, day, TRUE);
		} else {
			ivals = NULL;
		}
	} else {
		ivals = mrp_calendar_day_get_intervals (data->calendar, day, TRUE);
	}

	for (i = 0; i < 5; i++) {
		gtk_label_set_text (GTK_LABEL (data->from_label[i]), "");
		gtk_label_set_text (GTK_LABEL (data->to_label[i]), "");
		gtk_label_set_text (GTK_LABEL (data->dash_label[i]), "");
	}

	/* No intervals. */
	if (!ivals) {
		gchar *tmp;

		tmp = g_strconcat ("<i>", _("No working time"), "</i>", NULL);
		gtk_label_set_markup (GTK_LABEL (data->from_label[0]), tmp);
		g_free (tmp);
	}
	
	i = 0;
	for (l = ivals; l; l = l->next) {
		ival = l->data;
		
		mrp_interval_get_absolute (ival, 0, &start, &end);
		
		str = mrp_time_format (_("%H:%M"), start);
		gtk_label_set_text (GTK_LABEL (data->from_label[i]), str);
		g_free (str);

		str = mrp_time_format (_("%H:%M"), end);
		gtk_label_set_text (GTK_LABEL (data->to_label[i]), str);
		g_free (str);

		gtk_label_set_text (GTK_LABEL (data->dash_label[i]), "-");

		if (i++ > 5) {
			break;
		}
	}
}
	
static void
default_week_dialog_weekday_selected_cb (GtkOptionMenu *option_menu,
					 DialogData    *data)
{
	gint    weekday;
	MrpDay *day;

	weekday = default_week_dialog_get_selected_weekday (data);
	day = mrp_calendar_get_default_day (data->calendar, weekday);
	
	default_week_dialog_set_selected_day (data, day);

	default_week_dialog_update_labels (data);
}

static void
default_week_dialog_day_selected_cb (GtkOptionMenu *option_menu,
				     DialogData    *data)
{
	default_week_dialog_update_labels (data);
}
	
static void
default_week_dialog_setup_day_option_menu (GtkOptionMenu *option_menu,
					   MrpProject    *project,
					   MrpCalendar   *calendar)
{
	GtkWidget   *menu;
	GtkWidget   *menu_item;
	const gchar *name;
	GList       *days, *l;
	MrpDay      *day;
	MrpCalendar *parent, *root;

	days = mrp_day_get_all (project);
	days = g_list_prepend (days, mrp_day_get_use_base ());
	days = g_list_prepend (days, mrp_day_get_nonwork ());
	days = g_list_prepend (days, mrp_day_get_work ());

	parent = mrp_calendar_get_parent (calendar);
	root = mrp_project_get_root_calendar (project);
	
	menu = gtk_option_menu_get_menu (option_menu);

	if (menu) {
		gtk_widget_destroy (menu);
	}

	menu = gtk_menu_new ();

	for (l = days; l; l = l->next) {
		day = l->data;

		name = mrp_day_get_name (day);

		menu_item = gtk_menu_item_new_with_label (name);
		gtk_widget_show (menu_item);
		gtk_menu_append (GTK_MENU (menu), menu_item);

		/* "Use base" is not possible for toplevel calendars. */
		if (parent == root && day == mrp_day_get_use_base ()) {
			gtk_widget_set_sensitive (menu_item, FALSE);
		}
		
		g_object_set_data (G_OBJECT (menu_item),
				   "data",
				   l->data);
	}

	gtk_widget_show (menu);
	gtk_option_menu_set_menu (option_menu, menu);

	/* Need to unref the days here? */
	g_list_free (days);
}

static gint
default_week_dialog_get_selected_weekday (DialogData *data)
{
	GtkWidget *menu;
	GtkWidget *item;
	gint       id;
	
	menu = gtk_option_menu_get_menu (GTK_OPTION_MENU (data->weekday_option_menu));

	item = gtk_menu_get_active (GTK_MENU (menu));

	id = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (item), "data"));

	return id;
}

static void
default_week_dialog_setup_weekday_option_menu (GtkOptionMenu *option_menu)
{
	GtkWidget *menu;
	GtkWidget *menu_item;
	gint       i;
	
	menu = gtk_option_menu_get_menu (option_menu);
	if (menu) {
		gtk_widget_destroy (menu);
	}

	menu = gtk_menu_new ();

	for (i = 0; i < 7; i++) {
		menu_item = gtk_menu_item_new_with_label (_(days[i].name));

		gtk_widget_show (menu_item);
		gtk_menu_append (GTK_MENU (menu), menu_item);
		
		g_object_set_data (G_OBJECT (menu_item),
				   "data",
				   GINT_TO_POINTER (days[i].day));
	}

	gtk_widget_show (menu);
	gtk_option_menu_set_menu (option_menu, menu);
}

static MrpDay *
default_week_dialog_get_selected_day (DialogData *data)
{
	GtkWidget *menu;
	GtkWidget *item;
	
	menu = gtk_option_menu_get_menu (GTK_OPTION_MENU (data->day_option_menu));

	item = gtk_menu_get_active (GTK_MENU (menu));

	return g_object_get_data (G_OBJECT (item), "data");
}

static void
default_week_dialog_set_selected_day (DialogData *data,
				      MrpDay     *day)
{
	GtkWidget *menu;
	GtkWidget *item;
	GList     *children, *l;
	gint       i;
	
	menu = gtk_option_menu_get_menu (GTK_OPTION_MENU (data->day_option_menu));

	children = GTK_MENU_SHELL (menu)->children;
	for (i = 0, l = children; l; i++, l = l->next) {
		item = l->data;

		if (day == g_object_get_data (G_OBJECT (item), "data")) {
			gtk_option_menu_set_history (GTK_OPTION_MENU (data->day_option_menu), i);
			break;
		}
	}
}

static gboolean
is_day_builtin (MrpDay *day)
{
	if (day == mrp_day_get_work ()) {
		return TRUE;
	}
	else if (day == mrp_day_get_nonwork ()) {
		return TRUE;
	}
	else if (day == mrp_day_get_use_base ()) {
		return TRUE;
	} else {
		return FALSE;
	}
}

static gboolean
default_week_cmd_edit_do (PlannerCmd *cmd_base)
{
	DefaultWeekCmdEdit *cmd;
	MrpDay             *day;

	cmd = (DefaultWeekCmdEdit *) cmd_base;

	day = mrp_calendar_get_default_day (cmd->calendar, cmd->weekday);

	if (is_day_builtin (day)) {
		cmd->old_day = day;
	} else {
		cmd->old_day = NULL;
		cmd->old_day_id = mrp_day_get_id (day);
	}

	if (cmd->day) {
		day = cmd->day;
	} else {
		day = mrp_project_get_calendar_day_by_id (cmd->project, cmd->day_id);
	}
	
	mrp_calendar_set_default_days (cmd->calendar,
				       cmd->weekday, day,
				       -1);
	
	return TRUE;
}

static void
default_week_cmd_edit_undo (PlannerCmd *cmd_base)
{
	DefaultWeekCmdEdit *cmd;
	MrpDay             *day;

	cmd = (DefaultWeekCmdEdit *) cmd_base;

	if (is_day_builtin (cmd->old_day)) {
		day = cmd->old_day;
	} else {
		day = mrp_project_get_calendar_day_by_id (cmd->project, cmd->old_day_id);
	}

	mrp_calendar_set_default_days (cmd->calendar,
				       cmd->weekday, day,
				       -1);
}

static void
default_week_cmd_edit_free (PlannerCmd *cmd_base)
{
	DefaultWeekCmdEdit *cmd;

	cmd = (DefaultWeekCmdEdit *) cmd_base;
}

static PlannerCmd *
default_week_cmd_edit (DialogData *data,
		       gint        weekday,
		       MrpDay     *day)
{
	PlannerCmd         *cmd_base;
	DefaultWeekCmdEdit *cmd;

	cmd_base = planner_cmd_new (DefaultWeekCmdEdit,
				    _("Edit default week"),
 				    default_week_cmd_edit_do,
				    default_week_cmd_edit_undo,
				    default_week_cmd_edit_free);

	cmd = (DefaultWeekCmdEdit *) cmd_base;

	cmd->project = data->project;
	cmd->calendar = data->calendar;
	cmd->weekday = weekday;

	if (is_day_builtin (day)) {
		cmd->day = day;
	} else {
		cmd->day_id = mrp_day_get_id (day);
	}
	
	planner_cmd_manager_insert_and_do (planner_window_get_cmd_manager (data->main_window),
					   cmd_base);
	
	return cmd_base;
}
