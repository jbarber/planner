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

#ifndef __MG_RELATION_ARROW_H__
#define __MG_RELATION_ARROW_H__

#include <glib-object.h>
#include <libgnomecanvas/gnome-canvas.h>
#include "planner-gantt-row.h"

#define MG_TYPE_RELATION_ARROW            (planner_relation_arrow_get_type ())
#define MG_RELATION_ARROW(obj)            (GTK_CHECK_CAST ((obj), MG_TYPE_RELATION_ARROW, MgRelationArrow))
#define MG_RELATION_ARROW_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), MG_TYPE_RELATION_ARROW, MgRelationArrowClass))
#define MG_IS_RELATION_ARROW(obj)         (GTK_CHECK_TYPE ((obj), MG_TYPE_RELATION_ARROW))
#define MG_IS_RELATION_ARROW_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), MG_TYPE_RELATION_ARROW))
#define MG_RELATION_ARROW_GET_CLASS(obj)  (GTK_CHECK_GET_CLASS ((obj), MG_TYPE_RELATION_ARROW, MgRelationArrowClass))


typedef struct _MgRelationArrow      MgRelationArrow;
typedef struct _MgRelationArrowClass MgRelationArrowClass;
typedef struct _MgRelationArrowPriv  MgRelationArrowPriv;

struct _MgRelationArrow {
	GnomeCanvasItem      parent;
	MgRelationArrowPriv *priv;
};

struct _MgRelationArrowClass {
	GnomeCanvasItemClass parent_class;
};


GType            planner_relation_arrow_get_type (void) G_GNUC_CONST;

MgRelationArrow *planner_relation_arrow_new      (MgGanttRow *successor,
					     MgGanttRow *predecessor);
void
planner_relation_arrow_set_successor             (MgRelationArrow *arrow,
					     MgGanttRow      *successor);
void
planner_relation_arrow_set_predecessor           (MgRelationArrow *arrow,
					     MgGanttRow      *predecessor);


#endif /* __MG_RELATION_ARROW_H__ */

