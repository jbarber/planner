/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2003      Imendio HB
 * Copyright (C) 2002-2003 CodeFactory AB
 * Copyright (C) 2002-2003 Richard Hult <richard@imendio.com>
 * Copyright (C) 2002-2003 Mikael Hallendal <micke@imendio.com>
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
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gtk/gtkbutton.h>
#include <gtk/gtkmain.h>
#include <gtk/gtkstock.h>
#include <gtk/gtkmessagedialog.h>
#include <gtk/gtkalignment.h>
#include <gtk/gtknotebook.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkimage.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkfilechooserdialog.h>
#include <glade/glade.h>
#include <libgnome/gnome-help.h>
#include <glib/gi18n.h>
#include <libgnomeui/gnome-about.h>
#include <libgnomeui/gnome-href.h>
#include <libgnomeui/gnome-stock-icons.h>
#include <libgnomeprintui/gnome-print-dialog.h>
#include <libgnomeprintui/gnome-print-job-preview.h>
#include <libplanner/mrp-error.h>
#include <libplanner/mrp-project.h>
#include <libegg/recent-files/egg-recent-view.h>
#include "planner-marshal.h"
#include "planner-conf.h"
#include "planner-sidebar.h"
#include "planner-window.h"
#include "planner-view-loader.h"
#include "planner-plugin-loader.h"
#include "planner-project-properties.h"
#include "planner-phase-dialog.h"
#include "planner-calendar-dialog.h"
#include "planner-day-type-dialog.h"
#include "planner-print-dialog.h"
#include "planner-view.h"
#include "planner-cmd-manager.h"

#define d(x)

struct _PlannerWindowPriv {
	PlannerApplication  *application;

	GtkUIManager        *ui_manager;
	GtkActionGroup      *actions;
	GtkActionGroup      *view_actions;

	PlannerCmdManager   *cmd_manager;

	MrpProject          *project;

	GtkWidget           *statusbar;
	GtkWidget           *ui_box;
	GtkWidget           *sidebar;
	GtkWidget           *notebook;

	GtkWidget           *properties_dialog;
	GtkWidget           *calendar_dialog;
	GtkWidget           *day_type_dialog;
	GtkWidget           *phase_dialog;

	PlannerView         *current_view;
	GList               *views;
	GList               *plugins;
	GTimer              *last_saved;

	/* FIXME: remove that
	   EggRecentViewBonobo *view;
	*/
};

/* Signals */
enum {
	CLOSED,
	NEW_WINDOW,
	EXIT,
	LAST_SIGNAL
};

static void       window_class_init                      (PlannerWindowClass           *klass);
static void       window_init                            (PlannerWindow                *window);
static void       window_finalize                        (GObject                      *object);
static void       window_populate                        (PlannerWindow                *window);
static void       window_view_selected                   (PlannerSidebar               *sidebar,
							  gint                          index,
							  PlannerWindow                *window);
static void       window_view_cb                         (GtkAction                    *action,
							  GtkRadioAction               *current,
							  gpointer                      data);
static void       window_new_cb                          (GtkAction                    *action,
							  gpointer                      data);
static void       window_open_cb                         (GtkAction                    *action,
							  gpointer                      data);
static void       window_save_as_cb                      (GtkAction                    *action,
							  gpointer                      data);
static void       window_save_cb                         (GtkAction                    *action,
							  gpointer                      data);
static void       window_print_cb                        (GtkAction                    *action,
							  gpointer                      data);
static void       window_print_preview_cb                (GtkAction                    *action,
							  gpointer                      data);
static void       window_close_cb                        (GtkAction                    *action,
							  gpointer                      data);
static void       window_exit_cb                         (GtkAction                    *action,
							  gpointer                      data);
static void       window_undo_cb                         (GtkAction                    *action,
							  gpointer                      data);
static void       window_redo_cb                         (GtkAction                    *action,
							  gpointer                      data);
static void       window_project_props_cb                (GtkAction                    *action,
							  gpointer                      data);
static void       window_manage_calendars_cb             (GtkAction                    *action,
							  gpointer                      data);
static void       window_edit_day_types_cb               (GtkAction                    *action,
							  gpointer                      data);
static void       window_edit_phases_cb                  (GtkAction                    *action,
							  gpointer                      data);
static void       window_help_cb                         (GtkAction                    *action,
							  gpointer                      data);
static void       window_about_cb                        (GtkAction                    *action,
							  gpointer                      data);
static gboolean   window_delete_event_cb                 (PlannerWindow                *window,
							  gpointer                      user_data);
static void       window_undo_state_changed_cb           (PlannerCmdManager            *manager,
							  gboolean                      state,
							  const gchar                  *label,
							  PlannerWindow                *window);
static void       window_redo_state_changed_cb           (PlannerCmdManager            *manager,
							  gboolean                      state,
							  const gchar                  *label,
							  PlannerWindow                *window);
static void       window_project_needs_saving_changed_cb (MrpProject                   *project,
							  gboolean                      needs_saving,
							  PlannerWindow                *window);
static void       window_project_notify_name_cb          (MrpProject                   *project,
							  GParamSpec                   *pspec,
							  PlannerWindow                *window);
static gboolean   window_confirm_exit_run                (PlannerWindow                *window);
static gboolean   window_do_save                         (PlannerWindow                *window,
							  gboolean                      force);
static gboolean   window_do_save_as                      (PlannerWindow                *window);
static gchar *    window_get_name                        (PlannerWindow                *window);
static void       window_update_title                    (PlannerWindow                *window);
static GtkWidget *window_create_dialog_button            (const gchar                  *icon_name,
							  const gchar                  *text);
#ifdef FIXME
static gchar *    window_recent_tooltip_func             (EggRecentItem                *item,
							  gpointer                      user_data);
#endif
static void       window_save_state                      (PlannerWindow *window);
static void       window_restore_state                   (PlannerWindow *window);

static void window_disconnect_proxy_cb (GtkUIManager  *manager,
					GtkAction     *action,
					GtkWidget     *proxy,
					PlannerWindow *window);

static void window_connect_proxy_cb (GtkUIManager  *manager,
				     GtkAction     *action,
				     GtkWidget     *proxy,
				     PlannerWindow *window);


#define CONF_MAIN_WINDOW_DIR       "/ui"
#define CONF_MAIN_WINDOW_MAXIMIZED "/ui/main_window_maximized"
#define CONF_MAIN_WINDOW_WIDTH     "/ui/main_window_width"
#define CONF_MAIN_WINDOW_HEIGHT    "/ui/main_window_height"
#define CONF_MAIN_WINDOW_POS_X     "/ui/main_window_position_x"
#define CONF_MAIN_WINDOW_POS_Y     "/ui/main_window_position_y"
#define CONF_MAIN_LAST_DIR         "/general/last_dir"

#define VIEW_PATH "/menu/View/Views placeholder"
#define VIEW_GROUP "view group"
	
static GtkWindowClass *parent_class = NULL;
static guint signals[LAST_SIGNAL];


static GtkActionEntry entries[] = {
	{ "File",
	  NULL, N_("_File"), NULL, NULL, 
	  NULL
	},
	{ "FileNew",
	  GTK_STOCK_NEW,           N_("_New Project"),             "<Control>n",        N_("Create a new project"),
	  G_CALLBACK (window_new_cb)
	},
	{ "FileOpen",
	  GTK_STOCK_OPEN,          N_("_Open..."),                 "F3",                N_("Open a project"),
	  G_CALLBACK (window_open_cb) },
	{ "Import",
	  NULL,                    N_("_Import"),                  NULL,                NULL,
	  NULL },

	{ "FileSave",
	  GTK_STOCK_SAVE,          N_("_Save"),                    "<Control>s",        N_("Save the current project"),
	  G_CALLBACK (window_save_cb) },
	{ "Export",
	  NULL,                    N_("_Export"),                  NULL,                NULL,
	  NULL },
	{ "FileSaveAs",
	  GTK_STOCK_SAVE_AS,       N_("Save _As..."),              "<Shift><Control>s", N_("Save the current project with a different name"),
	  G_CALLBACK (window_save_as_cb) },
	{ "FilePrint",
	  GTK_STOCK_PRINT,         N_("_Print..."),                "<Control>p",        N_("Print the current project"),
	  G_CALLBACK (window_print_cb) },
	{ "FilePrintPreview",
	  GTK_STOCK_PRINT_PREVIEW, N_("Print Pre_view"),           "<Shift><Control>p", N_("Print preview of the current project"),
	  G_CALLBACK (window_print_preview_cb) },
	{ "FileClose",
	  GTK_STOCK_CLOSE,         N_("_Close"),                   "<Control>w",        N_("Close the current file"),
	  G_CALLBACK (window_close_cb) },
	{ "FileExit",
	  GTK_STOCK_QUIT,          N_("_Quit"),                    "<Control>q",        N_("Exit the program"),
	  G_CALLBACK (window_exit_cb) },
	
	{ "Edit",
	  NULL,                    N_("_Edit"),                    NULL,                NULL,
	  NULL },
	{ "EditUndo",
	  GTK_STOCK_UNDO,          N_("_Undo"),                    "<Control>z",        N_("Undo the last action"),
	  G_CALLBACK (window_undo_cb) },
	{ "EditRedo",
	  GTK_STOCK_REDO,          N_("_Redo"),                    "<Control>r",        N_("Redo the undone action"),
	  G_CALLBACK (window_redo_cb) },
	
	{ "View",
	  NULL,                    N_("_View"),                    NULL,                NULL,
	  NULL },
	
	{ "Actions",
	  NULL,                    N_("_Actions"),                 NULL,                NULL,
	  NULL },
	
	{ "Project",
	  NULL,                    N_("_Project"),                 NULL,                NULL,
	  NULL },
	{ "ManageCalendars",
	  NULL,                    N_("_Manage Calendars"),        NULL,                NULL,
	  G_CALLBACK (window_manage_calendars_cb) },
	{ "EditDayTypes",
	  NULL,                    N_("Edit Day _Types"),          NULL,                NULL,
	  G_CALLBACK (window_edit_day_types_cb) },
	{ "EditPhases",
	  NULL,                    N_("Edit Project _Phases"),     NULL,                NULL,
	  G_CALLBACK (window_edit_phases_cb) },
	{ "EditProjectProps",
	  GTK_STOCK_PROPERTIES,    N_("_Edit Project Properties"), NULL,                N_("Edit the project properties"),
	  G_CALLBACK (window_project_props_cb) },
	
/*	{ "PreferencesEditPreferences",
	NULL,                    NULL,                           NULL,                NULL,
	G_CALLBACK (window_preferences_cb) },*/
	
	{ "Help",
	  NULL,                    N_("_Help"),                    NULL,                NULL,
	  NULL },
	{ "HelpHelp",
	  GTK_STOCK_HELP,          N_("_User Guide"),              "F1",                N_("Show the Planner User Guide"),
	  G_CALLBACK (window_help_cb) },
	{ "HelpAbout",
	  GNOME_STOCK_ABOUT,       N_("_About"),                   NULL,                N_("About this application"),
	  G_CALLBACK (window_about_cb) },
};

static guint n_entries = G_N_ELEMENTS(entries);


GType
planner_window_get_type (void)
{
	static GtkType type = 0;

	if (!type) {
		static const GTypeInfo info = {
			sizeof (PlannerWindowClass),
			NULL,		/* base_init */
			NULL,		/* base_finalize */
			(GClassInitFunc) window_class_init,
			NULL,		/* class_finalize */
			NULL,		/* class_data */
			sizeof (PlannerWindow),
			0,              /* n_preallocs */
			(GInstanceInitFunc) window_init
		};

		type = g_type_register_static (GTK_TYPE_WINDOW,
					       "PlannerWindow", &info, 0);
	}
	
	return type;
}

static void
window_class_init (PlannerWindowClass *klass)
{
	GObjectClass *o_class;
	
	parent_class = g_type_class_peek_parent (klass);

	o_class = (GObjectClass *) klass;

	/* GObject functions */
	o_class->finalize = window_finalize;

	/* Signals */
	signals[CLOSED] = g_signal_new 
		("closed",
		 G_TYPE_FROM_CLASS (klass),
		 G_SIGNAL_RUN_LAST,
		 0, /*G_STRUCT_OFFSET (PlannerWindowClass, method), */
		 NULL, NULL,
		 planner_marshal_VOID__VOID,
		 G_TYPE_NONE, 0);
}

static void
window_init (PlannerWindow *window)
{
	PlannerWindowPriv *priv;

	priv = g_new0 (PlannerWindowPriv, 1);
	window->priv = priv;

	priv->cmd_manager = planner_cmd_manager_new ();

	g_signal_connect (priv->cmd_manager,
			  "undo_state_changed",
			  G_CALLBACK (window_undo_state_changed_cb),
			  window);

	g_signal_connect (priv->cmd_manager,
			  "redo_state_changed",
			  G_CALLBACK (window_redo_state_changed_cb),
			  window);
}

static void
window_finalize (GObject *object)
{
	PlannerWindow     *window = PLANNER_WINDOW (object);
	PlannerWindowPriv *priv = window->priv;

	d(g_print ("Window::Finalize\n"));

	if (priv->application) {
		g_object_unref (priv->application);
	}
	if (priv->last_saved) {
		g_timer_destroy (priv->last_saved);
	}
	if (priv->cmd_manager) {
		g_object_unref (priv->cmd_manager);
	}
	
	g_free (window->priv);

	if (G_OBJECT_CLASS (parent_class)->finalize) {
		(* G_OBJECT_CLASS (parent_class)->finalize) (object);
	}
}


#ifdef FIXME
static void
planner_window_open_recent (GtkWidget           *widget,
			    const EggRecentItem *item,
			    PlannerWindow        *window)
{
	gchar     *uri, *filename;
	GtkWidget *new_window;
	
	uri = egg_recent_item_get_uri (item);

	/* FIXME: Is this necessary? */
	filename = g_filename_from_uri (uri, NULL, NULL);
	g_free (uri);

	if (mrp_project_is_empty (window->priv->project)) {
		planner_window_open (window, filename);
	} else {
		new_window = 
			planner_application_new_window (window->priv->application);
		if (planner_window_open (PLANNER_WINDOW (new_window),
					 filename)) {
			gtk_widget_show_all (new_window);
		} else {
			g_signal_emit (new_window, signals[CLOSED], 0, NULL);
			
			gtk_widget_destroy (new_window);
		}
	}
	
	g_free (filename);
}
#endif

static void
window_add_widget (GtkUIManager  *merge, 
		   GtkWidget     *widget, 
		   PlannerWindow *window)
{
	PlannerWindowPriv *priv;

	priv = window->priv;

	gtk_box_pack_start (GTK_BOX (priv->ui_box), widget, FALSE, FALSE, 0);
}

static void
window_populate (PlannerWindow *window)
{
	PlannerWindowPriv    *priv;
	GtkWidget            *hbox;
	GList                *l;
	GtkWidget            *view_widget;
	PlannerView          *view;
	gint                  view_num;
	GtkRadioActionEntry  *r_entries;
	gchar                *xml_string_tmp, *xml_string;
	GError               *error = NULL;
	const gchar          *xml_string_full =
		"<ui>"
		"<menubar name='MenuBar'>"
		"<menu action='View'>"
		"<placeholder name='Views placeholder'>"
		"%s"
		"</placeholder>"
		"</menu>"
		"</menubar>"
		"</ui>";

	priv = window->priv;


	/* Handle recent file stuff */
	/* FIXME: update the bonobo recent files
	priv->view = egg_recent_view_bonobo_new (priv->ui_component,
						 "/menu/File/EggRecentDocuments/");
	egg_recent_view_set_model (EGG_RECENT_VIEW (priv->view),
				   planner_application_get_recent_model (priv->application));
	egg_recent_view_bonobo_set_tooltip_func (priv->view,
						 window_recent_tooltip_func,
						 NULL);

	g_signal_connect (priv->view, "activate",
			  G_CALLBACK (planner_window_open_recent), window);
	*/

	priv->ui_box = gtk_vbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (window), priv->ui_box);

	priv->actions = gtk_action_group_new ("Planner");
	gtk_action_group_set_translation_domain (priv->actions, GETTEXT_PACKAGE);
	gtk_action_group_add_actions (priv->actions, entries, n_entries, window);

	priv->ui_manager = gtk_ui_manager_new ();

	g_signal_connect (priv->ui_manager,
			  "add_widget",
			  G_CALLBACK (window_add_widget),
			  window);
	g_signal_connect (priv->ui_manager,
			  "connect_proxy",
			  G_CALLBACK (window_connect_proxy_cb),
			  window);
	g_signal_connect (priv->ui_manager,
			  "disconnect_proxy",
			  G_CALLBACK (window_disconnect_proxy_cb),
			  window);
	
	gtk_ui_manager_insert_action_group (priv->ui_manager, priv->actions, 0);
	gtk_window_add_accel_group (GTK_WINDOW (window),
				    gtk_ui_manager_get_accel_group (priv->ui_manager));

	if (!gtk_ui_manager_add_ui_from_file (priv->ui_manager,
					      DATADIR "/planner/ui/main-window.ui",
					      &error)) {
		g_message ("Building menus failed: %s", error->message);
		g_message ("Couldn't load: %s",DATADIR"/planner/ui/main-window.ui");
		g_error_free (error);
	}

	g_object_set (gtk_action_group_get_action (priv->actions, "EditUndo"),
		      "sensitive", FALSE, 
		      NULL);
	g_object_set (gtk_action_group_get_action (priv->actions, "EditRedo"),
		      "sensitive", FALSE, 
		      NULL);
	g_object_set (gtk_action_group_get_action (priv->actions, "FileSave"),
		      "sensitive", FALSE, 
		      NULL);

	hbox = gtk_hbox_new (FALSE, 0);

	priv->sidebar = planner_sidebar_new ();
	gtk_box_pack_start (GTK_BOX (hbox), priv->sidebar, FALSE, TRUE, 0); 
	g_signal_connect (priv->sidebar, 
			  "icon-selected",
			  G_CALLBACK (window_view_selected),
			  window);

	priv->notebook = gtk_notebook_new ();
	gtk_notebook_set_show_tabs (GTK_NOTEBOOK (priv->notebook), FALSE);
	gtk_box_pack_start (GTK_BOX (hbox), priv->notebook, TRUE, TRUE, 0); 

	priv->statusbar = gtk_statusbar_new ();
	gtk_box_pack_end (GTK_BOX (priv->ui_box), priv->statusbar, FALSE, TRUE, 0);
	gtk_widget_show (priv->statusbar);

	gtk_box_pack_end (GTK_BOX (priv->ui_box), hbox, TRUE, TRUE, 0);

	/* Load views. */
	priv->view_actions = gtk_action_group_new ("View Actions");
	gtk_ui_manager_insert_action_group (priv->ui_manager, priv->view_actions, 0);
	priv->views = planner_view_loader_load (window);

	view_num = 0;
	xml_string = g_strdup ("");
	r_entries  = g_new0 (GtkRadioActionEntry, g_list_length (priv->views));
	for (l = priv->views; l; l = l->next, view_num++ ) {
		view = l->data;
		
		view_widget = planner_view_get_widget (view);
		gtk_widget_show (view_widget);

		planner_sidebar_append (PLANNER_SIDEBAR (priv->sidebar),
					planner_view_get_icon (view),
					planner_view_get_label (view));

		r_entries[view_num].name  = planner_view_get_name (view);
		r_entries[view_num].label = planner_view_get_menu_label (view);
		r_entries[view_num].value = view_num;

		/* Note: these strings are leaked. */
		r_entries[view_num].tooltip = g_strdup_printf (_("Switch to the view \"%s\""),
							       planner_view_get_label (view));

		xml_string_tmp = xml_string;
		xml_string = g_strdup_printf ("%s<menuitem action='%s'/>",
					      xml_string, r_entries[view_num].name);
		g_free (xml_string_tmp);

		gtk_notebook_append_page (
			GTK_NOTEBOOK (priv->notebook),
			view_widget,
			gtk_label_new (planner_view_get_label (view)));
	}

	gtk_action_group_add_radio_actions (priv->view_actions, r_entries,
					    g_list_length (priv->views), 0,
					    G_CALLBACK (window_view_cb), window);

	xml_string_tmp = g_strdup_printf (xml_string_full, xml_string);
	if (!gtk_ui_manager_add_ui_from_string (priv->ui_manager, xml_string_tmp, -1, &error)) {
		g_message("Building menu failed: %s", error->message);
		g_message("Couldn't build the view menu item");
		g_error_free(error);
	}
	g_free(xml_string);
	g_free(xml_string_tmp);
	
	gtk_ui_manager_ensure_update (priv->ui_manager);

	/* Load plugins. */
	priv->plugins = planner_plugin_loader_load (window);

	window_view_selected (PLANNER_SIDEBAR (priv->sidebar), 0, window);
}

static void
window_view_selected (PlannerSidebar *sidebar,
		      gint            index,
		      PlannerWindow  *window)
{
	PlannerWindowPriv *priv;
	GList             *list;
	PlannerView       *view;
	GtkAction         *action;

	priv = window->priv;
	
	list = g_list_nth (priv->views, index);
	if (list != NULL) {
		view = list->data;
	} else {
		view = NULL;
	}

	if (view == priv->current_view) {
		return;
	}
	
	if (priv->current_view != NULL) {
		planner_view_deactivate (priv->current_view);
	}

	if (view != NULL) {
		planner_view_activate (view);
	}

	gtk_notebook_set_current_page (GTK_NOTEBOOK (priv->notebook), index);

	g_signal_handlers_block_by_func (sidebar, window_view_selected, window);
	planner_sidebar_set_active (PLANNER_SIDEBAR (sidebar), index);
	g_signal_handlers_unblock_by_func (sidebar, window_view_selected, window);
	
	action = gtk_action_group_get_action (priv->view_actions,
					      planner_view_get_name (view));
	if (!gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action)) ) {
		gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), TRUE);
	}
	
	priv->current_view = view;
}

static void
window_view_cb (GtkAction      *action,
		GtkRadioAction *current,
		gpointer        data)
{
	PlannerWindow     *window;
	GtkWidget         *sidebar;
	guint              index;

	window  = PLANNER_WINDOW (data);
	sidebar = window->priv->sidebar;

	index = gtk_radio_action_get_current_value(current);

	planner_sidebar_set_active (PLANNER_SIDEBAR (sidebar), index);
}

static void
window_new_cb (GtkAction *action,
	       gpointer   data)
{
	PlannerWindow     *window;
	PlannerWindowPriv *priv;
	GtkWidget         *new_window;
	
	window = PLANNER_WINDOW (data);
	priv   = window->priv;

	new_window = planner_application_new_window (priv->application);
	gtk_widget_show_all (new_window);
}

static gchar *
get_last_dir (PlannerWindow *window)
{
	PlannerWindowPriv *priv;
	gchar             *last_dir;
	
	priv = window->priv;
	
	last_dir = planner_conf_get_string (CONF_MAIN_LAST_DIR, NULL);
	
	if (last_dir == NULL) {
		last_dir = g_strdup (g_get_home_dir ());
	}
	
	return last_dir;
}

static void
window_open_cb (GtkAction *action,
		gpointer   data)
{
	PlannerWindow     *window;
	PlannerWindowPriv *priv;
	GtkWidget         *file_chooser;
	gint               response;
	gchar             *filename = NULL;
	gchar             *last_dir;
	GtkWidget         *new_window;

	window = PLANNER_WINDOW (data);
	priv = window->priv;

	file_chooser = gtk_file_chooser_dialog_new (_("Open a file"),
						    GTK_WINDOW (window),
						    GTK_FILE_CHOOSER_ACTION_OPEN,
						    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
						    GTK_STOCK_OPEN, GTK_RESPONSE_OK,
						    NULL);
	
	last_dir = get_last_dir (window);
	gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (file_chooser), last_dir);
	g_free (last_dir);

	gtk_window_set_modal (GTK_WINDOW (file_chooser), TRUE);

	gtk_widget_show (file_chooser);

	response = gtk_dialog_run (GTK_DIALOG (file_chooser));

	if (response == GTK_RESPONSE_OK) {
		filename = gtk_file_chooser_get_filename (
			GTK_FILE_CHOOSER (file_chooser));
	}
	
	gtk_widget_destroy (file_chooser);

	if (filename != NULL) {
		if (mrp_project_is_empty (priv->project)) {
			planner_window_open (window, filename);
		} else {
			new_window = 
				planner_application_new_window (priv->application);
			if (planner_window_open (PLANNER_WINDOW (new_window),
						 filename)) {
				gtk_widget_show_all (new_window);
			} else {
				g_signal_emit (new_window, signals[CLOSED], 
					       0, NULL);
				
				gtk_widget_destroy (new_window);
			}
		}
		
		last_dir = g_path_get_dirname (filename);
		planner_conf_set_string (CONF_MAIN_LAST_DIR, last_dir, NULL);
		g_free (last_dir);

		g_free (filename);		
	}
}

static void
window_save_as_cb (GtkAction *action,
		   gpointer   data)
{
	PlannerWindow *window;

	window = PLANNER_WINDOW (data);

        window_do_save_as (window);
}

static void
window_save_cb (GtkAction *action,
		gpointer   data)
{
	PlannerWindow     *window;

	window = PLANNER_WINDOW (data);

	window_do_save (window, FALSE);
}

static void
window_print_preview_cb (GtkAction *action,
			 gpointer   data)
{
	PlannerWindow     *window;
	PlannerWindowPriv *priv;
	GnomePrintJob    *gpj;
	GtkWidget        *preview;
	GList            *l;
	PlannerView           *view;
	PlannerPrintJob       *job;
	gint              n_pages;

	window = PLANNER_WINDOW (data);
	priv = window->priv;

	gpj = gnome_print_job_new (NULL);
	
	job = planner_print_job_new (gpj);
	
	n_pages = 0;
	for (l = priv->views; l; l = l->next) {
		view = l->data;

		planner_view_print_init (view, job);
		
		n_pages += planner_view_print_get_n_pages (view);
	}

	planner_print_job_set_total_pages (job, n_pages);
	
	for (l = priv->views; l; l = l->next) {
		view = l->data;
		
		planner_view_print (view);

		planner_view_print_cleanup (view);
	}

	gnome_print_job_close (job->pj);
	
	preview = gnome_print_job_preview_new (job->pj, "Planner");
	gtk_widget_show (preview);
	
	g_object_unref (job);
}

static void
window_print_cb (GtkAction *action,
		 gpointer   data)
{
	PlannerWindow     *window;
	PlannerWindowPriv *priv;
	GnomePrintJob     *gpj;
	GnomePrintConfig  *config;
	GtkWidget         *dialog;
	gint               response;
	gboolean           summary;
	GList             *views, *l;
	PlannerView       *view;
	PlannerPrintJob   *job;
	gint               n_pages;
	
	window = PLANNER_WINDOW (data);
	priv = window->priv;

	/* Load printer settings */
	config = planner_print_dialog_load_config ();
	gpj = gnome_print_job_new (config);
	gnome_print_config_unref (config);

	dialog = planner_print_dialog_new (data, gpj, priv->views);

	response = gtk_dialog_run (GTK_DIALOG (dialog));

	if (response == GTK_RESPONSE_CANCEL) {
		gtk_widget_destroy (dialog);
		g_object_unref (gpj);
		return;
	}
	else if (response == GTK_RESPONSE_DELETE_EVENT) {
		g_object_unref (gpj);
		return;
	}
	
	/* Save printer settings */
	planner_print_dialog_save_config (config);

	views = planner_print_dialog_get_print_selection (GTK_DIALOG (dialog), &summary);

	if (summary) {
		/*g_print ("Print summary\n");*/
	}

	job = planner_print_job_new (gpj);
	
	n_pages = 0;
	for (l = views; l; l = l->next) {
		view = l->data;

		planner_view_print_init (view, job);
		
		n_pages += planner_view_print_get_n_pages (view);
	}

	planner_print_job_set_total_pages (job, n_pages);
	
	for (l = views; l; l = l->next) {
		view = l->data;
		
		planner_view_print (view);
		planner_view_print_cleanup (view);
	}

	gtk_widget_destroy (dialog);

	gnome_print_job_close (job->pj);

	if (response == GNOME_PRINT_DIALOG_RESPONSE_PREVIEW) {
		GtkWidget *preview;
		
		preview = gnome_print_job_preview_new (job->pj, "Planner");
		gtk_widget_show (preview);
	}
	else if (response == GNOME_PRINT_DIALOG_RESPONSE_PRINT) {
		gnome_print_job_print (job->pj);
	}
	
	g_list_free (views);

	g_object_unref (job);
	g_object_unref (config);
}

static void
window_close_cb (GtkAction *action,
		 gpointer   data)
{
	planner_window_close (PLANNER_WINDOW (data));
}

static void
window_exit_cb (GtkAction *action, 
		gpointer   data)
{
	PlannerWindow     *window;
	PlannerWindowPriv *priv;
	
	window = PLANNER_WINDOW (data);
	priv   = window->priv;
	
	planner_application_exit (priv->application);
}

static void
window_redo_cb (GtkAction *action,
		gpointer           data)
{
	PlannerWindow     *window;
	PlannerWindowPriv *priv;

	window = PLANNER_WINDOW (data);

	priv = window->priv;
	
	planner_cmd_manager_redo (priv->cmd_manager);
}

static void
window_undo_cb (GtkAction *action,
		gpointer           data)
{
	PlannerWindow     *window;
	PlannerWindowPriv *priv;

	window = PLANNER_WINDOW (data);

	priv = window->priv;

	planner_cmd_manager_undo (priv->cmd_manager);
}

static void
window_project_props_cb (GtkAction *action,
			 gpointer           data)
{
	PlannerWindow     *window;
	PlannerWindowPriv *priv;
	
	window = PLANNER_WINDOW (data);
	priv = window->priv;
	
	if (priv->properties_dialog) {
		gtk_window_present (GTK_WINDOW (priv->properties_dialog));
	} else {
		priv->properties_dialog = planner_project_properties_new (window);

		g_object_add_weak_pointer (G_OBJECT (priv->properties_dialog),
					   (gpointer *) &priv->properties_dialog);

		gtk_widget_show (priv->properties_dialog);
	}
}

static void
window_manage_calendars_cb (GtkAction *action,
			    gpointer           data)
{
	PlannerWindow *window;
	
	window = PLANNER_WINDOW (data);

	planner_window_show_calendar_dialog (window);
}

static void
window_edit_day_types_cb (GtkAction *action,
			  gpointer           data)
{
	PlannerWindow *window;
	
	window = PLANNER_WINDOW (data);

	planner_window_show_day_type_dialog (window);
}

static void
window_edit_phases_cb (GtkAction *action,
		       gpointer           data)
{
	PlannerWindow     *window;
	PlannerWindowPriv *priv;
	
	window = PLANNER_WINDOW (data);
	priv = window->priv;
	
	if (priv->phase_dialog) {
		gtk_window_present (GTK_WINDOW (priv->phase_dialog));
	} else {
		priv->phase_dialog = planner_phase_dialog_new (window);
		
		g_object_add_weak_pointer (G_OBJECT (priv->phase_dialog),
					   (gpointer *) &priv->phase_dialog);
		
		gtk_widget_show (priv->phase_dialog);
	}
}

static void
window_help_cb (GtkAction *action, 
		gpointer           data)
{
	GError    *error = NULL;
	GtkWidget *dialog;
	
	if (!gnome_help_display ("planner.xml", NULL, &error)) {
		dialog = gtk_message_dialog_new (GTK_WINDOW (data),
						 GTK_DIALOG_MODAL |
						 GTK_DIALOG_DESTROY_WITH_PARENT,
						 GTK_MESSAGE_ERROR,
						 GTK_BUTTONS_OK,
						 "%s", error->message);
		gtk_widget_show (dialog);

		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
		g_error_free (error);
	}
}

static void
window_about_cb (GtkAction *action, 
		 gpointer   data)
{
	GtkWidget *about;
	GtkWidget *hbox;
	GtkWidget *href;

	const gchar *authors[] = {
		"Richard Hult <richard@imendio.com>",
		"Mikael Hallendal <micke@imendio.com>",
		"Alvaro del Castillo <acs@barrapunto.com>",
		NULL
	};

	const gchar *documenters[] = {
		"Kurt Maute <kmaute@yahoo.com>",
		"Alvaro del Castillo <acs@barrapunto.com>",
		"Pedro Soria-Rodriguez <sorrodp@alum.wpi.edu>",
		NULL
	};

	/* I18n: Translators, list your names here, newline separated if there
	 * are more than one, to appear in the about box.
	 */
	const gchar *translator_credits = N_("translator-credits");
	
	about = gnome_about_new ("Imendio Planner", VERSION,
				 "", /*"Copyright \xc2\xa9"*/
				 _("A Project Management application for the GNOME desktop"),
				 authors,
				 documenters,
				 strcmp (translator_credits, _("translator-credits")) != 0 ? _(translator_credits) : NULL,
				 NULL);
	
	hbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (about)->vbox), hbox, FALSE, FALSE, 0);
	
	href = gnome_href_new ("http://www.imendio.com/projects/planner/",
			       _("The Planner Homepage"));
	gtk_box_pack_start (GTK_BOX (hbox), href, TRUE, FALSE, 0);

	gtk_widget_show_all (about);
}

static gboolean
window_delete_event_cb (PlannerWindow *window,
			gpointer       user_data)
{
	planner_window_close (window);
	return TRUE;
}

static void
window_undo_state_changed_cb (PlannerCmdManager *manager,
			      gboolean           state,
			      const gchar       *label,
			      PlannerWindow     *window)
{
	PlannerWindowPriv *priv;

	priv = window->priv;

	g_object_set (gtk_action_group_get_action (priv->actions, "EditUndo"),
		      "sensitive", state,
		      NULL);
	g_object_set (gtk_action_group_get_action (priv->actions, "EditUndo"),
		      "tooltip", label,
		      NULL);
	g_object_set (gtk_action_group_get_action (priv->actions, "EditUndo"),
		      "label", label,
		      NULL);
}

static void
window_redo_state_changed_cb (PlannerCmdManager *manager,
			      gboolean           state,
			      const gchar       *label,
			      PlannerWindow     *window)
{
	PlannerWindowPriv *priv;
	
	priv = window->priv;
	
	g_object_set (gtk_action_group_get_action (priv->actions, "EditRedo"),
		      "sensitive", state,
		      NULL);
	g_object_set (gtk_action_group_get_action (priv->actions, "EditRedo"),
		      "tooltip", label,
		      NULL);
	g_object_set (gtk_action_group_get_action (priv->actions, "EditRedo"),
		      "label", label,
		      NULL);
}

static void
window_project_needs_saving_changed_cb (MrpProject   *project,
					gboolean      needs_saving,
					PlannerWindow *window)
{
	PlannerWindowPriv *priv;
	
	priv = window->priv;
	
	if (needs_saving) {
		g_timer_reset (priv->last_saved);
	}

	g_object_set (gtk_action_group_get_action (priv->actions, "FileSave"),
		      "sensitive", needs_saving, 
		      NULL);	
}

static void
window_project_notify_name_cb (MrpProject   *project,
			       GParamSpec   *pspec,
			       PlannerWindow *window)
{
	window_update_title (window);
}

static gboolean
window_confirm_exit_run (PlannerWindow *window)
{
	PlannerWindowPriv *priv;
	gchar            *time_str;
	GtkWidget        *dialog;
	GtkWidget        *quit_button, *cancel_button, *save_button;
	gint              ret;
	gint              minutes;
	gint              hours;
	gchar            *name;
	gchar            *tmp;
	const gchar      *uri;
	gboolean          is_sql = FALSE;
	
	priv = window->priv;

	minutes = (gint) (g_timer_elapsed (priv->last_saved, NULL) / 60);

	minutes = MAX (1, minutes);
	hours = floor (minutes / 60.0 + 0.5);
	minutes -= 60 * hours;

	if (hours == 0 && minutes == 1) {
		time_str = g_strdup (_("If you don't save, changes made "
				       "the last minute will be discarded."));
	}
	else if (hours == 0) {
		time_str = 
			g_strdup_printf (_("If you don't save, changes made "
					   "the last %d minutes will be "
					   "discarded."), minutes);
	}
	else if (hours == 1) {
		time_str = g_strdup (_("If you don't save, changes made "
				       "the last hour will be "
				       "discarded."));
	} else {
		time_str = 
			g_strdup_printf (_("If you don't save, changes made "
					   "the last %d hours will be "
					   "discarded."), hours);
	}

	name = window_get_name (window);
	
	tmp = g_strdup_printf (_("Save changes to document '%s' before closing?"), name);
	
	dialog = gtk_message_dialog_new (GTK_WINDOW (window),
					 GTK_DIALOG_MODAL |
					 GTK_DIALOG_DESTROY_WITH_PARENT,
					 GTK_MESSAGE_WARNING,
					 GTK_BUTTONS_NONE,
					 "<b>%s</b>\n%s", tmp, time_str);
	g_free (name);
	g_free (time_str);
	g_free (tmp);
	
	g_object_set (GTK_MESSAGE_DIALOG (dialog)->label,
		      "use-markup", TRUE,
		      "wrap", FALSE,
		      NULL);

	uri = mrp_project_get_uri (priv->project);
	if (uri && strncmp (uri, "sql://", 6) == 0) {
		/* Hack. */
		is_sql = TRUE;
	}
	
	quit_button = window_create_dialog_button (GTK_STOCK_QUIT, 
						   _("C_lose without saving"));
	gtk_dialog_add_action_widget (GTK_DIALOG (dialog),
				      quit_button,
				      GTK_RESPONSE_NO);

	cancel_button = window_create_dialog_button (GTK_STOCK_CANCEL,
						     _("_Cancel"));
	gtk_dialog_add_action_widget (GTK_DIALOG (dialog),
				      cancel_button,
				      GTK_RESPONSE_CANCEL);

	if (!is_sql) {
		save_button = window_create_dialog_button (GTK_STOCK_SAVE,
							   _("_Save"));
		gtk_dialog_add_action_widget (GTK_DIALOG (dialog),
					      save_button,
					      GTK_RESPONSE_YES);
		gtk_widget_grab_focus (save_button);
	}
	
	gtk_widget_show_all (dialog);
	ret = gtk_dialog_run (GTK_DIALOG (dialog));
        gtk_widget_destroy (dialog);
        
	switch (ret) {
	case GTK_RESPONSE_NO: /* Don't save */
                return TRUE;
		break;
	case GTK_RESPONSE_DELETE_EVENT:
	case GTK_RESPONSE_CANCEL: /* Don't quit */
                return FALSE;
		break;
	case GTK_RESPONSE_YES: /* Save and quit */
		return window_do_save (window, FALSE);
		break;
	default:
		g_assert_not_reached ();
	};

        return FALSE;
}

static gboolean
window_do_save (PlannerWindow *window, gboolean force)
{
	PlannerWindowPriv *priv;
	const gchar      *uri = NULL;
	GError           *error = NULL;
	gint              response;
	
	priv = window->priv;
	
	uri = mrp_project_get_uri (priv->project);

	if (uri == NULL) {
		return window_do_save_as (window);
	} else {
		if (!mrp_project_save (priv->project, force, &error)) {
			GtkWidget *dialog;

			if (!force && error->code == MRP_ERROR_SAVE_FILE_CHANGED) {
				dialog = gtk_message_dialog_new (GTK_WINDOW (window),
								 GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
								 GTK_MESSAGE_WARNING,
								 GTK_BUTTONS_YES_NO,
								 error->message);
				
				response = gtk_dialog_run (GTK_DIALOG (dialog));
				gtk_widget_destroy (dialog);
				
				if (response == GTK_RESPONSE_YES) {
					return window_do_save (window, TRUE);
				}

				return FALSE;
			}

			dialog = gtk_message_dialog_new (GTK_WINDOW (window),
							 GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
							 GTK_MESSAGE_ERROR,
							 GTK_BUTTONS_OK,
							 error->message);
			
			gtk_dialog_run (GTK_DIALOG (dialog));
			gtk_widget_destroy (dialog);
			
			return FALSE;
		}
	}
	
	return TRUE;
}

static gboolean
window_do_save_as (PlannerWindow *window)
{
	PlannerWindowPriv *priv;
	GtkWidget        *file_chooser;
	gint              response;
	gchar            *filename = NULL;
	gchar            *last_dir;
	EggRecentItem    *item;

	priv = window->priv;

	file_chooser = gtk_file_chooser_dialog_new (_("Save a file"),
						    GTK_WINDOW (window),
						    GTK_FILE_CHOOSER_ACTION_SAVE,
						    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
						    GTK_STOCK_SAVE, GTK_RESPONSE_OK,
						    NULL);
	gtk_window_set_modal (GTK_WINDOW (file_chooser), TRUE);

	last_dir = get_last_dir (window);
	gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (file_chooser), last_dir);
	g_free (last_dir);
	
	response = gtk_dialog_run (GTK_DIALOG (file_chooser));
	if (response == GTK_RESPONSE_OK) {
		filename = gtk_file_chooser_get_filename (
			GTK_FILE_CHOOSER (file_chooser));
	}
	
	gtk_widget_destroy (file_chooser);

	if (filename != NULL) {
		gboolean  success;
		GError   *error = NULL;
		
		/* Save the file. */
		success = mrp_project_save_as (window->priv->project, filename, 
					       FALSE, &error);

		if (!success && error && 
		    error->code == MRP_ERROR_SAVE_FILE_EXIST) {
			GtkWidget *dialog;
			gint       ret;
			
			dialog = gtk_message_dialog_new (GTK_WINDOW (window),
							 GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
							 GTK_MESSAGE_WARNING,
							 GTK_BUTTONS_YES_NO,
							 _("File \"%s\" exists, do you want to overwrite it?"),
							 error->message);
			ret = gtk_dialog_run (GTK_DIALOG (dialog));
			gtk_widget_destroy (dialog);
			
			switch (ret) {
			case GTK_RESPONSE_YES:
				success = mrp_project_save_as (priv->project, 
							       filename,
							       TRUE, 
							       &error);
				break;
			case GTK_RESPONSE_NO:
			case GTK_RESPONSE_DELETE_EVENT:
				return window_do_save_as (window);
				break;
			default:
				g_assert_not_reached ();
			};
		}

		if (success) {
			/* Add the file to the recent list */
			item = egg_recent_item_new_from_uri (mrp_project_get_uri (priv->project));
			egg_recent_item_set_mime_type (item, "application/x-planner");
			egg_recent_model_add_full (planner_application_get_recent_model (priv->application), item);
			egg_recent_item_unref (item);
		} else {
			GtkWidget *dialog;
			
			dialog = gtk_message_dialog_new (GTK_WINDOW (window),
							 GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
							 GTK_MESSAGE_ERROR,
							 GTK_BUTTONS_OK,
							 error->message);
			gtk_dialog_run (GTK_DIALOG (dialog));
			gtk_widget_destroy (dialog);

			return FALSE;
		}

		last_dir = g_path_get_dirname (filename);
		planner_conf_set_string (CONF_MAIN_LAST_DIR, last_dir, NULL);
		g_free (last_dir);

		g_free (filename);
		
		return TRUE;
	} else {
		return FALSE;
	}
}

static GtkWidget *
window_create_dialog_button (const gchar *icon_name, const gchar *text)
{
	GtkWidget *button;
	GtkWidget *image;
	GtkWidget *label;
	GtkWidget *align;
	GtkWidget *hbox;

	button = gtk_button_new ();

	image = gtk_image_new_from_stock (icon_name, GTK_ICON_SIZE_BUTTON);
	label = gtk_label_new_with_mnemonic (text);

	gtk_label_set_mnemonic_widget (GTK_LABEL (label), GTK_WIDGET (button));
      
	hbox = gtk_hbox_new (FALSE, 2);

	align = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
      
	gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
      
	gtk_container_add (GTK_CONTAINER (button), align);
	gtk_container_add (GTK_CONTAINER (align), hbox);
	gtk_widget_show_all (align);

	return button;
}

GtkWidget *
planner_window_new (PlannerApplication *application)
{
	PlannerWindow     *window;
	PlannerWindowPriv *priv;
	
	window = g_object_new (PLANNER_TYPE_MAIN_WINDOW, NULL);
	priv = window->priv;
	
	priv->application = g_object_ref (application);
	
	priv->project = mrp_project_new (MRP_APPLICATION (application));

	priv->last_saved = g_timer_new ();
	
	g_signal_connect (priv->project, "needs_saving_changed",
			  G_CALLBACK (window_project_needs_saving_changed_cb),
			  window);

	g_signal_connect (priv->project, "notify::name",
			  G_CALLBACK (window_project_notify_name_cb),
			  window);
	
	window_restore_state (window);

	g_signal_connect (window, "delete_event", 
			  G_CALLBACK (window_delete_event_cb),
			  NULL);

	window_populate (window);
	
	window_update_title (window);

	return GTK_WIDGET (window);
}

gboolean
planner_window_open (PlannerWindow *window, const gchar *uri)
{
	PlannerWindowPriv *priv;
	GError           *error = NULL;
	GtkWidget        *dialog;
	EggRecentItem    *item;
	
	g_return_val_if_fail (PLANNER_IS_MAIN_WINDOW (window), FALSE);
	g_return_val_if_fail (uri != NULL, FALSE);

	priv = window->priv;
	
	if (!mrp_project_load (priv->project, uri, &error)) {
		dialog = gtk_message_dialog_new (
			GTK_WINDOW (window),
			GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_MESSAGE_ERROR,
			GTK_BUTTONS_OK,
			error->message);
		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);

		return FALSE;
	} 

	planner_window_check_version (window);

	/* Add the file to the recent list */
	item = egg_recent_item_new_from_uri (uri);
	egg_recent_item_set_mime_type (item, "application/x-planner");
	egg_recent_model_add_full (planner_application_get_recent_model (priv->application), item);
	egg_recent_item_unref (item);

	return TRUE;
}

GtkUIManager *
planner_window_get_ui_manager (PlannerWindow *window)
{
	g_return_val_if_fail (PLANNER_IS_MAIN_WINDOW (window), NULL);
	
	return window->priv->ui_manager;
}

MrpProject *
planner_window_get_project (PlannerWindow *window)
{
	g_return_val_if_fail (PLANNER_IS_MAIN_WINDOW (window), NULL);
	
	return window->priv->project;
}

PlannerApplication *
planner_window_get_application (PlannerWindow  *window)
{
	g_return_val_if_fail (PLANNER_IS_MAIN_WINDOW (window), NULL);
	
	return window->priv->application;
}

void
planner_window_check_version (PlannerWindow *window)
{
	PlannerWindowPriv *priv;
	GtkWidget        *dialog;
	gint              version;

	g_return_if_fail (PLANNER_IS_MAIN_WINDOW (window));

	priv = window->priv;
	
	version = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (priv->project),
						      "version"));
	
	if (version == 1) {
		dialog = gtk_message_dialog_new (
			GTK_WINDOW (window),
			GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_MESSAGE_INFO,
			GTK_BUTTONS_OK,
			_("You have opened a file that was saved with an old "
			  "version of Planner.\n"
			  "\n"
			  "When loading old files, all tasks will use the "
			  "constraint 'Must Start On', since the old version "
			  "did not fully support automatic rescheduling. To "
			  "take advantage of this new feature, you should add "
			  "predecessor relations between tasks that are "
			  "dependant on each other.\n"
			  "\n"
			  "You can add a predecessor relation by clicking on "
			  "the predecessor and dragging to the successor.\n"
			  "\n"
			  "After doing this, you can remove all constraints by "
			  "selecting the menu item 'Remove all constraints' in "
			  "the 'Edit' menu."));
		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
	}
}

/* Returns the project name, filename, or "Unnamed", in that order. */
static gchar *
window_get_name (PlannerWindow *window)
{
	PlannerWindowPriv *priv;
	gchar             *name, *tmp;
	const gchar       *uri;

	priv = window->priv;

	uri = mrp_project_get_uri (priv->project);

	if (uri && strncmp (uri, "sql://", 6) == 0) {
		/* Hack. */
		uri = _("Unnamed database project");
	}
	
	g_object_get (priv->project, "name", &name, NULL);
	if (name == NULL || name[0] == 0) {
		g_free (name);
		
		if (uri == NULL || uri[0] == 0) {
			name = g_strdup (_("Unnamed"));
		} else {
			tmp = g_path_get_basename (uri);
			name = g_markup_escape_text (tmp, -1);
			g_free (tmp);
		}
	}

	return name;
}

static void
window_update_title (PlannerWindow *window)
{
 	PlannerWindowPriv *priv;
	gchar             *name;
	gchar             *title;

	priv = window->priv;

	name = window_get_name (window);

	title = g_strconcat (name, " - Imendio Planner", NULL);

	gtk_window_set_title (GTK_WINDOW (window), title);

	g_free (name);
	g_free (title);
}

void
planner_window_close (PlannerWindow *window)
{
	PlannerWindowPriv *priv;
        gboolean           close = TRUE;
        
	g_return_if_fail (PLANNER_IS_MAIN_WINDOW (window));

	priv = window->priv;

	if (mrp_project_needs_saving (priv->project)) {
		close = window_confirm_exit_run (window);
	}

        if (close) {
		window_save_state (window);

                g_signal_emit (window, signals[CLOSED], 0, NULL);
                
                gtk_widget_destroy (GTK_WIDGET (window));
        }
}

void
planner_window_show_day_type_dialog (PlannerWindow *window)
{
	PlannerWindowPriv *priv;
	
	g_return_if_fail (PLANNER_IS_MAIN_WINDOW (window));

	priv = window->priv;
	
	if (priv->day_type_dialog) {
		gtk_window_present (GTK_WINDOW (priv->day_type_dialog));
	} else {
		priv->day_type_dialog = planner_day_type_dialog_new (window);

		g_object_add_weak_pointer (G_OBJECT (priv->day_type_dialog),
					   (gpointer *) &priv->day_type_dialog);

		gtk_widget_show (priv->day_type_dialog);
	}
}

void
planner_window_show_calendar_dialog (PlannerWindow *window)
{
	PlannerWindowPriv *priv;
	
	g_return_if_fail (PLANNER_IS_MAIN_WINDOW (window));

	priv = window->priv;
	
	if (priv->calendar_dialog) {
		gtk_window_present (GTK_WINDOW (priv->calendar_dialog));
	} else {
		priv->calendar_dialog = planner_calendar_dialog_new (window);
		
		g_object_add_weak_pointer (G_OBJECT (priv->calendar_dialog),
					   (gpointer *) &priv->calendar_dialog);
		
		gtk_widget_show (priv->calendar_dialog);
	}
}

#ifdef FIXME
static gchar *
window_recent_tooltip_func (EggRecentItem *item,
			    gpointer user_data)
{
	return egg_recent_item_get_uri_for_display (item);
}
#endif

PlannerCmdManager *
planner_window_get_cmd_manager (PlannerWindow *window)
{
	g_return_val_if_fail (PLANNER_IS_MAIN_WINDOW (window), NULL);
	
	return window->priv->cmd_manager;
}

static void
window_save_state (PlannerWindow *window)
{
	PlannerWindowPriv *priv;
	GdkWindowState     state;
	gboolean           maximized;

	priv = window->priv;

	state = gdk_window_get_state (GTK_WIDGET (window)->window);
	if (state & GDK_WINDOW_STATE_MAXIMIZED) {
		maximized = TRUE;
	} else {
		maximized = FALSE;
	}

	planner_conf_set_bool (CONF_MAIN_WINDOW_MAXIMIZED, maximized, NULL);

	/* If maximized don't save the size and position */
	if (!maximized) {
		int width, height;
		int x, y;

		gtk_window_get_size (GTK_WINDOW (window), &width, &height);
		planner_conf_set_int (CONF_MAIN_WINDOW_WIDTH, width, NULL);
		planner_conf_set_int (CONF_MAIN_WINDOW_HEIGHT, height, NULL);

		gtk_window_get_position (GTK_WINDOW (window), &x, &y);
		planner_conf_set_int (CONF_MAIN_WINDOW_POS_X, x, NULL);
		planner_conf_set_int (CONF_MAIN_WINDOW_POS_Y, y, NULL);
	}
}

static void
window_restore_state (PlannerWindow *window)
{
	PlannerWindowPriv *priv;
	gboolean exists;
	gboolean maximized;
	int      width, height;
	int      x, y;

	priv = window->priv;

	exists = planner_conf_dir_exists (CONF_MAIN_WINDOW_DIR, NULL);
	
	if (exists) {	
		maximized = planner_conf_get_bool (CONF_MAIN_WINDOW_MAXIMIZED,
						   NULL);
	
		if (maximized) {
			gtk_window_maximize (GTK_WINDOW (window));
		} else {
			width = planner_conf_get_int (CONF_MAIN_WINDOW_WIDTH,
						      NULL);
		
			height = planner_conf_get_int (CONF_MAIN_WINDOW_HEIGHT,
						       NULL);
		
			gtk_window_set_default_size (GTK_WINDOW (window), 
						     width, height);

			x = planner_conf_get_int (CONF_MAIN_WINDOW_POS_X,
						  NULL);
			y = planner_conf_get_int (CONF_MAIN_WINDOW_POS_Y,
						  NULL);

			gtk_window_move (GTK_WINDOW (window), x, y);
		}
	} else {
		gtk_window_set_default_size (GTK_WINDOW (window), 800, 600);
	}
}

void
planner_window_set_status (PlannerWindow *window, const gchar *message)
{
	PlannerWindowPriv *priv;

	priv = window->priv;

	gtk_statusbar_pop (GTK_STATUSBAR (priv->statusbar), 0);
	gtk_statusbar_push (GTK_STATUSBAR (priv->statusbar), 0,
			    message ? message : "");
}

static void
window_menu_item_select_cb (GtkMenuItem   *proxy,
			    PlannerWindow *window)
{
	PlannerWindowPriv *priv;

	GtkAction *action;
	gchar     *message;

	priv = window->priv;

	action = g_object_get_data (G_OBJECT (proxy),  "gtk-action");
	g_return_if_fail (action != NULL);
	
	g_object_get (action, "tooltip", &message, NULL);
	if (message) {
		gtk_statusbar_push (GTK_STATUSBAR (priv->statusbar), 0, message);
		g_free (message);
	}
}

static void
window_menu_item_deselect_cb (GtkMenuItem   *proxy,
			      PlannerWindow *window)
{
	PlannerWindowPriv *priv;

	priv = window->priv;

	gtk_statusbar_pop (GTK_STATUSBAR (priv->statusbar), 0);
}

static void
window_disconnect_proxy_cb (GtkUIManager  *manager,
			    GtkAction     *action,
			    GtkWidget     *proxy,
			    PlannerWindow *window)
{
	if (GTK_IS_MENU_ITEM (proxy)) {
		g_signal_handlers_disconnect_by_func (
			proxy, G_CALLBACK (window_menu_item_select_cb), window);
		g_signal_handlers_disconnect_by_func
			(proxy, G_CALLBACK (window_menu_item_deselect_cb), window);
	}
}

static void
window_connect_proxy_cb (GtkUIManager  *manager,
			 GtkAction     *action,
			 GtkWidget     *proxy,
			 PlannerWindow *window)
{
	if (GTK_IS_MENU_ITEM (proxy)) {
		g_signal_connect (proxy, "select",
				  G_CALLBACK (window_menu_item_select_cb), window);
		g_signal_connect (proxy, "deselect",
				  G_CALLBACK (window_menu_item_deselect_cb), window);
	}
}
