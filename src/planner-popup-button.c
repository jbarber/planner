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
#include <stdlib.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include "planner-marshal.h"
#include "planner-popup-button.h"

typedef struct {
	GtkWidget *popup_window;
	GtkWidget *popup_widget;
} PlannerPopupButtonPriv;

static gboolean popup_button_press_event_cb  (GtkWidget          *popup_window,
					      GdkEventButton     *event,
					      PlannerPopupButton *popup_button);
static void     planner_popup_button_toggled (GtkToggleButton    *toggle_button);
static void     popup_button_popup           (PlannerPopupButton *button);
static void     popup_button_popdown         (PlannerPopupButton *button,
					      gboolean            ok);


#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), PLANNER_TYPE_POPUP_BUTTON, PlannerPopupButtonPriv))

G_DEFINE_TYPE (PlannerPopupButton, planner_popup_button, GTK_TYPE_TOGGLE_BUTTON);

/* Signals */
enum {
	POPUP,
	POPDOWN,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL];

static void
planner_popup_button_class_init (PlannerPopupButtonClass *klass)
{
	GtkToggleButtonClass *toggle_button_class;

	toggle_button_class = GTK_TOGGLE_BUTTON_CLASS (klass);
	toggle_button_class->toggled = planner_popup_button_toggled;

	signals[POPUP] = g_signal_new ("popup",
				       G_TYPE_FROM_CLASS (klass),
				       G_SIGNAL_RUN_LAST,
				       0,
				       NULL, NULL,
				       planner_marshal_OBJECT__VOID,
				       GTK_TYPE_WIDGET, 0);

	signals[POPDOWN] = g_signal_new ("popdown",
					 G_TYPE_FROM_CLASS (klass),
					 G_SIGNAL_RUN_LAST,
					 0,
					 NULL, NULL,
					 planner_marshal_VOID__OBJECT_BOOLEAN,
					 G_TYPE_NONE, 2,
					 GTK_TYPE_WIDGET, G_TYPE_BOOLEAN);

	g_type_class_add_private (klass, sizeof (PlannerPopupButtonPriv));
}

static void
planner_popup_button_init (PlannerPopupButton *button)
{
	PlannerPopupButtonPriv *priv;

	priv = GET_PRIV (button);

	priv->popup_window = gtk_window_new (GTK_WINDOW_POPUP);
	gtk_window_set_resizable (GTK_WINDOW (priv->popup_window), FALSE);
	gtk_window_set_screen (GTK_WINDOW (priv->popup_window),
			       gtk_widget_get_screen (GTK_WIDGET (button)));

	g_signal_connect (priv->popup_window,
			  "button-press-event",
			  G_CALLBACK (popup_button_press_event_cb),
			  button);
}

static void
planner_popup_button_toggled (GtkToggleButton *toggle_button)
{
	PlannerPopupButton *button;

	button = PLANNER_POPUP_BUTTON (toggle_button);

	if (gtk_toggle_button_get_active (toggle_button)) {
		popup_button_popup (button);
	} else {
		popup_button_popdown (button, TRUE);
	}
}

GtkWidget *
planner_popup_button_new (const gchar *label)
{
	GtkWidget *button;

	button = g_object_new (PLANNER_TYPE_POPUP_BUTTON,
			       "label", label,
			       NULL);

	return button;
}

static gboolean
popup_button_press_event_cb (GtkWidget          *popup_window,
			     GdkEventButton     *event,
			     PlannerPopupButton *popup_button)
{
	GtkAllocation           alloc;
	gdouble                 x, y;
	gint                    xoffset, yoffset;
	gint                    x1, y1;
	gint                    x2, y2;

	/* Popdown the window if the click is outside of it. */

	if (event->button != 1) {
		return FALSE;
	}

	x = event->x_root;
	y = event->y_root;

	gdk_window_get_root_origin (popup_window->window,
				    &xoffset,
				    &yoffset);

	xoffset += popup_window->allocation.x;
	yoffset += popup_window->allocation.y;

	alloc = popup_window->allocation;
	x1 = alloc.x + xoffset;
	y1 = alloc.y + yoffset;
	x2 = x1 + alloc.width;
	y2 = y1 + alloc.height;

	if (x > x1 && x < x2 && y > y1 && y < y2) {
		return FALSE;
	}

	planner_popup_button_popdown (popup_button, FALSE);

	return FALSE;
}

static gboolean
popup_button_grab_on_window (GdkWindow *window,
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
popup_button_position (PlannerPopupButton *button,
		       gint               *x,
		       gint               *y)
{
	PlannerPopupButtonPriv *priv;
	GtkWidget              *button_widget;
	GtkRequisition          popup_req;
	GdkScreen              *screen;
	gint                    monitor_num;
	GdkRectangle            monitor;

	priv = GET_PRIV (button);

	button_widget = GTK_WIDGET (button);

	gdk_window_get_origin (button_widget->window, x, y);

	if (GTK_WIDGET_NO_WINDOW (button_widget)) {
		*x += button_widget->allocation.x;
		*y += button_widget->allocation.y;
	}

	/* The popup should be placed below the button, right-aligned to it. */
	*y += button_widget->allocation.height;
	*x += button_widget->allocation.width;

	gtk_widget_size_request (priv->popup_widget, &popup_req);

	*x -= popup_req.width;

	/* Don't popup outside the monitor edges. */
	screen = gtk_widget_get_screen (GTK_WIDGET (button));
	monitor_num = gdk_screen_get_monitor_at_window (
		screen, GTK_WIDGET (button)->window);
	gdk_screen_get_monitor_geometry (screen, monitor_num, &monitor);

	if (*x < monitor.x) {
		*x = monitor.x;
	}
	else if (*x + popup_req.width > monitor.x + monitor.width) {
		*x = monitor.x + monitor.width - popup_req.width;
	}

	if (*y + popup_req.height > monitor.y + monitor.height) {
		*y -= popup_req.height + button_widget->allocation.height;
	}
}

static void
popup_button_popup (PlannerPopupButton *button)
{
	PlannerPopupButtonPriv *priv;
	gint                    x, y;

	priv = GET_PRIV (button);

	if (priv->popup_widget) {
		return;
	}

	g_signal_emit (button, signals[POPUP], 0, &priv->popup_widget);

	gtk_container_add (GTK_CONTAINER (priv->popup_window), priv->popup_widget);
	gtk_widget_show_all (priv->popup_widget);

	popup_button_position (button, &x, &y);

	gtk_window_move (GTK_WINDOW (priv->popup_window), x, y);
	gtk_widget_show (priv->popup_window);

        gtk_widget_grab_focus (priv->popup_widget);

	if (!popup_button_grab_on_window (GTK_WIDGET (priv->popup_widget)->window,
					  gtk_get_current_event_time ())) {

		popup_button_popdown (button, FALSE);
		return;
	}

	gtk_grab_add (priv->popup_window);
}

static void
popup_button_popdown (PlannerPopupButton *button,
		      gboolean            ok)
{
	PlannerPopupButtonPriv *priv;

	priv = GET_PRIV (button);

	if (!priv->popup_widget) {
		return;
	}

	gtk_widget_hide (priv->popup_window);

	g_object_ref (priv->popup_widget);
	gtk_container_remove (GTK_CONTAINER (priv->popup_window),
			      priv->popup_widget);

	g_signal_emit (button, signals[POPDOWN], 0, priv->popup_widget, ok);

	g_object_unref (priv->popup_widget);
	priv->popup_widget = NULL;

	if (GTK_WIDGET_HAS_GRAB (priv->popup_window)) {
		gtk_grab_remove (priv->popup_window);
		gdk_keyboard_ungrab (GDK_CURRENT_TIME);
		gdk_pointer_ungrab (GDK_CURRENT_TIME);
	}
}

void
planner_popup_button_popup (PlannerPopupButton *button)
{
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button))) {
		return;
	}

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
}

void
planner_popup_button_popdown (PlannerPopupButton *button,
			      gboolean            ok)
{
	if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button))) {
		return;
	}

	popup_button_popdown (button, ok);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), FALSE);
}

