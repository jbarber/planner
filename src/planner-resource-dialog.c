/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
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
#include <time.h>
#include <string.h>
#include <glade/glade.h>
#include <gtk/gtkentry.h>
#include <gtk/gtkdialog.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkcombo.h>
#include <gtk/gtkliststore.h>
#include <gtk/gtkoptionmenu.h>
#include <gtk/gtkmenu.h>
#include <gtk/gtkmenuitem.h>
#include <gtk/gtktreeview.h>
#include <gtk/gtktreestore.h>
#include <gtk/gtkcellrenderertext.h>
#include <gtk/gtkcellrenderertoggle.h>
#include <gtk/gtktextview.h>
#include <gtk/gtktextbuffer.h>
#include <libgnome/gnome-i18n.h>
#include <libplanner/mrp-resource.h>
#include <libplanner/mrp-calendar.h>
#include <libplanner/mrp-time.h>
#include "planner-format.h"
#include "planner-resource-dialog.h"

typedef struct {
	PlannerWindow  *main_window;
	MrpResource   *resource;
	GtkWidget     *dialog;
	GtkWidget     *name_entry;
	GtkWidget     *type_menu;
	GtkWidget     *email_entry;
	GtkWidget     *group_menu;
	GtkWidget     *cost_entry;
	GtkWidget     *calendar_tree_view;
	GtkWidget     *note_textview;
	GtkTextBuffer *note_buffer;

	/* Slightly ugly, keep the last selected one around so we can toggle it
	 * off later.
	 */
	MrpCalendar   *selected_calendar;
} DialogData;

typedef struct {
	DialogData    *data;
	MrpCalendar   *calendar;
	gboolean       found;
	GtkTreeIter    found_iter;
} FindCalData;

enum {
	COL_CALENDAR,
	COL_NAME,
	COL_SELECTED,
	NUM_COLS
};


static void  resource_dialog_name_changed_cb              (GtkWidget    *w,
							   DialogData   *data);
static void  resource_dialog_resource_name_changed_cb     (MrpResource  *resource,  
							   GParamSpec   *pspec, 
							   GtkWidget    *dialog);
static void  resource_dialog_type_changed_cb              (GtkWidget    *w,
							   DialogData   *data);
static void  resource_dialog_resource_type_changed_cb     (MrpResource  *resource,  
							   GParamSpec   *pspec, 
							   GtkWidget    *dialog);
static void  resource_dialog_group_changed_cb             (GtkWidget    *w,
							   DialogData   *data);
static void  resource_dialog_resource_group_changed_cb    (MrpResource  *resource,  
							   GParamSpec   *pspec, 
							   GtkWidget    *dialog);
static void  resource_dialog_email_changed_cb             (GtkWidget    *w,
							   DialogData   *data);
static void  resource_dialog_resource_email_changed_cb    (MrpResource  *resource,  
							   GParamSpec   *pspec, 
							   GtkWidget    *dialog);
static void  resource_dialog_resource_cost_changed_cb     (MrpResource  *resource,  
							   MrpProperty  *property,
							   GValue       *value,
							   GtkWidget    *dialog);
static void  resource_dialog_resource_calendar_changed_cb (MrpResource  *resource,
							   GParamSpec   *pspec,
							   GtkWidget    *dialog);
static void  resource_dialog_edit_calendar_clicked_cb     (GtkWidget    *button,
							   DialogData   *data);
static void  resource_dialog_resource_note_changed_cb     (MrpResource  *resource, 
							   GParamSpec   *pspec, 
							   GtkWidget    *dialog);
static void  resource_dialog_note_changed_cb              (GtkWidget    *w,
							   DialogData   *data);
static void  resource_dialog_note_stamp_clicked_cb        (GtkWidget    *w,
							   DialogData   *data);
static void  resource_dialog_build_calendar_tree_recurse  (GtkTreeStore *store,
							   GtkTreeIter  *parent,
							   MrpCalendar  *calendar);
static void  resource_dialog_build_calendar_tree          (DialogData   *data);
static void  resource_dialog_calendar_tree_changed_cb     (MrpProject   *project,
							   MrpCalendar  *root,
							   GtkWidget    *dialog);
static GtkTreeModel *resource_dialog_create_calendar_model(DialogData   *data);
static void  resource_dialog_setup_calendar_tree_view     (DialogData   *data);
static void  resource_dialog_update_title                 (DialogData   *data);


/* Keep the dialogs here so that we can just raise the dialog if it's
 * opened twice for the same resource.
 */
static GHashTable *dialogs = NULL;

#define DIALOG_GET_DATA(d) g_object_get_data ((GObject*)d, "data")

static gboolean
foreach_find_calendar (GtkTreeModel *model,
		       GtkTreePath  *path,
		       GtkTreeIter  *iter,
		       FindCalData  *data)
{
	MrpCalendar *calendar;

	gtk_tree_model_get (model,
			    iter,
			    COL_CALENDAR, &calendar,
			    -1);

	if (calendar == data->calendar) {
		data->found = TRUE;
		data->found_iter = *iter;
		return TRUE;
	}
	
	return FALSE;
}

static gboolean
resource_dialog_find_calendar (DialogData  *data,
			       MrpCalendar *calendar,
			       GtkTreeIter *iter)
{
	GtkTreeModel *model;
	FindCalData    find_data;

	find_data.found = FALSE;
	find_data.calendar = calendar;
	find_data.data = data;

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (data->calendar_tree_view));

	gtk_tree_model_foreach (model,
				(GtkTreeModelForeachFunc) foreach_find_calendar,
				&find_data);

	if (find_data.found) {
		*iter = find_data.found_iter;
		return TRUE;
	}

	return FALSE;
}

static void
resource_dialog_setup_option_menu (GtkWidget     *option_menu,
				   GCallback      func,
				   gpointer       user_data,
				   gconstpointer  str1, ...)
{
	GtkWidget    *menu;
	GtkWidget    *menu_item;
	gint          i;
	va_list       args;
	gconstpointer str;
	gint          type;

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

static void
resource_dialog_setup_option_groups (GtkWidget *menu_groups,
				     GList     *groups)
{
	GtkWidget *menu;
	GtkWidget *menu_item;
	gchar     *name;
	GList     *l;
	
	menu = gtk_option_menu_get_menu (GTK_OPTION_MENU (menu_groups));

	if (menu) {
		gtk_widget_destroy (menu);
	}

	menu = gtk_menu_new ();

	/* Put "no group" at the top. */
	menu_item = gtk_menu_item_new_with_label (_("(None)"));
	gtk_widget_show (menu_item);
	gtk_menu_append (GTK_MENU (menu), menu_item);

	for (l = groups; l; l = l->next) {
		g_object_get (G_OBJECT (l->data),
			      "name", &name,
			      NULL);

		if (name == NULL) {
			name = g_strdup (_("(No name)"));
		}

		menu_item = gtk_menu_item_new_with_label (name);
		gtk_widget_show (menu_item);
		gtk_menu_append (GTK_MENU (menu), menu_item);

		g_object_set_data (G_OBJECT (menu_item),
				   "data",
				   l->data);
	}

	gtk_widget_show (menu);
	gtk_option_menu_set_menu (GTK_OPTION_MENU (menu_groups), menu);
}

static gint
resource_dialog_option_menu_get_type_selected (GtkWidget *option_menu)
{
	GtkWidget *menu;
	GtkWidget *item;
	gint       ret;
	
	menu = gtk_option_menu_get_menu (GTK_OPTION_MENU (option_menu));
		
	item = gtk_menu_get_active (GTK_MENU (menu));

	ret = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (item), "data"));

	return ret;
}

static MrpGroup *
resource_dialog_option_menu_get_group_selected (GtkWidget *option_menu)
{
	GtkWidget *menu;
	GtkWidget *item;
	MrpGroup  *ret;
	
	menu = gtk_option_menu_get_menu (GTK_OPTION_MENU (option_menu));
		
	item = gtk_menu_get_active (GTK_MENU (menu));

	ret = g_object_get_data (G_OBJECT (item), "data");

	return ret;
}

static void
resource_dialog_connect_to_resource (DialogData *data)
{
	MrpResource *resource;

	resource = data->resource;

	g_signal_connect_object (resource,
				 "notify::name",
				 G_CALLBACK (resource_dialog_resource_name_changed_cb),
				 data->dialog,
				 0);

	g_signal_connect_object (resource,
				 "notify::type",
				 G_CALLBACK (resource_dialog_resource_type_changed_cb),
				 data->dialog,
				 0);

	g_signal_connect_object (resource,
				 "notify::group",
				 G_CALLBACK (resource_dialog_resource_group_changed_cb),
				 data->dialog,
				 0);

	g_signal_connect_object (resource,
				 "notify::email",
				 G_CALLBACK (resource_dialog_resource_email_changed_cb),
				 data->dialog,
				 0);

	g_signal_connect_object (resource,
				 "prop-changed::cost",
				 G_CALLBACK (resource_dialog_resource_cost_changed_cb),
				 data->dialog,
				 0);

	g_signal_connect_object (resource,
				 "notify::calendar",
				 G_CALLBACK (resource_dialog_resource_calendar_changed_cb),
				 data->dialog,
				 0);
}

static void
resource_dialog_close_clicked_cb (GtkWidget  *w,
				  DialogData *data)
{
	gtk_widget_destroy (data->dialog);
}

static void  
resource_dialog_resource_name_changed_cb (MrpResource *resource,  
					  GParamSpec  *pspec, 
					  GtkWidget   *dialog)
{
	DialogData *data;
	gchar      *name;

	g_return_if_fail (MRP_IS_RESOURCE (resource));

	data = DIALOG_GET_DATA (dialog);
	
	g_object_get (data->resource, "name", &name, NULL);

	g_signal_handlers_block_by_func (data->name_entry,
					 resource_dialog_name_changed_cb, 
					 dialog);

	gtk_entry_set_text (GTK_ENTRY (data->name_entry), name);

	resource_dialog_update_title (data);
	
	g_signal_handlers_unblock_by_func (data->name_entry, 
					   resource_dialog_name_changed_cb,
					   dialog);

	g_free (name);
}

static void
resource_dialog_name_changed_cb (GtkWidget  *w,
				 DialogData *data)
{
	const gchar *name;

	name = gtk_entry_get_text (GTK_ENTRY (w));

	g_signal_handlers_block_by_func (data->resource,
					 resource_dialog_resource_name_changed_cb,
					 data);

	g_object_set (data->resource, "name", name, NULL);

	resource_dialog_update_title (data);

	g_signal_handlers_unblock_by_func (data->resource,
					   resource_dialog_resource_name_changed_cb,
					   data);
}

static void  
resource_dialog_resource_type_changed_cb (MrpResource *resource,  
					  GParamSpec  *pspec, 
					  GtkWidget   *dialog)
{
	DialogData      *data;
	MrpResourceType  type;
	gint             index = 0;

	g_return_if_fail (MRP_IS_RESOURCE (resource));

	data = DIALOG_GET_DATA (dialog);
	
	g_object_get (data->resource, "type", &type, NULL);

	g_signal_handlers_block_by_func (data->type_menu, 
					 resource_dialog_type_changed_cb, 
					 data);
	
	switch (type) {
	case MRP_RESOURCE_TYPE_NONE:
	case MRP_RESOURCE_TYPE_WORK:
		index = 0;
		break;
	case MRP_RESOURCE_TYPE_MATERIAL:
		index = 1;
		break;
	}

	gtk_option_menu_set_history (GTK_OPTION_MENU (data->type_menu), index);

	g_signal_handlers_unblock_by_func (data->type_menu, 
					   resource_dialog_type_changed_cb,
					   data);
}

static void
resource_dialog_type_changed_cb (GtkWidget  *w,
				 DialogData *data)
{
	MrpResourceType type;

	type = resource_dialog_option_menu_get_type_selected (data->type_menu);
	
	g_signal_handlers_block_by_func (data->resource,
					 resource_dialog_resource_type_changed_cb, 
					 data);

	g_object_set (data->resource, "type", type, NULL);

	g_signal_handlers_unblock_by_func (data->resource,
					   resource_dialog_resource_type_changed_cb, 
					   data);
}

static void  
resource_dialog_resource_group_changed_cb (MrpResource *resource,  
					   GParamSpec  *pspec, 
					   GtkWidget   *dialog)
{	
	DialogData *data;
	MrpProject *project;
	MrpGroup   *group;
	GList      *groups;
	gint        index;

	g_return_if_fail (MRP_IS_RESOURCE (resource));

	data = DIALOG_GET_DATA (dialog);
	
	g_object_get (resource,
		      "group",   &group,
		      "project", &project,
		      NULL);
	
	g_signal_handlers_block_by_func (data->group_menu, 
					 resource_dialog_group_changed_cb, 
					 dialog);

	groups = mrp_project_get_groups (project);
	if (groups == NULL) {
		index = 0;
	} else {
		index = g_list_index (groups, group) + 1;
	}

	gtk_option_menu_set_history (GTK_OPTION_MENU (data->group_menu), index);
	
	g_signal_handlers_unblock_by_func (data->group_menu, 
					   resource_dialog_group_changed_cb,
					   dialog);
}

static void
resource_dialog_group_changed_cb (GtkWidget  *w,
				  DialogData *data)
{
	MrpGroup *group;

	group = resource_dialog_option_menu_get_group_selected (data->group_menu);

	g_signal_handlers_block_by_func (data->resource,
					 resource_dialog_resource_group_changed_cb, 
					 data);

	g_object_set (data->resource, "group", group, NULL);

	g_signal_handlers_unblock_by_func (data->resource,
					   resource_dialog_resource_group_changed_cb, 
					   data);
}

static void  
resource_dialog_email_changed_cb (GtkWidget  *w,
				  DialogData *data) 
{
	const gchar *email;

	email = gtk_entry_get_text (GTK_ENTRY (w));

	g_signal_handlers_block_by_func (data->resource,
					 resource_dialog_resource_email_changed_cb, 
					 data);

	g_object_set (data->resource, "email", email, NULL);

	g_signal_handlers_unblock_by_func (data->resource,
					   resource_dialog_resource_email_changed_cb, 
					   data);
}

static void  
resource_dialog_resource_email_changed_cb (MrpResource *resource,  
					   GParamSpec  *pspec, 
					   GtkWidget   *dialog)
{
	DialogData *data;
	gchar      *email;
   
	g_return_if_fail (MRP_IS_RESOURCE (resource));

	data = DIALOG_GET_DATA (dialog);
	
	g_object_get (data->resource, "email", &email, NULL);

	g_signal_handlers_block_by_func (data->email_entry, 
					 resource_dialog_email_changed_cb, 
					 dialog);

	gtk_entry_set_text (GTK_ENTRY (data->email_entry), email);

	g_signal_handlers_unblock_by_func (data->email_entry, 
					   resource_dialog_email_changed_cb,
					   dialog);

	g_free (email);
}

static void  
resource_dialog_cost_changed_cb (GtkWidget  *w,
				 DialogData *data) 
{
	const gchar *cost;
	gfloat       fvalue;

	cost = gtk_entry_get_text (GTK_ENTRY (w));
	
	g_signal_handlers_block_by_func (data->resource,
					 resource_dialog_resource_cost_changed_cb, 
					 data);

	fvalue = g_ascii_strtod (cost, NULL);
	mrp_object_set (data->resource, "cost", fvalue, NULL);

	g_signal_handlers_unblock_by_func (data->resource,
					   resource_dialog_resource_cost_changed_cb, 
					   data);
}

static void  
resource_dialog_resource_cost_changed_cb (MrpResource *resource,  
					  MrpProperty *property,
					  GValue      *value,
					  GtkWidget   *dialog)
{
	DialogData *data;
	gfloat      cost;
	gchar      *str;
	
	g_return_if_fail (MRP_IS_RESOURCE (resource));

	data = DIALOG_GET_DATA (dialog);
	
	cost = g_value_get_float (value);
	
	g_signal_handlers_block_by_func (data->cost_entry, 
					 resource_dialog_cost_changed_cb, 
					 dialog);

	str = planner_format_float (cost, 2, FALSE);
	gtk_entry_set_text (GTK_ENTRY (data->cost_entry), str);
	g_free (str);

	g_signal_handlers_unblock_by_func (data->cost_entry, 
					   resource_dialog_cost_changed_cb,
					   dialog);
}

static void  
resource_dialog_resource_calendar_changed_cb (MrpResource *resource,  
					      GParamSpec  *pspec, 
					      GtkWidget   *dialog)
{
	DialogData   *data;
	MrpCalendar  *calendar;
	GtkTreeIter   iter;
	GtkTreeModel *model;

	g_return_if_fail (MRP_IS_RESOURCE (resource));

	data = DIALOG_GET_DATA (dialog);

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (data->calendar_tree_view));
	
	/* Toggle the old calendar off. */
	if (resource_dialog_find_calendar (data, data->selected_calendar, &iter)) {
		gtk_tree_store_set (GTK_TREE_STORE (model),
				    &iter,
				    COL_SELECTED, FALSE,
				    -1);
	}
	
	calendar = mrp_resource_get_calendar (data->resource);

	/* Find the iter of the new calendar so we can toggle it on. */
	if (resource_dialog_find_calendar (data, calendar, &iter)) {
		gtk_tree_store_set (GTK_TREE_STORE (model),
				    &iter,
				    COL_SELECTED, TRUE,
				    -1);
		
		data->selected_calendar = calendar;
	}
}

static void
resource_dialog_edit_calendar_clicked_cb (GtkWidget  *button,
					  DialogData *data)
{
	planner_window_show_calendar_dialog (data->main_window);
}

static void
resource_dialog_resource_note_changed_cb (MrpResource *resource, 
					  GParamSpec  *pspec, 
					  GtkWidget   *dialog)
{
	DialogData *data;
	gchar      *note;
	
	g_return_if_fail (MRP_IS_RESOURCE (resource));
	g_return_if_fail (GTK_IS_DIALOG (dialog));

	data = DIALOG_GET_DATA (dialog);
	
	g_object_get (resource, "note", &note, NULL);

	g_signal_handlers_block_by_func (data->note_buffer,
					 resource_dialog_note_changed_cb,
					 dialog);
	
	gtk_text_buffer_set_text (data->note_buffer, note, -1);

	g_signal_handlers_unblock_by_func (data->note_buffer,
					   resource_dialog_note_changed_cb,
					   dialog);
	
	g_free (note);	
}

static void
resource_dialog_note_changed_cb (GtkWidget  *w,
				 DialogData *data)
{
	const gchar   *note;
	GtkTextIter    start, end;
	GtkTextBuffer *buffer;
	
	buffer = GTK_TEXT_BUFFER (w);
	
	gtk_text_buffer_get_bounds (buffer, &start, &end);
	
	note = gtk_text_buffer_get_text (buffer, &start, &end, FALSE);

	g_signal_handlers_block_by_func (data->resource,
					 resource_dialog_resource_note_changed_cb,
					 data->dialog);

	g_object_set (data->resource, "note", note, NULL);

	g_signal_handlers_unblock_by_func (data->resource,
					   resource_dialog_resource_note_changed_cb,
					   data->dialog);
}

static void
resource_dialog_note_stamp_clicked_cb (GtkWidget  *w,
				       DialogData *data)
{
	GtkTextIter  end;
	time_t       t;
	struct tm   *tm;
	gchar        stamp[128];
	gchar       *utf8;
	GtkTextMark *mark;
		
	t = time (NULL);
	tm = localtime (&t);

	/* i18n: time stamp format for notes in task dialog, see strftime(3) for
	 * a detailed description. */
	strftime (stamp, sizeof (stamp), _("%a %d %b %Y, %H:%M\n"), tm);

	utf8 = g_locale_to_utf8 (stamp, -1, NULL, NULL, NULL);
	
	gtk_text_buffer_get_end_iter (data->note_buffer, &end);
	
	if (!gtk_text_iter_starts_line (&end)) {
		gtk_text_buffer_insert (data->note_buffer, &end, "\n", 1);
		gtk_text_buffer_get_end_iter (data->note_buffer, &end);
	}

	gtk_text_buffer_insert (data->note_buffer, &end, utf8, -11);

	g_free (utf8);

	gtk_text_buffer_get_end_iter (data->note_buffer, &end);

	mark = gtk_text_buffer_create_mark (data->note_buffer,
					    NULL,
					    &end,
					    TRUE);

	gtk_text_view_scroll_mark_onscreen (GTK_TEXT_VIEW (data->note_textview), mark);
}

static void
resource_dialog_calendar_toggled_cb (GtkCellRendererToggle *cell,
				     const gchar           *path_str,
				     DialogData            *data)
{
	GtkTreePath  *path;
	GtkTreeModel *model;
	GtkTreeIter   iter;
	gboolean      selected;
	MrpCalendar  *calendar;
	
	path = gtk_tree_path_new_from_string (path_str);

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (data->calendar_tree_view));
	
	gtk_tree_model_get_iter (model, &iter, path);

	gtk_tree_model_get (model,
			    &iter,
			    COL_SELECTED, &selected,
			    COL_CALENDAR, &calendar,
			    -1);

	if (!selected) {
		mrp_resource_set_calendar (data->resource, calendar);
	}
	
	gtk_tree_path_free (path);
}

static void
resource_dialog_build_calendar_tree_recurse (GtkTreeStore *store,
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
			    COL_SELECTED, FALSE,
			    -1);

	children = mrp_calendar_get_children (calendar);	
	for (l = children; l; l = l->next) {
		resource_dialog_build_calendar_tree_recurse (store, &iter, l->data);
	}
}

static void
resource_dialog_build_calendar_tree (DialogData *data)
{
	MrpProject   *project;
	MrpCalendar  *root;
	GtkTreeStore *store;
	GtkTreeIter   iter;
	GList        *children, *l;
	MrpCalendar  *calendar;

	project = planner_window_get_project (data->main_window);
	root = mrp_project_get_root_calendar (project);

	store = GTK_TREE_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (data->calendar_tree_view)));
	
	gtk_tree_store_append (store, &iter, NULL);
	gtk_tree_store_set (store,
			    &iter,
			    COL_NAME, _("None (use project default)"),
			    COL_CALENDAR, NULL,
			    COL_SELECTED, FALSE,
			    -1);

	children = mrp_calendar_get_children (root);	
	for (l = children; l; l = l->next) {
		resource_dialog_build_calendar_tree_recurse (store, NULL, l->data);
	}

	gtk_tree_view_expand_all (GTK_TREE_VIEW (data->calendar_tree_view));

	calendar = mrp_resource_get_calendar (data->resource);
	
	/* Select the current calendar. */
	if (resource_dialog_find_calendar (data, calendar, &iter)) {
		gtk_tree_store_set (store,
				    &iter,
				    COL_SELECTED, TRUE,
				    -1);
		
		data->selected_calendar = calendar;
	}
}

static void
resource_dialog_calendar_tree_changed_cb (MrpProject  *project,
					  MrpCalendar *root,
					  GtkWidget   *dialog)
{
	DialogData   *data;
	GtkTreeView  *tree_view;
	GtkTreeStore *store;

	data = DIALOG_GET_DATA (dialog);

	tree_view = GTK_TREE_VIEW (data->calendar_tree_view);
	store = GTK_TREE_STORE (gtk_tree_view_get_model (tree_view));
	
	gtk_tree_store_clear (store);
	
	resource_dialog_build_calendar_tree (data);
}

static GtkTreeModel *
resource_dialog_create_calendar_model (DialogData *data)
{
	MrpProject   *project;
	GtkTreeStore *store;
	MrpCalendar  *root;

	project = planner_window_get_project (data->main_window);

	root = mrp_project_get_root_calendar (project);

	store = gtk_tree_store_new (NUM_COLS,
				    G_TYPE_OBJECT,
				    G_TYPE_STRING,
				    G_TYPE_BOOLEAN);

	g_signal_connect_object (project,
				 "calendar_tree_changed",
				 G_CALLBACK (resource_dialog_calendar_tree_changed_cb),
				 data->dialog,
				 0);
	
	return GTK_TREE_MODEL (store);
}

static void
resource_dialog_setup_calendar_tree_view (DialogData *data)
{
	GtkTreeView       *tree_view;
	MrpProject        *project;
	GtkTreeModel      *model;
	GtkCellRenderer   *cell;
	GtkTreeViewColumn *col;
	
	project = planner_window_get_project (data->main_window);
	tree_view = GTK_TREE_VIEW (data->calendar_tree_view);
	
	model = resource_dialog_create_calendar_model (data);

	gtk_tree_view_set_model (tree_view, model);

	col = gtk_tree_view_column_new ();

	cell = gtk_cell_renderer_toggle_new ();
	g_object_set (cell,
		      "activatable", TRUE,
		      "radio", TRUE,
		      NULL);		    
	
	g_signal_connect (cell,
			  "toggled",
			  G_CALLBACK (resource_dialog_calendar_toggled_cb),
			  data);
	
	gtk_tree_view_column_pack_start (col, cell, FALSE);

	gtk_tree_view_column_set_attributes (col,
					     cell,
					     "active", COL_SELECTED,
					     NULL);
	
	cell = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (col, cell, FALSE);

	gtk_tree_view_column_set_attributes (col,
					     cell,
					     "text", COL_NAME,
					     NULL);
	
	gtk_tree_view_append_column (tree_view, col);

	resource_dialog_build_calendar_tree (data);
}

static void
resource_dialog_update_title (DialogData *data)
{
	gchar *title;
	gchar *name;

	g_object_get (data->resource, "name", &name, NULL);

	if (!name || strlen (name) == 0) {
		title = g_strdup (_("Edit resource properties"));
	} else {
		title = g_strconcat (name, " - ", _("Edit resource properties"), NULL);
	}

	gtk_window_set_title (GTK_WINDOW (data->dialog), title);

	g_free (name);
	g_free (title);
}

static void
resource_dialog_parent_destroy_cb (GtkWidget *parent,
				   GtkWidget *dialog)
{
	gtk_widget_destroy (dialog);
}

static void
resource_dialog_destroy_cb (GtkWidget  *parent,
			    DialogData *data)
{
	g_hash_table_remove (dialogs, data->resource);
}

GtkWidget *
planner_resource_dialog_new (PlannerWindow *window,
			MrpResource  *resource)
{
	GladeXML        *glade;
	GtkWidget       *dialog;
	GtkWidget       *w;
	DialogData      *data;
	gchar           *name, *email;
	MrpProject      *project;	
	MrpGroup        *group;
	MrpResourceType  type;
	gfloat           cost;
	GList           *groups;
	gint             index = 0;
	gchar           *note;

	g_return_val_if_fail (PLANNER_IS_MAIN_WINDOW (window), NULL);
	g_return_val_if_fail (MRP_IS_RESOURCE (resource), NULL);

	if (!dialogs) {
		dialogs = g_hash_table_new (NULL, NULL);
	}

	dialog = g_hash_table_lookup (dialogs, resource);
	if (dialog) {
		gtk_window_present (GTK_WINDOW (dialog));
		return dialog;
	}
	
	g_object_get (resource, "project", &project, NULL);
	
	glade = glade_xml_new (GLADEDIR "/resource-dialog.glade",
			       NULL,
			       GETTEXT_PACKAGE);
	if (!glade) {
		g_warning ("Could not create resource dialog.");
		return NULL;
	}

	dialog = glade_xml_get_widget (glade, "resource_dialog");
	
	data = g_new0 (DialogData, 1);

	data->main_window = window;
	data->dialog = dialog;
	data->resource = resource;

	g_hash_table_insert (dialogs, resource, dialog);

	g_signal_connect (dialog,
			  "destroy",
			  G_CALLBACK (resource_dialog_destroy_cb),
			  data);

	g_signal_connect_object (window,
				 "destroy",
				 G_CALLBACK (resource_dialog_parent_destroy_cb),
				 dialog,
				 0);

	data->name_entry = glade_xml_get_widget (glade, "entry_name");
	data->type_menu = glade_xml_get_widget (glade, "menu_type");
	data->group_menu = glade_xml_get_widget (glade, "menu_group");
	data->email_entry = glade_xml_get_widget (glade, "entry_email");
	data->cost_entry = glade_xml_get_widget (glade, "entry_cost");
	data->calendar_tree_view = glade_xml_get_widget (glade, "calendar_treeview");
	data->note_textview = glade_xml_get_widget (glade, "note_textview");
	
	data->note_buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (data->note_textview));

	resource_dialog_setup_calendar_tree_view (data);
	
	g_object_get (data->resource, "note", &note, NULL);
	if (note) {
		gtk_text_buffer_set_text (data->note_buffer, note, -1);
		g_free (note);
	}
	
	g_signal_connect (data->note_buffer,
			  "changed",
			  G_CALLBACK (resource_dialog_note_changed_cb),
			  data);

	w = glade_xml_get_widget (glade, "calendar_button");
	g_signal_connect (w,
			  "clicked",
			  G_CALLBACK (resource_dialog_edit_calendar_clicked_cb),
			  data);

	w = glade_xml_get_widget (glade, "stamp_button");
	g_signal_connect (w,
			  "clicked",
			  G_CALLBACK (resource_dialog_note_stamp_clicked_cb),
			  data);

	w = glade_xml_get_widget (glade, "close_button");
	g_signal_connect (w,
			  "clicked",
			  G_CALLBACK (resource_dialog_close_clicked_cb),
			  data);

	g_object_set_data_full (G_OBJECT (dialog),
				"data", data,
				g_free);
	
	mrp_object_get (MRP_OBJECT (resource),
			"name",  &name,
			"type",  &type,
			"group", &group,
			"email", &email,
			"cost",  &cost,
			NULL); 

	gtk_entry_set_text (GTK_ENTRY (data->name_entry), name);
	g_signal_connect (data->name_entry,
			  "changed",
			  G_CALLBACK (resource_dialog_name_changed_cb),
			  data);

	resource_dialog_setup_option_menu (data->type_menu,
					   NULL,
					   NULL,
					   _("Work"), MRP_RESOURCE_TYPE_WORK,
					   _("Material"), MRP_RESOURCE_TYPE_MATERIAL,
					   NULL);

	/* Select the right type. */
	switch (type) {
	case MRP_RESOURCE_TYPE_NONE:
	case MRP_RESOURCE_TYPE_WORK:
		index = 0;
		break;
	case MRP_RESOURCE_TYPE_MATERIAL:
		index = 1;
		break;
	}
		
	gtk_option_menu_set_history (GTK_OPTION_MENU (data->type_menu), index);

	g_signal_connect (data->type_menu,
			  "changed",
			  G_CALLBACK (resource_dialog_type_changed_cb),
			  data);

	groups = mrp_project_get_groups (project);
	resource_dialog_setup_option_groups (data->group_menu, groups);

	/* Select the right group. + 1 is for the empty group at the top. */
	if (groups == NULL) {
		index = 0;
	} else {
		index = g_list_index (groups, group) + 1;
	}
	
	gtk_option_menu_set_history (GTK_OPTION_MENU (data->group_menu),
				     index);

	g_signal_connect (data->group_menu,
			  "changed",
			  G_CALLBACK (resource_dialog_group_changed_cb),
			  data);

	gtk_entry_set_text (GTK_ENTRY (data->email_entry), email);

	g_signal_connect (data->email_entry,
			  "changed",
			  G_CALLBACK (resource_dialog_email_changed_cb),
			  data);

	gtk_entry_set_text (GTK_ENTRY (data->cost_entry), planner_format_float (cost, 2, FALSE)); 

	g_signal_connect (data->cost_entry,
			  "changed",
			  G_CALLBACK (resource_dialog_cost_changed_cb),
			  data);

	g_free (name);
	g_free (email);

	resource_dialog_connect_to_resource (data);

	resource_dialog_update_title (data);

	g_object_unref (glade);
	
	return dialog;
}
