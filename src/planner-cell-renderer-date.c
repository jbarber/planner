/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2001-2002 CodeFactory AB
 * Copyright (C) 2001-2002 Richard Hult <richard@imendio.com>
 * Copyright (C) 2001-2002 Mikael Hallendal <micke@imendio.com>
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
#include <gdk/gdkkeysyms.h>
#include <gtk/gtkmain.h>
#include <gtk/gtkcellrenderertext.h>
#include <gtk/gtkeditable.h>
#include <gtk/gtkhseparator.h>
#include <gtk/gtkbutton.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkoptionmenu.h>
#include <gtk/gtkmenuitem.h>
#include <gtk/gtkarrow.h>
#include <gtk/gtkwindow.h>
#include <gtk/gtkframe.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkhbbox.h>
#include <gtk/gtkstock.h>
#include <gtk/gtkcalendar.h>
#include <libgnome/gnome-i18n.h>
#include "planner-marshal.h"
#include "planner-format.h"
#include "planner-cell-renderer-date.h"
#include "planner-popup-entry.h"

enum {
	PROP_0,
	PROP_USE_CONSTRAINT,
};

static void     mcrd_init                    (MgCellRendererDate      *date);
static void     mcrd_class_init              (MgCellRendererDateClass *class);
static void     mcrd_set_property            (GObject                 *object,
					      guint                    param_id,
					      const GValue            *value,
					      GParamSpec              *pspec);
static void     mcrd_get_property            (GObject                 *object,
					      guint                    param_id,
					      GValue                  *value,
					      GParamSpec              *pspec);
static void     mcrd_cancel_clicked          (GtkWidget               *popup_window,
					      MgCellRendererDate      *cell);
static void     mcrd_ok_clicked              (GtkWidget               *popup_window,
					      MgCellRendererDate      *cell);
static void     mcrd_day_selected            (GtkWidget               *popup_window,
					      MgCellRendererDate      *cell);
static void     mcrd_constraint_activated_cb (GtkWidget               *widget,
					      MgCellRendererDate      *cell);
GtkCellEditable *mcrd_start_editing          (GtkCellRenderer         *cell,
					      GdkEvent                *event,
					      GtkWidget               *widget,
					      const gchar             *path,
					      GdkRectangle            *background_area,
					      GdkRectangle            *cell_area,
					      GtkCellRendererState     flags);
static void     mcrd_show                    (MgCellRendererPopup     *cell,
					      const gchar             *path,
					      gint                     x1,
					      gint                     y1,
					      gint                     x2,
					      gint                     y2);
static void     mcrd_hide                    (MgCellRendererPopup     *cell);
static void     mcrd_setup_option_menu       (GtkWidget               *option_menu,
					      GtkSignalFunc            func,
					      gpointer                 user_data,
					      gpointer                 str1, ...);


static MgCellRendererPopupClass *parent_class;

GType
planner_cell_renderer_date_get_type (void)
{
	static GType cell_text_type = 0;
	
	if (!cell_text_type) {
		static const GTypeInfo cell_text_info = {
			sizeof (MgCellRendererDateClass),
			NULL,		/* base_init */
			NULL,		/* base_finalize */
			(GClassInitFunc) mcrd_class_init,
			NULL,		/* class_finalize */
			NULL,		/* class_data */
			sizeof (MgCellRendererDate),
			0,              /* n_preallocs */
			(GInstanceInitFunc) mcrd_init,
		};
		
		cell_text_type = g_type_register_static (MG_TYPE_CELL_RENDERER_POPUP,
							 "MgCellRendererDate",
							 &cell_text_info,
							 0);
	}
	
	return cell_text_type;
}

static void
mcrd_init (MgCellRendererDate *date)
{
	MgCellRendererPopup *popup;
	GtkWidget           *frame;
	GtkWidget           *vbox;
	GtkWidget           *hbox;
	GtkWidget           *bbox;
	GtkWidget           *button;

	popup = MG_CELL_RENDERER_POPUP (date);

	frame = gtk_frame_new (NULL);
	gtk_container_add (GTK_CONTAINER (popup->popup_window), frame);
	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);

	vbox = gtk_vbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (frame), vbox);

	date->calendar = gtk_calendar_new ();
	popup->focus_window = date->calendar;
	gtk_box_pack_start (GTK_BOX (vbox), date->calendar, TRUE, TRUE, 0);

	date->constraint_vbox = gtk_vbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), date->constraint_vbox, FALSE, TRUE, 0);
	
	hbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), gtk_label_new (_("Constraint type")),
			    FALSE, TRUE, 4);
	
	date->option_menu = gtk_option_menu_new ();
	mcrd_setup_option_menu (date->option_menu,
				G_CALLBACK (mcrd_constraint_activated_cb),
				date,
				_("As soon as possible"), MRP_CONSTRAINT_ASAP,
				_("Start no earlier than"), MRP_CONSTRAINT_SNET,
				_("Must start on"), MRP_CONSTRAINT_MSO,
				NULL);
	gtk_box_pack_end (GTK_BOX (hbox), date->option_menu, TRUE, TRUE, 4);
	
	gtk_box_pack_start (GTK_BOX (date->constraint_vbox), hbox, TRUE, TRUE, 4);
	
	hbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), gtk_hseparator_new (), TRUE, TRUE, 4);
	gtk_box_pack_start (GTK_BOX (date->constraint_vbox), hbox, FALSE, FALSE, 0);
	
	bbox = gtk_hbutton_box_new ();
	gtk_container_set_border_width (GTK_CONTAINER (bbox), 4);
	gtk_box_set_spacing (GTK_BOX (bbox), 2);
	gtk_box_pack_start (GTK_BOX (vbox), bbox, FALSE, FALSE, 0);

	button = gtk_button_new_with_label (_("Today"));
	gtk_container_add (GTK_CONTAINER (bbox), button);
	gtk_widget_set_sensitive (button, FALSE);

/*	gtk_signal_connect (GTK_OBJECT (button), "clicked",
			    GTK_SIGNAL_FUNC (m_cell_date_today_clicked),
			    date);
*/
	button = gtk_button_new_with_label (_("Cancel"));
	gtk_container_add (GTK_CONTAINER (bbox), button);

	g_signal_connect (button,
			  "clicked",
			  G_CALLBACK (mcrd_cancel_clicked),
			  date);

	button = gtk_button_new_with_label (_("OK"));
	gtk_container_add (GTK_CONTAINER (bbox), button);
	g_signal_connect (button, "clicked",
			  G_CALLBACK (mcrd_ok_clicked),
			  date);

	g_signal_connect (date->calendar, "day-selected",
			  G_CALLBACK (mcrd_day_selected),
			  date);

/*	gtk_signal_connect (GTK_OBJECT (cell->calendar), "day-selected-double-click", 
			    m_cell_date_double_click, 
			    date);
*/

        gtk_widget_show_all (frame);
}

static void
mcrd_class_init (MgCellRendererDateClass *class)
{
	MgCellRendererPopupClass *popup_class;
	GtkCellRendererClass     *cell_class;
	GObjectClass             *gobject_class;

	popup_class = MG_CELL_RENDERER_POPUP_CLASS (class);
	cell_class = GTK_CELL_RENDERER_CLASS (class);	
	parent_class = MG_CELL_RENDERER_POPUP_CLASS (g_type_class_peek_parent (class));
	gobject_class = G_OBJECT_CLASS (class);

	gobject_class->set_property = mcrd_set_property;
	gobject_class->get_property = mcrd_get_property;

	cell_class->start_editing = mcrd_start_editing;
		
	popup_class->show_popup = mcrd_show;
	popup_class->hide_popup = mcrd_hide;

	g_object_class_install_property (
		gobject_class,
                 PROP_USE_CONSTRAINT,
                 g_param_spec_boolean ("use-constraint",
				       NULL,
				       NULL,
				       TRUE,
				       G_PARAM_READWRITE |
				       G_PARAM_CONSTRUCT_ONLY));
}

static void
mcrd_set_property (GObject      *object,
		   guint         param_id,
		   const GValue *value,
		   GParamSpec   *pspec)
{
	MgCellRendererDate *date;

	date = MG_CELL_RENDERER_DATE (object);
	
	switch (param_id) {
	case PROP_USE_CONSTRAINT:
		date->use_constraint = g_value_get_boolean (value);

		if (date->use_constraint) {
			gtk_widget_show (date->constraint_vbox);
		} else {
			gtk_widget_hide (date->constraint_vbox);
			gtk_widget_set_sensitive (date->calendar, TRUE);
		}
			
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static void
mcrd_get_property (GObject    *object,
		   guint       param_id,
		   GValue     *value,
		   GParamSpec *pspec)
{
	MgCellRendererDate *date;

	date = MG_CELL_RENDERER_DATE (object);
	
	switch (param_id) {
	case PROP_USE_CONSTRAINT:
		g_value_set_boolean (value, date->use_constraint);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

GtkCellEditable *
mcrd_start_editing (GtkCellRenderer      *cell,
		    GdkEvent             *event,
		    GtkWidget            *widget,
		    const gchar          *path,
		    GdkRectangle         *background_area,
		    GdkRectangle         *cell_area,
		    GtkCellRendererState  flags)
{
	MG_CELL_RENDERER_POPUP (cell)->editing_canceled = TRUE;
	
	if (GTK_CELL_RENDERER_CLASS (parent_class)->start_editing) {
		return GTK_CELL_RENDERER_CLASS (parent_class)->start_editing (
							cell,
							event,
							widget,
							path,
							background_area,
							cell_area,
							flags);
	}

	return NULL;
}


static void
mcrd_hide (MgCellRendererPopup *cell)
{
	if (parent_class->hide_popup) {
		parent_class->hide_popup (cell);
	}
}

static void
mcrd_show (MgCellRendererPopup *cell,
	   const gchar         *path,
	   gint                 x1,
	   gint                 y1,
	   gint                 x2,
	   gint                 y2)
{
	MgCellRendererDate *date;
	gint                year;
	gint                month;
	gint                day;
	gint                index;
	gboolean            sensitive;

	if (parent_class->show_popup) {
		parent_class->show_popup (cell,
					  path,
					  x1, y1,
					  x2, y2);
	}

	date = MG_CELL_RENDERER_DATE (cell);

	mrp_time_decompose (date->time, &year, &month, &day, NULL, NULL, NULL);
	
	index = 0;
	
	switch (date->type) {
	case MRP_CONSTRAINT_ASAP:
		index = 0;
		break;
	case MRP_CONSTRAINT_SNET:
		index = 1;
		break;
	case MRP_CONSTRAINT_MSO:
		index = 2;
		break;
	default:
		g_assert_not_reached ();
	}

	sensitive = (!date->use_constraint ||
		     (date->type != MRP_CONSTRAINT_ASAP &&
		      date->type != MRP_CONSTRAINT_ALAP));
	
	gtk_widget_set_sensitive (date->calendar,
				  sensitive);
	
	gtk_calendar_clear_marks (GTK_CALENDAR (date->calendar));
	gtk_calendar_select_month (GTK_CALENDAR (date->calendar),
				   month - 1, year);
	gtk_calendar_select_day (GTK_CALENDAR (date->calendar), day);
	gtk_calendar_mark_day (GTK_CALENDAR (date->calendar), day);
	
	gtk_option_menu_set_history (GTK_OPTION_MENU (date->option_menu),
				     index);
}

GtkCellRenderer *
planner_cell_renderer_date_new (gboolean use_constraint)
{
	GObject *cell;

	cell = g_object_new (MG_TYPE_CELL_RENDERER_DATE,
			     "use-constraint", use_constraint,
			     NULL);
	
	return GTK_CELL_RENDERER (cell);
}

static void
mcrd_cancel_clicked (GtkWidget          *popup_window,
		     MgCellRendererDate *cell)
{
	MgCellRendererPopup *popup;
	
	popup = MG_CELL_RENDERER_POPUP (cell);

	popup->editing_canceled = TRUE;
	planner_cell_renderer_popup_hide (popup);
}

static void
mcrd_ok_clicked (GtkWidget          *popup_window,
		 MgCellRendererDate *cell)
{
	MgCellRendererPopup *popup;
	
	popup = MG_CELL_RENDERER_POPUP (cell);

	mcrd_day_selected (popup_window, cell);

	popup->editing_canceled = FALSE;
	planner_cell_renderer_popup_hide (popup);
}

static void
mcrd_day_selected (GtkWidget          *popup_window,
		   MgCellRendererDate *cell)
{
	guint    year;
	guint    month;
	guint    day;
	mrptime  t;
	gchar   *str;

	gtk_calendar_get_date (GTK_CALENDAR (cell->calendar),
			       &year,
			       &month,
			       &day);

	t = mrp_time_compose (year, month + 1, day, 0, 0, 0);

	cell->time = t;

	str = planner_format_date (t);
	planner_popup_entry_set_text (
		MG_POPUP_ENTRY (MG_CELL_RENDERER_POPUP (cell)->editable), str);
	g_free (str);
}

static gboolean
mcrd_grab_on_window (GdkWindow *window,
		     guint32    activate_time)
{
	if ((gdk_pointer_grab (window, TRUE,
			       GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK |
			       GDK_POINTER_MOTION_MASK,
			       NULL, NULL, activate_time) == 0)) {
		if (gdk_keyboard_grab (window, TRUE,
			       activate_time) == 0)
			return TRUE;
		else {
			gdk_pointer_ungrab (activate_time);
			return FALSE;
		}
	}

	return FALSE;
}

static void
mcrd_constraint_activated_cb (GtkWidget          *widget,
			      MgCellRendererDate *cell)
{
	gpointer data;
	gboolean sensitive;
	
	data = g_object_get_data (G_OBJECT (widget), "data");

	cell->type = GPOINTER_TO_INT (data);

	sensitive = (!cell->use_constraint ||
		     (cell->type != MRP_CONSTRAINT_ASAP &&
		      cell->type != MRP_CONSTRAINT_ALAP));
	
	gtk_widget_set_sensitive (cell->calendar, sensitive);
	
	/* A bit hackish. Grab focus on the popup window again when the
	 * optionmenu is activated, since focus is transferred to the optionmenu
	 * when it's popped up.
	 */
	mcrd_grab_on_window (MG_CELL_RENDERER_POPUP (cell)->popup_window->window,
			     gtk_get_current_event_time ());
}

/* Utility function to use before optionmenus work with libglade again.
 */

static void
mcrd_setup_option_menu (GtkWidget     *option_menu,
			GtkSignalFunc  func,
			gpointer       user_data,
			gpointer       str1, ...)
{
	GtkWidget *menu, *menu_item;
	gint       i;
	va_list    args;
	gpointer   str;
	gint       type;

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
		gtk_signal_connect (GTK_OBJECT (menu_item),
				    "activate",
				    func,
				    user_data);
	}
	va_end (args);

	gtk_widget_show (menu);
	gtk_option_menu_set_menu (GTK_OPTION_MENU (option_menu), menu);
}

