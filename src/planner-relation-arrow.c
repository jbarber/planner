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
#include <math.h>
#include <libgnomecanvas/gnome-canvas.h>
#include <libgnomecanvas/gnome-canvas-util.h>
#include <libgnomecanvas/gnome-canvas-line.h>
#include <libplanner/mrp-relation.h>
#include "planner-gantt-row.h"
#include "planner-relation-arrow.h"

#undef USE_AFFINE

#define MIN_SPACING 4
#define ARROW_SIZE 6

/*

SS:

  1:      A 
  	 +--XXXXXX
       B | 
	 +---XXXXX
          C
	  
FF:

  1:               A 
  	   XXXXXX----+
	             | B
	     XXXXX---+
		   C
		   
SF:

  1:                       A (we have a maximum value for A)
  	   		 +------XXXXXX
	               B |
		     XXXXX


  2:	          A (max value)  
     	         +-XXXXXX
	       B |        C
	 	 +---------------+
	  		         |D
	 		   XXXXX-+
          		        E (max value)

FS:

  1:               A (we have a minimum value for A)
  	   XXXXXX----+
	             | B
		     XXXXX


  2:	                A (min value)  
     	          XXXXXX-+
	                 | B
	 +---------------+
	 | D	    C	 
	 +-XXXXX
          E (min value)


*/


typedef struct {
	gdouble x;
	gdouble y;
} PlannerPoint;

typedef enum {
	PLANNER_ARROW_UP,
	PLANNER_ARROW_DOWN,
	PLANNER_ARROW_RIGHT,
	PLANNER_ARROW_LEFT
} PlannerArrowDir;
	
struct _PlannerRelationArrowPriv {
	PlannerGanttRow      *successor;
	PlannerGanttRow      *predecessor;
	MrpRelationType  type;

	gboolean         successor_visible;
	gboolean         predecessor_visible;

	guint            num_points;
	PlannerPoint          points[6];
	PlannerArrowDir       arrow_dir;
		
	gdouble          x1;
	gdouble          y1;
	gdouble          x2;
	gdouble          y2;
	gdouble          y2_top;
	gdouble          y2_bottom;
};


static void    relation_arrow_init         (PlannerRelationArrow       *mra);
static void    relation_arrow_class_init   (PlannerRelationArrowClass  *klass);
static void    relation_arrow_finalize     (GObject               *object);
static void    relation_arrow_set_property (GObject               *object,
					    guint                  prop_id,
					    const GValue          *value,
					    GParamSpec            *pspec);
static void    relation_arrow_update       (GnomeCanvasItem       *item,
					    gdouble               *affine,
					    ArtSVP                *clip_path,
					    gint                   flags);
static void    relation_arrow_draw         (GnomeCanvasItem       *item,
					    GdkDrawable           *drawable,
					    gint                   x,
					    gint                   y,
					    gint                   width,
					    gint                   height);
static gdouble relation_arrow_point        (GnomeCanvasItem       *item,
					    gdouble                x,
					    gdouble                y,
					    gint                   cx,
					    gint                   cy,
					    GnomeCanvasItem      **actual_item);

/* Properties */
enum {
	PROP_0,
	PROP_TYPE,
	PROP_PREDECESSOR,
	PROP_SUCCESSOR
};

static GnomeCanvasItemClass *parent_class = NULL;


GType
planner_relation_arrow_get_type (void)
{
	static GType type = 0;

	if (!type) {
		static const GTypeInfo info = {
			sizeof (PlannerRelationArrowClass),
			NULL,		/* base_init */
			NULL,		/* base_finalize */
			(GClassInitFunc) relation_arrow_class_init,
			NULL,		/* class_finalize */
			NULL,		/* class_data */
			sizeof (PlannerRelationArrow),
			0,              /* n_preallocs */
			(GInstanceInitFunc) relation_arrow_init
		};

		type = g_type_register_static (GNOME_TYPE_CANVAS_ITEM,
					       "PlannerRelationArrow",
					       &info,
					       0);
	}
	
	return type;
}

static void
relation_arrow_class_init (PlannerRelationArrowClass *klass)
{
	GnomeCanvasItemClass *item_class;
	GObjectClass         *o_class;

	parent_class = g_type_class_peek_parent (klass);

	item_class = GNOME_CANVAS_ITEM_CLASS (klass);
	o_class = G_OBJECT_CLASS (klass);

	o_class->set_property = relation_arrow_set_property;
	o_class->finalize = relation_arrow_finalize;

	item_class->update = relation_arrow_update;
	item_class->draw = relation_arrow_draw;
	item_class->bounds = NULL;
	item_class->point = relation_arrow_point;

	/* Properties. */
	g_object_class_install_property (o_class,
					 PROP_TYPE,
					 g_param_spec_enum ("type",
							    NULL,
							    NULL,
							    MRP_TYPE_RELATION_TYPE,
							    MRP_RELATION_FS,
							    G_PARAM_WRITABLE));
}

static void
relation_arrow_init (PlannerRelationArrow *item)
{
	PlannerRelationArrowPriv *priv;

	priv = g_new0 (PlannerRelationArrowPriv, 1);
	item->priv = priv;
	
	priv->successor_visible = TRUE;
	priv->predecessor_visible = TRUE;

	priv->type = MRP_RELATION_FS;
	priv->arrow_dir = PLANNER_ARROW_DOWN;
}

static void
relation_arrow_finalize (GObject *object)
{
	PlannerRelationArrow     *arrow = PLANNER_RELATION_ARROW (object);
	PlannerRelationArrowPriv *priv;

	priv = arrow->priv;
	
	if (priv->predecessor) {
		g_object_remove_weak_pointer (G_OBJECT (priv->predecessor),
					      (gpointer)&priv->predecessor);
	}
	
	if (priv->successor) {
		g_object_remove_weak_pointer (G_OBJECT (priv->successor),
					      (gpointer)&priv->successor);
	}
	
	g_free (priv);
	arrow->priv = NULL;

	if (G_OBJECT_CLASS (parent_class)->finalize) {
		(* G_OBJECT_CLASS (parent_class)->finalize) (object);
	}
}

static void
relation_arrow_set_property (GObject      *object,
			     guint         prop_id,
			     const GValue *value,
			     GParamSpec   *pspec)
{
	PlannerRelationArrow *arrow;

	arrow = PLANNER_RELATION_ARROW (object);

	switch (prop_id) {
	case PROP_TYPE:
		arrow->priv->type = g_value_get_enum (value);
		break;
		
	default:
		break;
	}
}

static void
relation_arrow_update_line_segments (PlannerRelationArrow *arrow)
{
	PlannerRelationArrowPriv *priv;
	gdouble                   px1, py1, px2, py2;
	gdouble                   sx1, sy1, sx2, sy2;
	gdouble                   y;
	MrpRelationType           type;

	priv = arrow->priv;
	type = priv->type;

	planner_gantt_row_get_geometry (priv->predecessor,
					&px1,
					&py1,
					&px2,
					&py2);
	
	planner_gantt_row_get_geometry (priv->successor,
					&sx1,
					&sy1,
					&sx2,
					&sy2);
	if (type == MRP_RELATION_SS) {
		priv->num_points = 4;
		priv->arrow_dir = PLANNER_ARROW_RIGHT;
		
		/* LHS of pred */
		priv->points[0].x = px1;
		priv->points[0].y = py1 + (py2 - py1) / 2;

		/* Next two, a bit left of the most left */
		if (sx1 < px1) {
			priv->points[1].x = sx1 - MIN_SPACING - ARROW_SIZE;
			priv->points[1].y = py1 + (py2 - py1) / 2;
			priv->points[2].x = sx1 - MIN_SPACING - ARROW_SIZE;
			priv->points[2].y = sy1 + (sy2 - sy1) / 2;
		} else {
			priv->points[1].x = px1 - MIN_SPACING - ARROW_SIZE;
			priv->points[1].y = py1 + (py2 - py1) / 2;
			priv->points[2].x = px1 - MIN_SPACING - ARROW_SIZE;
			priv->points[2].y = sy1 + (sy2 - sy1) / 2;
		}
		
		priv->points[3].x = sx1;
		priv->points[3].y = sy1 + (sy2 - sy1) / 2;
		
	}
	else if (type == MRP_RELATION_FF) {
		
		priv->num_points = 4;
		priv->arrow_dir = PLANNER_ARROW_LEFT;

		/* LHS of pred */
		priv->points[0].x = px2;
		priv->points[0].y = py1 + (py2 - py1) / 2;

		/* Next two, a bit left of the most left */
		if (sx2 > px2) {
			priv->points[1].x = sx2 + MIN_SPACING + ARROW_SIZE;
			priv->points[1].y = py1 + (py2 - py1) / 2;
			priv->points[2].x = sx2 + MIN_SPACING + ARROW_SIZE;
			priv->points[2].y = sy1 + (sy2 - sy1) / 2;
		} else {
			priv->points[1].x = px2 + MIN_SPACING + ARROW_SIZE;
			priv->points[1].y = py1 + (py2 - py1) / 2;
			priv->points[2].x = px2 + MIN_SPACING + ARROW_SIZE;
			priv->points[2].y = sy1 + (sy2 - sy1) / 2;
		}
		
		priv->points[3].x = sx2;
		priv->points[3].y = sy1 + (sy2 - sy1) / 2;
	}
	else if (type == MRP_RELATION_SF) {
		/* Two cases for SF, as shown at the top of this file. */
		if (px1 >= sx2) {
			priv->num_points = 3;
	
			priv->points[0].x = px1;
			priv->points[0].y = py1 + (py2 - py1) / 2;

			priv->points[1].x = MIN (px1 - MIN_SPACING, sx2);
			priv->points[1].y = py1 + (py2 - py1) / 2;
	
			priv->points[2].x = MIN (px1 - MIN_SPACING, sx2);
			if (sy1 > py1) {
				priv->points[2].y = sy1;
				priv->arrow_dir = PLANNER_ARROW_DOWN;
			} else {
				priv->points[2].y = sy2;
				priv->arrow_dir = PLANNER_ARROW_UP;
			}
	
			
		} else {
			priv->num_points = 6;
			priv->arrow_dir = PLANNER_ARROW_LEFT;
	
			priv->points[0].x = px1;
			priv->points[0].y = py1 + (py2 - py1) / 2;
	
			priv->points[1].x = px1 - MIN_SPACING;
			priv->points[1].y = py1 + (py2 - py1) / 2;
	
	 		if (sy1 > py1) {
				y = py2 + (py2 - py1) / 2 - 1;
			} else {
				y = py1 - (py2 - py1) / 2 + 2;
			}
			
			priv->points[2].x = px1 - MIN_SPACING;
			priv->points[2].y = y;
	
			priv->points[3].x = sx2 + ARROW_SIZE + MIN_SPACING;
			priv->points[3].y = y;
			
			priv->points[4].x = sx2 + ARROW_SIZE + MIN_SPACING;
			priv->points[4].y = sy1 + (sy2 - sy1) / 2;
	
			priv->points[5].x = sx2;
			priv->points[5].y = sy1 + (sy2 - sy1) / 2;
		}
	
	
	} else {
	/* Two cases for FS, as shown at the top of this file. */
	if (px2 <= sx1) {
		priv->num_points = 3;

		priv->points[0].x = px2;
		priv->points[0].y = py1 + (py2 - py1) / 2;

		priv->points[1].x = MAX (px2 + MIN_SPACING, sx1);
		priv->points[1].y = py1 + (py2 - py1) / 2;

		priv->points[2].x = MAX (px2 + MIN_SPACING, sx1);
		if (sy1 > py1) {
			priv->points[2].y = sy1;
			priv->arrow_dir = PLANNER_ARROW_DOWN;
		} else {
			priv->points[2].y = sy2;
			priv->arrow_dir = PLANNER_ARROW_UP;
		}
	
			
	} else {
		priv->num_points = 6;
		priv->arrow_dir = PLANNER_ARROW_RIGHT;

		priv->points[0].x = px2;
		priv->points[0].y = py1 + (py2 - py1) / 2;

		priv->points[1].x = px2 + MIN_SPACING;
		priv->points[1].y = py1 + (py2 - py1) / 2;

 		if (sy1 > py1) {
			y = py2 + (py2 - py1) / 2 - 1;
		} else {
			y = py1 - (py2 - py1) / 2 + 2;
		}
		
		priv->points[2].x = px2 + MIN_SPACING;
		priv->points[2].y = y;

		priv->points[3].x = sx1 - ARROW_SIZE - MIN_SPACING;
		priv->points[3].y = y;
		
		priv->points[4].x = sx1 - ARROW_SIZE - MIN_SPACING;
		priv->points[4].y = sy1 + (sy2 - sy1) / 2;

		priv->points[5].x = sx1;
		priv->points[5].y = sy1 + (sy2 - sy1) / 2;
	}
	}

	gnome_canvas_item_request_update (GNOME_CANVAS_ITEM (arrow));
}

static void
relation_arrow_geometry_changed (PlannerGanttRow      *row,
				 gdouble               x1,
				 gdouble               y1,
				 gdouble               x2,
				 gdouble               y2,
				 PlannerRelationArrow *arrow)
{
	relation_arrow_update_line_segments (arrow);
}

static void
relation_arrow_successor_visibility_changed (PlannerGanttRow      *row,
					     gboolean              visible,
					     PlannerRelationArrow *arrow)
{
	arrow->priv->successor_visible = visible;
	
	if (!visible) {
		gnome_canvas_item_hide (GNOME_CANVAS_ITEM (arrow));
	} else if (arrow->priv->predecessor_visible) {
		gnome_canvas_item_show (GNOME_CANVAS_ITEM (arrow));
	}
}

static void
relation_arrow_predecessor_visibility_changed (PlannerGanttRow      *row,
					       gboolean              visible,
					       PlannerRelationArrow *arrow)
{
	arrow->priv->predecessor_visible = visible;

	if (!visible) {
		gnome_canvas_item_hide (GNOME_CANVAS_ITEM (arrow));
	} else if (arrow->priv->successor_visible) {
		gnome_canvas_item_show (GNOME_CANVAS_ITEM (arrow));
	}
}

void
planner_relation_arrow_set_predecessor (PlannerRelationArrow *arrow,
					PlannerGanttRow      *predecessor)
{
	PlannerRelationArrowPriv *priv; 
	
	g_return_if_fail (PLANNER_IS_RELATION_ARROW (arrow));
	g_return_if_fail (PLANNER_IS_GANTT_ROW (predecessor));

	priv = arrow->priv;
	
	if (priv->predecessor) {
		g_object_remove_weak_pointer (G_OBJECT (priv->predecessor),
					      (gpointer)&priv->predecessor);
	}		
	
	priv->predecessor = predecessor;

	g_object_add_weak_pointer (G_OBJECT (predecessor),
				   (gpointer)&priv->predecessor);
	
	g_signal_connect_object (predecessor,
				 "geometry-changed",
				 G_CALLBACK (relation_arrow_geometry_changed),
				 arrow,
				 0);

	g_signal_connect_object (predecessor,
				 "visibility-changed",
				 G_CALLBACK (relation_arrow_predecessor_visibility_changed),
				 arrow,
				 0);

	if (priv->predecessor != NULL && priv->successor != NULL) {
		relation_arrow_update_line_segments (arrow);
	}
}

void
planner_relation_arrow_set_successor (PlannerRelationArrow *arrow,
				      PlannerGanttRow      *successor)
{
	PlannerRelationArrowPriv *priv; 

	g_return_if_fail (PLANNER_IS_RELATION_ARROW (arrow));
	g_return_if_fail (PLANNER_IS_GANTT_ROW (successor));

	priv = arrow->priv;

	if (priv->successor) {
		g_object_remove_weak_pointer (G_OBJECT (priv->successor),
					      (gpointer)&priv->successor);
	}
	
	priv->successor = successor;

	g_object_add_weak_pointer (G_OBJECT (successor), (gpointer)&priv->successor);

	g_signal_connect_object (successor,
				 "geometry-changed",
				 G_CALLBACK (relation_arrow_geometry_changed),
				 arrow,
				 0);
	
	g_signal_connect_object (successor,
				 "visibility-changed",
				 G_CALLBACK (relation_arrow_successor_visibility_changed),
				 arrow,
				 0);

	if (priv->predecessor != NULL && priv->successor != NULL) {
		relation_arrow_update_line_segments (arrow);
	}
}

PlannerRelationArrow *
planner_relation_arrow_new (PlannerGanttRow *successor,
			    PlannerGanttRow *predecessor,
			    MrpRelationType  type)
{
	PlannerRelationArrow *arrow;
	GnomeCanvasGroup     *root;

	root = gnome_canvas_root (GNOME_CANVAS_ITEM (successor)->canvas);
	
	arrow = PLANNER_RELATION_ARROW (
		gnome_canvas_item_new (root,
				       PLANNER_TYPE_RELATION_ARROW,
				       NULL));
	
	arrow->priv->type = type;

	planner_relation_arrow_set_successor (arrow, successor);
	planner_relation_arrow_set_predecessor (arrow, predecessor);

	return arrow;
}

static void
relation_arrow_get_bounds (PlannerRelationArrow *arrow,
			   gdouble              *x1,
			   gdouble              *y1,
			   gdouble              *x2,
			   gdouble              *y2)
{
	PlannerRelationArrowPriv *priv;
	GnomeCanvasItem          *item;
	gdouble                   wx1, wy1, wx2, wy2;
	gint                      cx1, cy1, cx2, cy2;
	gint                      i;

	item = GNOME_CANVAS_ITEM (arrow);
	priv = arrow->priv;
	
	/* Get the items bbox in canvas pixel coordinates. */

	/* Silence warning. */
	wx1 = G_MAXDOUBLE;
	wy1 = G_MAXDOUBLE;
	wx2 = -G_MAXDOUBLE;
	wy2 = -G_MAXDOUBLE;
	
	for (i = 0; i < priv->num_points; i++) {
		wx1 = MIN (wx1, priv->points[i].x);
		wy1 = MIN (wy1, priv->points[i].y);
		wx2 = MAX (wx2, priv->points[i].x);
		wy2 = MAX (wy2, priv->points[i].y);
	}

	/* We could do better here, but this is good enough. */
	wx1 -= ARROW_SIZE / 2;
	wy1 -= ARROW_SIZE / 2;
	wx2 += ARROW_SIZE / 2;
	wy2 += ARROW_SIZE / 2;
	
	gnome_canvas_item_i2w (item, &wx1, &wy1);
	gnome_canvas_item_i2w (item, &wx2, &wy2);
	gnome_canvas_w2c (item->canvas, wx1, wy1, &cx1, &cy1);
	gnome_canvas_w2c (item->canvas, wx2, wy2, &cx2, &cy2);

	*x1 = cx1 - 1;
	*y1 = cy1 - 1;
	*x2 = cx2 + 1;
	*y2 = cy2 + 1;
}

static void
relation_arrow_update (GnomeCanvasItem *item,
		       double          *affine,
		       ArtSVP          *clip_path,
		       gint             flags)
{
	PlannerRelationArrow *arrow;
	gdouble               x1, y1, x2, y2;

	arrow = PLANNER_RELATION_ARROW (item);
	
	GNOME_CANVAS_ITEM_CLASS (parent_class)->update (item,
							affine,
							clip_path,
							flags);
	
	relation_arrow_get_bounds (arrow, &x1, &y1, &x2, &y2);

	gnome_canvas_update_bbox (item, x1, y1, x2, y2);
}

static void
relation_arrow_setup_arrow (PlannerArrowDir dir,
			    GdkPoint        points[4],
			    gdouble         cx1,
			    gdouble         cy1,
			    gdouble         cx2,
			    gdouble         cy2)
{
	switch (dir) {
	case PLANNER_ARROW_DOWN:
		points[0].x = cx2;
		points[0].y = cy2;
		points[1].x = cx2 - ARROW_SIZE / 2;
		points[1].y = cy2 - ARROW_SIZE;
		points[2].x = cx2 + ARROW_SIZE / 2;
		points[2].y = cy2 - (ARROW_SIZE - 1);
		points[3].x = cx2;
		points[3].y = cy2 + 1;
		break;
	case PLANNER_ARROW_UP:
		points[0].x = cx2;
		points[0].y = cy2 + 1;
		points[1].x = cx2 + ARROW_SIZE / 2;
		points[1].y = cy2 + ARROW_SIZE;
		points[2].x = cx2 - ARROW_SIZE / 2;
		points[2].y = cy2 + ARROW_SIZE;
		points[3].x = cx2 + 1;
		points[3].y = cy2;
		break;
	case PLANNER_ARROW_RIGHT:
		points[0].x = cx2;
		points[0].y = cy2;
		points[1].x = cx2 - ARROW_SIZE;
		points[1].y = cy2 + ARROW_SIZE / 2;
		points[2].x = cx2 - ARROW_SIZE;
		points[2].y = cy2 - ARROW_SIZE / 2;
		points[3].x = cx2;
		points[3].y = cy2;
		break;
	case PLANNER_ARROW_LEFT:
		points[0].x = cx2;
		points[0].y = cy2;
		points[1].x = cx2 + (ARROW_SIZE - 0);
		points[1].y = cy2 + ARROW_SIZE / 2;
		points[2].x = cx2 + (ARROW_SIZE - 0);
		points[2].y = cy2 - ARROW_SIZE / 2;
		points[3].x = cx2;
		points[3].y = cy2;
		break;
	default:
		g_assert_not_reached ();
		break;
	}
}

static void
relation_arrow_draw (GnomeCanvasItem *item,
		     GdkDrawable     *drawable,
		     gint             x,
		     gint             y,
		     gint             width,
		     gint             height)
{
	PlannerRelationArrow     *arrow;
	PlannerRelationArrowPriv *priv;
#ifdef USE_AFFINE
	gdouble                   i2c[6];
	ArtPoint                  i1, i2, c1, c2;
#else
	gdouble                   i2w_dx;
	gdouble                   i2w_dy;
	gdouble                   dx1, dy1, dx2, dy2;
#endif
	gint                      cx1, cy1, cx2, cy2;
	GdkGC                    *gc;
	GdkPoint                  points[4];
	gint                      i;
	
	arrow = PLANNER_RELATION_ARROW (item);
	priv = arrow->priv;

	gc = gdk_gc_new (drawable);
	gdk_gc_set_line_attributes (gc,
				    0,
				    GDK_LINE_SOLID,
				    GDK_CAP_BUTT,
				    GDK_JOIN_MITER);

	/* Silence warning. */
	cx1 = 0;
	cy1 = 0;
	cx2 = 0;
	cy2 = 0;
	
#ifdef USE_AFFINE
	/* Get item area in canvas coordinates. */
	gnome_canvas_item_i2c_affine (item, i2c);
#endif
	
	for (i = 0; i < priv->num_points - 1; i++) {
#ifdef USE_AFFINE
		i1.x = priv->points[i].x;
		i1.y = priv->points[i].y;
		i2.x = priv->points[i+1].x;
		i2.y = priv->points[i+1].y;
		art_affine_point (&c1, &i1, i2c);
		art_affine_point (&c2, &i2, i2c);
		cx1 = floor (c1.x + 0.5) - x;
		cy1 = floor (c1.y + 0.5) - y;
		cx2 = floor (c2.x + 0.5) - x;
		cy2 = floor (c2.y + 0.5) - y;
#else
		i2w_dx = 0.0;
		i2w_dy = 0.0;
		gnome_canvas_item_i2w (item, &i2w_dx, &i2w_dy);

		dx1 = priv->points[i].x;
		dy1 = priv->points[i].y;
		dx2 = priv->points[i+1].x;
		dy2 = priv->points[i+1].y;
		
		gnome_canvas_w2c (item->canvas,
				  dx1 + i2w_dx,
				  dy1 + i2w_dy,
				  &cx1,
				  &cy1);
		
		gnome_canvas_w2c (item->canvas,
				  dx2 + i2w_dx,
				  dy2 + i2w_dy,
				  &cx2,
				  &cy2);
		
		cx1 -= x;
		cy1 -= y;
		cx2 -= x;
		cy2 -= y;
#endif
		
		gdk_draw_line (drawable,
			       gc,
			       cx1,
			       cy1,
			       cx2,
			       cy2);
	}

	relation_arrow_setup_arrow (priv->arrow_dir,
					points,
					cx1,
					cy1,
					cx2,
					cy2);

	gdk_draw_polygon (drawable,
			  gc,
			  TRUE,
			  (GdkPoint *) &points,
			  4);

	g_object_unref (gc);
}

static double
relation_arrow_point (GnomeCanvasItem  *item,
		      gdouble           x,
		      gdouble           y,
		      gint              cx,
		      gint              cy,
		      GnomeCanvasItem **actual_item)
{
	*actual_item = item;

	return 10.0;
}

