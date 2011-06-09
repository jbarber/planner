/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2004 Alvaro del Castillo <acs@barrapunto.com>
 *
 * This program is free software; you can redistribute it and/or
 * modfy it under the terms of the GNU General Public License as
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
 *
 * TODO:
 *
 *  - Caching answers issues
 */

#include <config.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <string.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <glade/glade.h>
#include <gtk/gtk.h>
#include <gconf/gconf-client.h>
#include <libplanner/mrp-object.h>
#include <libplanner/mrp-property.h>
#include "libplanner/mrp-paths.h"
#include "planner-window.h"
#include "planner-plugin.h"
#include "planner-resource-cmd.h"

/* Evolution Data Server sources */
#include <libedataserver/e-source-list.h>
#include <libedataserver/e-source-group.h>
#include <libedataserver/e-uid.h>
/* Calendar */
#include <libecal/e-cal.h>
/* Addressbook */
#include <libebook/e-book.h>

struct _PlannerPluginPriv {
	MrpProject    *project;
	/* Manage groups of source from e-d-s */
	GtkComboBox   *select_group;
	GtkTreeModel  *groups_model;
	/* Manage resource from a group */
	GtkTreeView   *resources_tree_view;
	GtkTreeModel  *resources_model;
	/* Main widgets */
	GladeXML      *glade;
	GtkWidget     *dialog_get_resources;
	/* Timer to update GUI */
	guint          pulse;
	gboolean       busy;
	/* Progress query id */
	/* Only one not cancelled query is supported */
	gchar         *current_query_id;
	GList         *queries_cancelled;
	/* Books open */
	GList         *books;

	GtkActionGroup *actions;
};

typedef struct {
	PlannerPlugin *plugin;
	const gchar   *uid;
	const gchar   *search;
} AsyncQuery;

enum {
	COL_RESOURCE_NAME,
	COL_RESOURCE_EMAIL,
	COL_RESOURCE_SELECTED,
	COL_RESOURCE_PHOTO,
	COL_RESOURCE_OBJECT,
	NUM_RESOURCE_COLS
};

enum {
	COL_GROUP_NAME,
	COL_GROUP_OBJECT,
	NUM_GROUP_COLS
};

static void eds_plugin_import           (GtkAction             *action,
					 gpointer               user_data,
					 const gchar           *cname);
static void eds_create_groups_model     (GSList                *groups,
					 PlannerPlugin         *plugin);
static void eds_ok_button_clicked       (GtkButton             *button,
					 PlannerPlugin         *plugin);
static void eds_cancel_button_clicked   (GtkButton             *button,
					 PlannerPlugin         *plugin);
static void eds_search_button_clicked   (GtkButton             *button,
					 PlannerPlugin         *plugin);
static void eds_import_change_all       (PlannerPlugin         *plugin,
					 gboolean               state);
static void eds_all_button_clicked      (GtkButton             *button,
					 PlannerPlugin         *plugin);
static void eds_none_button_clicked     (GtkButton             *button,
					 PlannerPlugin         *plugin);
static void eds_stop_button_clicked     (GtkButton             *button,
					 PlannerPlugin         *plugin);
static gboolean eds_search_key_pressed  (GtkEntry              *entry,
					 GdkEventKey           *event,
					 PlannerPlugin         *plugin);
static void eds_column_clicked          (GtkTreeViewColumn     *treeviewcolumn,
					 PlannerPlugin         *plugin);
static void eds_group_selected          (GtkComboBox           *select_group,
					 PlannerPlugin         *plugin);
static void eds_resource_selected       (GtkCellRendererToggle *toggle,
					 const gchar           *path_str,
					 PlannerPlugin         *plugin);
static void eds_import_resource         (gchar                 *name,
					 gchar                 *email,
					 gchar                 *uid,
					 PlannerPlugin         *plugin,
					 GList                 *resources_orig);
static MrpResource * eds_find_resource  (PlannerPlugin         *plugin,
					 const gchar           *uid,
					 GList                 *resources_orig);
static gboolean eds_create_uid_property (PlannerPlugin         *plugin);
static void eds_load_resources          (ESourceGroup          *group,
					 PlannerPlugin         *plugin,
					 const gchar           *search);
static void eds_receive_contacts_cb     (EBook                 *book,
					 EBookStatus            status,
					 GList                 *contacts,
					 gpointer               plugin);
static void eds_receive_book_cb         (EBook                 *book,
					 EBookStatus            status,
					 gpointer               user_data);
static void eds_plugin_busy             (PlannerPlugin         *plugin,
					 gboolean               busy);
static gboolean eds_query_cancelled     (PlannerPlugin         *plugin,
					 const gchar           *uid);
static void eds_dialog_close            (PlannerPlugin         *plugin);

void        plugin_init                 (PlannerPlugin         *plugin);
void        plugin_exit                 (PlannerPlugin         *plugin);


static GtkActionEntry action_entries[] = {
	{ "EDS Import", NULL,
	  N_("EDS"), NULL,
	  N_("Import resources from Evolution Data Server"),
	  G_CALLBACK (eds_plugin_import) },
};
static guint n_action_entries = G_N_ELEMENTS (action_entries);

static
gboolean eds_check_query (gpointer data)
{
	PlannerPlugin *plugin = data;

	GtkProgressBar *progress = GTK_PROGRESS_BAR (glade_xml_get_widget
			(plugin->priv->glade, "progressbar"));

	gtk_progress_bar_pulse (progress);

	return TRUE;
}

static void
eds_plugin_busy (PlannerPlugin *plugin,
		 gboolean       busy)
{
	GdkCursor      *cursor;
	GtkProgressBar *progress = GTK_PROGRESS_BAR (glade_xml_get_widget
			(plugin->priv->glade, "progressbar"));

	if (busy) {
		gint check_time = 1; /* second */

		if (plugin->priv->busy) {
			return;
		}
		plugin->priv->pulse = g_timeout_add_seconds (check_time, eds_check_query, plugin);
		cursor = gdk_cursor_new_for_display (gdk_display_get_default (), GDK_WATCH);
		gtk_widget_set_sensitive
			(glade_xml_get_widget (plugin->priv->glade, "search_box"), FALSE);
		gtk_widget_set_sensitive
			(glade_xml_get_widget (plugin->priv->glade, "progress"), TRUE);
		plugin->priv->busy = TRUE;

	} else {
		g_source_remove (plugin->priv->pulse);
		gtk_progress_bar_set_fraction (progress, 0);

		gtk_widget_set_sensitive
			(glade_xml_get_widget (plugin->priv->glade, "progress"), FALSE);
		gtk_widget_set_sensitive
			(glade_xml_get_widget (plugin->priv->glade, "search_box"), TRUE);
		cursor = gdk_cursor_new_for_display (gdk_display_get_default (), GDK_LEFT_PTR);
		plugin->priv->busy = FALSE;
	}

	gdk_window_set_cursor (gtk_widget_get_parent_window
			       (glade_xml_get_widget (plugin->priv->glade, "ok_button")),
			       cursor);
}

static gint
eds_compare_field (GtkTreeModel *model,
		   GtkTreeIter  *resource1,
		   GtkTreeIter  *resource2,
		   gpointer      user_data)
{
	gchar *field1, *field2;
	gint   res = 0;

	gtk_tree_model_get (model, resource1, GPOINTER_TO_INT (user_data), &field1, -1);
	gtk_tree_model_get (model, resource2, GPOINTER_TO_INT (user_data), &field2, -1);

	if (field1 && field2) {
		res = strcasecmp (field1, field2);
	}
	return res;
}

static void
eds_plugin_import (GtkAction   *action,
		   gpointer     user_data,
		   const gchar *cname)
{
	PlannerPlugin     *plugin;
	PlannerPluginPriv *priv;
	GtkCellRenderer   *renderer;
	GConfClient       *gconf_client;
	ESourceList       *source_list;
	GSList            *groups;
	gchar             *filename;

	plugin = PLANNER_PLUGIN (user_data);
	priv = plugin->priv;

	filename = mrp_paths_get_glade_dir ("/eds.glade");
	priv->glade = glade_xml_new (filename, NULL, NULL);
	g_free (filename);

	priv->dialog_get_resources = glade_xml_get_widget (priv->glade, "resources_get");

	gtk_window_set_transient_for (GTK_WINDOW (priv->dialog_get_resources),
				      GTK_WINDOW (plugin->main_window));
	priv->select_group = GTK_COMBO_BOX (glade_xml_get_widget (priv->glade,
								  "select_group"));
	g_signal_connect (priv->select_group, "changed",
			  G_CALLBACK (eds_group_selected),
			  user_data);

	priv->resources_tree_view = GTK_TREE_VIEW (glade_xml_get_widget (priv->glade,
									 "resources"));

	g_signal_connect (glade_xml_get_widget (priv->glade, "ok_button"),
			  "clicked",
			  G_CALLBACK (eds_ok_button_clicked),
			  plugin);
	g_signal_connect (glade_xml_get_widget (priv->glade, "cancel_button"),
			  "clicked",
			  G_CALLBACK (eds_cancel_button_clicked),
			  plugin);
	g_signal_connect (glade_xml_get_widget (priv->glade, "search_button"),
			  "clicked",
			  G_CALLBACK (eds_search_button_clicked),
			  plugin);
	g_signal_connect (glade_xml_get_widget (priv->glade, "all_button"),
			  "clicked",
			  G_CALLBACK (eds_all_button_clicked),
			  plugin);
	g_signal_connect (glade_xml_get_widget (priv->glade, "none_button"),
			  "clicked",
			  G_CALLBACK (eds_none_button_clicked),
			  plugin);
	g_signal_connect (glade_xml_get_widget (priv->glade, "stop_button"),
			  "clicked",
			  G_CALLBACK (eds_stop_button_clicked),
			  plugin);
	g_signal_connect (glade_xml_get_widget (priv->glade, "search_entry"),
			  "key-press-event",
			  G_CALLBACK (eds_search_key_pressed),
			  plugin);

	gtk_widget_show (priv->dialog_get_resources);


	gconf_client = gconf_client_get_default ();
	source_list = e_source_list_new_for_gconf (gconf_client,
						   "/apps/evolution/addressbook/sources");
	/* List with addressbook groups */
	groups = e_source_list_peek_groups (source_list);
	eds_create_groups_model (groups, plugin);
	gtk_combo_box_set_model (priv->select_group, priv->groups_model);
	renderer = gtk_cell_renderer_text_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (priv->select_group),
				    renderer, TRUE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (priv->select_group),
					renderer, "text", 0, NULL);
	/* g_object_unref (source_list); */
}

static void
eds_create_groups_model (GSList        *groups,
			 PlannerPlugin *plugin)
{
	GtkListStore *model;
	GtkTreeIter   iter;
	GSList       *sl;
	const gchar  *name;

	if (groups == NULL) {
		return;
	}

	model = gtk_list_store_new (NUM_GROUP_COLS, G_TYPE_STRING, G_TYPE_OBJECT);

	for (sl = groups; sl; sl = sl->next) {
		name = e_source_group_peek_name (sl->data);
		gtk_list_store_append (model, &iter);
		gtk_list_store_set (model, &iter,
				    COL_GROUP_NAME, name,
				    COL_GROUP_OBJECT, sl->data, -1);
	}
	plugin->priv->groups_model = GTK_TREE_MODEL (model);
}

/* For now we show all the sources from a group in a List.
   Later we will us a Tree to show them usings groups. */
static void
eds_load_resources (ESourceGroup  *group,
		    PlannerPlugin *plugin,
		    const gchar   *search)
{
	GtkListStore      *model;
	GSList            *sources, *sl;
	PlannerPluginPriv *priv;

	g_return_if_fail (E_IS_SOURCE_GROUP (group));
	sources = e_source_group_peek_sources (group);

	priv = plugin->priv;
	model = GTK_LIST_STORE (priv->resources_model);

	if (sources == NULL) {
		if (model) {
			gtk_list_store_clear (model);
		}
		gtk_widget_set_sensitive (glade_xml_get_widget (priv->glade,
								"search_box"), FALSE);
		return;
	}

	if (model) {
		gtk_list_store_clear (model);
	} else {
		GtkCellRenderer   *toggle;
		guint              column_pos;
		GtkTreeViewColumn *column;

		model = gtk_list_store_new (NUM_RESOURCE_COLS,
					    G_TYPE_STRING,   /* name */
					    G_TYPE_STRING,   /* email */
					    G_TYPE_BOOLEAN,  /* import */
					    GDK_TYPE_PIXBUF, /* photo */
					    G_TYPE_OBJECT);  /* full contact */

		priv->resources_model = GTK_TREE_MODEL (model);

		gtk_tree_view_set_model (priv->resources_tree_view,
					 priv->resources_model);

		/* Name Column with sorting features */
		column_pos = gtk_tree_view_insert_column_with_attributes
			(priv->resources_tree_view,
			 -1, _("Name"),
			 gtk_cell_renderer_text_new (), "text", COL_RESOURCE_NAME,
			 NULL);

		column = gtk_tree_view_get_column (priv->resources_tree_view, column_pos-1);
		gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (priv->resources_model),
						 column_pos-1,
						 eds_compare_field,
						 GINT_TO_POINTER (column_pos-1),
						 NULL);
		gtk_tree_view_column_set_sort_column_id (column, column_pos-1);
		g_signal_connect (gtk_tree_view_get_column
				  (priv->resources_tree_view, column_pos-1),
				  "clicked",
				  G_CALLBACK (eds_column_clicked),
				  plugin);

		/* Email column with sorting features */
		column_pos = gtk_tree_view_insert_column_with_attributes
			(priv->resources_tree_view,
			 -1, _("Email"),
			 gtk_cell_renderer_text_new (), "text", COL_RESOURCE_EMAIL,
			 NULL);

		column = gtk_tree_view_get_column (priv->resources_tree_view, column_pos-1);
		gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (priv->resources_model),
						 column_pos-1,
						 eds_compare_field,
						 GINT_TO_POINTER (column_pos-1),
						 NULL);
		gtk_tree_view_column_set_sort_column_id (column, column_pos-1);
		g_signal_connect (gtk_tree_view_get_column
				  (priv->resources_tree_view, column_pos-1),
				  "clicked",
				  G_CALLBACK (eds_column_clicked),
				  plugin);

		/* Import */
		toggle = gtk_cell_renderer_toggle_new ();
		gtk_tree_view_insert_column_with_attributes
			(priv->resources_tree_view,
			 -1, _("Import"),
			 toggle, "active", COL_RESOURCE_SELECTED,
			 NULL);
		g_signal_connect (toggle, "toggled",
				  G_CALLBACK (eds_resource_selected),
				  plugin);

		/* Photo */
		gtk_tree_view_insert_column_with_attributes
			(priv->resources_tree_view,
			 -1, _("Photo"),
			 gtk_cell_renderer_pixbuf_new (), "pixbuf", COL_RESOURCE_PHOTO,
			 NULL);

		gtk_tree_view_set_headers_clickable (priv->resources_tree_view, TRUE);
	}

	for (sl = sources; sl; sl = sl->next) {
		EBook      *client;
		AsyncQuery *async_query = g_new0 (AsyncQuery, 1);

		g_free (plugin->priv->current_query_id);
		plugin->priv->current_query_id = e_uid_new ();

		async_query->plugin = plugin;
		async_query->uid = plugin->priv->current_query_id;
		async_query->search = search;

		client = e_book_new (sl->data, NULL);
		g_message ("Open book async query: %s", async_query->uid);
		e_book_async_open (client, TRUE, eds_receive_book_cb, async_query);
		eds_plugin_busy (plugin, TRUE);
	}
}

static gboolean
eds_query_cancelled (PlannerPlugin *plugin,
		     const gchar   *uid)
{
	GList *l;

	for (l = plugin->priv->queries_cancelled; l; l = l->next) {
		if (!strcmp (uid, l->data)) {
			g_message ("Received answer from a cancelled query: %s (%s)",
				   (gchar *) l->data, uid);
			/* We can receive several answer for a cancelled query
			   so we must preserver the list */
			/* plugin->priv->queries_cancelled = g_list_remove
			   (plugin->priv->queries_cancelled, l->data); */
			return TRUE;
		}
	}
	return FALSE;
}

static void
eds_receive_book_cb (EBook         *client,
		     EBookStatus    status,
		     gpointer       user_data)
{
	PlannerPlugin *plugin;
	EBookQuery    *query;
	AsyncQuery    *async_query;
	const gchar   *search;
	const gchar   *uid;
	const gchar   *book_uri;
	GtkListStore  *model;

	async_query = user_data;
	plugin = async_query->plugin;
	search = async_query->search;
	uid = async_query->uid;
	model = GTK_LIST_STORE (plugin->priv->resources_model);

	gtk_list_store_clear (model);
	g_free (async_query);

	book_uri = e_book_get_uri (client);

	if (eds_query_cancelled (plugin, uid)) {
		g_message ("Open book query cancelled: %s (%s)", book_uri, uid);
		gtk_widget_set_sensitive (glade_xml_get_widget (plugin->priv->glade,
								"search_box"), TRUE);
		eds_plugin_busy (plugin, FALSE);
		return;
	}

	if (status != E_BOOK_ERROR_OK) {
		g_warning ("Problems opening: %s", book_uri);
		gtk_widget_set_sensitive (glade_xml_get_widget (plugin->priv->glade,
								"search_box"), TRUE);
		eds_plugin_busy (plugin, FALSE);
		return;
	}

	g_message ("Looking the book: %s", book_uri);
	plugin->priv->books = g_list_append (plugin->priv->books, client);

	async_query = g_new0 (AsyncQuery, 1);
	g_free (plugin->priv->current_query_id);
	plugin->priv->current_query_id = e_uid_new ();
	async_query->uid = plugin->priv->current_query_id;
	async_query->plugin = plugin;

	query = e_book_query_any_field_contains (search);
	e_book_async_get_contacts (client, query,
				   eds_receive_contacts_cb,
				   (gpointer) async_query);

	eds_plugin_busy (plugin, TRUE);
	e_book_query_unref (query);
}

static void
eds_receive_contacts_cb (EBook         *book,
			 EBookStatus    status,
			 GList         *contacts,
			 gpointer       user_data)
{
	GtkTreeIter        iter;
	GList             *l;
	GtkListStore      *model;
	PlannerPlugin     *plugin;
	PlannerPluginPriv *priv;
	GdkPixbuf         *pixbuf;
	AsyncQuery        *async_query;
	const gchar       *uid;
	gchar             *filename;

	async_query = (AsyncQuery *) user_data;

	uid = async_query->uid;
	plugin = async_query->plugin;
	priv = plugin->priv;
	model = GTK_LIST_STORE (priv->resources_model);

	g_free (async_query);

	if (eds_query_cancelled (plugin, uid)) {
		g_message ("Answer for query cancelled: %s", uid);
		return;
	}

	g_message ("Book status response: %d", status);
	g_message ("Answer for the query: %s", uid);

	/* Exceed limit is E_BOOK_ERROR_OTHER_ERROR :( */
	if (status == E_BOOK_ERROR_OK || status == E_BOOK_ERROR_OTHER_ERROR) {
		filename = mrp_paths_get_image_dir ("/resources.png");
		pixbuf = gdk_pixbuf_new_from_file (filename, NULL);
		g_free (filename);
		for (l = contacts; l; l = l->next) {
			gchar *name, *email;
			name = e_contact_get (l->data, E_CONTACT_FULL_NAME);
			g_message ("Resource name: %s\n", name);
			email = e_contact_get (l->data, E_CONTACT_EMAIL_1);
			gtk_list_store_append (model, &iter);
			gtk_list_store_set (model, &iter,
					    COL_RESOURCE_NAME, name,
					    COL_RESOURCE_EMAIL, email,
					    COL_RESOURCE_SELECTED, FALSE,
					    COL_RESOURCE_PHOTO, pixbuf,
					    COL_RESOURCE_OBJECT, l->data, -1);
		}
	} else {
		g_warning ("Problem getting contacts ...");
	}

	eds_plugin_busy (plugin, FALSE);
}

static void
eds_all_button_clicked (GtkButton *button,
			PlannerPlugin *plugin)
{
	eds_import_change_all (plugin, TRUE);
}

static void
eds_none_button_clicked (GtkButton *button,
			 PlannerPlugin *plugin)
{
	eds_import_change_all (plugin, FALSE);
}

static void
eds_import_change_all (PlannerPlugin *plugin,
		       gboolean       state)
{
	GtkTreeIter        iter;
	PlannerPluginPriv *priv = plugin->priv;

	gtk_tree_model_get_iter_first (priv->resources_model, &iter);

	if (!gtk_list_store_iter_is_valid (GTK_LIST_STORE (priv->resources_model),
					  &iter)) {
		return;
	}

	do {
		gtk_list_store_set (GTK_LIST_STORE (priv->resources_model),
				    &iter,
				    COL_RESOURCE_SELECTED, state,
				    -1);
	} while (gtk_tree_model_iter_next (priv->resources_model, &iter));
}

static MrpResource *
eds_find_resource (PlannerPlugin *plugin,
		   const gchar   *uid,
		   GList         *resources_orig)
{
	GList *l;
	MrpResource *resource = NULL;

	for (l = resources_orig; l; l = l->next) {
		gchar *current_uid;

		mrp_object_get (l->data, "eds-uid", &current_uid, NULL);
		if (!current_uid) {
			continue;
		}
		if (!strcmp (uid, current_uid)) {
			resource = l->data;
			break;
		}
	}
	return resource;
}

/* If the resource is already imported, update it */
static void
eds_import_resource (gchar         *name,
		     gchar         *email,
		     gchar         *uid,
		     PlannerPlugin *plugin,
		     GList         *resources_orig)
{
	MrpResource *resource;
	gchar       *note = _("Imported from Evolution Data Server");
	gchar       *note_update = _("Updated from Evolution Data Server");


	resource = eds_find_resource (plugin, uid, resources_orig);
	if (!resource) {
		resource = mrp_resource_new ();
		planner_resource_cmd_insert (plugin->main_window, resource);
		mrp_object_set (resource,
				"type", MRP_RESOURCE_TYPE_WORK,
				"units", 1,
				"note", g_strdup_printf ("%s:\n%s", note, uid),
				"eds-uid", g_strdup (uid),
				NULL);
	} else {
		gchar *note_now;
		mrp_object_get (resource, "note", &note_now, NULL);
		mrp_object_set (resource, "note",
				g_strdup_printf ("%s\n%s", note_now, note_update), NULL);
		g_free (note_now);
	}

	if (name) {
		mrp_object_set (resource, "name", name, NULL);
	}
	if (email) {
		mrp_object_set (resource, "email", email, NULL);
	}
}

static void
eds_group_selected (GtkComboBox   *select_group,
		    PlannerPlugin *plugin)
{
	GtkTreeIter        iter;
	PlannerPluginPriv *priv = plugin->priv;
	ESourceGroup      *group;

	gtk_widget_set_sensitive (glade_xml_get_widget (priv->glade, "search_box"), TRUE);

	if (gtk_combo_box_get_active_iter (select_group, &iter)) {
		gtk_tree_model_get (priv->groups_model, &iter, COL_GROUP_OBJECT, &group, -1);
		eds_load_resources (group, plugin, "");
	}
}

static void
eds_resource_selected (GtkCellRendererToggle *toggle,
		       const gchar           *path_str,
		       PlannerPlugin         *plugin)
{
	GtkTreePath  *path;
	GtkTreeModel *model;
	GtkTreeIter   iter;
	gboolean      selected;
	EContact     *contact;

	path = gtk_tree_path_new_from_string (path_str);

	model = plugin->priv->resources_model;

	gtk_tree_model_get_iter (model, &iter, path);

	gtk_tree_model_get (model,
			    &iter,
			    COL_RESOURCE_SELECTED, &selected,
			    COL_RESOURCE_OBJECT, &contact,
			    -1);

	gtk_list_store_set (GTK_LIST_STORE (model),
			    &iter,
			    COL_RESOURCE_SELECTED, !selected,
			    -1);

	gtk_tree_path_free (path);
}

static void
eds_ok_button_clicked (GtkButton     *button,
		       PlannerPlugin *plugin)
{
	GtkTreeIter        iter;
	PlannerPluginPriv *priv = plugin->priv;
	GList             *resources_orig;

	/* We are going to modify the resources. Work with a copy */
	resources_orig = mrp_project_get_resources (plugin->priv->project);

	if (!priv->resources_model) {
		eds_dialog_close (plugin);
		return;
	}

	gtk_tree_model_get_iter_first (priv->resources_model, &iter);

	if (!gtk_list_store_iter_is_valid (GTK_LIST_STORE (priv->resources_model),
					   &iter)) {
		eds_dialog_close (plugin);
		return;
	}

	/* Custom property for e-d-s resource UID */
	if (!mrp_project_has_property (plugin->priv->project,
				       MRP_TYPE_RESOURCE, "eds-uid")) {
		eds_create_uid_property (plugin);
	}

	do {
		EContact *contact;
		gboolean  selected;

		gtk_tree_model_get (priv->resources_model, &iter,
				    COL_RESOURCE_SELECTED, &selected,
				    COL_RESOURCE_OBJECT, &contact,
				    -1);

		if (selected) {
			gchar *name = e_contact_get (contact, E_CONTACT_FULL_NAME);
			gchar *email = e_contact_get (contact, E_CONTACT_EMAIL_1);
			gchar *eds_uid = e_contact_get (contact, E_CONTACT_UID);
			eds_import_resource (name, email, eds_uid, plugin, resources_orig);
			g_free (name);
			g_free (email);
			g_free (eds_uid);
		}
	} while (gtk_tree_model_iter_next (priv->resources_model, &iter));

	eds_dialog_close (plugin);
}

static void
eds_cancel_button_clicked (GtkButton     *button,
			   PlannerPlugin *plugin)
{
	eds_dialog_close (plugin);
}

static void
eds_search_button_clicked (GtkButton     *button,
			   PlannerPlugin *plugin)
{
	const gchar       *search;
	PlannerPluginPriv *priv = plugin->priv;
	GtkTreeIter        iter;
	ESourceGroup      *group;

	search = gtk_entry_get_text (GTK_ENTRY
				     (glade_xml_get_widget (priv->glade,"search_entry")));

	if (gtk_combo_box_get_active_iter (priv->select_group, &iter)) {
		gtk_tree_model_get (priv->groups_model, &iter, COL_GROUP_OBJECT, &group, -1);
		eds_load_resources (group, plugin, search);
	}
}

static gboolean
eds_search_key_pressed (GtkEntry      *entry,
			GdkEventKey   *event,
			PlannerPlugin *plugin)
{
	PlannerPluginPriv *priv = plugin->priv;
	GtkTreeIter        iter;
	ESourceGroup      *group;

	if (event->keyval == GDK_Return) {
		if (gtk_combo_box_get_active_iter (priv->select_group, &iter)) {
			gtk_tree_model_get (priv->groups_model, &iter,
					    COL_GROUP_OBJECT, &group, -1);
			eds_load_resources (group, plugin, gtk_entry_get_text (entry));
		}
	}
	return FALSE;
}

static void
eds_stop_button_clicked (GtkButton      *button,
			 PlannerPlugin  *plugin)
{
	GtkProgressBar *progress = GTK_PROGRESS_BAR
		(glade_xml_get_widget (plugin->priv->glade, "progressbar"));

	g_message ("Cancelling the query: %s", plugin->priv->current_query_id);

	plugin->priv->queries_cancelled =
		g_list_append (plugin->priv->queries_cancelled,
			       g_strdup (plugin->priv->current_query_id));
	gtk_progress_bar_set_text (progress,_("Query cancelled."));
	eds_plugin_busy (plugin, FALSE);
}

static void
eds_column_clicked (GtkTreeViewColumn *column,
		    PlannerPlugin     *plugin)
{
	GtkSortType order = gtk_tree_view_column_get_sort_order (column);
	GtkSortType new_order;

	if (order == GTK_SORT_ASCENDING) {
		new_order = GTK_SORT_DESCENDING;
	} else {
		new_order = GTK_SORT_ASCENDING;
	}
	gtk_tree_view_column_set_sort_order (column, new_order);
}

static void
eds_dialog_close (PlannerPlugin *plugin)
{
	PlannerPluginPriv *priv = plugin->priv;
	GList             *l;

	/* Cancel the actual query */
	eds_stop_button_clicked
		(GTK_BUTTON (glade_xml_get_widget (priv->glade, "stop_button")), plugin);

	/* Close books with e-d-s */
	for (l = priv->books; l; l = l->next) {
		g_object_unref (l->data);
	}
	g_list_free (priv->books);
	priv->books = NULL;


	/* Models */
	if (priv->groups_model) {
		g_object_unref (priv->groups_model);
		priv->groups_model = NULL;
	}
	/* FIXME: We need to free the EContacts */
	if (priv->resources_model) {
		g_object_unref (priv->resources_model);
		priv->resources_model = NULL;
	}

	g_object_unref (priv->glade);

	gtk_widget_destroy (priv->dialog_get_resources);
}

/* FIXME: Undo support : planner-property-dialog.c */
static gboolean
eds_create_uid_property (PlannerPlugin *plugin)
{
	MrpProperty *property;

	property = mrp_property_new ("eds-uid",
				     MRP_PROPERTY_TYPE_STRING,
				     _("Evolution Data Server UID"),
				     _("Identifier used by Evolution Data Server for resources"),
				     FALSE);

	mrp_project_add_property (plugin->priv->project,
				  MRP_TYPE_RESOURCE,
				  property,
				  FALSE);
	if (!mrp_project_has_property (plugin->priv->project,
				       MRP_TYPE_RESOURCE, "eds-uid")) {
		return FALSE;
	}
	return TRUE;
}

/* FIXME: Undo support */
G_MODULE_EXPORT void
plugin_init (PlannerPlugin *plugin)
{
	PlannerPluginPriv *priv;
	GtkUIManager      *ui;
	gchar		  *filename;

	priv = g_new0 (PlannerPluginPriv, 1);
	plugin->priv = priv;
	priv->project = planner_window_get_project (plugin->main_window);

	priv->actions = gtk_action_group_new ("EDS plugin actions");
	gtk_action_group_set_translation_domain (priv->actions, GETTEXT_PACKAGE);

	gtk_action_group_add_actions (priv->actions, action_entries, n_action_entries, plugin);

	ui = planner_window_get_ui_manager (plugin->main_window);
	gtk_ui_manager_insert_action_group (ui, priv->actions, 0);

	filename = mrp_paths_get_ui_dir ("eds-plugin.ui");
	gtk_ui_manager_add_ui_from_file (ui, filename, NULL);
	g_free (filename);
	gtk_ui_manager_ensure_update (ui);
}

G_MODULE_EXPORT void
plugin_exit (PlannerPlugin *plugin)
{
	PlannerPluginPriv *priv;
	GtkUIManager      *ui;
	GList             *l;

	priv = plugin->priv;

	for (l = priv->queries_cancelled; l; l = l->next) {
		g_free (l->data);
	}
	g_list_free (priv->queries_cancelled);

	ui = planner_window_get_ui_manager (plugin->main_window);
	gtk_ui_manager_remove_action_group (ui, priv->actions);
	g_object_unref (priv->actions);

	g_free (priv);
	/*g_message ("Test exit");*/
}
