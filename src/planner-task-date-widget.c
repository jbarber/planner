/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2005 Imendio AB
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
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <glade/glade.h>
#include "planner-marshal.h"
#include "libplanner/mrp-time.h"
#include "libplanner/mrp-task.h"
#include "libplanner/mrp-paths.h"
#include "planner-task-date-widget.h"

typedef struct {
	GtkWidget   *calendar;
	GtkWidget   *combo;
	GtkWidget   *today_button;

	mrptime            time;
	MrpConstraintType  type;
} PlannerTaskDateWidgetPriv;


static void task_date_widget_setup  (PlannerTaskDateWidget *widget);


#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), PLANNER_TYPE_TASK_DATE_WIDGET, PlannerTaskDateWidgetPriv))

G_DEFINE_TYPE (PlannerTaskDateWidget, planner_task_date_widget, GTK_TYPE_FRAME);

/* Signals */
enum {
	DATE_SELECTED,
	CANCELLED,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL];

static void
planner_task_date_widget_class_init (PlannerTaskDateWidgetClass *klass)
{
	signals[DATE_SELECTED] = g_signal_new ("date_selected",
					       G_TYPE_FROM_CLASS (klass),
					       G_SIGNAL_RUN_LAST,
					       0,
					       NULL, NULL,
					       planner_marshal_VOID__VOID,
					       G_TYPE_NONE, 0);

	signals[CANCELLED] = g_signal_new ("cancelled",
					   G_TYPE_FROM_CLASS (klass),
					   G_SIGNAL_RUN_LAST,
					   0,
					   NULL, NULL,
					   planner_marshal_VOID__VOID,
					   G_TYPE_NONE, 0);

	g_type_class_add_private (klass, sizeof (PlannerTaskDateWidgetPriv));
}

static void
planner_task_date_widget_init (PlannerTaskDateWidget *widget)
{
	task_date_widget_setup (widget);
}

GtkWidget *
planner_task_date_widget_new (void)
{
	GtkWidget *widget;

	widget = g_object_new (PLANNER_TYPE_TASK_DATE_WIDGET,
			       "shadow_type", GTK_SHADOW_OUT,
			       NULL);

	return widget;
}

static void
task_date_widget_today_clicked_cb (GtkWidget             *button,
				   PlannerTaskDateWidget *widget)
{
	mrptime                    today;

	today = mrp_time_current_time ();
	planner_task_date_widget_set_date (widget, today);
}

static void
task_date_widget_cancel_clicked_cb (GtkWidget             *button,
				    PlannerTaskDateWidget *widget)
{
	g_signal_emit (widget, signals[CANCELLED], 0);
}

static void
task_date_widget_select_clicked_cb (GtkWidget             *button,
				    PlannerTaskDateWidget *widget)
{
	g_signal_emit (widget, signals[DATE_SELECTED], 0);
}

static void
task_date_day_selected_double_click_cb (GtkWidget             *calendar,
					PlannerTaskDateWidget *widget)
{
	g_signal_emit (widget, signals[DATE_SELECTED], 0);
}

static gboolean
grab_on_window (GdkWindow *window,
		guint32    time)
{
	if ((gdk_pointer_grab (window, TRUE,
			       GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK |
			       GDK_POINTER_MOTION_MASK,
			       NULL, NULL, time) == 0)) {
		if (gdk_keyboard_grab (window, TRUE, time) == 0)
			return TRUE;
		else {
			gdk_pointer_ungrab (time);
			return FALSE;
		}
	}

	return FALSE;
}

static void
task_date_widget_combo_changed_cb (GtkComboBox           *combo,
				   PlannerTaskDateWidget *widget)
{
	PlannerTaskDateWidgetPriv *priv;
	MrpConstraintType          type;

	priv = GET_PRIV (widget);

	type = planner_task_date_widget_get_constraint_type (widget);
	gtk_widget_set_sensitive (priv->calendar, type != MRP_CONSTRAINT_ASAP);

	/* A bit hackish. Grab focus on the popup widget again when the combo
	 * has been used, since focus is transferred to the combo when it's
	 * popped up.
	 */
	if (GTK_WIDGET (widget)->window) {
		grab_on_window (GTK_WIDGET (widget)->window, gtk_get_current_event_time ());
	}
}

static void
task_date_widget_setup (PlannerTaskDateWidget *widget)
{
	PlannerTaskDateWidgetPriv *priv;
	GladeXML                  *glade;
	GtkWidget                 *vbox;
	GtkWidget                 *root_vbox;
	GtkWidget                 *button;
	gchar                     *filename;

	priv = GET_PRIV (widget);

	vbox = gtk_vbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (widget), vbox);

	filename = mrp_paths_get_glade_dir ("task-date-widget.glade");
	glade = glade_xml_new (filename,
			       "root_vbox",
			       GETTEXT_PACKAGE);
	g_free (filename);
	if (!glade) {
		g_assert_not_reached ();
	}

	root_vbox = glade_xml_get_widget (glade, "root_vbox");
	gtk_container_add (GTK_CONTAINER (vbox), root_vbox);

	priv->calendar = glade_xml_get_widget (glade, "schedule_calendar");
	priv->combo = glade_xml_get_widget (glade, "schedule_combo");

	g_signal_connect (priv->combo,
			  "changed",
			  G_CALLBACK (task_date_widget_combo_changed_cb),
			  widget);

	priv->today_button = glade_xml_get_widget (glade, "today_button");
	g_signal_connect (priv->today_button, "clicked",
			  G_CALLBACK (task_date_widget_today_clicked_cb),
			  widget);

	button = glade_xml_get_widget (glade, "cancel_button");
	g_signal_connect (button, "clicked",
			  G_CALLBACK (task_date_widget_cancel_clicked_cb),
			  widget);

	button = glade_xml_get_widget (glade, "select_button");
	g_signal_connect (button, "clicked",
			  G_CALLBACK (task_date_widget_select_clicked_cb),
			  widget);

	g_signal_connect (priv->calendar, "day-selected-double-click",
			  G_CALLBACK (task_date_day_selected_double_click_cb),
			  widget);

        gtk_widget_show_all (vbox);
}

void
planner_task_date_widget_set_date (PlannerTaskDateWidget *widget, mrptime t)
{
	PlannerTaskDateWidgetPriv *priv;
	gint                       year, month, day;

	priv = GET_PRIV (widget);

	if (!mrp_time_decompose (t, &year, &month, &day, NULL, NULL, NULL)) {
		return;
	}

	gtk_calendar_select_month (GTK_CALENDAR (priv->calendar), month - 1, year);
	gtk_calendar_select_day (GTK_CALENDAR (priv->calendar), day);
}

mrptime
planner_task_date_widget_get_date (PlannerTaskDateWidget *widget)
{
	PlannerTaskDateWidgetPriv *priv;
	gint                       year, month, day;

	priv = GET_PRIV (widget);

	gtk_calendar_get_date (GTK_CALENDAR (priv->calendar),
			       &year, &month, &day);

	month++;
	return mrp_time_compose (year, month, day, 0, 0, 0);
}

void
planner_task_date_widget_set_constraint_type (PlannerTaskDateWidget *widget,
					      MrpConstraintType      type)
{
	PlannerTaskDateWidgetPriv *priv;
	gint                       index;

	priv = GET_PRIV (widget);

	switch (type) {
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
		index = 0;
		g_assert_not_reached ();
	}

	gtk_combo_box_set_active (GTK_COMBO_BOX (priv->combo), index);
}

MrpConstraintType
planner_task_date_widget_get_constraint_type (PlannerTaskDateWidget *widget)
{
	PlannerTaskDateWidgetPriv *priv;
	gint                       index;

	priv = GET_PRIV (widget);

	index = gtk_combo_box_get_active (GTK_COMBO_BOX (priv->combo));

	switch (index) {
	case 0:
		return MRP_CONSTRAINT_ASAP;
	case 1:
		return MRP_CONSTRAINT_SNET;
	case 2:
		return MRP_CONSTRAINT_MSO;
	}

	return MRP_CONSTRAINT_ASAP;
}

