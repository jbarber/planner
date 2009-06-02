/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2002 CodeFactory AB
 * Copyright (C) 2002 Richard Hult <richard@imendio.com>
 * Copyright (C) 2002 Mikael Hallendal <micke@imendio.com>
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

#ifndef __PLANNER_GANTT_BACKGROUND_H__
#define __PLANNER_GANTT_BACKGROUND_H__

#include <gtk/gtk.h>
#include <libgnomecanvas/gnome-canvas.h>
#include <libgnomecanvas/gnome-canvas-util.h>

#define PLANNER_TYPE_GANTT_BACKGROUND		(planner_gantt_background_get_type ())
#define PLANNER_GANTT_BACKGROUND(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), PLANNER_TYPE_GANTT_BACKGROUND, PlannerGanttBackground))
#define PLANNER_GANTT_BACKGROUND_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), PLANNER_TYPE_GANTT_BACKGROUND, PlannerGanttBackgroundClass))
#define PLANNER_IS_GANTT_BACKGROUND(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), PLANNER_TYPE_GANTT_BACKGROUND))
#define PLANNER_IS_GANTT_BACKGROUND_CLASS(klass)	(G_TYPE_CHECK_TYPE ((obj), PLANNER_TYPE_GANTT_BACKGROUND))
#define PLANNER_GANTT_BACKGROUND_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), PLANNER_TYPE_GANTT_BACKGROUND, PlannerGanttBackgroundClass))

typedef struct _PlannerGanttBackground      PlannerGanttBackground;
typedef struct _PlannerGanttBackgroundClass PlannerGanttBackgroundClass;
typedef struct _PlannerGanttBackgroundPriv  PlannerGanttBackgroundPriv;

struct _PlannerGanttBackground {
	GnomeCanvasItem         parent;
	PlannerGanttBackgroundPriv  *priv;
};

struct _PlannerGanttBackgroundClass {
	GnomeCanvasItemClass parent_class;
};


GType planner_gantt_background_get_type (void) G_GNUC_CONST;


#endif /* __PLANNER_GANTT_BACKGROUND_H__ */

