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

#ifndef __MG_CANVAS_LINE_H__
#define __MG_CANVAS_LINE_H__

#include <glib-object.h>
#include <libgnomecanvas/gnome-canvas-line.h>

#define MG_TYPE_CANVAS_LINE            (planner_canvas_line_get_type ())
#define MG_CANVAS_LINE(obj)            (GTK_CHECK_CAST ((obj), MG_TYPE_CANVAS_LINE, MgCanvasLine))
#define MG_CANVAS_LINE_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), MG_TYPE_CANVAS_LINE, MgCanvasLineClass))
#define MG_IS_CANVAS_LINE(obj)         (GTK_CHECK_TYPE ((obj), MG_TYPE_CANVAS_LINE))
#define MG_IS_CANVAS_LINE_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), MG_TYPE_CANVAS_LINE))
#define MG_CANVAS_LINE_GET_CLASS(obj)  (GTK_CHECK_GET_CLASS ((obj), MG_TYPE_CANVAS_LINE, MgCanvasLineClass))


typedef struct _MgCanvasLine      MgCanvasLine;
typedef struct _MgCanvasLineClass MgCanvasLineClass;

struct _MgCanvasLine {
	GnomeCanvasLine  parent;
};

struct _MgCanvasLineClass {
	GnomeCanvasLineClass parent_class;
};


GType planner_canvas_line_get_type (void) G_GNUC_CONST;


#endif /* __MG_CANVAS_LINE_H__ */

