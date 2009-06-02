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

#include <config.h>
#include <libgnomecanvas/gnome-canvas-line.h>
#include "planner-canvas-line.h"

static void   mcl_init	     (PlannerCanvasLine       *mcl);
static void   mcl_class_init (PlannerCanvasLineClass  *klass);
static double mcl_point      (GnomeCanvasItem    *item,
			      double              x,
			      double              y,
			      int                 cx,
			      int                 cy,
			      GnomeCanvasItem   **actual_item);


GType
planner_canvas_line_get_type (void)
{
	static GType type = 0;

	if (!type) {
		static const GTypeInfo info = {
			sizeof (PlannerCanvasLineClass),
			NULL,		/* base_init */
			NULL,		/* base_finalize */
			(GClassInitFunc) mcl_class_init,
			NULL,		/* class_finalize */
			NULL,		/* class_data */
			sizeof (PlannerCanvasLine),
			0,              /* n_preallocs */
			(GInstanceInitFunc) mcl_init
		};

		type = g_type_register_static (GNOME_TYPE_CANVAS_LINE, "PlannerCanvasLine",
					       &info, 0);
	}

	return type;
}

static void
mcl_class_init (PlannerCanvasLineClass *klass)
{
	GnomeCanvasItemClass *item_class;

	item_class = GNOME_CANVAS_ITEM_CLASS (klass);

	item_class->bounds = NULL;
	item_class->point = mcl_point;
}

static void
mcl_init (PlannerCanvasLine *item)
{
}

static double
mcl_point (GnomeCanvasItem  *item,
	   double            x,
	   double            y,
	   int               cx,
	   int               cy,
	   GnomeCanvasItem **actual_item)
{
	*actual_item = item;

	return 1000.0;
}
