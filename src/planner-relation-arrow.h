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

#ifndef __PLANNER_RELATION_ARROW_H__
#define __PLANNER_RELATION_ARROW_H__

#include <glib-object.h>
#include <libgnomecanvas/gnome-canvas.h>
#include "planner-gantt-row.h"

#define PLANNER_TYPE_RELATION_ARROW            (planner_relation_arrow_get_type ())
#define PLANNER_RELATION_ARROW(obj)            (GTK_CHECK_CAST ((obj), PLANNER_TYPE_RELATION_ARROW, PlannerRelationArrow))
#define PLANNER_RELATION_ARROW_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), PLANNER_TYPE_RELATION_ARROW, PlannerRelationArrowClass))
#define PLANNER_IS_RELATION_ARROW(obj)         (GTK_CHECK_TYPE ((obj), PLANNER_TYPE_RELATION_ARROW))
#define PLANNER_IS_RELATION_ARROW_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), PLANNER_TYPE_RELATION_ARROW))
#define PLANNER_RELATION_ARROW_GET_CLASS(obj)  (GTK_CHECK_GET_CLASS ((obj), PLANNER_TYPE_RELATION_ARROW, PlannerRelationArrowClass))


typedef struct _PlannerRelationArrow      PlannerRelationArrow;
typedef struct _PlannerRelationArrowClass PlannerRelationArrowClass;
typedef struct _PlannerRelationArrowPriv  PlannerRelationArrowPriv;

struct _PlannerRelationArrow {
	GnomeCanvasItem      parent;
	PlannerRelationArrowPriv *priv;
};

struct _PlannerRelationArrowClass {
	GnomeCanvasItemClass parent_class;
};


GType            planner_relation_arrow_get_type (void) G_GNUC_CONST;

PlannerRelationArrow *planner_relation_arrow_new      (PlannerGanttRow *successor,
					     PlannerGanttRow *predecessor,
					     MrpRelationType type);
void
planner_relation_arrow_set_successor             (PlannerRelationArrow *arrow,
					     PlannerGanttRow      *successor);
void
planner_relation_arrow_set_predecessor           (PlannerRelationArrow *arrow,
					     PlannerGanttRow      *predecessor);


#endif /* __PLANNER_RELATION_ARROW_H__ */

