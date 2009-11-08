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

#ifndef __PLANNER_POPUP_BUTTON_H__
#define __PLANNER_POPUP_BUTTON_H__

#include <gtk/gtk.h>

#define PLANNER_TYPE_POPUP_BUTTON            (planner_popup_button_get_type ())
#define PLANNER_POPUP_BUTTON(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PLANNER_TYPE_POPUP_BUTTON, PlannerPopupButton))
#define PLANNER_POPUP_BUTTON_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), PLANNER_TYPE_POPUP_BUTTON, PlannerPopupButtonClass))
#define PLANNER_IS_POPUP_BUTTON(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PLANNER_TYPE_POPUP_BUTTON))
#define PLANNER_IS_POPUP_BUTTON_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), PLANNER_TYPE_POPUP_BUTTON))
#define PLANNER_POPUP_BUTTON_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), PLANNER_TYPE_POPUP_BUTTON, PlannerPopupButtonClass))


typedef struct _PlannerPopupButton      PlannerPopupButton;
typedef struct _PlannerPopupButtonClass PlannerPopupButtonClass;

struct _PlannerPopupButton {
	GtkToggleButton  parent;
};

struct _PlannerPopupButtonClass {
	GtkToggleButtonClass parent_class;
};

GType      planner_popup_button_get_type (void) G_GNUC_CONST;
GtkWidget *planner_popup_button_new      (const gchar *label);

void planner_popup_button_popup (PlannerPopupButton *button);
void planner_popup_button_popdown (PlannerPopupButton *button,
				   gboolean            ok);

#endif /* __PLANNER_POPUP_BUTTON_H__ */
