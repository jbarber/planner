/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2003 Benjamin BAYART <benjamin@sitadelle.com>
 * Copyright (C) 2003 Xavier Ordoquy <xordoquy@wanadoo.fr>
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

/* FIXME: This code needs a SERIOUS clean-up. */

#include <config.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <libplanner/mrp-project.h>
#include <libplanner/mrp-resource.h>
#include <libplanner/mrp-task.h>
#include <libgnome/gnome-i18n.h>
#include <libgnomecanvas/gnome-canvas.h>
#include "planner-marshal.h"
#include "planner-format.h"
#include "planner-ttable-row.h"
#include "planner-ttable-chart.h"
#include "planner-canvas-line.h"
#include "eel-canvas-rect.h"
#include "planner-scale-utils.h"

/* The padding between the gantt bar and the text. */
#define TEXT_PADDING 10.0

/* Snap to this amount of time when dragging duration. */
#define SNAP (60.0*15.0)

/* Constants for the summary bracket. */
#define HEIGHT 5
#define THICKNESS 2
#define SLOPE 4

/* Constants for the milestone diamond.*/
#define MILESTONE_SIZE 5

enum {
	PROP_0,
	PROP_X,
	PROP_Y,
	PROP_WIDTH,
	PROP_HEIGHT,
	PROP_SCALE,
	PROP_ZOOM,
	PROP_ASSIGNMENT,
	PROP_RESOURCE,
	PROP_HIGHLIGHT,
	PROP_MOUSE_OVER_INDEX,
};

enum {
	/* For relation arrows. */
	GEOMETRY_CHANGED,
	VISIBILITY_CHANGED,
	LAST_SIGNAL
};

typedef enum {
	STATE_NONE		= 0,
	STATE_DRAG_MOVE		= 1 << 0,
	STATE_DRAG_DURATION	= 1 << 1,

	STATE_DRAG_ANY		= STATE_DRAG_MOVE | STATE_DRAG_DURATION
} State;

struct _PlannerTtableRowPriv {
	GdkGC		*complete_gc;
	GdkGC		*break_gc;
	GdkGC		*fill_gc;
	PangoLayout	*layout;

	MrpAssignment	*assignment;
	MrpResource	*resource;

	guint		visible:1;
	guint		fixed_duration:1;

	gdouble		scale;
	gdouble		zoom;

	gdouble		x;
	gdouble		y;
	gdouble		x_start;

	gdouble		width;
	gdouble		height;
	gdouble		text_width;

	guint		scroll_timeout_id;
	State		state;
};

static void	ttable_row_class_init		(PlannerTtableRowClass	*class);
static void	ttable_row_init			(PlannerTtableRow		*row);
static void	ttable_row_destroy		(GtkObject		*object);
static void	ttable_row_set_property		(GObject		*object,
						 guint			 param_id,
						 const GValue		*value,
						 GParamSpec		*pspec);
static void	ttable_row_get_property		(GObject		*object,
						 guint			 param_id,
						 GValue			*value,
						 GParamSpec		*pspec);
static void	ttable_row_update		(GnomeCanvasItem	*item,
						 double			*affine,
						 ArtSVP			*clip_path,
						 int			 flags);
static void	ttable_row_realize		(GnomeCanvasItem	*item);
static void	ttable_row_unrealize		(GnomeCanvasItem	*item);
static void	ttable_row_draw			(GnomeCanvasItem	*item,
						 GdkDrawable		*drawable,
						 gint			 x,
						 gint			 y,
						 gint			 width,
						 gint			 height);
static double	ttable_row_point		(GnomeCanvasItem	*item,
						 double			 x,
						 double			 y,
						 gint			 cx,
						 gint			 cy,
						 GnomeCanvasItem       **actual_item);
static void	ttable_row_bounds		(GnomeCanvasItem	*item,
						 double			*x1,
						 double			*y1,
						 double			*x2,
						 double			*y2);
static gboolean	ttable_row_event		(GnomeCanvasItem	*item,
						 GdkEvent		*event);
static void	ttable_row_geometry_changed	(PlannerTtableRow	*row);
/*
static gboolean	ttable_row_canvas_scroll	(GtkWidget		*widget,
						 gint			 delta_x,
						 gint			 delta_y);
*/
static void	ttable_row_ensure_layout	(PlannerTtableRow	*row);
static void	ttable_row_resource_notify_cb	(MrpResource		*resource,
						 GParamSpec		*pspec,
						 PlannerTtableRow		*row);
static void	ttable_row_assignment_notify_cb (MrpAssignment		*assign,
						 GParamSpec		*pspec,
						 PlannerTtableRow		*row);
static void	ttable_row_task_notify_cb	(MrpTask		*task,
						 GParamSpec		*pspec,
						 PlannerTtableRow		*row);
static void	ttable_row_resource_assignment_added_cb	(MrpResource	*resource,
						 MrpAssignment		*assign, 
						 PlannerTtableRow		*row);
static GdkGC *	ttable_row_create_frame_gc	(GnomeCanvas		*canvas);


static GnomeCanvasItemClass	*parent_class;
static guint			 signals[LAST_SIGNAL];
static GdkBitmap		*complete_stipple = NULL;
static gchar			 complete_stipple_pattern[] = { 0x02, 0x01 };
static GdkBitmap		*break_stipple = NULL;
static gchar			 break_stipple_pattern[] = { 0x03 };
/*
static GdkColor			 color_normal;
static GdkColor			 color_free;
static GdkColor			 color_underuse;
static GdkColor			 color_overuse;
static GdkColor			 color_high;
static GdkColor			 color_shadow;
static gboolean			 colors_initialised = FALSE;
*/

GType
planner_ttable_row_get_type(void)
{
	static GType type = 0;

	if (!type) {
		static const GTypeInfo info = {
			sizeof (PlannerTtableRowClass),
			NULL,           /* base_init */
			NULL,           /* base_finalize */
			(GClassInitFunc) ttable_row_class_init,
			NULL,           /* class_finalize */
			NULL,           /* class_data */
			sizeof (PlannerTtableRow),
			0,              /* n_preallocs */
			(GInstanceInitFunc) ttable_row_init
		};
		type = g_type_register_static (GNOME_TYPE_CANVAS_ITEM,
				"PlannerTtableRow",
				&info,
				0);
	}
	return type;
};


static void
ttable_row_class_init (PlannerTtableRowClass *class)
{
	GObjectClass         *gobject_class;
	GtkObjectClass       *object_class;
	GnomeCanvasItemClass *item_class;

	gobject_class = (GObjectClass *) class;
	object_class = (GtkObjectClass *) class;
	item_class = (GnomeCanvasItemClass *) class;

	parent_class = g_type_class_peek_parent (class);

	gobject_class->set_property = ttable_row_set_property;
	gobject_class->get_property = ttable_row_get_property;

	item_class->event = ttable_row_event;
	
	signals[GEOMETRY_CHANGED] =
		g_signal_new ("geometry-changed",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      0,
			      NULL, NULL,
			      planner_marshal_VOID__DOUBLE_DOUBLE_DOUBLE_DOUBLE,
			      G_TYPE_NONE, 4,
			      G_TYPE_DOUBLE, G_TYPE_DOUBLE, G_TYPE_DOUBLE, G_TYPE_DOUBLE);
	
	signals[VISIBILITY_CHANGED] =
		g_signal_new ("visibility-changed",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      0,
			      NULL, NULL,
			      planner_marshal_VOID__BOOLEAN,
			      G_TYPE_NONE, 1,
			      G_TYPE_BOOLEAN);

        g_object_class_install_property
                (gobject_class,
                 PROP_SCALE,
                 g_param_spec_double ("scale", NULL, NULL,
				      0.000001, G_MAXDOUBLE, 1.0,
				      G_PARAM_READWRITE));

	g_object_class_install_property (
                gobject_class,
		PROP_ZOOM,
		g_param_spec_double ("zoom",
				     NULL,
				     NULL,
				     -G_MAXDOUBLE, G_MAXDOUBLE, 7.0,
				     G_PARAM_WRITABLE));
	
	g_object_class_install_property
                (gobject_class,
                 PROP_Y,
                 g_param_spec_double ("y", NULL, NULL,
				      -G_MAXDOUBLE, G_MAXDOUBLE, 0.0,
				      (G_PARAM_READABLE | G_PARAM_WRITABLE)));

        g_object_class_install_property
                (gobject_class,
                 PROP_HEIGHT,
                 g_param_spec_double ("height", NULL, NULL,
				      -G_MAXDOUBLE, G_MAXDOUBLE, 0.0,
				      (G_PARAM_READABLE | G_PARAM_WRITABLE)));

        g_object_class_install_property
                (gobject_class,
                 PROP_ASSIGNMENT,
                 g_param_spec_object ("assignment", NULL, NULL,
				      MRP_TYPE_ASSIGNMENT,
				      G_PARAM_READWRITE));

        g_object_class_install_property
                (gobject_class,
                 PROP_RESOURCE,
                 g_param_spec_object ("resource", NULL, NULL,
				      MRP_TYPE_RESOURCE,
				      G_PARAM_READWRITE));

        g_object_class_install_property
                (gobject_class,
                 PROP_HIGHLIGHT,
                 g_param_spec_boolean ("highlight", NULL, NULL,
				       FALSE,
				       G_PARAM_READWRITE));
	
	g_object_class_install_property
                (gobject_class,
                 PROP_MOUSE_OVER_INDEX,
                 g_param_spec_int ("mouse-over-index", NULL, NULL,
				   -1, G_MAXINT, -1,
				   G_PARAM_WRITABLE));
	
	object_class->destroy = ttable_row_destroy;

	item_class->update = ttable_row_update;
	item_class->realize = ttable_row_realize;
	item_class->unrealize = ttable_row_unrealize;
	item_class->draw = ttable_row_draw;
	item_class->point = ttable_row_point;
	item_class->bounds = ttable_row_bounds;
}

static void
ttable_row_init (PlannerTtableRow *row)
{
	PlannerTtableRowPriv *priv;
	
	row->priv = g_new0 (PlannerTtableRowPriv, 1);
	priv = row->priv;
	
	priv->x = 0.0;
	priv->y = 0.0;
	priv->width = 0.0;
	priv->height = 0.0;
	priv->scale = 1.0;
	priv->visible = TRUE;

	priv->assignment = NULL;
	priv->fixed_duration = 0;
	priv->resource = NULL;
	priv->state = STATE_NONE;
	/*
	g_message("Timetable: New PlannerTtableRow (%p)",priv);
	priv->highlight = FALSE;
	priv->mouse_over_index = -1;
	priv->resource_widths = g_array_new (TRUE, FALSE, sizeof (gint));
	*/
}

static void
ttable_row_destroy (GtkObject *object)
{
	PlannerTtableRow     *row;
	PlannerTtableRowPriv *priv;

	g_return_if_fail (PLANNER_IS_TTABLE_ROW (object));

	row = PLANNER_TTABLE_ROW (object);
	priv = row->priv;

	if (priv) {
		if (priv->scroll_timeout_id) {
			g_source_remove (priv->scroll_timeout_id);
			priv->scroll_timeout_id = 0;
		}

		/* g_array_free (priv->resource_widths, FALSE); */
		
		g_free (priv);
		row->priv = NULL;
	}

	if (GTK_OBJECT_CLASS (parent_class)->destroy) {
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
	}
}

static void
ttable_row_get_bounds (PlannerTtableRow *row,
		       double     *px1,
		       double     *py1,
		       double     *px2,
		       double     *py2)
{
	GnomeCanvasItem *item;
	gdouble          wx1, wy1, wx2, wy2;
	gint             cx1, cy1, cx2, cy2;

	item = GNOME_CANVAS_ITEM (row);

	/* Get the items bbox in canvas pixel coordinates. */

	wx1 = row->priv->x - MILESTONE_SIZE - 1;
	wy1 = row->priv->y;
	wx2 = row->priv->x + MAX (row->priv->width, 2*MILESTONE_SIZE) + 1;
	wy2 = row->priv->y + row->priv->height;

	gnome_canvas_item_i2w (item, &wx1, &wy1);
	gnome_canvas_item_i2w (item, &wx2, &wy2);
	gnome_canvas_w2c (item->canvas, wx1, wy1, &cx1, &cy1);
	gnome_canvas_w2c (item->canvas, wx2, wy2, &cx2, &cy2);

	*px1 = cx1 - 1;
	*py1 = cy1 - 1;
	*px2 = cx2 + row->priv->text_width + 1;
	*py2 = cy2 + 1;
}

static void
get_assignment_bounds	(MrpAssignment	*assign,
			 gdouble	 scale,
			 gdouble	*x_debut,
			 gdouble	*x_fin,
			 gdouble	*x_start_reel)
{
	MrpTask	*task;
	mrptime  t;
	task = mrp_assignment_get_task(assign);
	t = mrp_task_get_work_start(task);
	*x_debut = t*scale;
	
	t = mrp_task_get_finish(task);
	*x_fin = t*scale;

	t = mrp_task_get_start(task);
	*x_start_reel = t*scale;
}

static void
get_resource_bounds	(MrpResource	*resource,
			 gdouble	 scale,
			 gdouble	*x_debut,
			 gdouble	*x_fin,
			 gdouble	*x_start_reel)
{
	MrpProject	*project;
	MrpTask		*root;
	mrptime		 t;

	g_object_get(G_OBJECT(resource),"project",&project,NULL);
	root = mrp_project_get_root_task(project);
	t = mrp_project_get_project_start(project);
	*x_debut = t*scale;
	*x_start_reel = t*scale;
	t = mrp_task_get_finish(root);
	*x_fin = t*scale;
}

/* FIXME: Rename this function to something more descriptive. */
static void
recalc_bounds (PlannerTtableRow *row)
{
	PlannerTtableRowPriv *priv;
	GnomeCanvasItem *item;
	/*
	gint             width;
	mrptime          t;
	*/
	gdouble		 x_debut=-1.0;
	gdouble		 x_fin=-1.0;
	gdouble		 x_debut_reel=-1.0;

	item = GNOME_CANVAS_ITEM (row);

	priv = row->priv;
	
	ttable_row_ensure_layout (row);
	/*
	pango_layout_get_pixel_size (priv->layout,
				     &width,
				     NULL);
		
	if (width > 0) {
		width += TEXT_PADDING;
	}

	priv->text_width = width;
	*/

	if (priv->assignment!=NULL) {
		get_assignment_bounds(priv->assignment,priv->scale,&x_debut,&x_fin,&x_debut_reel);
	}
	if (priv->resource!=NULL) {
		get_resource_bounds(priv->resource,priv->scale,&x_debut,&x_fin,&x_debut_reel);
		/*
		GList *assigns,*a;
		assigns = mrp_resource_get_assignments(priv->resource);
		for (a=assigns; a; a=a->next) {
			gdouble		 loc_debut;
			gdouble		 loc_fin;
			gdouble		 loc_debut_reel;
			MrpAssignment	*assign;
			assign = MRP_ASSIGNMENT(a->data);
			get_assignment_bounds(assign,
					      priv->scale,
					      &loc_debut,
					      &loc_fin,
					      &loc_debut_reel);
			if (loc_debut<x_debut || x_debut==-1.0) {
				x_debut = loc_debut;
			}
			if (loc_fin>x_fin || x_fin==-1.0) {
				x_fin = loc_fin;
			}
			if (loc_debut_reel<x_debut_reel || x_debut_reel) {
				x_debut_reel = loc_debut_reel;
			}
		}
		*/
	}
	priv->x = x_debut;
	priv->width = x_fin - x_debut;
	priv->x_start = x_debut_reel;
}


static void
ttable_row_set_property (GObject      *object,
			 guint         param_id,
			 const GValue *value,
			 GParamSpec   *pspec)
{
	GnomeCanvasItem *item;
	PlannerTtableRow      *row;
	PlannerTtableRowPriv  *priv;
	gboolean         changed = FALSE;
	gfloat           tmp_scale;
	gdouble          tmp_dbl;
	MrpTask		*task;
	
	g_return_if_fail (PLANNER_IS_TTABLE_ROW (object));

	item = GNOME_CANVAS_ITEM (object);
	row  = PLANNER_TTABLE_ROW (object);
	priv = row->priv;
	
	switch (param_id) {
	case PROP_SCALE:
		tmp_scale = g_value_get_double (value);
		if (tmp_scale != priv->scale) {
			row->priv->scale = tmp_scale;
			changed = TRUE;
		}
		break;

	case PROP_ZOOM:
		priv->zoom = g_value_get_double (value);
		break;

	case PROP_Y:
		tmp_dbl = g_value_get_double (value);
		if (tmp_dbl != priv->y) {
			priv->y = tmp_dbl;
			changed = TRUE;
		}
		break;

	case PROP_HEIGHT:
		tmp_dbl = g_value_get_double (value);
		if (tmp_dbl != priv->height) {
			priv->height = tmp_dbl;
			changed = TRUE;
		}
		break;

	case PROP_RESOURCE:
		g_return_if_fail (priv->assignment==NULL || g_value_get_object(value)==NULL);
		if (priv->resource != NULL) {
		        /* FIXME: check handlers to modify !!! */
			g_object_unref (priv->resource);
		}
		if (g_value_get_object(value)==NULL) {
			priv->resource=NULL;
		} else {
			GList *a;
			priv->resource = g_object_ref(g_value_get_object(value));
			g_signal_connect_object (priv->resource,
						 "notify",
						 G_CALLBACK (ttable_row_resource_notify_cb),
						 row,
						 0);
			g_signal_connect_object (priv->resource,
						 "assignment_added",
						 G_CALLBACK (ttable_row_resource_assignment_added_cb),
						 row,
						 0);
			a = mrp_resource_get_assignments(priv->resource);
			for (; a; a=a->next) {
				MrpAssignment	*assign;
				MrpTask		*task;
				assign = MRP_ASSIGNMENT(a->data);
				task = mrp_assignment_get_task(assign);
				g_signal_connect_object (assign,
							 "notify",
							 G_CALLBACK (ttable_row_assignment_notify_cb),
							 row,
							 0);
		
				g_signal_connect_object (task,
							 "notify",
							 G_CALLBACK (ttable_row_task_notify_cb),
							 row,
							 0);
			}
		}
		/*		
		g_signal_connect_object (priv->resource,
					 "assignment-added",
					 G_CALLBACK (ttable_row_res_assignment_added),
					 row,
					 0);

		g_signal_connect_object (priv->resource,
					 "assignment-removed",
					 G_CALLBACK (ttable_row_res_assignment_removed),
					 row,
					 0);
		*/
		changed=TRUE;
		break;
		
		
	case PROP_ASSIGNMENT:
		g_return_if_fail(priv->resource == NULL || g_value_get_object(value)==NULL);
		if (priv->assignment != NULL) {
		        /* gantt_row_disconnect_all_resources (priv->task, row); */
			g_object_unref (priv->assignment);
			/* FIXME: Disconnect notify handlers. */
		}
		if (g_value_get_object(value)==NULL) {
			priv->assignment = NULL;
		} else {
			MrpTaskSched	sched;
			priv->assignment = g_object_ref (g_value_get_object (value));
			task = mrp_assignment_get_task(priv->assignment);
			g_object_get (G_OBJECT(task),"sched",&sched,NULL);
			if (sched == MRP_TASK_SCHED_FIXED_DURATION) {
				priv->fixed_duration=1;
			} else {
				priv->fixed_duration=0;
			}

			g_signal_connect_object (priv->assignment,
						 "notify",
						 G_CALLBACK (ttable_row_assignment_notify_cb),
						 row,
						 0);
		
			g_signal_connect_object (task,
						 "notify",
						 G_CALLBACK (ttable_row_task_notify_cb),
						 row,
						 0);
		}
		
		/* ttable_row_connect_all_resources (priv->assignment, row); */

		changed = TRUE;
		break;

		/*
	case PROP_HIGHLIGHT:
		tmp_bool = g_value_get_boolean (value);
		if (tmp_bool != priv->highlight) {
			priv->highlight = tmp_bool;
			changed = TRUE;
		}
		break;

	case PROP_MOUSE_OVER_INDEX:
		tmp_int = g_value_get_int (value);
		if (tmp_int != priv->mouse_over_index) {
			priv->mouse_over_index = tmp_int;
			changed = TRUE;
		}
		break;
		*/

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}

	if (changed) {
		recalc_bounds (row);
		ttable_row_geometry_changed (row);
		gnome_canvas_item_request_update (item);
	}
}

static void
ttable_row_get_property (GObject    *object,
			 guint       param_id,
			 GValue     *value,
			 GParamSpec *pspec)
{
	PlannerTtableRow     *row;
	PlannerTtableRowPriv *priv;

	g_return_if_fail (PLANNER_IS_TTABLE_ROW (object));

	row = PLANNER_TTABLE_ROW (object);
	priv = row->priv;
	
	switch (param_id) {
	case PROP_SCALE:
		g_value_set_double (value, priv->scale);
		break;

	case PROP_ZOOM:
		g_value_set_double (value, priv->zoom);
		break;

	case PROP_Y:
		g_value_set_double (value, priv->y);
		break;

	case PROP_HEIGHT:
		g_value_set_double (value, priv->height);
		break;

	case PROP_RESOURCE:
		g_value_set_object (value, priv->resource);
		break;
		
	case PROP_ASSIGNMENT:
		g_value_set_object (value, priv->assignment);
		break;

		/*
	case PROP_HIGHLIGHT:
		g_value_set_boolean (value, priv->highlight);
		break;
		*/

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static void
ttable_row_ensure_layout (PlannerTtableRow *row)
{
	if (row->priv->layout == NULL) {
		row->priv->layout = gtk_widget_create_pango_layout (
			GTK_WIDGET (GNOME_CANVAS_ITEM (row)->canvas), NULL);

		/* ttable_row_update_resources (row); */
	}
}

/*
static void
gantt_row_update_resources (PlannerGanttRow *row)
{
	PlannerGanttRowPriv *priv;
	GList          *l;
	GList          *assignments;
	MrpAssignment  *assignment;
	MrpResource    *resource;
	gchar          *name, *name_unit;
	gchar          *tmp_str;
	gchar          *text = NULL;
	PangoRectangle  rect;
	gint            spacing, x;
	gint            units;

	priv = row->priv;
	
	g_array_set_size (priv->resource_widths, 0);

	// Measure the spacing between resource names. 
	pango_layout_set_text (priv->layout, ", ", 2);
	pango_layout_get_extents (priv->layout, NULL, &rect);
	spacing = rect.width / PANGO_SCALE;
	
	x = 0;
	assignments = mrp_task_get_assignments (priv->task);
		
	for (l = assignments; l; l = l->next) {
		assignment = l->data;

		resource = mrp_assignment_get_resource (assignment);
		units = mrp_assignment_get_units (assignment);
		
		g_object_get (resource, "name", &name, NULL);

		if (name && name[0] == 0) {
			g_free (name);
			name = NULL;
		}
		
		g_array_append_val (priv->resource_widths, x);

		if (units != 100) {
			name_unit = g_strdup_printf ("%s [%i]", name ? name : _("Unnamed"), units);
		} else {
			name_unit = g_strdup_printf ("%s", name ? name : _("Unnamed"));
		}

		g_free (name);

		pango_layout_set_text (priv->layout, name_unit, -1);
		pango_layout_get_extents (priv->layout, NULL, &rect);
		x += rect.width / PANGO_SCALE;
		g_array_append_val (priv->resource_widths, x);

		x += spacing;
			
		if (text == NULL) { // First resource 
			text = g_strdup_printf ("%s", name_unit);
			g_free (name_unit);
			continue;
		}
		
		tmp_str = g_strdup_printf ("%s, %s", text, name_unit);
		
		g_free (text);
		g_free (name_unit);
		text = tmp_str;
	}

	if (assignments) {
		g_list_free (assignments);
	}
	
	if (text == NULL) {
		pango_layout_set_text (priv->layout, "", 0);
	} else {
		pango_layout_set_text (priv->layout, text, -1);
	}		

	g_free (text);
}
*/

static void
ttable_row_update (GnomeCanvasItem *item,
		   double          *affine,
		   ArtSVP          *clip_path,
		   gint             flags)
{
	PlannerTtableRow *row;
	double      x1, y1, x2, y2;

	row = PLANNER_TTABLE_ROW (item);

	GNOME_CANVAS_ITEM_CLASS (parent_class)->update (item,
							affine,
							clip_path,
							flags);

	ttable_row_ensure_layout (row);
	ttable_row_get_bounds (row, &x1, &y1, &x2, &y2);

	gnome_canvas_update_bbox (item, x1, y1, x2, y2);
}

static void
ttable_row_realize (GnomeCanvasItem *item)
{
	PlannerTtableRow     *row;
	PlannerTtableRowPriv *priv;

	row = PLANNER_TTABLE_ROW (item);
	priv = row->priv;
	
	GNOME_CANVAS_ITEM_CLASS (parent_class)->realize (item);
	
	if (complete_stipple == NULL) {
		complete_stipple = gdk_bitmap_create_from_data (
			NULL,
			complete_stipple_pattern,
			2,
			2);
		
		g_object_add_weak_pointer (G_OBJECT (complete_stipple),
					   (gpointer) &complete_stipple);
	} else {
		g_object_ref (complete_stipple);
	}
	
	if (break_stipple == NULL) {
		break_stipple = gdk_bitmap_create_from_data (
			NULL,
			break_stipple_pattern,
			6,
			1);
		
		g_object_add_weak_pointer (G_OBJECT (break_stipple),
					   (gpointer) &break_stipple);
	} else {
		g_object_ref (break_stipple);
	}
	
	priv->complete_gc = gdk_gc_new (item->canvas->layout.bin_window);
	gdk_gc_set_stipple (priv->complete_gc, complete_stipple);
	gdk_gc_set_fill (priv->complete_gc, GDK_STIPPLED);

	priv->break_gc = gdk_gc_new (item->canvas->layout.bin_window);
	gdk_gc_set_stipple (priv->break_gc, break_stipple);
	gdk_gc_set_fill (priv->break_gc, GDK_STIPPLED);

	priv->fill_gc = gdk_gc_new (item->canvas->layout.bin_window);
}

static void
ttable_row_unrealize (GnomeCanvasItem *item)
{
	PlannerTtableRow *row;

	row = PLANNER_TTABLE_ROW (item);

	gdk_gc_unref (row->priv->complete_gc);
	row->priv->complete_gc = NULL;

	gdk_gc_unref (row->priv->break_gc);
	row->priv->break_gc = NULL;

	gdk_gc_unref (row->priv->fill_gc);
	row->priv->fill_gc = NULL;

	if (break_stipple) {
		g_object_unref (break_stipple);
	}

	if (complete_stipple) {
		g_object_unref (complete_stipple);
	}

	GNOME_CANVAS_ITEM_CLASS (parent_class)->unrealize (item);
}

static GdkGC *
ttable_row_create_frame_gc (GnomeCanvas *canvas)
{
	GdkGC    *gc;
 
	gc = gdk_gc_new (canvas->layout.bin_window);

	gdk_gc_set_line_attributes (gc,
				    0,
				    GDK_LINE_SOLID,
				    GDK_CAP_BUTT,
				    GDK_JOIN_MITER);

	return gc;
}

typedef enum {
	START_ASSIGN,
	END_ASSIGN
} date_type;

typedef struct {
	date_type	 type;
	mrptime		 time;
	gint		 units;
	MrpAssignment	*assignment;
	MrpTask		*task;
} Date;

static gint
ttable_row_date_compare	(gconstpointer	date1,
			 gconstpointer	date2)
{
	const Date	*a, *b;
	a=date1;
	b=date2;
	if (a->time < b->time) {
		return -1;
	} else if (a->time == b->time) {
		if (a->type < b->type) {
			return -1;
		} else if (a->type == b->type) {
			return 0;
		} else {
			return 1;
		}
	} else {
		return 1;
	}
}

typedef enum {
	ROW_START,
	ROW_MIDDLE,
	ROW_END
} row_chunk;

static void
ttable_row_draw_resource_ival	(mrptime		 start,
				 mrptime		 end,
				 gint			 units,
				 row_chunk		 chunk,
				 GdkDrawable		*drawable,
				 GnomeCanvasItem	*item,
				 gint			 x,
				 gint			 y,
				 gint			 width,
				 gint			 height)
{
	PlannerTtableRow	*row;
	PlannerTtableRowPriv	*priv;
	GdkColor	 color_normal;
	GdkColor	 color_free;
	GdkColor	 color_underuse;
	GdkColor	 color_overuse;
	GdkColor	 color_high;
	GdkColor	 color_shadow;
	GdkGC		*frame_gc;

	/* World coord */
	gdouble		 xoffset, yoffset;
	gdouble		 xstart, ystart, xend, yend;

	/* Canvas coord */
	/*
	 c_XXX: global
	 cr_XXX: central rectangle
	 cs_XXX: shadow
	*/
	gint		 c_xstart, c_ystart, c_xend, c_yend;
	gint		 cr_xstart, cr_ystart, cr_xend, cr_yend;
	gint		 cs_xstart, cs_ystart, cs_xend, cs_yend;
	/* Real coord (in the exposed area) */
	gint		 r_xstart, r_ystart, r_xend, r_yend;
	gint		 rr_xstart, rr_ystart, rr_xend, rr_yend;
	gint		 rs_xstart, rs_ystart, rs_xend, rs_yend;
	
	gnome_canvas_get_color (item->canvas,
				"LightSkyBlue3",
				&color_normal);
	gnome_canvas_get_color (item->canvas,
				"indian red",
				&color_overuse);
	gnome_canvas_get_color (item->canvas,
				"light sea green",
				&color_underuse);
	gnome_canvas_get_color (item->canvas,
				"medium sea green",
				&color_free);
	gnome_canvas_get_color (item->canvas,
				"gray75",
				&color_high);
	gnome_canvas_get_color (item->canvas,
				"gray40",
				&color_shadow);

	row = PLANNER_TTABLE_ROW(item);
	priv = row->priv;

	/* Graphical Context */
	frame_gc = ttable_row_create_frame_gc (item->canvas);

	/* Compute offset in world coord */
	xoffset = 0.0;
	yoffset = 0.0;
	gnome_canvas_item_i2w(item, &xoffset, &yoffset);
	
	/* world coordinates */
	xstart = start * priv->scale;
	ystart = priv->y + 0.15*priv->height;
	xend   = end *priv->scale;
	yend   = priv->y + 0.70*priv->height;

	/* Convert to canvas */
	gnome_canvas_w2c (item->canvas,
			  xstart + xoffset,
			  ystart + yoffset,
			  &c_xstart,
			  &c_ystart);
	gnome_canvas_w2c (item->canvas,
			  xend + xoffset,
			  yend + yoffset,
			  &c_xend,
			  &c_yend);
	
	/* Shift by (x,y) */
	c_xstart -= x;
	c_xend -= x;
	c_ystart -= y;
	c_yend -= y;
	/*
	c_xstart -= 2;
	c_xend += 2;
	*/

	/* Compute shadow coord: */
	cs_xstart = c_xstart+1;
	cs_ystart = c_ystart+1;
	cs_xend   = c_xend  -1;
	cs_yend   = c_yend  -1;

	/* Compute rectangle coord: */
	cr_xstart = c_xstart;
	cr_ystart = cs_ystart+1;
	cr_xend   = c_xend  ;
	cr_yend   = cs_yend  -1;

	/* Clip to the expose area */
	r_xstart = MAX(c_xstart,0);
	r_xend   = MIN(c_xend  ,width);
	r_ystart = MAX(c_ystart,0);
	r_yend   = MIN(c_yend  ,height);

	rs_xstart = MAX(cs_xstart,0);
	rs_xend   = MIN(cs_xend  ,width);
	rs_ystart = MAX(cs_ystart,0);
	rs_yend   = MIN(cs_yend  ,height);

	rr_xstart = MAX(cr_xstart,0);
	rr_xend   = MIN(cr_xend  ,width);
	rr_ystart = MAX(cr_ystart,0);
	rr_yend   = MIN(cr_yend  ,height);


	if (r_xend <= r_xstart || r_yend <= r_ystart) {
	        /* Nothing to draw */
		return;
	}
	
	if (units == 0) {
		gdk_gc_set_foreground (priv->fill_gc, &color_free);
	} else if (units<100) {
		gdk_gc_set_foreground (priv->fill_gc, &color_underuse);
	} else if (units == 100) {
		gdk_gc_set_foreground (priv->fill_gc, &color_normal);
	} else {
		gdk_gc_set_foreground (priv->fill_gc, &color_overuse);
	}
	
	/* Draw the central part of the chunk */
	gdk_draw_rectangle (drawable,
			    priv->fill_gc,
			    TRUE,
			    rr_xstart, rr_ystart,
			    rr_xend - rr_xstart +1,
			    rr_yend - rr_ystart +1);

	/* Draw the shadow */
	gdk_gc_set_foreground (priv->fill_gc, &color_high);
	/* Draw the top of the shadow, if it can be seen */
	if (cs_ystart == rs_ystart) {
		gdk_draw_line (drawable, priv->fill_gc, r_xstart, rs_ystart, r_xend, rs_ystart);
	}
	/* Draw the left of the shadow, if it exists and can be seen */
	if (chunk == ROW_START && cs_xstart == rs_xstart) {
		gdk_draw_line (drawable, priv->fill_gc, rs_xstart, rs_ystart, rs_xstart, cs_yend);
	}

	gdk_gc_set_foreground (priv->fill_gc, &color_shadow);
	/* Draw the bottom of the shadow, if it can be seen */
	if (cs_yend == rs_yend) {
		gdk_draw_line (drawable, priv->fill_gc, r_xstart, rs_yend, r_xend, rs_yend);
	}
	/* Draw the right of the shadow, if it exists and can be seen */
	if (chunk == ROW_END && cs_xend == rs_xend) {
		gdk_draw_line (drawable, priv->fill_gc, rs_xend, rs_ystart, rs_xend, cs_yend);
	}
	
	/* Draw the top line, if it can be seen */
	if (c_ystart == r_ystart)
		gdk_draw_line (drawable, frame_gc, r_xstart, r_ystart, r_xend, r_ystart);
	/* Draw the bottom line, if it can be seen */
	if (c_yend == r_yend)
		gdk_draw_line (drawable, frame_gc, r_xstart, r_yend, r_xend, r_yend);
	/* Draw the left line, if it exists and can be seen */
	if (chunk == ROW_START && c_xstart == r_xstart)
		gdk_draw_line (drawable, frame_gc, r_xstart, r_ystart, r_xstart, r_yend);
	/* Draw the right line, if it exists and can be seen */
	if (chunk == ROW_END && c_xend == r_xend)
		gdk_draw_line (drawable, frame_gc, r_xend, r_ystart, r_xend, r_yend);

	g_object_unref(frame_gc);

	/*	
	gchar *couleur;
	if (units == 0) {
		couleur = "vert";
	} else if (units<100) {
		couleur = "bleu clair";
	} else if (units==100) {
		couleur = "bleu";
	} else {
		couleur = "rouge";
	}
	*/
}

static void
ttable_row_draw_resource	(PlannerTtableRow		*row,
				 GdkDrawable		*drawable,
				 GnomeCanvasItem	*item,
				 gint			 x,
				 gint			 y,
				 gint			 width,
				 gint			 height)
{
	GList		*dates;
	MrpResource	*resource;
	MrpTask		*root;
	MrpAssignment	*assignment;
	MrpTask		*task;
	MrpProject	*project;
	GList		*assignments;
	GList		*a, *d;
	Date		*date, *date0, *date1;
	mrptime		 work_start, finish, previous_time;
	gint		 units;
	row_chunk	 chunk;

	resource = row->priv->resource;
#ifdef VERBOSE
	g_object_get(G_OBJECT(resource),"name",&name,NULL);
	g_message("Timetable: Resource %s\n",name);
#endif

	dates = NULL;
	g_object_get(G_OBJECT(resource),"project",&project,NULL);
	root = mrp_project_get_root_task(project);
	assignments = mrp_resource_get_assignments(resource);

	for (a=assignments; a; a=a->next) {
		assignment = MRP_ASSIGNMENT(a->data);
		task = mrp_assignment_get_task(assignment);
		work_start = mrp_task_get_work_start(task);
		finish = mrp_task_get_finish(task);
		units = mrp_assignment_get_units(assignment);
		date0 = g_new0(Date,1);
		date0->type = START_ASSIGN;
		date0->time = work_start;
		date0->units = units;
		date0->assignment = assignment;
		date0->task = task;
		date1 = g_new0(Date,1);
		date1->type = END_ASSIGN;
		date1->time = finish;
		date1->units = units;
		date1->assignment = assignment;
		date1->task = task;
		dates = g_list_insert_sorted(dates,date0,ttable_row_date_compare);
		dates = g_list_insert_sorted(dates,date1,ttable_row_date_compare);
	}

	units = 0;
	previous_time = mrp_project_get_project_start(project);
	finish = mrp_task_get_finish(root);
	chunk = ROW_START;
#ifdef VERBOSE
	g_message("Timetable: initial load of %d units\n",units);
#endif
	for (d=dates; d; d = d->next) {
		date = d->data;
		if (date->time != previous_time) {
			if (date->time == finish)
				chunk = ROW_END;
			ttable_row_draw_resource_ival(
					previous_time,
					date->time,
					units,
					chunk,
					drawable, item,
					x, y, width, height
					);
			if (chunk == ROW_START)
				chunk = ROW_MIDDLE;
			previous_time = date->time;
		}
#ifdef VERBOSE
		g_object_get(G_OBJECT(date->task),"name",&name,NULL);
#endif
		if (date->type == START_ASSIGN) {
			units += date->units;
#ifdef VERBOSE
			   g_message ("Timetable: starting task (%s) and now using %d units",name,units);
			   mrp_time_debug_print(date->time);
#endif
		} else {
			units -= date->units;
#ifdef VERBOSE
			   g_message ("Timetable: ending task (%s) and now using %d units",name,units);
			   mrp_time_debug_print(date->time);
#endif
		}
#ifdef VERBOSE
		g_free(name);
#endif
		g_free(date);
	}
	g_list_free(dates);
#ifdef VERBOSE
	g_message("Timetable: final load of %d units",units);
#endif
	if (chunk != ROW_END) {
		chunk = ROW_END;
		ttable_row_draw_resource_ival(
				previous_time,
				finish,
				units,
				chunk,
				drawable, item,
				x, y, width, height);
	}
}

static void
ttable_row_draw_assignment	(PlannerTtableRow		*row,
				 MrpAssignment		*assign,
				 GnomeCanvasItem	*item,
		 		 GdkDrawable     	*drawable,
				 gint             	x,
				 gint             	y,
				 gint             	width,
				 gint             	height)
{
	PlannerTtableRowPriv	*priv;
	MrpTask 	*task;
	GdkGC           *frame_gc;
	gdouble          i2w_dx; 
	gdouble          i2w_dy;
	gdouble          dx1, dy1, dx2, dy2;
	gboolean         summary;
	gint             percent_complete;
	gint             complete_x2, complete_width;
	gint             rx1, ry1;
	gint             rx2, ry2;
	gint             cx1, cy1, cx2, cy2; 
	GdkColor         color;
	GdkColor         color_high, color_shadow;
	gdouble		 ass_x, ass_xend, ass_x_start;

	priv = row->priv;
	task = mrp_assignment_get_task(assign);
	/* Get item area in canvas coordinates. */
	i2w_dx = 0.0;
	i2w_dy = 0.0;
	gnome_canvas_item_i2w (item, &i2w_dx, &i2w_dy);

	get_assignment_bounds(assign,priv->scale,&ass_x,&ass_xend,&ass_x_start);
	dx1 = ass_x;
	dy1 = priv->y + 0.15 * priv->height;
	dx2 = ass_xend;
	dy2 = priv->y + 0.70 * priv->height;
	
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

	if (cy1 >= cy2 || cx1 >= cx2) {
		return;
	}

	/* summary_y = floor (priv->y + 2 * 0.15 * priv->height + 0.5) - y; */

	/* "Clip" the expose area. */
	rx1 = MAX (cx1, 0);
	rx2 = MIN (cx2, width);

	ry1 = MAX (cy1, 0);
	ry2 = MIN (cy2, height);

	summary = (mrp_task_get_n_children (task) > 0);
	complete_width = 0;
	complete_x2 = 0;

	if (!summary) {
		g_object_get (task,
			      "percent-complete", &percent_complete,
			      /* "critical", &critical, */
			      NULL);
		complete_width = floor ((cx2 - cx1) * (percent_complete / 100.0) + 0.5);
		complete_x2 = MIN (cx1 + complete_width, rx2);
	}

	frame_gc = ttable_row_create_frame_gc (item->canvas);

	if (rx1 <= rx2) {
		if (complete_width > 0) {
			gnome_canvas_set_stipple_origin (item->canvas,
							 priv->complete_gc);
		}

		gnome_canvas_get_color (item->canvas,
					"LightSkyBlue3",
					&color);
		
		/* FIXME: Color shades should be calculated instead of using
		 * hardcoded colors.
		 */
		gnome_canvas_get_color (item->canvas,
					"gray75",
					&color_high);
		gnome_canvas_get_color (item->canvas,
					"gray40",
					&color_shadow);

		gdk_gc_set_foreground (priv->fill_gc, &color);
		
		gdk_draw_rectangle (drawable,
				    priv->fill_gc,
				    TRUE,
				    rx1,
				    cy1 + 1,
				    rx2 - rx1,
				    cy2 - cy1 - 1);
		
		if (rx1 <= complete_x2) {
			gdk_draw_rectangle (drawable,
					    priv->complete_gc,
					    TRUE,
					    rx1,
					    cy1 + 4,
					    complete_x2 - rx1,
					    cy2 - cy1 - 7);
		}

		gdk_draw_line (drawable, frame_gc, rx1, cy1, rx2, cy1);
		gdk_draw_line (drawable, frame_gc, rx1, cy2, rx2, cy2);

		gdk_gc_set_foreground (priv->fill_gc, &color_high);
		gdk_draw_line (drawable,
			       priv->fill_gc,
			       rx1 + 0,
			       cy1 + 1,
			       rx2 - 0,
			       cy1 + 1);

		if (cx1 == rx1) {
			gdk_draw_line (drawable,
				       priv->fill_gc,
				       rx1 + 1,
				       cy1 + 1,
				       rx1 + 1,
				       cy2 - 1);
		}
		
		gdk_gc_set_foreground (priv->fill_gc, &color_shadow);
		gdk_draw_line (drawable,
			       priv->fill_gc,
			       rx1 + 0,
			       cy2 - 1,
			       rx2 - 0,
			       cy2 - 1);
		
		if (cx2 == rx2) {
			gdk_draw_line (drawable,
				       priv->fill_gc,
				       rx2 - 1,
				       cy1 + 1,
				       rx2 - 1,
				       cy2 - 1);
		}
		
		if (cx1 == rx1) {
			gdk_draw_line (drawable,
				       frame_gc,
				       rx1,
				       cy1,
				       rx1,
				       cy2);
		}
		
		if (cx2 == rx2) {
			gdk_draw_line (drawable,
				       frame_gc,
				       rx2,
				       cy1,
				       rx2,
				       cy2);
		}
	}
	g_object_unref (frame_gc);
}




static void
ttable_row_draw (GnomeCanvasItem *item,
		 GdkDrawable     *drawable,
		 gint             x,
		 gint             y,
		 gint             width,
		 gint             height)
{
	PlannerTtableRow     *row;
	PlannerTtableChart   *chart;

	row = PLANNER_TTABLE_ROW (item);
	chart = g_object_get_data (G_OBJECT (item->canvas), "chart");
	if (row->priv->assignment) {
		ttable_row_draw_assignment(row,
					   row->priv->assignment,
					   item,
					   drawable,
					   x,y,width,height
					   );
	} else if (row->priv->resource) {
		/*
		GList	*assigns,*a;
	        gchar	*name=NULL;
		assigns = mrp_resource_get_assignments(row->priv->resource);
		for (a=assigns; a; a=a->next) {
			MrpAssignment	*assign;
			assign = MRP_ASSIGNMENT(a->data);
			ttable_row_draw_assignment(row,assign,item,drawable,x,y,width,height);
		}
		*/
		ttable_row_draw_resource(row,drawable,item,x,y,width,height);
	}
}

static double
ttable_row_point (GnomeCanvasItem  *item,
		  double            x,
		  double            y,
		  gint              cx,
		  gint              cy,
		  GnomeCanvasItem **actual_item)
{
	PlannerTtableRow     *row;
	PlannerTtableRowPriv *priv;
	gdouble               x1, y1, x2, y2;
	gdouble               dx, dy;
	
	row = PLANNER_TTABLE_ROW (item);
	priv = row->priv;
	
	*actual_item = item;

	/*
	text_width = priv->text_width;
	if (text_width > 0) {
		text_width += TEXT_PADDING;
	}
	*/

	x1 = priv->x;
	y1 = priv->y;
	x2 = x1 + priv->width /* + text_width */ ;
	y2 = y1 + priv->height;
	
	if (x > x1 && x < x2 && y > y1 && y < y2) {
		return 0.0;
	}

	/* Point is outside rectangle */
	if (x < x1) {
		dx = x1 - x;
	}
	else if (x > x2) {
		dx = x - x2;
	} else {
		dx = 0.0;
	}

	if (y < y1) {
		dy = y1 - y;
	}
	else if (y > y2) {
		dy = y - y2;
	} else {
		dy = 0.0;
	}
	
	return sqrt (dx * dx + dy * dy);
}

static void
ttable_row_bounds (GnomeCanvasItem *item,
		   double          *x1,
		   double          *y1,
		   double          *x2,
		   double          *y2)
{
	PlannerTtableRow *row;

	row = PLANNER_TTABLE_ROW (item);

	ttable_row_get_bounds (row, x1, y1, x2, y2);
	
	if (GNOME_CANVAS_ITEM_CLASS (parent_class)->bounds) {
		GNOME_CANVAS_ITEM_CLASS (parent_class)->bounds (item, x1, y1, x2, y2);
	}
}

static void
ttable_row_resource_notify_cb (MrpResource *resource, GParamSpec *pspec, PlannerTtableRow *row)
{
	recalc_bounds (row);
	ttable_row_geometry_changed (row); 
	gnome_canvas_item_request_update (GNOME_CANVAS_ITEM (row));
}

static void
ttable_row_resource_assignment_added_cb	(MrpResource	*resource,
					 MrpAssignment	*assign,
					 PlannerTtableRow	*row)
{
	MrpTask	*task;
	task = mrp_assignment_get_task(assign);
	g_signal_connect_object (assign,
				 "notify",
				 G_CALLBACK (ttable_row_assignment_notify_cb),
				 row,
				 0);

	g_signal_connect_object (task,
				 "notify",
				 G_CALLBACK (ttable_row_task_notify_cb),
				 row,
				 0);
	recalc_bounds (row);
	ttable_row_geometry_changed (row);
	gnome_canvas_item_request_update (GNOME_CANVAS_ITEM (row));
}

static void
ttable_row_assignment_notify_cb (MrpAssignment *assignment, GParamSpec *pspec, PlannerTtableRow *row)
{
	recalc_bounds (row);
	ttable_row_geometry_changed (row); 
	gnome_canvas_item_request_update (GNOME_CANVAS_ITEM (row));
}

static void
ttable_row_task_notify_cb (MrpTask *task, GParamSpec *pspec, PlannerTtableRow *row)
{
	MrpTaskSched	sched;
	g_object_get (G_OBJECT(task),"sched",&sched,NULL);
	if (sched == MRP_TASK_SCHED_FIXED_DURATION) {
		row->priv->fixed_duration=1;
	} else {
		row->priv->fixed_duration=0;
	}
	recalc_bounds (row);
	ttable_row_geometry_changed (row); 
	gnome_canvas_item_request_update (GNOME_CANVAS_ITEM (row));
}

/*
static void
gantt_row_update_assignment_string (PlannerGanttRow *row)
{
	gantt_row_update_resources (row);
	
	recalc_bounds (row);
	gnome_canvas_item_request_update (GNOME_CANVAS_ITEM (row));
}
*/

/*
static void 
gantt_row_res_assignment_added (MrpResource   *resource, 
			        MrpAssignment *assignment,
			        PlannerGanttRow    *row)
{
	MrpTask *task;
	
	task = mrp_assignment_get_task (assignment);
	
	g_signal_connect_object (resource, "notify::name",
				 G_CALLBACK (gantt_row_resource_name_changed),
				 row, 0);

	g_signal_connect_object (assignment, "notify::units",
				 G_CALLBACK (gantt_row_assignment_units_changed),
				 row, 0);
		
	gantt_row_update_assignment_string (row);
}

static void 
gantt_row_assignment_removed (MrpTask       *task, 
			      MrpAssignment *assignment,
			      PlannerGanttRow    *row)
{
	MrpResource *resource;

	resource = mrp_assignment_get_resource (assignment);

	g_signal_handlers_disconnect_by_func (resource, 
					      gantt_row_resource_name_changed,
					      row);
	
	g_signal_handlers_disconnect_by_func (assignment, 
					      gantt_row_assignment_units_changed,
					      row);
	
	gantt_row_update_assignment_string (row);
}
*/
/*
static void
gantt_row_resource_name_changed (MrpResource *resource,
				 GParamSpec  *pspec,
				 PlannerGanttRow  *row)
{
	gantt_row_update_assignment_string (row);
}
*/
/*
static void
gantt_row_assignment_units_changed (MrpAssignment *assignment,
				    GParamSpec    *pspec,
				    PlannerGanttRow    *row)
{
	gantt_row_update_assignment_string (row);
}
*/

/* Returns the geometry of the actual bars, not the bounding box, not including
 * the text labels.
 */
void
planner_ttable_row_get_geometry (PlannerTtableRow *row,
			    gdouble    *x1,
			    gdouble    *y1,
			    gdouble    *x2,
			    gdouble    *y2)
{
	PlannerTtableRowPriv *priv;
	
	g_return_if_fail (PLANNER_IS_TTABLE_ROW (row));

	priv = row->priv;
	
	/* FIXME: Need to do recalc here? */
	/*recalc_bounds (row); */
	
	if (x1) {
		*x1 = priv->x;
	}
	if (x2) {
		*x2 = priv->x + priv->width;
	}
	if (y1) {
		*y1 = priv->y + 0.15 * priv->height;
	}
	if (y2) {
		*y2 = priv->y + 0.70 * priv->height;
	}
}

void
planner_ttable_row_set_visible (PlannerTtableRow *row,
			   gboolean    is_visible)
{
	if (is_visible ==row->priv->visible) {
		return;
	}
	
	row->priv->visible = is_visible;

	if (is_visible) {
		gnome_canvas_item_show (GNOME_CANVAS_ITEM (row));
	} else {
		gnome_canvas_item_hide (GNOME_CANVAS_ITEM (row));
	}

	g_signal_emit (row,
		       signals[VISIBILITY_CHANGED],
		       0,
		       is_visible);
}
			   
static void
ttable_row_geometry_changed (PlannerTtableRow *row)
{
	gdouble x1, y1, x2, y2;

	x1 = row->priv->x;
	y1 = row->priv->y;
	x2 = x1 + row->priv->width;
	y2 = y1 + row->priv->height;
	
	g_signal_emit (row,
		       signals[GEOMETRY_CHANGED],
		       0,
		       x1, y1,
		       x2, y2);
}

/*
static void
gantt_row_connect_all_resources (MrpTask *task, PlannerGanttRow *row)
{
	GList       *resources, *node;
	MrpResource *resource;
	
	resources = mrp_task_get_assigned_resources (task);
	
	for (node = resources; node; node = node->next) {
		resource = MRP_RESOURCE (node->data);

		g_signal_connect_object (resource, "notify::name",
					 G_CALLBACK (gantt_row_resource_name_changed),
					 row, 0);
	}
}

static void
gantt_row_disconnect_all_resources (MrpTask *task, PlannerGanttRow *row)
{
	GList       *resources, *node;
	MrpResource *resource;
	
	resources = mrp_task_get_assigned_resources (task);
	
	for (node = resources; node; node = node->next) {
		resource = MRP_RESOURCE (node->data);
		
		g_signal_handlers_disconnect_by_func (resource,
						      gantt_row_resource_name_changed,
						      row);
	}
}
*/
/*
static gboolean
ttable_row_scroll_timeout_cb (PlannerTtableRow *row)
{
	GtkWidget *widget;
	gint       width, height;
	gint       x, y, dx = 0, dy = 0;

	widget = GTK_WIDGET (GNOME_CANVAS_ITEM (row)->canvas);
	
	// Get the current mouse position so that we can decide if the pointer
	// is inside the viewport.
	 
	gdk_window_get_pointer (widget->window, &x, &y, NULL);

	width = widget->allocation.width;
	height = widget->allocation.height;

	if (x < 0) {
		dx = x;
	} else if (x >= widget->allocation.width) {
		dx = x - widget->allocation.width + 1;
	} else {
		dx = 0;
	}

	if (y < 0) {
		dy = y;
	} else if (y >= widget->allocation.height) {
		dy = y - widget->allocation.height + 1;
	} else {
		dy = 0;
	}
	
	ttable_row_canvas_scroll (widget, dx, dy);
	
	return TRUE;
}
*/

#define IN_DRAG_DURATION_SPOT(x,y,right,top,height) \
	((abs(x - (right)) <= 3) && \
	(y > top + 0.15 * height) && (y < top + 0.70 * height))

#define IN_DRAG_MOVE_SPOT(x,y,right,top,height) \
	((x <= right) && \
	(y > top + 0.15 * height) && (y < top + 0.70 * height))


static gboolean
ttable_row_event (GnomeCanvasItem *item, GdkEvent *event)
{
	PlannerTtableRow              *row;
	PlannerTtableRowPriv          *priv;
	PlannerTtableChart            *chart;
	GtkWidget                *canvas_widget;
	GdkCursor                *cursor;
	static gdouble            x1, y1;
	gdouble                   wx1, wy1;
	gdouble                   wx2, wy2;
	static MrpTask           *task = NULL;
	static gchar		 *task_name = NULL;
	static GnomeCanvasItem   *drag_item = NULL;
	gchar                    *message;

	row = PLANNER_TTABLE_ROW (item);
	priv = row->priv;
	canvas_widget = GTK_WIDGET (item->canvas);
	
	/* summary = (mrp_task_get_n_children (priv->task) > 0); */

	switch (event->type) {
	case GDK_BUTTON_PRESS:
		switch (event->button.button) {
			/*
		case 3:
			if (IN_DRAG_RELATION_SPOT (event->button.x,
						   event->button.y,
						   priv->x + priv->width,
						   priv->y,
						   priv->height)) {
			}
			break;
			*/			
		case 1:
			return FALSE; /* FIXME: Release hook for readonly view */
			if (priv->state != STATE_NONE) {
				break;
			}

			if (priv->assignment &&
			    !priv->fixed_duration &&
			    IN_DRAG_DURATION_SPOT (event->button.x,
						   event->button.y, priv->x + priv->width,
						   priv->y, priv->height)) {
				guint rgba;
				
				priv->state = STATE_DRAG_DURATION;

				wx1 = priv->x;
				wy1 = priv->y + 0.15 * priv->height;
				wx2 = event->button.x;
				wy2 = priv->y + 0.70 * priv->height;

				gnome_canvas_item_i2w (item, &wx1, &wy1);
				gnome_canvas_item_i2w (item, &wx2, &wy2);
				task = mrp_assignment_get_task(priv->assignment);
				g_object_get(G_OBJECT(task),"name",&task_name,NULL);

				/*      red            green          blue          alpha */
				rgba = (0xb7 << 24) | (0xc3 << 16) | (0xc9 << 8) | (127 << 0);
				
				if (drag_item == NULL) {
					drag_item = gnome_canvas_item_new (gnome_canvas_root (item->canvas),
									   EEL_TYPE_CANVAS_RECT,
									   "x1", wx1,
									   "y1", wy1,
									   "x2", wx2,
									   "y2", wy2,
									   "fill_color_rgba", rgba,
									   "outline_color_rgba", 0,
									   "width_pixels", 1,
									   NULL);
					gnome_canvas_item_hide (drag_item);
				}
			} else if (priv->assignment &&
				   IN_DRAG_MOVE_SPOT (event->button.x,
						      event->button.y,
						      priv->x + priv->width,
						      priv->y,
						      priv->height)) {
				guint rgba;

				priv->state = STATE_DRAG_MOVE;
				
				wx1 = priv->x;
				wy1 = priv->y + 0.15 * priv->height;
				wx2 = event->button.x;
				wy2 = priv->y + 0.70 * priv->height;

				gnome_canvas_item_i2w (item, &wx1, &wy1);
				gnome_canvas_item_i2w (item, &wx2, &wy2);

				/*      red            green          blue          alpha */
				rgba = (0xb7 << 24) | (0xc3 << 16) | (0xc9 << 8) | (127 << 0);
				
				drag_item = gnome_canvas_item_new (gnome_canvas_root (item->canvas),
								   EEL_TYPE_CANVAS_RECT,
								   "x1", wx1, "y1", wy1,
								   "x2", wx2, "y2", wy2,
								   "fill_color_rgba", rgba,
								   "outline_color_rgba", 0,
								   "width_pixels", 1,
								   NULL);
				gnome_canvas_item_hide (drag_item);
				task = mrp_assignment_get_task(priv->assignment);
				g_object_get(G_OBJECT(task),"name",&task_name,NULL);

				/*
				  Start the autoscroll timeout.
				*/
				/*
				priv->scroll_timeout_id = gtk_timeout_add (
					50,
					(GSourceFunc) gantt_row_scroll_timeout_cb,
					row);
				*/
			} else {
				/*
				gint         res_index;
				GList       *resources;
				MrpResource *resource;

				res_index = gantt_row_get_resource_index_at (row, event->button.x);
				if (res_index != -1) {
					resources = mrp_task_get_assigned_resources (priv->task);

					resource = g_list_nth_data (resources, res_index);
					if (resource) {
						PlannerGanttChart *chart;

						chart = g_object_get_data (G_OBJECT (item->canvas),
									   "chart");

						planner_gantt_chart_resource_clicked (chart, resource);
					}

					g_list_free (resources);
					
					return TRUE;
				} else {
					return FALSE;
				}
				*/
				return FALSE;
			}
			
			gnome_canvas_item_grab (item,
						GDK_POINTER_MOTION_MASK |
						GDK_POINTER_MOTION_HINT_MASK |
						GDK_BUTTON_RELEASE_MASK,
						NULL,
						event->button.time);

			x1 = event->button.x;
			y1 = event->button.y;

			return TRUE;

		default:
			return FALSE;
		}
		break;

	case GDK_LEAVE_NOTIFY:
		/*
		  We get a leave notify when pressing button 1 over the
		  item. We don't want to reset the cursor when that happens.
		*/
		if ((priv->state == STATE_NONE) &&
		    !(event->crossing.state & GDK_BUTTON1_MASK)) {
			gdk_window_set_cursor (canvas_widget->window, NULL);
		}
		/*
		g_object_set (row,
			      "mouse-over-index",
			      -1,
			      NULL);
		*/		
		return TRUE;
	
	case GDK_MOTION_NOTIFY:
		return FALSE; /* FIXME: Release hook for readonly view */
		if (event->motion.is_hint) {
			gint x, y;

			gdk_window_get_pointer (event->motion.window, &x, &y, NULL);
			gnome_canvas_c2w (item->canvas, x, y, &event->motion.x, &event->motion.y);
		}

		if (priv->state == STATE_NONE && priv->assignment !=NULL) {
			if (!priv->fixed_duration &&
			    IN_DRAG_DURATION_SPOT (event->button.x,
						   event->button.y,
						   priv->x + priv->width,
						   priv->y,
						   priv->height)) {
				cursor = gdk_cursor_new (GDK_RIGHT_SIDE);
				gdk_window_set_cursor (canvas_widget->window, cursor);
				if (cursor) {
					gdk_cursor_unref (cursor);
				}
			} else if (IN_DRAG_MOVE_SPOT(event->button.x,event->button.y,priv->x+priv->width,
						priv->y,priv->height)) {
				cursor = gdk_cursor_new (GDK_FLEUR);
				gdk_window_set_cursor (canvas_widget->window, cursor);
				if (cursor) {
					gdk_cursor_unref (cursor);
				}
			} else {
				gdk_window_set_cursor (canvas_widget->window, NULL);
			}
			return TRUE;
		}
		else if (priv->state == STATE_DRAG_MOVE) {
			mrptime	new_start;
			/*
			target_item = gnome_canvas_get_item_at (item->canvas,
								event->motion.x,
								event->motion.y);
			*/

			gnome_canvas_item_raise_to_top (drag_item);
			gnome_canvas_item_show (drag_item);
			
			wx1 = priv->x + event->motion.x - x1;
			wx2 = wx1 + priv->width;
			gnome_canvas_item_set (drag_item,
					       "x1", wx1,
					       "x2", wx2,
					       NULL);
			chart = g_object_get_data (G_OBJECT (item->canvas),
						   "chart");
			new_start = wx1/priv->scale;
			message = g_strdup_printf(_("Make task '%s' start on %s"),
					task_name,
					planner_format_date(new_start)
					);
			planner_ttable_chart_status_updated (chart, message);
			g_free (message);
			/*
			if (target_item && target_item != item) {
				message = g_strdup_printf (_("Make task '%s' a predecessor of '%s'"),
							   task_name,
							   target_name);

				planner_gantt_chart_status_updated (chart, message);

				g_free (message);
				g_free (target_name);
				g_free (task_name);
			}
			*/
			/*
			if (target_item == NULL) {
				planner_gantt_chart_status_updated (chart, NULL);
			}
			
			old_target_item = target_item;
			*/
		}
		else if (priv->state == STATE_DRAG_DURATION) {
			gint            duration;
			gint            work;
			gint		start;
			MrpProject     *project;
			MrpCalendar    *calendar;
			gint            hours_per_day;

			g_object_get (task, "project", &project, NULL);
			calendar = mrp_project_get_calendar (project);
			
			hours_per_day = mrp_calendar_day_get_total_work (
				calendar, mrp_day_get_work ()) / (60*60);

			wx2 = event->motion.x;
			wy2 = priv->y + 0.70 * priv->height;
			
			gnome_canvas_item_i2w (item, &wx2, &wy2);

			gnome_canvas_item_set (drag_item,
					       "x2", wx2,
					       "y2", wy2,
					       NULL);
			
			gnome_canvas_item_raise_to_top (drag_item);
			gnome_canvas_item_show (drag_item);

			chart = g_object_get_data (G_OBJECT (item->canvas),
						   "chart");

			start = mrp_task_get_work_start(task);
			duration = MAX (0, (event->motion.x - priv->x) / priv->scale);

			/* Snap to quarters. */
			duration = floor (duration / SNAP + 0.5) * SNAP;

			work = mrp_project_calculate_task_work (
					project,
					task, -1,
					start + duration);

			message = g_strdup_printf (
				_("Change task '%s' work to %s, duration to %s (%d)"),
				task_name,
				planner_format_duration (work, hours_per_day),
				planner_format_duration (duration, 24),duration);
			planner_ttable_chart_status_updated (chart, message);
			g_free (message);
		}
			
		break;
		
	case GDK_BUTTON_RELEASE:
		if (event->button.button != 1) {
			return FALSE;
		}
		
		if (priv->state == STATE_NONE) {
			return TRUE;
		}
		else if (priv->state == STATE_DRAG_DURATION) {
			MrpProject	*project;
			MrpTask		*task;
			gint		 duration;
			gint		 work;

			task = mrp_assignment_get_task(priv->assignment);
			g_object_get (task, "project", &project, NULL);
			
			duration = MAX (0, (event->button.x - priv->x_start) / priv->scale);
			/* Snap to quarters. */
			duration = floor (duration / SNAP + 0.5) * SNAP;

			work = mrp_project_calculate_task_work (
					project,
					task, -1,
					mrp_task_get_work_start(task) + duration);
			
			g_object_set (task,
				      "work", work,
				      NULL);

			gtk_object_destroy (GTK_OBJECT (drag_item));
			drag_item = NULL;
			g_free(task_name);
			task_name = NULL;
			task = NULL;

			chart = g_object_get_data (G_OBJECT (item->canvas),
						   "chart");

			planner_ttable_chart_status_updated (chart, NULL);
		}
		else if (priv->state == STATE_DRAG_MOVE) {
			mrptime new_start;
			MrpConstraint *constraint;

			wx1 = priv->x + event->motion.x - x1;
			wx2 = wx1 + priv->width;
			
			g_object_get(task,"constraint",&constraint,NULL);
			new_start = wx1/priv->scale;
			constraint->time = new_start;
			if (constraint->type == MRP_CONSTRAINT_ASAP) {
				constraint->type = MRP_CONSTRAINT_MSO;
			}
			g_object_set(task,"constraint",constraint,NULL);
			g_free(constraint);
			
			gtk_object_destroy (GTK_OBJECT (drag_item));
			drag_item = NULL;
			g_free(task_name);
			task_name = NULL;
			task = NULL;
		}
		/*
		else if (priv->state == STATE_DRAG_LINK) {
			if (old_target_item) {
				g_object_set (old_target_item,
					      "highlight",
					      FALSE,
					      NULL);
				old_target_item = NULL;
			}

			if (priv->scroll_timeout_id) {
				g_source_remove (priv->scroll_timeout_id);
				priv->scroll_timeout_id = 0;
			}
			
			gtk_object_destroy (GTK_OBJECT (drag_item));
			drag_item = NULL;
			
			target_item = gnome_canvas_get_item_at (item->canvas,
								event->button.x,
								event->button.y);
			
			if (target_item && target_item != item) {
				GError *error = NULL;
				
				task = priv->task;
				target_task = PLANNER_GANTT_ROW (target_item)->priv->task;
				
				if (!mrp_task_add_predecessor (target_task,
							       task,
							       MRP_RELATION_FS,
							       0,
							       &error)) {
					GtkWidget *dialog;

					gnome_canvas_item_ungrab (item, event->button.time);
					
					dialog = gtk_message_dialog_new (NULL,
									 GTK_DIALOG_DESTROY_WITH_PARENT,
									 GTK_MESSAGE_ERROR,
									 GTK_BUTTONS_OK,
									 "%s", error->message);
					gtk_dialog_run (GTK_DIALOG (dialog));
					gtk_widget_destroy (dialog);
					
					g_error_free (error);
				}
			}
			
			chart = g_object_get_data (G_OBJECT (item->canvas),
						   "chart");
			
			planner_gantt_chart_status_updated (chart, NULL);
		}
		*/

		/* We're done, reset the cursor and state, ungrab pointer. */
		gdk_window_set_cursor (canvas_widget->window, NULL);
			
		gnome_canvas_item_ungrab (item, event->button.time);
			
		priv->state = STATE_NONE;

		return TRUE;

	default:
		break;
	}

	return FALSE;
}

/*
static void
eel_gtk_adjustment_set_value (GtkAdjustment *adjustment,
			      float          value)
{
	float upper_page_start, clamped_value;

	g_return_if_fail (GTK_IS_ADJUSTMENT (adjustment));
	
	upper_page_start = MAX (adjustment->upper - adjustment->page_size,
				adjustment->lower);
	clamped_value = CLAMP (value, adjustment->lower, upper_page_start);
	if (clamped_value != adjustment->value) {
		adjustment->value = clamped_value;
		gtk_adjustment_value_changed (adjustment);
	}
}
*/
/*
static gboolean
ttable_row_canvas_scroll (GtkWidget *widget,
			  gint       delta_x,
			  gint       delta_y)
{
	GtkAdjustment *hadj, *vadj;
	int old_h_value, old_v_value;

	hadj = gtk_layout_get_hadjustment (GTK_LAYOUT (widget));
	vadj = gtk_layout_get_vadjustment (GTK_LAYOUT (widget));

	// Store the old ajustment values so we can tell if we ended up actually
	// scrolling. We may not have in a case where the resulting value got
	// pinned to the adjustment min or max.
	//
	old_h_value = hadj->value;
	old_v_value = vadj->value;
	
	eel_gtk_adjustment_set_value (hadj, hadj->value + delta_x);
	eel_gtk_adjustment_set_value (vadj, vadj->value + delta_y);

	// return TRUE if we did scroll 
	return hadj->value != old_h_value || vadj->value != old_v_value;
}
*/
/*
static gint
gantt_row_get_resource_index_at (PlannerGanttRow *row,
				 gint        x)
{
	PlannerGanttRowPriv *priv;
	gint            i, len;
	gint            left, right;
	gint            offset;

	priv = row->priv;

	offset = priv->x + priv->width + TEXT_PADDING;
	x -= offset;
	
	len = priv->resource_widths->len;
	for (i = 0; i < len; i += 2) {
		left = g_array_index (priv->resource_widths, gint, i);
		right = g_array_index (priv->resource_widths, gint, i+1);

		if (x > left && x < right) {
			return i / 2;
		}
	}
	
	return -1;
}

static gboolean
gantt_row_get_resource_by_index (PlannerGanttRow *row,
				 gint        index,
				 gint       *x1,
				 gint       *x2)
{
	PlannerGanttRowPriv *priv;

	g_return_val_if_fail (index >= 0, FALSE);

	// We can't catch too high indices using the g_return_val_if_fal, since
	// the array might have changed under us.
	//

	priv = row->priv;

	index *= 2;
	
	if (index >= priv->resource_widths->len) {
		return FALSE;
	}
	
	if (x1) {
		*x1 = g_array_index (priv->resource_widths, gint, index);
	}
	if (x2) {
		*x2 = g_array_index (priv->resource_widths, gint, index + 1);
	}

	return TRUE;
}
*/
