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
#include <bonobo/bonobo-ui-component.h>
#include <bonobo/bonobo-ui-util.h>
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
#include <gconf/gconf-client.h>
#include <libgnome/gnome-help.h>
#include <libgnome/gnome-i18n.h>
#include <libgnomeui/gnome-about.h>
#include <libgnomeui/gnome-href.h>
#include <libgnomeprintui/gnome-print-dialog.h>
#include <libgnomeprintui/gnome-print-job-preview.h>
#include <libplanner/mrp-error.h>
#include <libplanner/mrp-project.h>
#include <libegg/recent-files/egg-recent-view.h>
#include <libegg/recent-files/egg-recent-view-bonobo.h>
#include "planner-marshal.h"
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

	BonoboUIContainer   *ui_container;
	BonoboUIComponent   *ui_component;

	PlannerCmdManager   *cmd_manager;

	MrpProject          *project;

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

	EggRecentViewBonobo *view;	
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
static void       window_new_cb                          (BonoboUIComponent            *component,
							  gpointer                      data,
							  const char                   *cname);
static void       window_open_cb                         (BonoboUIComponent            *component,
							  gpointer                      data,
							  const char                   *cname);
static void       window_save_as_cb                      (BonoboUIComponent            *component,
							  gpointer                      data,
							  const char                   *cname);
static void       window_save_cb                         (BonoboUIComponent            *component,
							  gpointer                      data,
							  const char                   *cname);
static void       window_print_cb                        (BonoboUIComponent            *component,
							  gpointer                      data,
							  const char                   *cname);
static void       window_print_preview_cb                (BonoboUIComponent            *component,
							  gpointer                      data,
							  const char                   *cname);
static void       window_properties_cb                   (BonoboUIComponent            *component,
							  gpointer                      data,
							  const char                   *cname);
static void       window_close_cb                        (BonoboUIComponent            *component,
							  gpointer                      data,
							  const char                   *cname);
static void       window_exit_cb                         (BonoboUIComponent            *component,
							  gpointer                      data,
							  const char                   *cname);
static void       window_undo_cb                         (BonoboUIComponent            *component,
							  gpointer                      data,
							  const char                   *cname);
static void       window_redo_cb                         (BonoboUIComponent            *component,
							  gpointer                      data,
							  const char                   *cname);
static void       window_project_props_cb                (BonoboUIComponent            *component,
							  gpointer                      data,
							  const char                   *cname);
static void       window_manage_calendars_cb             (BonoboUIComponent            *component,
							  gpointer                      data,
							  const char                   *cname);
static void       window_edit_day_types_cb               (BonoboUIComponent            *component,
							  gpointer                      data,
							  const char                   *cname);
static void       window_edit_phases_cb                  (BonoboUIComponent            *component,
							  gpointer                      data,
							  const char                   *cname);
static void       window_preferences_cb                  (BonoboUIComponent            *component,
							  gpointer                      data,
							  const char                   *cname);
static void       window_ui_component_event_cb           (BonoboUIComponent            *comp,
							  const gchar                  *path,
							  Bonobo_UIComponent_EventType  type,
							  const gchar                  *state_string,
							  PlannerWindow                *window);
static void       window_help_cb                         (BonoboUIComponent            *component,
							  gpointer                      data,
							  const char                   *cname);
static void       window_about_cb                        (BonoboUIComponent            *component,
							  gpointer                      data,
							  const char                   *cname);
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
static gchar *    window_recent_tooltip_func             (EggRecentItem                *item,
							  gpointer                      user_data);
static void       window_save_state                      (PlannerWindow *window);
static void       window_restore_state                   (PlannerWindow *window);

#define GCONF_PATH                  "/apps/planner"
#define GCONF_MAIN_WINDOW_DIR       "/apps/planner/ui"
#define GCONF_MAIN_WINDOW_MAXIMIZED "/apps/planner/ui/main_window_maximized"
#define GCONF_MAIN_WINDOW_WIDTH     "/apps/planner/ui/main_window_width"
#define GCONF_MAIN_WINDOW_HEIGHT    "/apps/planner/ui/main_window_height"
#define GCONF_MAIN_WINDOW_POS_X     "/apps/planner/ui/main_window_position_x"
#define GCONF_MAIN_WINDOW_POS_Y     "/apps/planner/ui/main_window_position_y"

#define VIEW_PATH "/menu/View/Views placeholder"
#define VIEW_GROUP "view group"
	
static BonoboWindowClass *parent_class = NULL;
static guint signals[LAST_SIGNAL];

static BonoboUIVerb verbs[] = {
	BONOBO_UI_VERB ("FileNew",		window_new_cb),
	BONOBO_UI_VERB ("FileOpen",		window_open_cb),
	BONOBO_UI_VERB ("FileSave",		window_save_cb),
	BONOBO_UI_VERB ("FileSaveAs",		window_save_as_cb),
	BONOBO_UI_VERB ("FileProperties",	window_properties_cb),
	BONOBO_UI_VERB ("FilePrint",		window_print_cb),
	BONOBO_UI_VERB ("FilePrintPreview",	window_print_preview_cb),
	BONOBO_UI_VERB ("FileClose",		window_close_cb),
	BONOBO_UI_VERB ("FileExit",		window_exit_cb),

	BONOBO_UI_VERB ("EditUndo",		window_undo_cb),
	BONOBO_UI_VERB ("EditRedo",		window_redo_cb),
	BONOBO_UI_VERB ("EditProjectProps",	window_project_props_cb),

	BONOBO_UI_VERB ("ManageCalendars",      window_manage_calendars_cb),
	BONOBO_UI_VERB ("EditDayTypes",         window_edit_day_types_cb),
	BONOBO_UI_VERB ("EditPhases",           window_edit_phases_cb),

	BONOBO_UI_VERB ("PreferencesEditPreferences",
			window_preferences_cb),

	BONOBO_UI_VERB ("HelpHelp",		window_help_cb),
	BONOBO_UI_VERB ("HelpAbout",		window_about_cb),

	BONOBO_UI_VERB_END
};

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

		type = g_type_register_static (BONOBO_TYPE_WINDOW,
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

	priv->ui_container = 
		bonobo_window_get_ui_container (BONOBO_WINDOW (window));

	priv->ui_component = bonobo_ui_component_new ("Planner");

	bonobo_ui_component_set_container (priv->ui_component,
					   BONOBO_OBJREF (priv->ui_container),
					   NULL);

	g_signal_connect_object (priv->ui_component,
				 "ui-event",
				 G_CALLBACK (window_ui_component_event_cb),
				 window,
				 0);
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

static void
window_add_view_menu_item (BonoboUIComponent *ui,
			   guint              index,
			   const gchar       *label)
{
	gchar *xml_item, *xml_command; 
	gchar *command_name;
	gchar *path;

	command_name = g_strdup_printf ("view %u", index);

	xml_item = g_strdup_printf ("<menuitem name=\"%u\" id=\"%s\" type=\"radio\" group=\"%s\"/>\n", 
				    index, command_name, VIEW_GROUP);

	bonobo_ui_component_set (ui, VIEW_PATH, xml_item, NULL);

	g_free (xml_item);

	path = g_strdup_printf ("%s/%u", VIEW_PATH, index);
	bonobo_ui_component_set_prop (ui, path, "label", label, NULL);
	g_free (path);

	/* Make the command node here too, so callers can immediately set
	 * properties on it (otherwise it doesn't get created until some
	 * time later).
	 */
	xml_command = g_strdup_printf ("<cmd name=\"view %u\"/>\n", index);
	bonobo_ui_component_set (ui, "/commands", xml_command, NULL);

	g_free (xml_command);
	g_free (command_name);
}

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

static void
window_populate (PlannerWindow *window)
{
	PlannerWindowPriv *priv;
	GtkWidget        *hbox;
	GList            *l;
	GtkWidget        *view_widget;
	PlannerView           *view;
	gint              view_num;

	priv = window->priv;

	bonobo_ui_component_freeze (priv->ui_component, NULL);

	bonobo_ui_util_set_ui (priv->ui_component,
 			       DATADIR,
 			       "/planner/ui/main-window.ui",
 			       "planner",
 			       NULL);

	bonobo_ui_component_add_verb_list_with_data (priv->ui_component,
						     verbs, window);

	/* Handle recent file stuff */
	priv->view = egg_recent_view_bonobo_new (priv->ui_component,
						 "/menu/File/EggRecentDocuments/");
	egg_recent_view_set_model (EGG_RECENT_VIEW (priv->view),
				   planner_application_get_recent_model (priv->application));
	egg_recent_view_bonobo_set_tooltip_func (priv->view,
						 window_recent_tooltip_func,
						 NULL);

	g_signal_connect (priv->view, "activate",
			  G_CALLBACK (planner_window_open_recent), window);

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

	bonobo_window_set_contents (BONOBO_WINDOW (window), hbox);

	/* Load views. */
	priv->views = planner_view_loader_load (window);

	view_num = 0;
	for (l = priv->views; l; l = l->next) {
		view = l->data;
		
		view_widget = planner_view_get_widget (view);
		gtk_widget_show (view_widget);
		
		planner_sidebar_append (PLANNER_SIDEBAR (priv->sidebar),
					planner_view_get_icon (view),
					planner_view_get_label (view));

		window_add_view_menu_item (priv->ui_component,
					   view_num++,
					   planner_view_get_menu_label (view));
		
		gtk_notebook_append_page (
			GTK_NOTEBOOK (priv->notebook),
			view_widget,
			gtk_label_new (planner_view_get_label (view)));
	}

	/* Load plugins. */
	priv->plugins = planner_plugin_loader_load (window);

	bonobo_ui_component_thaw (priv->ui_component, NULL);

	window_view_selected (PLANNER_SIDEBAR (priv->sidebar), 0, window);
}

static void
window_view_selected (PlannerSidebar    *sidebar,
		      gint          index,
		      PlannerWindow *window)
{
	PlannerWindowPriv *priv;
	GList            *list;
	PlannerView           *view;
	gchar            *cmd;
	gchar            *state;
		
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
	
	priv->current_view = view;

	cmd = g_strdup_printf ("/commands/view %u", index);
	state = bonobo_ui_component_get_prop (priv->ui_component,
					      cmd,
					      "state",
					      NULL);

	if (state == NULL || !strcmp (state, "0")) {
		g_signal_handlers_block_by_func (priv->ui_component,
						 window_ui_component_event_cb,
						 window);
		
		bonobo_ui_component_set_prop (priv->ui_component,
					      cmd,
					      "state",
					      "1",
					      NULL);

		g_signal_handlers_unblock_by_func (priv->ui_component,
						   window_ui_component_event_cb,
						   window);
	}

	g_free (cmd);
	g_free (state);
}

static void
window_new_cb (BonoboUIComponent *component,
	       gpointer           data, 
	       const char        *cname)
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
	GConfClient       *gconf_client;
	gchar             *last_dir;
	
	priv = window->priv;
	
	gconf_client = planner_application_get_gconf_client ();
	
	last_dir = gconf_client_get_string (gconf_client,
					    GCONF_PATH "/general/last_dir",
					    NULL);
	
	if (last_dir == NULL) {
		last_dir = g_strdup (g_get_home_dir ());
	}
	
	if (last_dir[strlen (last_dir)] != G_DIR_SEPARATOR) {
		gchar *tmp;
		
		tmp = g_strconcat (last_dir, G_DIR_SEPARATOR_S, NULL);
		g_free (last_dir);
		last_dir = tmp;
	}

	return last_dir;
}

static void
window_open_cb (BonoboUIComponent *component,
		gpointer           data,
		const char        *cname)
{
	PlannerWindow     *window;
	PlannerWindowPriv *priv;
	GtkWidget         *file_chooser;
	gint               response;
	gchar             *filename = NULL;
	gchar             *last_dir;
	GtkWidget         *new_window;
	GConfClient       *gconf_client;

	window = PLANNER_WINDOW (data);
	priv = window->priv;

	gconf_client = planner_application_get_gconf_client ();

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
		gconf_client_set_string (gconf_client,
					 GCONF_PATH "/general/last_dir",
					 last_dir,
					 NULL);
		g_free (last_dir);
		g_free (filename);		
	}
}

static void
window_save_as_cb (BonoboUIComponent *component,
		   gpointer           data,
		   const char        *cname)
{
	PlannerWindow *window;

	window = PLANNER_WINDOW (data);

        window_do_save_as (window);
}

static void
window_save_cb (BonoboUIComponent *component,
		gpointer           data,
		const char        *cname)
{
	PlannerWindow     *window;

	window = PLANNER_WINDOW (data);

	window_do_save (window, FALSE);
}

static void
window_print_preview_cb (BonoboUIComponent *component,
			 gpointer           data,
			 const char        *cname)
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
window_print_cb (BonoboUIComponent *component,
		 gpointer           data,
		 const char        *cname)
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
window_properties_cb (BonoboUIComponent *component,
		      gpointer           data, 
		      const char        *cname)
{
}

static void
window_close_cb (BonoboUIComponent *component,
		 gpointer           data,
		 const char        *cname)
{
	planner_window_close (PLANNER_WINDOW (data));
}

static void
window_exit_cb (BonoboUIComponent *component, 
		gpointer           data, 
		const char        *cname)
{
	PlannerWindow     *window;
	PlannerWindowPriv *priv;
	
	window = PLANNER_WINDOW (data);
	priv   = window->priv;
	
	planner_application_exit (priv->application);
}

static void
window_redo_cb (BonoboUIComponent *component,
		gpointer           data,
		const char        *cname)
{
	PlannerWindow     *window;
	PlannerWindowPriv *priv;

	window = PLANNER_WINDOW (data);

	priv = window->priv;
	
	planner_cmd_manager_redo (priv->cmd_manager);
}

static void
window_undo_cb (BonoboUIComponent *component,
		gpointer           data,
		const char        *cname)
{
	PlannerWindow     *window;
	PlannerWindowPriv *priv;

	window = PLANNER_WINDOW (data);

	priv = window->priv;

	planner_cmd_manager_undo (priv->cmd_manager);
}

static void
window_project_props_cb (BonoboUIComponent *component,
			 gpointer           data,
			 const char        *cname)
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
window_manage_calendars_cb (BonoboUIComponent *component,
			    gpointer           data,
			    const char        *cname)
{
	PlannerWindow *window;
	
	window = PLANNER_WINDOW (data);

	planner_window_show_calendar_dialog (window);
}

static void
window_edit_day_types_cb (BonoboUIComponent *component,
			  gpointer           data,
			  const char        *cname)
{
	PlannerWindow *window;
	
	window = PLANNER_WINDOW (data);

	planner_window_show_day_type_dialog (window);
}

static void
window_edit_phases_cb (BonoboUIComponent *component,
		       gpointer           data,
		       const char        *cname)
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
window_preferences_cb (BonoboUIComponent *component,
		       gpointer           data,
		       const char        *cname)
{
}

static void
window_ui_component_event_cb (BonoboUIComponent            *comp,
			      const gchar                  *path,
			      Bonobo_UIComponent_EventType  type,
			      const gchar                  *state_string,
			      PlannerWindow                 *window)
{
	PlannerWindowPriv *priv;
	gint              index;

	priv = window->priv;
	
	/* View switching. */
	if (!strcmp (state_string, "1") && 
	    strlen (path) >= 5 && !strncmp (path, "view ", 5)) {
		index = atoi (path + 5);
		
		window_view_selected (PLANNER_SIDEBAR (priv->sidebar),
				      index,
				      window);
	}
}

static void
window_help_cb (BonoboUIComponent *component, 
		gpointer           data,
		const char        *cname)
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
window_about_cb (BonoboUIComponent *component, 
		 gpointer           data,
		 const char        *cname)
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
	const gchar *translator_credits = N_("translator_credits");
	
	about = gnome_about_new ("Imendio Planner", VERSION,
				 "", /*"Copyright \xc2\xa9"*/
				 _("A Project Management application for the GNOME desktop"),
				 authors,
				 documenters,
				 strcmp (translator_credits, _("translator_credits")) != 0 ? _(translator_credits) : NULL,
				 NULL);
	
	hbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (about)->vbox), hbox, FALSE, FALSE, 0);
	
	href = gnome_href_new ("http://planner.imendio.org/",
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

	bonobo_ui_component_set_prop (priv->ui_component, 
				      "/commands/EditUndo",
				      "sensitive", state ? "1" : "0",
				      NULL);
	bonobo_ui_component_set_prop (priv->ui_component, 
				      "/commands/EditUndo",
				      "tip", label,
				      NULL);

	bonobo_ui_component_set_prop (priv->ui_component, 
				      "/commands/EditUndo",
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
	
	bonobo_ui_component_set_prop (priv->ui_component, 
				      "/commands/EditRedo",
				      "sensitive", state ? "1" : "0", 
				      NULL);	
	bonobo_ui_component_set_prop (priv->ui_component, 
				      "/commands/EditRedo",
				      "tip", label,
				      NULL);	

	bonobo_ui_component_set_prop (priv->ui_component, 
				      "/commands/EditRedo",
				      "label", label,
				      NULL);	
}

static void
window_project_needs_saving_changed_cb (MrpProject   *project,
					gboolean      needs_saving,
					PlannerWindow *window)
{
	PlannerWindowPriv *priv;
	gchar            *value;
	
	priv = window->priv;
	
	if (needs_saving) {
		g_timer_reset (priv->last_saved);
	}

	value = needs_saving ? "1" : "0";

	bonobo_ui_component_set_prop (priv->ui_component, 
				      "/commands/FileSave",
				      "sensitive", value, 
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
	GConfClient      *gconf_client;
	EggRecentItem    *item;

	priv = window->priv;

	gconf_client = planner_application_get_gconf_client ();

	file_chooser = gtk_file_chooser_dialog_new (_("Save a file"),
						    GTK_WINDOW (window),
						    GTK_FILE_CHOOSER_ACTION_SAVE,
						    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
						    GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
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
		gconf_client_set_string (gconf_client,
					 GCONF_PATH "/general/last_dir",
					 last_dir,
					 NULL);
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

BonoboUIContainer *
planner_window_get_ui_container (PlannerWindow *window)
{
	g_return_val_if_fail (PLANNER_IS_MAIN_WINDOW (window), NULL);
	
	return window->priv->ui_container;
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

static gchar *
window_recent_tooltip_func (EggRecentItem *item,
			    gpointer user_data)
{
	return egg_recent_item_get_uri_for_display (item);
}

PlannerCmdManager *
planner_window_get_cmd_manager (PlannerWindow *window)
{
	g_return_val_if_fail (PLANNER_IS_MAIN_WINDOW (window), NULL);
	
	return window->priv->cmd_manager;
}

static void
window_save_state (PlannerWindow *window)
{
	GConfClient       *gconf_client;
	PlannerWindowPriv *priv;
	GdkWindowState     state;
	gboolean           maximized;

	priv = window->priv;

	gconf_client = planner_application_get_gconf_client ();

	state = gdk_window_get_state (GTK_WIDGET (window)->window);
	if (state & GDK_WINDOW_STATE_MAXIMIZED) {
		maximized = TRUE;
	} else {
		maximized = FALSE;
	}

	gconf_client_set_bool (gconf_client,
			       GCONF_MAIN_WINDOW_MAXIMIZED,
			       maximized, NULL);

	/* If maximized don't save the size and position */
	if (!maximized) {
		int width, height;
		int x, y;

		gtk_window_get_size (GTK_WINDOW (window), &width, &height);
		gconf_client_set_int (gconf_client,
				      GCONF_MAIN_WINDOW_WIDTH,
				      width, NULL);
		gconf_client_set_int (gconf_client,
				      GCONF_MAIN_WINDOW_HEIGHT,
				      height, NULL);

		gtk_window_get_position (GTK_WINDOW (window), &x, &y);
		gconf_client_set_int (gconf_client,
				      GCONF_MAIN_WINDOW_POS_X,
				      x, NULL);
		gconf_client_set_int (gconf_client,
				      GCONF_MAIN_WINDOW_POS_Y,
				      y, NULL);
	}
}

static void
window_restore_state (PlannerWindow *window)
{
	GConfClient *gconf_client;
	PlannerWindowPriv *priv;
	gboolean exists;
	gboolean maximized;
	int      width, height;
	int      x, y;

	priv = window->priv;
	gconf_client = planner_application_get_gconf_client ();

	exists = gconf_client_dir_exists (gconf_client,
					  GCONF_MAIN_WINDOW_DIR,
					  NULL);
	
	if (exists) {	
		maximized = gconf_client_get_bool (gconf_client,
						   GCONF_MAIN_WINDOW_MAXIMIZED,
						   NULL);
	
		if (maximized) {
			gtk_window_maximize (GTK_WINDOW (window));
		} else {
			width = gconf_client_get_int (gconf_client,
						      GCONF_MAIN_WINDOW_WIDTH,
						      NULL);
		
			height = gconf_client_get_int (gconf_client,
						       GCONF_MAIN_WINDOW_HEIGHT,
						       NULL);
		
			gtk_window_set_default_size (GTK_WINDOW (window), 
						     width, height);

			x = gconf_client_get_int (gconf_client,
						  GCONF_MAIN_WINDOW_POS_X,
						  NULL);
			y = gconf_client_get_int (gconf_client,
						  GCONF_MAIN_WINDOW_POS_Y,
						  NULL);

			gtk_window_move (GTK_WINDOW (window), x, y);
		}
	} else {
		gtk_window_set_default_size (GTK_WINDOW (window), 800, 600);
	}
}
