/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2004-2005 Imendio AB
 * Copyright (C) 2001-2003 CodeFactory AB
 * Copyright (C) 2001-2003 Richard Hult <richard@imendio.com>
 * Copyright (C) 2001-2003 Mikael Hallendal <micke@imendio.com>
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
 * Free Software Foundation, Inc., 59 Temgle Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <config.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <libplanner/mrp-project.h>
#include <libplanner/mrp-resource.h>
#include <libplanner/mrp-task.h>
#include <libplanner/mrp-calendar.h>
#include <glib/gi18n.h>
#include <libgnomecanvas/gnome-canvas.h>
#include "planner-marshal.h"
#include "planner-format.h"
#include "planner-gantt-row.h"
#include "planner-gantt-chart.h"
#include "planner-canvas-line.h"
#include "eel-canvas-rect.h"
#include "dummy-canvas-item.h"
#include "planner-scale-utils.h"
#include "planner-task-tree.h"
#include "planner-task-popup.h"
#include "planner-task-cmd.h"

/* The padding between the gantt bar and the text. */
#define TEXT_PADDING 10.0

/* Snap to this amount of time when dragging duration. */
#define SNAP (60.0*15.0)

/* Constants for the summary bracket. */
#define HEIGHT 5
#define THICKNESS 2
#define SLOPE 4

#define MILESTONE_SIZE 5

#define FUZZ  3

/* Minimum width for a task to keep it visible. */
#define MIN_WIDTH 2

/* Trim the value to short int work space. */
#define TRSH(a) ((int)((a) < SHRT_MIN ? (short int)SHRT_MIN : ((a) > SHRT_MAX ? (short int)SHRT_MAX : (a))))

/* Same gdk_draw_rectangle but with trimmed x,y,w,h args. */
#define draw_cut_rectangle(a,b,c,d,e,f,g)		\
		gdk_draw_rectangle ((a), (b), (c), TRSH (d), TRSH (e), TRSH (f), TRSH (g))

/* Same gdk_draw_line but with trimmed x1,y1,x2,y2 args. */
#define draw_cut_line(a,b,c,d,e,f)		 \
		gdk_draw_line ((a), (b), TRSH (c), TRSH (d), TRSH (e), TRSH (f))

/* Same gdk_draw_layout but with trimmed x,y args. */
#define draw_cut_layout(a,b,c,d,e)		         \
		gdk_draw_layout ((a), (b), TRSH (c), TRSH (d), (e))

enum {
	PROP_0,
	PROP_X,
	PROP_Y,
	PROP_WIDTH,
	PROP_HEIGHT,
	PROP_SCALE,
	PROP_ZOOM,
	PROP_TASK,
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
	STATE_NONE          = 0,
	STATE_DRAG_LINK     = 1 << 0,
	STATE_DRAG_DURATION = 1 << 1,
	STATE_DRAG_COMPLETE = 1 << 2,

	STATE_DRAG_ANY = STATE_DRAG_LINK | STATE_DRAG_DURATION | STATE_DRAG_COMPLETE
} State;

typedef enum
{
	DRAG_NONE_SPOT,
	DRAG_DURATION_SPOT,
	DRAG_COMPLETE_SPOT,
	DRAG_RELATION_SPOT
} DragSpot;

struct _PlannerGanttRowPriv {
	/* FIXME: Don't need those per gantt row. */
	GdkGC       *complete_gc;
	GdkGC       *break_gc;
	GdkGC       *fill_gc;
	GdkGC       *frame_gc;
	GdkGC       *ttask_gc;


	/* FIXME: Don't need those per gantt row. */
	GdkColor     color_normal;
	GdkColor     color_normal_light;
	GdkColor     color_normal_dark;

	GdkColor     color_critical;
	GdkColor     color_critical_light;
	GdkColor     color_critical_dark;

	/* FIXME: Don't need this per gantt row? */
	PangoLayout *layout;

	MrpTask     *task;

	State        state;

	guint        visible    : 1;
	guint        highlight  : 1;

	gdouble      scale;
	gdouble      zoom;

	gdouble      x;
	gdouble      y;

	/* The "imaginary" start, i.e. the start of the task even if it's in the
	 * middle of nonworking time. We need it to calculate the duration when
	 * dragging the length of a task.
	 */
	gdouble      x_start;

	gint         mouse_over_index;

	guint        scroll_timeout_id;

	/* Cached values for the geometry of the bar. */
	gdouble      width;
	gdouble      height;
	gdouble      bar_top; /* Top y position of the bar. */
	gdouble      bar_bot; /* Bottom y position of the bar. */
	gdouble      text_width;
	gdouble      text_height;

	/* Cached positions of each assigned resource. */
	GArray      *resource_widths;

	/* FIXME: Don't need this per row. */
	GtkItemFactory *popup_factory;
};

static void     gantt_row_class_init                  (PlannerGanttRowClass  *class);
static void     gantt_row_init                        (PlannerGanttRow       *row);
static void     gantt_row_destroy                     (GtkObject             *object);
static void     gantt_row_set_property                (GObject               *object,
						       guint                  param_id,
						       const GValue          *value,
						       GParamSpec            *pspec);
static void     gantt_row_get_property                (GObject               *object,
						       guint                  param_id,
						       GValue                *value,
						       GParamSpec            *pspec);
static void     gantt_row_update                      (GnomeCanvasItem       *item,
						       double                *affine,
						       ArtSVP                *clip_path,
						       int                    flags);
static void     gantt_row_realize                     (GnomeCanvasItem       *item);
static void     gantt_row_unrealize                   (GnomeCanvasItem       *item);
static void     gantt_row_draw                        (GnomeCanvasItem       *item,
						       GdkDrawable           *drawable,
						       gint                   x,
						       gint                   y,
						       gint                   width,
						       gint                   height);
static double   gantt_row_point                       (GnomeCanvasItem       *item,
						       double                 x,
						       double                 y,
						       gint                   cx,
						       gint                   cy,
						       GnomeCanvasItem      **actual_item);
static void     gantt_row_bounds                      (GnomeCanvasItem       *item,
						       double                *x1,
						       double                *y1,
						       double                *x2,
						       double                *y2);
static gboolean gantt_row_event                       (GnomeCanvasItem       *item,
						       GdkEvent              *event);
static void     gantt_row_notify_cb                   (MrpTask               *task,
						       GParamSpec            *pspec,
						       PlannerGanttRow       *row);
static void     gantt_row_update_assignment_string    (PlannerGanttRow       *row);
static void     gantt_row_assignment_added            (MrpTask               *task,
						       MrpAssignment         *assignment,
						       PlannerGanttRow       *row);
static void     gantt_row_assignment_removed          (MrpTask               *task,
						       MrpAssignment         *assignment,
						       PlannerGanttRow       *row);
static void     gantt_row_resource_name_changed       (MrpResource           *resource,
						       GParamSpec            *pspec,
						       PlannerGanttRow       *row);
static void     gantt_row_resource_short_name_changed (MrpResource           *resource,
						       GParamSpec            *pspec,
						       PlannerGanttRow       *row);
static void     gantt_row_assignment_units_changed    (MrpAssignment         *assignment,
						       GParamSpec            *pspec,
						       PlannerGanttRow       *row);
static void     gantt_row_ensure_layout               (PlannerGanttRow       *row);
static void     gantt_row_update_resources            (PlannerGanttRow       *row);
static void     gantt_row_geometry_changed            (PlannerGanttRow       *row);
static void     gantt_row_connect_all_resources       (MrpTask               *task,
						       PlannerGanttRow       *row);
static void     gantt_row_disconnect_all_resources    (MrpTask               *task,
						       PlannerGanttRow       *row);
static gboolean gantt_row_canvas_scroll               (GtkWidget             *widget,
						       gint                   delta_x,
						       gint                   delta_y);
static gint     gantt_row_get_resource_index_at       (PlannerGanttRow       *row,
						       gint                   x);
static gboolean gantt_row_get_resource_by_index       (PlannerGanttRow       *row,
						       gint                   index,
						       gint                  *x1,
						       gint                  *x2);

static PlannerCmd *task_cmd_edit_property             (PlannerWindow   *window,
					               PlannerTaskTree *tree,
						       MrpTask         *task,
						       const gchar     *property,
						       const GValue    *value);

static GList *  gantt_row_get_selected_tasks          (GtkTreeSelection      *selection);

static MrpUnitsInterval * mop_get_next_ival           (GList                **cur,
						       gint                  *is_a_gap,
                                                       GList                 *start,
                                                       MrpUnitsInterval      *buf);
static MrpInterval *      mop_get_next_mrp_ival       (GList                **cur,
                                                       gint                  *is_a_gap,
                                                       GList                 *start,
                                                       MrpInterval           *buf);
static gboolean gantt_draw_tasktime                   (GdkDrawable           *drawable,
						       GdkGC                 *gc,
						       GnomeCanvas           *canvas,
						       gdouble                start_wx,
						       gdouble                start_wy,
						       gdouble                end_wx,
						       gdouble                end_wy,
						       gint                   cy1,
						       gint                   cy2,
						       gint                   x,
                                                       gint                   y,
						       gboolean               is_up,
						       gchar                 *colorname);


static GnomeCanvasItemClass *parent_class;
static guint                 signals[LAST_SIGNAL];
static GdkBitmap            *complete_stipple = NULL;
static gchar                 complete_stipple_pattern[] = { 0x02, 0x01 };
static GdkBitmap            *break_stipple = NULL;
static gchar                 break_stipple_pattern[] = { 0x03 };

/* Data related to dragging items */
static GnomeCanvasItem   *drag_item;
static gdouble            drag_item_wx_min;
static gdouble            drag_item_wx_max;
static gdouble            drag_item_wy_min;
static gdouble            drag_item_wy_max;
static GnomeCanvasItem   *scroll_region_extender;
static gdouble            scroll_region_max_x;
static gdouble            drag_wx1, drag_wy1;
static GnomeCanvasPoints *drag_points = NULL;
static GnomeCanvasItem   *target_item;
static GnomeCanvasItem   *old_target_item;

GType
planner_gantt_row_get_type (void)
{
	static GType type = 0;

	if (!type) {
		static const GTypeInfo info = {
			sizeof (PlannerGanttRowClass),
			NULL,		/* base_init */
			NULL,		/* base_finalize */
			(GClassInitFunc) gantt_row_class_init,
			NULL,		/* class_finalize */
			NULL,		/* class_data */
			sizeof (PlannerGanttRow),
			0,              /* n_preallocs */
			(GInstanceInitFunc) gantt_row_init
		};

		type = g_type_register_static (GNOME_TYPE_CANVAS_ITEM,
					       "PlannerGanttRow",
					       &info,
					       0);
	}

	return type;
}

static void
gantt_row_class_init (PlannerGanttRowClass *class)
{
	GObjectClass         *gobject_class;
	GtkObjectClass       *object_class;
	GnomeCanvasItemClass *item_class;

	gobject_class = (GObjectClass *) class;
	object_class = (GtkObjectClass *) class;
	item_class = (GnomeCanvasItemClass *) class;

	parent_class = g_type_class_peek_parent (class);

	gobject_class->set_property = gantt_row_set_property;
	gobject_class->get_property = gantt_row_get_property;

	item_class->event = gantt_row_event;

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
                 PROP_TASK,
                 g_param_spec_object ("task", NULL, NULL,
				      MRP_TYPE_TASK,
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

	object_class->destroy = gantt_row_destroy;

	item_class->update = gantt_row_update;
	item_class->realize = gantt_row_realize;
	item_class->unrealize = gantt_row_unrealize;
	item_class->draw = gantt_row_draw;
	item_class->point = gantt_row_point;
	item_class->bounds = gantt_row_bounds;
}

static void
gantt_row_init (PlannerGanttRow *row)
{
	PlannerGanttRowPriv *priv;

	row->priv = g_new0 (PlannerGanttRowPriv, 1);
	priv = row->priv;

	priv->x = 0.0;
	priv->y = 0.0;
	priv->width = 0.0;
	priv->height = 0.0;
	priv->bar_top = 0.0;
	priv->bar_bot = 0.0;
	priv->scale = 1.0;
	priv->visible = TRUE;
	priv->highlight = FALSE;
	priv->mouse_over_index = -1;
	priv->resource_widths = g_array_new (TRUE, FALSE, sizeof (gint));
}

static void
gantt_row_destroy (GtkObject *object)
{
	PlannerGanttRow     *row;
	PlannerGanttRowPriv *priv;

	g_return_if_fail (PLANNER_IS_GANTT_ROW (object));

	row = PLANNER_GANTT_ROW (object);
	priv = row->priv;

	if (priv) {
		if (priv->scroll_timeout_id) {
			g_source_remove (priv->scroll_timeout_id);
			priv->scroll_timeout_id = 0;
		}

		g_array_free (priv->resource_widths, FALSE);

		g_free (priv);
		row->priv = NULL;
	}

	if (GTK_OBJECT_CLASS (parent_class)->destroy) {
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
	}
}

static void
gantt_row_get_bounds (PlannerGanttRow *row,
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
	wx2 = row->priv->x + MAX (row->priv->width, MILESTONE_SIZE) + 1;
	wy2 = row->priv->y + row->priv->height;

	wx2 = MAX (wx2, wx1 + MIN_WIDTH);

	gnome_canvas_item_i2w (item, &wx1, &wy1);
	gnome_canvas_item_i2w (item, &wx2, &wy2);
	gnome_canvas_w2c (item->canvas, wx1, wy1, &cx1, &cy1);
	gnome_canvas_w2c (item->canvas, wx2, wy2, &cx2, &cy2);

	*px1 = cx1 - 1;
	*py1 = cy1 - 1;
	*px2 = cx2 + row->priv->text_width + 1;
	*py2 = cy2 + 1;
}

static gboolean
recalc_bounds (PlannerGanttRow *row)
{
	PlannerGanttRowPriv *priv;
	gint                 width, height;
	mrptime              t;
	MrpTaskType          type;
	gdouble              old_x, old_x_start, old_width;
	gboolean             changed;

	priv = row->priv;

	old_x = priv->x;
	old_x_start = priv->x_start;
	old_width = priv->width;

	gantt_row_ensure_layout (row);

	pango_layout_get_pixel_size (priv->layout,
				     &width,
				     &height);

	if (width > 0) {
		width += TEXT_PADDING;
	}

	priv->text_width = width;
	priv->text_height = height;

	t = mrp_task_get_work_start (priv->task);
	priv->x = t * priv->scale;

	type = mrp_task_get_task_type (priv->task);
	if (type == MRP_TASK_TYPE_MILESTONE) {
		priv->width = MILESTONE_SIZE * 2;
	} else {
		t = mrp_task_get_finish (priv->task);
		priv->width = t * priv->scale - priv->x;
	}

	t = mrp_task_get_start (priv->task);
	priv->x_start = t * priv->scale;

	changed = (old_x != priv->x || old_x_start != priv->x_start ||
		   old_width != priv->width);

	return changed;
}

static void
gantt_row_set_property (GObject      *object,
			guint         param_id,
			const GValue *value,
			GParamSpec   *pspec)
{
	GnomeCanvasItem      *item;
	PlannerGanttRow      *row;
	PlannerGanttRowPriv  *priv;
	gboolean              changed = FALSE;
	gfloat                tmp_scale;
	gdouble               tmp_dbl;
	gboolean              tmp_bool;
	gint                  tmp_int;

	g_return_if_fail (PLANNER_IS_GANTT_ROW (object));

	item = GNOME_CANVAS_ITEM (object);
	row  = PLANNER_GANTT_ROW (object);
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
			double               htask;

			priv->height = tmp_dbl;

			htask = priv->height / 2.0;
			priv->bar_top = ((priv->height - htask) / 2.0);
			priv->bar_bot = ((priv->height + htask) / 2.0);

			changed = TRUE;
		}
		break;

	case PROP_TASK:
		if (priv->task != NULL) {
			gantt_row_disconnect_all_resources (priv->task, row);
			g_object_unref (priv->task);
			/* FIXME: Disconnect notify handlers. */
		}
		priv->task = g_object_ref (g_value_get_object (value));

		g_signal_connect_object (priv->task,
					 "notify",
					 G_CALLBACK (gantt_row_notify_cb),
					 row,
					 0);

		g_signal_connect_object (priv->task,
					 "assignment-added",
					 G_CALLBACK (gantt_row_assignment_added),
					 row,
					 0);

		g_signal_connect_object (priv->task,
					 "assignment-removed",
					 G_CALLBACK (gantt_row_assignment_removed),
					 row,
					 0);

		gantt_row_connect_all_resources (priv->task, row);

		changed = TRUE;
		break;

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

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}

	if (changed) {
		recalc_bounds (row);
		gantt_row_geometry_changed (row);
		gnome_canvas_item_request_update (item);
	}
}

static void
gantt_row_get_property (GObject    *object,
			guint       param_id,
			GValue     *value,
			GParamSpec *pspec)
{
	PlannerGanttRow     *row;
	PlannerGanttRowPriv *priv;

	g_return_if_fail (PLANNER_IS_GANTT_ROW (object));

	row = PLANNER_GANTT_ROW (object);
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

	case PROP_TASK:
		g_value_set_object (value, priv->task);
		break;

	case PROP_HIGHLIGHT:
		g_value_set_boolean (value, priv->highlight);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static void
gantt_row_ensure_layout (PlannerGanttRow *row)
{
	if (row->priv->layout == NULL) {
		row->priv->layout = gtk_widget_create_pango_layout (
			GTK_WIDGET (GNOME_CANVAS_ITEM (row)->canvas), NULL);
	}
	gantt_row_update_resources (row);
}

static void
gantt_row_update_resources (PlannerGanttRow *row)
{
	PlannerGanttRowPriv *priv;
	GList          *l;
	GList          *resources;
	MrpTask        *task;
	MrpAssignment  *assignment;
	MrpResource    *resource;
	const gchar    *name;
	gchar          *name_unit;
	gchar          *tmp_str;
	gchar          *text = NULL;
	PangoRectangle  rect;
	gint            spacing, x;
	gint            units;

	priv = row->priv;

	task = priv->task;

	g_array_set_size (priv->resource_widths, 0);

	/* Measure the spacing between resource names. */
	pango_layout_set_text (priv->layout, ", ", 2);
	pango_layout_get_extents (priv->layout, NULL, &rect);
	spacing = rect.width / PANGO_SCALE;

	x = 0;
	resources = mrp_task_get_assigned_resources (priv->task);

	for (l = resources; l; l = l->next) {
		resource = l->data;

		assignment = mrp_task_get_assignment (task, resource);
		units = mrp_assignment_get_units (assignment);

		/* Try short name first. */
		name = mrp_resource_get_short_name (resource);

		if (!name || name[0] == 0) {
			name = mrp_resource_get_name (resource);
		}

		if (!name || name[0] == 0) {
				name = _("Unnamed");
		}

		g_array_append_val (priv->resource_widths, x);

		if (units != 100) {
			name_unit = g_strdup_printf ("%s [%i]", name, units);
		} else {
			name_unit = g_strdup_printf ("%s", name);
		}

		pango_layout_set_text (priv->layout, name_unit, -1);
		pango_layout_get_extents (priv->layout, NULL, &rect);
		x += rect.width / PANGO_SCALE;
		g_array_append_val (priv->resource_widths, x);

		x += spacing;

		if (text == NULL) { /* First resource */
			text = g_strdup_printf ("%s", name_unit);
			g_free (name_unit);
			continue;
		}

		tmp_str = g_strdup_printf ("%s, %s", text, name_unit);

		g_free (text);
		g_free (name_unit);

		text = tmp_str;
	}

	g_list_free (resources);

	if (text == NULL) {
		pango_layout_set_text (priv->layout, "", 0);
	} else {
		pango_layout_set_text (priv->layout, text, -1);
	}

	g_free (text);
}

static void
gantt_row_update (GnomeCanvasItem *item,
		  double          *affine,
		  ArtSVP          *clip_path,
		  gint             flags)
{
	PlannerGanttRow *row;
	double      x1, y1, x2, y2;

	row = PLANNER_GANTT_ROW (item);

	GNOME_CANVAS_ITEM_CLASS (parent_class)->update (item,
							affine,
							clip_path,
							flags);

	gantt_row_ensure_layout (row);
	gantt_row_get_bounds (row, &x1, &y1, &x2, &y2);

	gnome_canvas_update_bbox (item, x1, y1, x2, y2);
}

static void
gantt_row_realize (GnomeCanvasItem *item)
{
	PlannerGanttRow     *row;
	PlannerGanttRowPriv *priv;

	row = PLANNER_GANTT_ROW (item);
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

	priv->frame_gc = gdk_gc_new (item->canvas->layout.bin_window);

	priv->ttask_gc = gdk_gc_new (item->canvas->layout.bin_window);

	gnome_canvas_get_color (item->canvas,
				"LightSkyBlue3",
				&priv->color_normal);
	gnome_canvas_get_color (item->canvas,
				"#9ac7e0",
				&priv->color_normal_light);
	gnome_canvas_get_color (item->canvas,
				"#7da1b5",
				&priv->color_normal_dark);

	gnome_canvas_get_color (item->canvas,
				"indian red",
				&priv->color_critical);
	gnome_canvas_get_color (item->canvas,
				"#de6464",
				&priv->color_critical_light);
	gnome_canvas_get_color (item->canvas,
				"#ba5454",
				&priv->color_critical_dark);
}

static void
gantt_row_unrealize (GnomeCanvasItem *item)
{
	PlannerGanttRow *row;

	row = PLANNER_GANTT_ROW (item);

	gdk_gc_unref (row->priv->complete_gc);
	row->priv->complete_gc = NULL;

	gdk_gc_unref (row->priv->break_gc);
	row->priv->break_gc = NULL;

	gdk_gc_unref (row->priv->fill_gc);
	row->priv->fill_gc = NULL;

	gdk_gc_unref (row->priv->frame_gc);
	row->priv->frame_gc = NULL;

	gdk_gc_unref (row->priv->ttask_gc);
	row->priv->ttask_gc = NULL;

	if (break_stipple) {
		g_object_unref (break_stipple);
	}

	if (complete_stipple) {
		g_object_unref (complete_stipple);
	}

	GNOME_CANVAS_ITEM_CLASS (parent_class)->unrealize (item);
}

static void
gantt_row_setup_frame_gc (PlannerGanttRow *row, gboolean highlight)
{
	gdk_gc_set_line_attributes (row->priv->frame_gc,
				    0,
				    highlight ? GDK_LINE_ON_OFF_DASH : GDK_LINE_SOLID,
				    GDK_CAP_BUTT,
				    GDK_JOIN_MITER);
}


static MrpUnitsInterval *mop_get_next_ival(GList **cur, gint *is_a_gap, GList *start, MrpUnitsInterval *buf)
{
	MrpUnitsInterval *cur_ival;
	gint last_end, md;

	if (start) {
		*cur = start;
		cur_ival = start->data;
		md = cur_ival->start % (24*60*60);
		if (md > 0) {
			buf->start = cur_ival->start - md;
			buf->end = cur_ival->start;
			buf->units = 0;
			buf->units_full = 0;
			buf->res_n = 0;

			*is_a_gap = 1;

			return (buf);
		}
		else {
			*is_a_gap = 0;
			return (cur_ival);
		}
	}
	if ((*cur) == NULL) {
		return (NULL);
	}

	cur_ival = (*cur)->data;

	if (*is_a_gap == 1) { /* last was a gap */
			*is_a_gap = 0;
			return (cur_ival);
	}
	else {
		if ((*cur)->next != NULL) {
			last_end = cur_ival->end;
			*cur = (*cur)->next;
			cur_ival = (*cur)->data;

			if (last_end < cur_ival->start) { /* another gap */
				buf->start = last_end;
				buf->end = cur_ival->start;
				buf->units = 0;
				buf->units_full = 0;
				buf->res_n = 0;

				*is_a_gap = 1;
				return (buf);
			}
			else {
				*is_a_gap = 0;
				return (cur_ival);
			}
		}
		else {
			md = cur_ival->end % (24*60*60);
			if (md > 0) {
				buf->start      = cur_ival->end;
				buf->end        = cur_ival->end - md + (24*60*60);
				buf->units = 0;
				buf->units_full = 0;
				buf->res_n = 0;

				*is_a_gap = 1;
				*cur = NULL;
				return (buf);
			}
			else {
				return (NULL);
			}
		}
	}

	return (NULL);
}



static MrpInterval *mop_get_next_mrp_ival(GList **cur, gint *is_a_gap, GList *start, MrpInterval *buf)
{
	MrpInterval *cur_ival;
	gint last_end, md;
	mrptime cur_start, cur_end;

	if (start) {
		*cur = start;
		cur_ival = start->data;

		mrp_interval_get_absolute (cur_ival, 0, &cur_start, &cur_end);
		md = cur_start % (24*60*60);
		if (md > 0) {
			mrp_interval_set_absolute (buf, 0, 0, cur_start);
			*is_a_gap = 1;

			return (buf);
		}
		else {
			*is_a_gap = 0;
			return (cur_ival);
		}
	}
	if ((*cur) == NULL) {
		return (NULL);
	}

	cur_ival = (*cur)->data;
	mrp_interval_get_absolute (cur_ival, 0, &cur_start, &cur_end);

	if (*is_a_gap == 1) { /* last was a gap */
			*is_a_gap = 0;
			return (cur_ival);
	}
	else {
		if ((*cur)->next != NULL) {
			last_end = cur_end;
			*cur = (*cur)->next;
			cur_ival = (*cur)->data;
			mrp_interval_get_absolute (cur_ival, 0, &cur_start, &cur_end);

			if (last_end < cur_start) { /* another gap */
				mrp_interval_set_absolute (buf, 0, last_end, cur_start);

				*is_a_gap = 1;
				return (buf);
			}
			else {
				*is_a_gap = 0;
				return (cur_ival);
			}
		}
		else {
			md = cur_end % (24*60*60);
			if (md > 0) {
				mrp_interval_set_absolute (buf, 0, cur_end, cur_end - md + (24*60*60));

				*is_a_gap = 1;
				*cur = NULL;
				return (buf);
			}
			else {
				return (NULL);
			}
		}
	}

	return (NULL);
}


static gboolean gantt_draw_tasktime(GdkDrawable *drawable,
							 GdkGC *gc,
							 GnomeCanvas *canvas,
							 gdouble start_wx, gdouble start_wy,
							 gdouble end_wx, gdouble end_wy,
							 gint cy1, gint cy2,
							 gint x, gint y,
							 gboolean is_up,
							 gchar *colorname)
{
	GdkColor color;
	gint ttime_x1, ttime_y1, ttime_x2, ttime_y2;

	gnome_canvas_w2c (canvas, start_wx,  start_wy, &ttime_x1, &ttime_y1);
	gnome_canvas_w2c (canvas,   end_wx,    end_wy, &ttime_x2, &ttime_y2);
	ttime_x1 -= x;  ttime_y1 -= y;
	ttime_x2 -= x;  ttime_y2 -= y;

	if (ttime_x2 > ttime_x1) {
		gnome_canvas_get_color (canvas, colorname, &color);
		gdk_gc_set_foreground (gc, &color);

		if (is_up) {
			draw_cut_rectangle (drawable,
								gc,
								TRUE,
								ttime_x1,
								ttime_y1,
								ttime_x2 - ttime_x1,
								cy1 - ttime_y1);

			gnome_canvas_get_color (canvas, "gray40", &color);
			gdk_gc_set_foreground (gc, &color);

		draw_cut_line (drawable, gc, ttime_x1, ttime_y1,
					   ttime_x2-1, ttime_y1);
		draw_cut_line (drawable, gc, ttime_x1, ttime_y1,
					   ttime_x1, cy1-1);
		}
		else {
			draw_cut_rectangle (drawable,
								gc,
								TRUE,
								ttime_x1, cy2+1,
								ttime_x2 - ttime_x1,
								ttime_y2 - cy2 - 1);

			gnome_canvas_get_color (canvas, "gray40", &color);
			gdk_gc_set_foreground (gc, &color);

			draw_cut_line (drawable, gc, ttime_x1, ttime_y2,
						   ttime_x2-1, ttime_y2);
			draw_cut_line (drawable, gc, ttime_x1, cy2+1,
						   ttime_x1, ttime_y2);
		}

		return (TRUE);
	}
	else {
		return (FALSE);
	}
}

static void
gantt_row_draw (GnomeCanvasItem *item,
		GdkDrawable     *drawable,
		gint             x,
		gint             y,
		gint             width,
		gint             height)
{
	PlannerGanttRow     *row;
	PlannerGanttRowPriv *priv;
	PlannerGanttChart   *chart;
	gdouble              i2w_dx;
	gdouble              i2w_dy;
	gdouble              dx1, dy1, dx2, dy2, dshay1, dshay2;
	gint                 level;
	MrpTaskType          type;
	gboolean             summary;
	gint                 summary_y;
	GdkPoint             points[4];
	gint                 percent_complete;
	gint                 complete_x2, complete_width;
	gboolean             highlight_critical;
	gboolean             display_nonstandard_days;
	gboolean             critical;
#ifdef WITH_SIMPLE_PRIORITY_SCHEDULING
	gboolean             is_dominant;
#endif
	gint                 rx1;
	gint                 rx2;
	gint                 cx1, cy1, cx2, cy2;

	GList               *unit_ivals, *cal_ivals, *cur_unit;
	GList		    *cur_cal = NULL;
	GdkColor             color;
	gint                 x_start, x_end, y_dumb, is_a_gap, is_a_subgap;
	MrpUnitsInterval    *unit_ival, ival_buf;
        MrpInterval         *ival_subbuf = NULL, *cal_ival = NULL;
	gint                 i, day_cur, cur_start, cur_end;

	MrpProject          *project;
	MrpDay              *day;
	MrpCalendar         *calendar;
	mrptime              cal_start, cal_end;
	gboolean             is_before_work;
	gboolean             shadup_is_cached, shadup_draw_it;
	gboolean             shaddo_is_cached, shaddo_draw_it;
	gint                 shadup_start, shadup_end;
	gint                 shaddo_start, shaddo_end;

	gboolean             workup_is_cached, workup_draw_it;
	gboolean             workdo_is_cached, workdo_draw_it;
	gint                 workup_start, workup_end;
	gint                 workdo_start, workdo_end;

	gboolean             wunits_is_first, wunits_is_cached, wunits_draw_it;
	gint                 wunits_x_start;

	gint                 last_end;

	gint                 topy,  nres, finish;
	gdouble              delta;
	GList               *assignments;

	shadup_start = -1;
	shadup_end = -1;
	shaddo_start = -1;
	shaddo_end = -1;
	workup_start = -1;
	workup_end = -1;
	workdo_start = -1;
	workdo_end = -1;
	wunits_is_first = FALSE;
	wunits_is_cached = FALSE;
	wunits_x_start = -1;
	last_end = 0;
	delta = -1.0;
	cur_start = 0;
	cur_end = 0;
	day_cur = 0;

	cur_unit = (GList *)NULL;
	cur_cal = (GList *)NULL;

	row = PLANNER_GANTT_ROW (item);
	priv = row->priv;
	project = mrp_object_get_project (MRP_OBJECT (priv->task));
	calendar = mrp_project_get_calendar (project);


	chart = g_object_get_data (G_OBJECT (item->canvas), "chart");
	highlight_critical = planner_gantt_chart_get_highlight_critical_tasks (chart);
	display_nonstandard_days = planner_gantt_chart_get_nonstandard_days (chart);
	level = planner_scale_clamp_zoom (priv->zoom);


	/*
	  NOTES

	  w -> world
	  i -> item
	  c -> canvas

	  priv->x = t * scale
	*/

	/* Get item area in canvas coordinates. */
	i2w_dx = 0.0;
	i2w_dy = 0.0;

	gnome_canvas_item_i2w (item, &i2w_dx, &i2w_dy);

	dx1 = priv->x;

	dy1 = priv->y + priv->bar_top;
	dx2 = priv->x + priv->width;
	dy2 = priv->y + priv->bar_bot;

	dx2 = MAX (dx2, dx1 + MIN_WIDTH);

	dshay1 = priv->y + 0.08 * priv->height;
	dshay2 = priv->y + 0.92 * priv->height;
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

	summary_y = floor (priv->y + 2 * priv->bar_top + 0.5) - y;

	/* "Clip" the expose area. */
	rx1 = MAX (cx1, 0);
	rx2 = MIN (cx2, width);

	summary = (mrp_task_get_n_children (priv->task) > 0);
	complete_width = 0;
	complete_x2 = 0;

	percent_complete = mrp_task_get_percent_complete (priv->task);
	critical = mrp_task_get_critical (priv->task);
	type = mrp_task_get_task_type (priv->task);
	finish = mrp_task_get_finish (priv->task);
	nres = mrp_task_get_nres (priv->task);
	assignments = mrp_task_get_assignments (priv->task);
#ifdef WITH_SIMPLE_PRIORITY_SCHEDULING
	is_dominant = mrp_task_is_dominant (priv->task);
#endif
	if (!summary) {
		complete_width = floor ((cx2 - cx1) * (percent_complete / 100.0) + 0.5);
		complete_x2 = MIN (cx1 + complete_width, rx2);
	}

	gantt_row_setup_frame_gc (row, !summary && priv->highlight);

	if (type == MRP_TASK_TYPE_NORMAL && !summary && rx1 <= rx2) {
		if (complete_width > 0) {
			gnome_canvas_set_stipple_origin (item->canvas,
							 priv->complete_gc);
		}

		if (assignments) {
			/* #define DRAW_BACKGROUND_CHECK 1 */
#ifdef DRAW_BACKGROUND_CHECK
			gnome_canvas_get_color (item->canvas, "indian red", &color);
			gdk_gc_set_foreground (priv->fill_gc, &color);
			draw_cut_rectangle (drawable,
								priv->fill_gc,
								TRUE,
								rx1,
								cy1,
								rx2 - rx1,
								cy2 - cy1);
#endif

			/* Start slices drawer */
			unit_ivals = mrp_task_get_unit_ivals (priv->task);
			ival_subbuf = mrp_interval_new (0,0);

			is_before_work = TRUE;
			shadup_is_cached = FALSE;
			shaddo_is_cached = FALSE;
			workup_is_cached = FALSE;
			workdo_is_cached = FALSE;
			for (i = 0, unit_ival = mop_get_next_ival (&cur_unit, &is_a_gap,
								  unit_ivals, &ival_buf)
					 ; unit_ival && (i == 0 || cur_start < finish) ; i++) {
				if (i == 0) {
					/* first iteration: read the day when start the task */
					day_cur = mrp_time_align_day (unit_ival->start);

					/* extract the intervals of the day */
					day = mrp_calendar_get_day (calendar, day_cur, TRUE);
					cal_ivals = mrp_calendar_day_get_intervals (calendar, day, TRUE);

					if (cal_ivals == NULL) {
						mrp_interval_set_absolute (ival_subbuf, 0, 0, (24*60*60));
						is_a_subgap = 1;
						cal_ival = ival_subbuf;
					}
					else {
						for (cal_ival = mop_get_next_mrp_ival (&cur_cal, &is_a_subgap,
										       cal_ivals, ival_subbuf) ;
						   cal_ival ;
						   cal_ival = mop_get_next_mrp_ival (&cur_cal, &is_a_subgap,
											NULL, ival_subbuf)) {
							mrp_interval_get_absolute (cal_ival, day_cur, &cal_start, &cal_end);

							if (cal_end > unit_ival->start) {
								break;
							}
						}
					}
					cur_start = MAX (unit_ival->start, cal_start);
					cur_end   = MIN (unit_ival->end  , cal_end  );
					wunits_is_first = TRUE;
				}
				g_assert (cal_ival != NULL);

				gnome_canvas_w2c (item->canvas, (cur_start * priv->scale)
								  + i2w_dx, i2w_dy, &x_start, &y_dumb);
				gnome_canvas_w2c (item->canvas, (cur_end * priv->scale)
								  + i2w_dx, i2w_dy, &x_end, &y_dumb);
				x_start -= x;
				x_end   -= x;

				if (is_before_work) {
					if (unit_ival->units_full == 0) {
						goto cont_shadowloop;
					}
					else {
						is_before_work = FALSE;
					}

				}
				if (display_nonstandard_days) {
					shadup_draw_it = FALSE;
					shaddo_draw_it = FALSE;
					workup_draw_it = FALSE;
					workdo_draw_it = FALSE;

					if (unit_ival->res_n == 0 && (!is_a_subgap ||
								  (is_a_subgap && planner_scale_conf[level].nonworking_limit
								   > cal_end - cal_start))) {
						if (shadup_is_cached == FALSE) {
							shadup_start = cur_start;
							shadup_is_cached = TRUE;
						}
					}
					else {
						if (shadup_is_cached == TRUE) {
							if (planner_scale_conf[level].nonworking_limit <=
								cur_start - shadup_start) {
								shadup_end = cur_start;
								shadup_draw_it = TRUE;
							}
							shadup_is_cached = FALSE;
						}
					}

					if (unit_ival->res_n < nres && (!is_a_subgap ||
									(is_a_subgap && planner_scale_conf[level].nonworking_limit
									 > cal_end - cal_start))) {

						if (shaddo_is_cached == FALSE) {
							shaddo_start = cur_start;
							shaddo_is_cached = TRUE;
						}
					}
					else {
						if (shaddo_is_cached == TRUE) {
							if (planner_scale_conf[level].nonworking_limit <=
								cur_start - shaddo_start) {
								shaddo_end = cur_start;
								shaddo_draw_it = TRUE;
							}
							shaddo_is_cached = FALSE;
						}
					}

					if ((unit_ival->res_n == nres && is_a_subgap) ||
						(unit_ival->res_n == 0 &&
						 (cal_start % (24*60*60) == 0) && (cal_end % (24*60*60) == 0) &&
						 planner_scale_conf[level].nonworking_limit > cur_end - cur_start)) {
						if (workup_is_cached == FALSE) {
							workup_start = cur_start;
							workup_is_cached = TRUE;
						}
					}
					else {
						if (workup_is_cached == TRUE) {
							if (planner_scale_conf[level].nonworking_limit <=
								cur_start - workup_start) {
								workup_end = cur_start;
								workup_draw_it = TRUE;
							}
							workup_is_cached = FALSE;
						}
					}

					if ((unit_ival->res_n > 0 && is_a_subgap) ||
						(unit_ival->res_n == 0 &&
						 (cal_start % (24*60*60) == 0) && (cal_end % (24*60*60) == 0) &&
						 planner_scale_conf[level].nonworking_limit > cur_end - cur_start)) {

						if (workdo_is_cached == FALSE) {
							workdo_start = cur_start;
							workdo_is_cached = TRUE;
						}
					}
					else {
						if (workdo_is_cached == TRUE) {
							if (planner_scale_conf[level].nonworking_limit <=
								cur_start - workdo_start) {
								workdo_end = cur_start;
								workdo_draw_it = TRUE;
							}
							workdo_is_cached = FALSE;
						}
					}

					/* Show shadow up. */
					if (shadup_draw_it ||
						(shadup_is_cached &&
						 ((cur_end % (24*60*60)) == 0))) {
						if (!shadup_draw_it && ((cur_end % (24*60*60)) == 0)) {
							shadup_end = cur_end;
						}
						if (planner_scale_conf[level].nonworking_limit <=
							shadup_end - shadup_start) {

							gantt_draw_tasktime (drawable, priv->ttask_gc, item->canvas,
										(shadup_start * priv->scale) + i2w_dx,
										dshay1 + i2w_dy,
										(shadup_end * priv->scale) + i2w_dx,
										dshay2 + i2w_dy,
										cy1, cy2, x, y,  TRUE, "grey96");
						}
						shadup_draw_it = FALSE;
						shadup_is_cached = FALSE;
					}

					/* Show shadow down. */
					if (shaddo_draw_it ||
						(shaddo_is_cached &&
						 ((cur_end % (24*60*60)) == 0))) {
						if (!shaddo_draw_it && ((cur_end % (24*60*60)) == 0)) {
							shaddo_end = cur_end;
						}
						if (planner_scale_conf[level].nonworking_limit <=
							shaddo_end - shaddo_start) {

							gantt_draw_tasktime (drawable, priv->ttask_gc, item->canvas,
									     (shaddo_start * priv->scale) + i2w_dx,
									     dshay1 + i2w_dy,
									     (shaddo_end * priv->scale) + i2w_dx,
									     dshay2 + i2w_dy,
									     cy1, cy2, x, y,  FALSE, "grey96");
						}
						shaddo_draw_it = FALSE;
						shaddo_is_cached = FALSE;
					}

					/* Show work up. */
					if (workup_draw_it ||
						(workup_is_cached &&
						 ((cur_end % (24*60*60)) == 0))) {
						if (!workup_draw_it && ((cur_end % (24*60*60)) == 0)) {
							workup_end = cur_end;
						}
						if (planner_scale_conf[level].nonworking_limit <=
							workup_end - workup_start) {

							gantt_draw_tasktime (drawable, priv->ttask_gc, item->canvas,
									     (workup_start * priv->scale) + i2w_dx,
									     dshay1 + i2w_dy,
									     (workup_end * priv->scale) + i2w_dx,
									     dshay2 + i2w_dy,
									     cy1, cy2, x, y,  TRUE, "white");
						}
						workup_draw_it = FALSE;
						workup_is_cached = FALSE;
					}

					/* Show work down. */
					if (workdo_draw_it ||
						(workdo_is_cached &&
						 ((cur_end % (24*60*60)) == 0))) {
						if (!workdo_draw_it && ((cur_end % (24*60*60)) == 0)) {
							workdo_end = cur_end;
						}
						if (planner_scale_conf[level].nonworking_limit <=
							workdo_end - workdo_start) {

							gantt_draw_tasktime (drawable, priv->ttask_gc, item->canvas,
									     (workdo_start * priv->scale) + i2w_dx,
									     dshay1 + i2w_dy,
									     (workdo_end * priv->scale) + i2w_dx,
									     dshay2 + i2w_dy,
									     cy1, cy2, x, y,  FALSE, "white");

							workdo_draw_it = FALSE;
							workdo_is_cached = FALSE;
						}
					}
				}
				/* Draw area. */
				wunits_draw_it = TRUE;
				if (unit_ival->res_n > 0 && unit_ival->units_full > 0) {
					delta = (double)unit_ival->units / (double)unit_ival->units_full;
				}
				else {
					if (unit_ival->res_n == 0) {  /* It is a nonworking interval. */
						if (planner_scale_conf[level].nonworking_limit <=
						    cur_end - cur_start) {
							delta = 1.0;
						}   /* else it use the last selected value */
					}
					/* It isn't a nonworking interval. */
					else if (planner_scale_conf[level].nonworking_limit <=
						cur_end - cur_start) { /* Visible non working interval. */
						delta = 1.0;
					}
					else {  /* use the last selected value */
						if (wunits_is_first == TRUE) {
							wunits_is_cached = TRUE;
							wunits_x_start = x_start;
							wunits_draw_it = FALSE;
						}
					}
				}

				topy = floor ((cy1 + ((1.0 - (double)delta) * (double)(cy2 - cy1 - 3))) + 0.5);
				if (!highlight_critical || !critical) {
					gdk_gc_set_foreground (priv->fill_gc, &priv->color_normal);
				} else {
					gdk_gc_set_foreground (priv->fill_gc, &priv->color_critical);
				}

				if (wunits_draw_it) {
					if (wunits_is_cached) {
						wunits_is_cached = FALSE;
					}
					else {
						wunits_x_start = x_start;
					}

					if ((cy2 - topy - 3) > 0) {
						draw_cut_rectangle (drawable,
							            	priv->fill_gc,
									TRUE,
									wunits_x_start,
									topy + 2,
									x_end - wunits_x_start,
									cy2 - topy - 3);
					}
					if (topy  - cy1 > 0) {
						gnome_canvas_get_color (item->canvas, "white", &color);
						gdk_gc_set_foreground (priv->fill_gc, &color);
						draw_cut_rectangle (drawable,
									priv->fill_gc,
									TRUE,
									wunits_x_start,
									cy1+2,
									x_end - wunits_x_start,
									topy - cy1);
					}
				}

			cont_shadowloop:
				if (display_nonstandard_days) {
					last_end = cur_end;
				}
				wunits_is_first = FALSE;

				if (cur_end == unit_ival->end) {
					if ((unit_ival = mop_get_next_ival (&cur_unit, &is_a_gap,
									    NULL, &ival_buf)) == NULL) {
						break;
					}
				}
				if (cur_end == cal_end) {
					if ((cal_ival = mop_get_next_mrp_ival (&cur_cal, &is_a_subgap,
									       NULL, ival_subbuf)) == NULL) {
						/* End of the day intervals, read next. */
						day_cur += (24*60*60);
						day = mrp_calendar_get_day (calendar, day_cur, TRUE);
						cal_ivals = mrp_calendar_day_get_intervals (calendar, day, TRUE);
						if (cal_ivals) {
							/* Not empty day. */
							cal_ival = mop_get_next_mrp_ival (&cur_cal, &is_a_subgap,
											  cal_ivals, ival_subbuf);
						}
						else {
							/* Empty day. */
							mrp_interval_set_absolute (ival_subbuf, 0, 0, (24*60*60));
							is_a_subgap = 1;
							cal_ival = ival_subbuf;
						}
						wunits_is_first = TRUE;
					}
					mrp_interval_get_absolute (cal_ival, day_cur, &cal_start, &cal_end);
				}

				cur_start = MAX (unit_ival->start, cal_start);
				cur_end   = MIN (unit_ival->end  , cal_end  );
			}

			if (display_nonstandard_days) {
				/*
				  H O L Y D A Y S
				*/

				/* Show shad up. */
				if (shadup_is_cached) {
					shadup_end = last_end;

					gantt_draw_tasktime (drawable, priv->ttask_gc, item->canvas,
								(shadup_start * priv->scale) + i2w_dx,
								dshay1 + i2w_dy,
								(shadup_end * priv->scale) + i2w_dx,
								dshay2 + i2w_dy,
								cy1, cy2, x, y,  TRUE, "grey96");
				}

				/* Show shad down. */
				if (shaddo_is_cached) {
					shaddo_end = last_end;

					gantt_draw_tasktime (drawable, priv->ttask_gc, item->canvas,
								(shaddo_start * priv->scale) + i2w_dx,
								dshay1 + i2w_dy,
								(shaddo_end * priv->scale) + i2w_dx,
								dshay2 + i2w_dy,
								cy1, cy2, x, y,  FALSE, "grey96");
				}

				/*
				  W O R K
				*/

				/* Show work up. */
				if (workup_is_cached) {
					workup_end = last_end;

					gantt_draw_tasktime (drawable, priv->ttask_gc, item->canvas,
								(workup_start * priv->scale) + i2w_dx,
								dshay1 + i2w_dy,
								(workup_end * priv->scale) + i2w_dx,
								dshay2 + i2w_dy,
								cy1, cy2, x, y,	 TRUE, "white");
				}

				/* Show work down. */
				if (workdo_is_cached) {
					workdo_end = last_end;

					gantt_draw_tasktime (drawable, priv->ttask_gc, item->canvas,
								(workdo_start * priv->scale) + i2w_dx,
								dshay1 + i2w_dy,
								(workdo_end * priv->scale) + i2w_dx,
								dshay2 + i2w_dy,
								cy1, cy2, x, y,  FALSE, "white");
				}
			}
		}
		else { /* if (assignments) ... */
			if (!highlight_critical || !critical) {
				gdk_gc_set_foreground (priv->fill_gc, &priv->color_normal);
			} else {
				gdk_gc_set_foreground (priv->fill_gc, &priv->color_critical);
			}
			draw_cut_rectangle (drawable,
						priv->fill_gc,
						TRUE,
						rx1,
						cy1,
						rx2 - rx1,
						cy2 - cy1);
		}


		gnome_canvas_get_color (item->canvas, "black", &color);
		gdk_gc_set_foreground (priv->frame_gc, &color);

		if (ival_subbuf) {
			mrp_interval_unref (ival_subbuf);
		}

		if (!highlight_critical || !critical) {
			gdk_gc_set_foreground (priv->fill_gc, &priv->color_normal);
		} else {
			gdk_gc_set_foreground (priv->fill_gc, &priv->color_critical);
		}

		if (rx1 <= complete_x2) {
			draw_cut_rectangle (drawable,
					    priv->complete_gc,
					    TRUE,
					    rx1,
					    cy1 + 4,
					    complete_x2 - rx1,
					    cy2 - cy1 - 7);
		}

#ifdef WITH_SIMPLE_PRIORITY_SCHEDULING
 		for (i = 0 ; i < (is_dominant && !(!summary && priv->highlight) ? 2 : 1) ; ++i) {
 			if (i == 1) {
 				gnome_canvas_get_color (item->canvas, "indian red", &color);
 				gdk_gc_set_foreground (priv->frame_gc, &color);

 				gdk_gc_set_line_attributes (priv->frame_gc,
 								0,
 								GDK_LINE_ON_OFF_DASH,
 								GDK_CAP_BUTT,
 								GDK_JOIN_MITER);
 				draw_cut_line (drawable, priv->frame_gc, cx1, cy1, cx2, cy1);
 				draw_cut_line (drawable, priv->frame_gc, cx1, cy2, cx2, cy2);
 				gantt_row_setup_frame_gc (row, !summary && priv->highlight);
 				gnome_canvas_get_color (item->canvas, "black", &color);
 				gdk_gc_set_foreground (priv->frame_gc, &color);
 			}
 			else {
#endif
 				draw_cut_line (drawable, priv->frame_gc, rx1, cy1, rx2, cy1);
 				draw_cut_line (drawable, priv->frame_gc, rx1, cy2, rx2, cy2);
#ifdef WITH_SIMPLE_PRIORITY_SCHEDULING
 			}
 		}
#endif

		if (!highlight_critical || !critical) {
			gdk_gc_set_foreground (priv->fill_gc, &priv->color_normal_light);
		} else {
			gdk_gc_set_foreground (priv->fill_gc, &priv->color_critical_light);
		}

		draw_cut_line (drawable,
			       priv->fill_gc,
			       rx1 + 0,
			       cy1 + 1,
			       rx2 - 0,
			       cy1 + 1);

		if (cx1 == rx1) {
			draw_cut_line (drawable,
				       priv->fill_gc,
				       rx1 + 1,
				       cy1 + 1,
				       rx1 + 1,
				       cy2 - 1);
		}

		if (!highlight_critical || !critical) {
			gdk_gc_set_foreground (priv->fill_gc, &priv->color_normal_dark);
		} else {
			gdk_gc_set_foreground (priv->fill_gc, &priv->color_critical_dark);
		}

		draw_cut_line (drawable,
			       priv->fill_gc,
			       rx1 + 0,
			       cy2 - 1,
			       rx2 - 0,
			       cy2 - 1);

		if (cx2 == rx2) {
			draw_cut_line (drawable,
				       priv->fill_gc,
				       rx2 - 1,
				       cy1 + 1,
				       rx2 - 1,
				       cy2 - 1);
		}
#ifdef WITH_SIMPLE_PRIORITY_SCHEDULING
		for (i = 0 ; i < (is_dominant && !(!summary && priv->highlight) ? 2 : 1) ; ++i) {
			if (i == 1) {
				gnome_canvas_get_color (item->canvas, "red", &color);
				gdk_gc_set_foreground (priv->frame_gc, &color);

				gdk_gc_set_line_attributes (priv->frame_gc,
								0,
								GDK_LINE_ON_OFF_DASH,
								GDK_CAP_BUTT,
								GDK_JOIN_MITER);
			}
#endif
			if (cx1 == rx1) {
				draw_cut_line (drawable, priv->frame_gc,
						cx1, cy1, cx1, cy2);
			}
			if (cx2 == rx2) {
				draw_cut_line (drawable, priv->frame_gc,
						cx2, cy1, cx2, cy2);
			}

#ifdef WITH_SIMPLE_PRIORITY_SCHEDULING
			if (i == 1) {
				gantt_row_setup_frame_gc (row, !summary && priv->highlight);
				gnome_canvas_get_color (item->canvas, "black", &color);
				gdk_gc_set_foreground (priv->frame_gc, &color);
			}
		}
#endif
	}
	else if (type == MRP_TASK_TYPE_MILESTONE && !summary && rx1 <= rx2) {
		points[0].x = cx1;
		points[0].y = cy1;

		points[1].x = cx1 + MILESTONE_SIZE + 1;
		points[1].y = cy1 + MILESTONE_SIZE + 1;

		points[2].x = cx1;
		points[2].y = cy1 + (MILESTONE_SIZE + 1) * 2;

		points[3].x = cx1 - MILESTONE_SIZE;
		points[3].y = cy1 + MILESTONE_SIZE + 1;

		gdk_draw_polygon (drawable,
				  priv->frame_gc,
				  TRUE,
				  (GdkPoint *) &points,
				  4);
	}
	else if (summary && rx1 <= rx2) {
		/* FIXME: Maybe we should try and make the summary be thicker
		 * for larger heights and always centered vertically?
		 */
		draw_cut_rectangle (drawable,
				    priv->frame_gc,
				    TRUE,
				    rx1,
				    summary_y,
				    rx2 - rx1 + 1,
				    THICKNESS);

		if ((rx1 >= cx1 && rx1 <= cx1 + SLOPE) ||
		    (rx2 >= cx1 && rx2 <= cx1 + SLOPE)) {
			points[0].x = cx1;
			points[0].y = summary_y + THICKNESS;

			points[1].x = cx1;
			points[1].y = summary_y + THICKNESS + HEIGHT - 1;

			points[2].x = cx1 + SLOPE;
			points[2].y = summary_y + THICKNESS;

			points[3].x = cx1;
			points[3].y = summary_y + THICKNESS;

			gdk_draw_polygon (drawable,
					  priv->frame_gc,
					  TRUE,
					  (GdkPoint *) &points,
					  4);
		}

		if ((rx1 >= cx2 - SLOPE && rx1 <= cx2) ||
		    (rx2 >= cx2 - SLOPE && rx2 <= cx2)) {
			points[0].x = cx2 + 1;
			points[0].y = summary_y + THICKNESS;

			points[1].x = cx2 + 1 - SLOPE;
			points[1].y = summary_y + THICKNESS;

			points[2].x = cx2 + 1;
			points[2].y = summary_y + THICKNESS + HEIGHT;

			points[3].x = cx2 + 1;
			points[3].y = summary_y + THICKNESS;

			gdk_draw_polygon (drawable,
					  priv->frame_gc,
					  TRUE,
					  (GdkPoint *) &points,
					  4);
		}
	}

	/* FIXME: the padding is already included in text_width. */
	rx1 = MAX (cx2 + TEXT_PADDING, 0);
	rx2 = MIN (cx2 + TEXT_PADDING + priv->text_width, width);

	if (priv->layout != NULL && rx1 < rx2) {
		/* NOTE: cy1 is the pixel coordinate of the top of the bar.
			 subtract round(priv->bar_top) to bring us to the top of the row
			 add priv->height / 2 to get to the center of the row
			 subtract priv->text_height / 2 to get to the top of the text */

		draw_cut_layout (drawable,
				 GTK_WIDGET (item->canvas)->style->text_gc[GTK_STATE_NORMAL],
				 cx2 + TEXT_PADDING,
				 cy1 - (gint)(priv->bar_top + 0.5) + (priv->height - priv->text_height) / 2,
				 priv->layout);

		if (priv->mouse_over_index != -1) {
			gint x1, x2;

			x1 = 0;
			x2 = 0;

			gantt_row_get_resource_by_index (row,
							 priv->mouse_over_index,
							 &x1, &x2);

			x1 += cx2 + TEXT_PADDING;
			x2 += cx2 + TEXT_PADDING;

			draw_cut_line (drawable,
				       GTK_WIDGET (item->canvas)->style->text_gc[GTK_STATE_NORMAL],
				       x1,
				       cy2 + 2,
				       x2,
				       cy2 + 2);
		}
	}
}

static double
gantt_row_point (GnomeCanvasItem  *item,
		 double            x,
		 double            y,
		 gint              cx,
		 gint              cy,
		 GnomeCanvasItem **actual_item)
{
	PlannerGanttRow     *row;
	PlannerGanttRowPriv *priv;
	gint                 text_width;
	gdouble              x1, y1, x2, y2;
	gdouble              dx, dy;

	row = PLANNER_GANTT_ROW (item);
	priv = row->priv;

	*actual_item = item;

	text_width = priv->text_width;
	if (text_width > 0) {
		text_width += TEXT_PADDING;
	}

	x1 = priv->x;
	y1 = priv->y;
	x2 = x1 + priv->width + text_width;
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
gantt_row_bounds (GnomeCanvasItem *item,
		  double          *x1,
		  double          *y1,
		  double          *x2,
		  double          *y2)
{
	PlannerGanttRow *row;

	row = PLANNER_GANTT_ROW (item);

	gantt_row_get_bounds (row, x1, y1, x2, y2);

	if (GNOME_CANVAS_ITEM_CLASS (parent_class)->bounds) {
		GNOME_CANVAS_ITEM_CLASS (parent_class)->bounds (item, x1, y1, x2, y2);
	}
}

static void
gantt_row_notify_cb (MrpTask *task, GParamSpec *pspec, PlannerGanttRow *row)
{
	if (recalc_bounds (row)) {
		gantt_row_geometry_changed (row);
	}
	/* Note: This is not really nice, it's bug-prone, but we can live with
	 * it since it's a good optimization.
	 */
	else if (strcmp (pspec->name, "critical") != 0 &&
		 strcmp (pspec->name, "sched") != 0 &&
		 strcmp (pspec->name, "priority") != 0 &&
		 strcmp (pspec->name, "percent-complete")) {
		return;
	}

	gnome_canvas_item_request_update (GNOME_CANVAS_ITEM (row));
}

static void
gantt_row_update_assignment_string (PlannerGanttRow *row)
{
	gantt_row_update_resources (row);

	recalc_bounds (row);
	gnome_canvas_item_request_update (GNOME_CANVAS_ITEM (row));
}

static void
gantt_row_assignment_added (MrpTask         *task,
			    MrpAssignment   *assignment,
			    PlannerGanttRow *row)
{
	MrpResource *resource;

	resource = mrp_assignment_get_resource (assignment);

	g_signal_connect_object (resource, "notify::name",
				 G_CALLBACK (gantt_row_resource_name_changed),
				 row, 0);

	g_signal_connect_object (resource, "notify::short-name",
				 G_CALLBACK (gantt_row_resource_short_name_changed),
				 row, 0);

	g_signal_connect_object (assignment, "notify::units",
				 G_CALLBACK (gantt_row_assignment_units_changed),
				 row, 0);

	gantt_row_update_assignment_string (row);
}

static void
gantt_row_assignment_removed (MrpTask         *task,
			      MrpAssignment   *assignment,
			      PlannerGanttRow *row)
{
	MrpResource *resource;

	resource = mrp_assignment_get_resource (assignment);

	g_signal_handlers_disconnect_by_func (resource,
					      gantt_row_resource_name_changed,
					      row);

	g_signal_handlers_disconnect_by_func (resource,
					      gantt_row_resource_short_name_changed,
					      row);

	g_signal_handlers_disconnect_by_func (assignment,
					      gantt_row_assignment_units_changed,
					      row);

	gantt_row_update_assignment_string (row);
}

static void
gantt_row_resource_name_changed (MrpResource     *resource,
				 GParamSpec      *pspec,
				 PlannerGanttRow *row)
{
	gantt_row_update_assignment_string (row);
}

static void
gantt_row_resource_short_name_changed (MrpResource     *resource,
				       GParamSpec      *pspec,
				       PlannerGanttRow *row)
{
	gantt_row_update_assignment_string (row);
}

static void
gantt_row_assignment_units_changed (MrpAssignment   *assignment,
				    GParamSpec      *pspec,
				    PlannerGanttRow *row)
{
	gantt_row_update_assignment_string (row);
}

/* Returns the geometry of the actual bars, not the bounding box, not including
 * the text labels.
 */
void
planner_gantt_row_get_geometry (PlannerGanttRow *row,
				gdouble         *x1,
				gdouble         *y1,
				gdouble         *x2,
				gdouble         *y2)
{
	PlannerGanttRowPriv *priv;

	g_return_if_fail (PLANNER_IS_GANTT_ROW (row));

	priv = row->priv;

	if (x1) {
		*x1 = priv->x;
	}
	if (x2) {
		*x2 = priv->x + priv->width;
	}
	if (y1) {
		/* preMOP *y1 = priv->y + 0.15 * priv->height; */
		*y1 = priv->y;
	}
	if (y2) {
		/* preMOP *y2 = priv->y + 0.70 * priv->height; */
		*y2 = priv->y + priv->height;
	}
}

void
planner_gantt_row_set_visible (PlannerGanttRow *row,
			       gboolean         is_visible)
{
	if (is_visible == row->priv->visible) {
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
gantt_row_geometry_changed (PlannerGanttRow *row)
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

		g_signal_connect_object (resource, "notify::short-name",
					 G_CALLBACK (gantt_row_resource_short_name_changed),
					 row, 0);

	}

	g_list_free (resources);
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

		g_signal_handlers_disconnect_by_func (resource,
						      gantt_row_resource_short_name_changed,
						      row);
	}

	g_list_free (resources);
}

static gboolean
gantt_row_drag_item_to_pointer (PlannerGanttRow *row, gboolean scroll)
{
	PlannerGanttChart *chart;
	GnomeCanvas *canvas;
	gint       width, height;
	gint       x, y, item_cx, item_cy;
	gint       cx, cy;
	gdouble    wx2, wy2;
	gint dx, dy;

	gint         duration;
	gint         work;
	MrpProject  *project;
	gchar       *message;

	gint        percent_complete;

	canvas = GNOME_CANVAS_ITEM (row)->canvas;
	chart = g_object_get_data (G_OBJECT (GNOME_CANVAS_ITEM (row)->canvas), "chart");


	/* Get the current mouse position so that we can decide if the pointer
	 * is inside the viewport.
	 */
	gdk_window_get_pointer (GTK_WIDGET(canvas)->window, &x, &y, NULL);

	gnome_canvas_get_scroll_offsets (canvas, &cx, &cy);

	width = GTK_WIDGET(canvas)->allocation.width;
	height = GTK_WIDGET(canvas)->allocation.height;

	if(!scroll)
	{
		x = CLAMP(x, 0, width - 1);
		y = CLAMP(y, 0, height - 1);
	}

	gnome_canvas_c2w (canvas, cx + x, cy + y, &wx2, &wy2);

  	if(drag_item_wx_min != -1.0) wx2 = MAX(wx2, drag_item_wx_min);
  	if(drag_item_wx_max != -1.0) wx2 = MIN(wx2, drag_item_wx_max);
  	if(drag_item_wy_min != -1.0) wy2 = MAX(wy2, drag_item_wy_min);
  	if(drag_item_wy_max != -1.0) wy2 = MIN(wy2, drag_item_wy_max);

	switch(row->priv->state)
	{
	case STATE_DRAG_DURATION:

		project = mrp_object_get_project (MRP_OBJECT (row->priv->task));

		duration = MAX (0, (wx2 - row->priv->x_start) / row->priv->scale);

		/* Snap to quarters. */
		duration = floor (duration / SNAP + 0.5) * SNAP;

		work = mrp_project_calculate_task_work (
			project,
			row->priv->task,
			-1,
			mrp_task_get_start (row->priv->task) + duration);

		message = g_strdup_printf (_("Change work to %s"),
					   planner_format_duration (project, work));
		planner_gantt_chart_status_updated (chart, message);
		g_free (message);

		gnome_canvas_item_set (drag_item,
				       "x2", wx2,
				       NULL);
		scroll_region_max_x = MAX(wx2, scroll_region_max_x);
		gnome_canvas_item_set (scroll_region_extender,
				       "x2", scroll_region_max_x,
				       NULL);
		break;

	case STATE_DRAG_COMPLETE:
		percent_complete = floor ((CLAMP(wx2 - row->priv->x, 0, row->priv->width) * 100.0) / row->priv->width + 0.5);
		message = g_strdup_printf (_("Change progress to %u%% complete"), percent_complete);
		planner_gantt_chart_status_updated (chart, message);
		g_free (message);

		gnome_canvas_item_set (drag_item,
				       "x2", wx2,
				       NULL);
		break;
	case STATE_DRAG_LINK:
		target_item = gnome_canvas_get_item_at (canvas, wx2, wy2);

		drag_points->coords[0] = drag_wx1;
		drag_points->coords[1] = drag_wy1;
		drag_points->coords[2] = wx2;
		drag_points->coords[3] = wy2;

		gnome_canvas_item_set (drag_item,
				       "points", drag_points,
				       NULL);

		if (old_target_item && old_target_item != target_item) {
			g_object_set (old_target_item,
				      "highlight",
				      FALSE,
				      NULL);
		}

		if (target_item && target_item != GNOME_CANVAS_ITEM(row)) {
			const gchar *task_name, *target_name;

			g_object_set (target_item,
				      "highlight",
				      TRUE,
				      NULL);

			target_name = mrp_task_get_name (PLANNER_GANTT_ROW (target_item)->priv->task);
			task_name = mrp_task_get_name (row->priv->task);

			if (target_name == NULL || target_name[0] == 0) {
				target_name = _("No name");
			}
			if (task_name == NULL || task_name[0] == 0) {
				task_name = _("No name");
			}

			message = g_strdup_printf (_("Make task '%s' a predecessor of '%s'"),
						   task_name,
						   target_name);

			planner_gantt_chart_status_updated (chart, message);

			g_free (message);
		}

		if (target_item == NULL) {
			planner_gantt_chart_status_updated (chart, NULL);
		}

		old_target_item = target_item;
		break;
	default:
		g_assert (FALSE);
		break;
	}

	if(!scroll) return TRUE;

	gnome_canvas_w2c (canvas, wx2, wy2, &item_cx, &item_cy);

	if (x < 0) {
		dx = MAX(x, (item_cx - cx) - width / 2);
		dx = MIN(dx, 0);
	} else if (x >= width) {
		dx = MIN(x - width + 1, (item_cx - cx) - width/2);
		dx = MAX(dx, 0);
	} else {
		dx = 0;
	}

	if (y < 0) {
		dy = y;
	} else if (y >= height) {
		dy = y - height + 1;
	} else {
		dy = 0;
	}

	if(dx > 0)
	{
		planner_gantt_chart_reflow_now(chart);
		gantt_row_canvas_scroll (GTK_WIDGET(canvas), dx, dy);
	}
	else if(dx < 0)
	{
		gantt_row_canvas_scroll (GTK_WIDGET(canvas), dx, dy);
		planner_gantt_chart_reflow_now(chart);
	}
	else
	{
		if(dy != 0)
		{
			gantt_row_canvas_scroll (GTK_WIDGET(canvas), dx, dy);
		}
		planner_gantt_chart_reflow_now(chart);
	}

	return TRUE;
}

static gboolean
gantt_row_scroll_timeout_cb (PlannerGanttRow *row)
{
	return gantt_row_drag_item_to_pointer(row, TRUE);
}

static DragSpot
get_drag_spot(gdouble x, gdouble y, PlannerGanttRowPriv *priv)
{
	gdouble x2 = priv->x + priv->width;
	if( (y > priv->y + priv->bar_top) &&
	    (y < priv->y + priv->bar_bot) &&
	    (x < x2 + FUZZ) ) {
		gdouble complete_x2 = priv->x + floor(priv->width * (mrp_task_get_percent_complete (priv->task) / 100.0) + 0.5);

		/* if not way left of end of completion bar */
		if(x > complete_x2 - FUZZ) {

			/* if end of completion bar and end of task bar are not very close together */
			if(x2 - complete_x2 > 2 * FUZZ) {

				/* if not way right of the completion bar */
				if(x < complete_x2 + FUZZ) {
					return DRAG_COMPLETE_SPOT;
				} else { /* if way right of completion bar and ... */

					/* if less than FUZZ from the end of the task bar */
					if(x > x2 - FUZZ) {
						return DRAG_DURATION_SPOT;
					} else { /* if more than FUZZ left of the end of the task bar */
						return DRAG_RELATION_SPOT;
					}
				}
			} else { /* if DRAG_DURATION_SPOT and DRAG_COMPLETE_SPOT are connected and ... */

				/* if closer to the end of the task bar than to the end of the completion bar */
				if(x > complete_x2 + (x2 - complete_x2) / 2) {
					return DRAG_DURATION_SPOT;
				} else {
					return DRAG_COMPLETE_SPOT;
				}
			}
		} else { /* if way left of end of completion bar */
			return DRAG_RELATION_SPOT;
		}
	} else { /* above, below or too far to the right of bar */
		return DRAG_NONE_SPOT;
	}
}

static gboolean
gantt_row_event (GnomeCanvasItem *item, GdkEvent *event)
{
	PlannerGanttRow          *row;
	PlannerGanttRowPriv      *priv;
	PlannerGanttChart        *chart;
	GtkWidget                *canvas_widget;
	gdouble                   wx1, wy1;
	gdouble                   wx2, wy2;
	MrpTask                  *task;
	MrpTask                  *target_task;
	GdkCursor                *cursor;
	gboolean                  summary;
	MrpTaskType               type;
	DragSpot                  drag_spot;

	row = PLANNER_GANTT_ROW (item);
	priv = row->priv;
	canvas_widget = GTK_WIDGET (item->canvas);

	summary = (mrp_task_get_n_children (priv->task) > 0);
	type = mrp_task_get_task_type (priv->task);

	switch (event->type) {
	case GDK_BUTTON_PRESS:
		switch (event->button.button) {
		case 3:
			drag_spot = get_drag_spot(event->button.x, event->button.y, priv);

			if (drag_spot == DRAG_RELATION_SPOT) {
				PlannerGanttChart *chart;
				PlannerTaskTree   *tree;
				GtkTreePath       *path;
				GtkTreeSelection  *selection;
				GtkTreeView       *tree_view;
				GtkTreeIter        iter;
				GList             *tasks;

				chart = g_object_get_data (G_OBJECT (item->canvas), "chart");
				tree = planner_gantt_chart_get_view (chart);
				gtk_widget_grab_focus (GTK_WIDGET (tree));

				path = planner_gantt_model_get_path_from_task (
					PLANNER_GANTT_MODEL (planner_gantt_chart_get_model (chart)),
					priv->task);

				tree_view = GTK_TREE_VIEW (tree);

				selection = gtk_tree_view_get_selection (tree_view);

				gtk_tree_model_get_iter (gtk_tree_view_get_model (tree_view), &iter, path);
				if (!gtk_tree_selection_iter_is_selected (selection, &iter)) {

					gtk_tree_selection_unselect_all (selection);
					gtk_tree_selection_select_path (selection, path);
					planner_task_tree_set_anchor (tree, path);
				}

				tasks = gantt_row_get_selected_tasks (selection);
				planner_task_popup_update_sensitivity (priv->popup_factory, tasks);
				g_list_free (tasks);

				gtk_item_factory_popup (priv->popup_factory,
							event->button.x_root,
							event->button.y_root,
							0,
							gtk_get_current_event_time ());

				return TRUE;
			}
			break;

		case 1:
			if (priv->state != STATE_NONE) {
				break;
			}

			drag_spot = get_drag_spot(event->button.x, event->button.y, priv);

			if (type != MRP_TASK_TYPE_MILESTONE &&
			    !summary && drag_spot == DRAG_DURATION_SPOT) {
				guint rgba;

				priv->state = STATE_DRAG_DURATION;

				wx1 = priv->x;
				wy1 = priv->y + priv->bar_top;
				wx2 = event->button.x;
				wy2 = priv->y + priv->bar_bot;

				gnome_canvas_item_i2w (item, &wx1, &wy1);
				gnome_canvas_item_i2w (item, &wx2, &wy2);

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
					scroll_region_extender = gnome_canvas_item_new (gnome_canvas_root (item->canvas),
									   gnome_canvas_dummy_get_type(),
									   "x1", wx2,
									   "y1", wy1,
									   "x2", wx2,
									   "y2", wy2,
									   NULL);
					gnome_canvas_item_hide (scroll_region_extender);
					scroll_region_max_x = wx2;

					/* set limits */
					drag_item_wx_min = wx1;
					drag_item_wx_max = -1.0;
					drag_item_wy_min = wy1;
					drag_item_wy_max = wy2;
				}

				/* Start the autoscroll timeout. */
				priv->scroll_timeout_id = gtk_timeout_add (
					50,
					(GSourceFunc) gantt_row_scroll_timeout_cb,
					row);
			} else if (type != MRP_TASK_TYPE_MILESTONE &&
			    !summary && drag_spot == DRAG_COMPLETE_SPOT) {
				guint rgba;

				priv->state = STATE_DRAG_COMPLETE;

				wx1 = priv->x;
				wy1 = priv->y + priv->bar_top + 4;
				wx2 = event->button.x;
				wy2 = priv->y + priv->bar_bot - 4;

				gnome_canvas_item_i2w (item, &wx1, &wy1);
				gnome_canvas_item_i2w (item, &wx2, &wy2);

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

					/* set limits */
					drag_item_wx_min = wx1;
					drag_item_wx_max = priv->x + priv->width;
					drag_item_wy_min = wy1;
					drag_item_wy_max = wy2;
				}

				/* Start the autoscroll timeout. */
				priv->scroll_timeout_id = gtk_timeout_add (
					50,
					(GSourceFunc) gantt_row_scroll_timeout_cb,
					row);
			} else if (drag_spot == DRAG_RELATION_SPOT) {
				priv->state = STATE_DRAG_LINK;
				if (drag_points == NULL) {
					drag_points = gnome_canvas_points_new (2);
				}

				drag_item = gnome_canvas_item_new (gnome_canvas_root (item->canvas),
								   PLANNER_TYPE_CANVAS_LINE,
								   "points", drag_points,
								   "last_arrowhead", TRUE,
								   "arrow_shape_a", 6.0,
								   "arrow_shape_b", 6.0,
								   "arrow_shape_c", 3.0,
								   "fill_color", "black",
								   "width_pixels", (guint) 0,
								   "join_style", GDK_JOIN_MITER,
								   NULL);
				gnome_canvas_item_hide (drag_item);

				/* set limits */
				gnome_canvas_get_scroll_region(item->canvas, &drag_item_wx_min,
									     &drag_item_wy_min,
									     &drag_item_wx_max,
									     &drag_item_wy_max);

				old_target_item = NULL;

				/* Start the autoscroll timeout. */
				priv->scroll_timeout_id = gtk_timeout_add (
					50,
					(GSourceFunc) gantt_row_scroll_timeout_cb,
					row);
			} else {
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
			}

			gnome_canvas_item_grab (item,
						GDK_POINTER_MOTION_MASK |
						GDK_POINTER_MOTION_HINT_MASK |
						GDK_BUTTON_PRESS_MASK |
						GDK_BUTTON_RELEASE_MASK,
						NULL,
						event->button.time);

			drag_wx1 = event->button.x;
			drag_wy1 = event->button.y;

			return TRUE;

		default:
			return FALSE;
		}
		break;

	case GDK_LEAVE_NOTIFY:
		/* We get a leave notify when pressing button 1 over the
		 * item. We don't want to reset the cursor when that happens.
		 */
		if (!(priv->state & STATE_DRAG_ANY) &&
		    !(event->crossing.state & GDK_BUTTON1_MASK)) {
			gdk_window_set_cursor (canvas_widget->window, NULL);
		}

		g_object_set (row,
			      "mouse-over-index",
			      -1,
			      NULL);

		return TRUE;

	case GDK_MOTION_NOTIFY:
		if (event->motion.is_hint) {
			gint x, y;

			gdk_window_get_pointer (event->motion.window, &x, &y, NULL);
			gnome_canvas_window_to_world(item->canvas, x, y, &event->motion.x, &event->motion.y);
		}

		if (!(priv->state & STATE_DRAG_ANY)) {
			drag_spot = get_drag_spot(event->button.x, event->button.y, priv);

			if (type != MRP_TASK_TYPE_MILESTONE && !summary &&
			    ( (drag_spot == DRAG_DURATION_SPOT) || (drag_spot == DRAG_COMPLETE_SPOT) ) ) {
				if(drag_spot == DRAG_DURATION_SPOT) {
					cursor = gdk_cursor_new (GDK_RIGHT_SIDE);
					gdk_window_set_cursor (canvas_widget->window, cursor);
					if (cursor) {
						gdk_cursor_unref (cursor);
					}
				}
				else { /* DRAG_COMPLETE_SPOT */
					cursor = gdk_cursor_new (GDK_HAND2);
					gdk_window_set_cursor (canvas_widget->window, cursor);
					if (cursor) {
						gdk_cursor_unref (cursor);
					}
				}
			} else { /* Mouse over resource names (or short_name) ? */
 				gint res_index;

				res_index = gantt_row_get_resource_index_at (row,
									     event->motion.x);

				g_object_set (row,
					      "mouse-over-index", res_index,
					      NULL);

				if (res_index != -1) {
					cursor = gdk_cursor_new (GDK_HAND2);
				} else {
					cursor = NULL;
				}
				gdk_window_set_cursor (canvas_widget->window, cursor);
				if (cursor) {
					gdk_cursor_unref (cursor);
				}
			}
			return TRUE;
		}
		else {
			gnome_canvas_item_raise_to_top (drag_item);
			gnome_canvas_item_show (drag_item);
			if(priv->state & STATE_DRAG_DURATION) {
				gnome_canvas_item_show (scroll_region_extender);
			}

			gantt_row_drag_item_to_pointer (row, FALSE);
		}

		break;

	case GDK_BUTTON_RELEASE: {
		PlannerTaskTree  *tree;
		GtkTreePath      *path;
		GtkTreeSelection *selection;

		if (event->button.button != 1) {
			return FALSE;
		}

		if (priv->state == STATE_NONE) {
			return TRUE;
		}
		else if (priv->state == STATE_DRAG_DURATION) {
			MrpProject *project;
			gint        duration;
			gint        work;
			GValue      value = { 0 };

			project = mrp_object_get_project (MRP_OBJECT (priv->task));

			duration = MAX (0, (event->button.x - priv->x_start) / priv->scale);

			/* Snap to quarters. */
			duration = floor (duration / SNAP + 0.5) * SNAP;

			work = mrp_project_calculate_task_work (
				project,
				priv->task,
				-1,
				mrp_task_get_start (priv->task) + duration);

			chart = g_object_get_data (G_OBJECT (item->canvas), "chart");
			tree = planner_gantt_chart_get_view (chart);

			g_value_init (&value, G_TYPE_INT);
			g_value_set_int (&value, work);

			task_cmd_edit_property (planner_task_tree_get_window (tree),
						tree,
						priv->task,
						"work",
						&value);

			if (priv->scroll_timeout_id) {
				g_source_remove (priv->scroll_timeout_id);
				priv->scroll_timeout_id = 0;
			}

			/*g_object_set (priv->task,
				      "work", work,
				      NULL);
			*/
			gtk_object_destroy (GTK_OBJECT (drag_item));
			drag_item = NULL;
			gtk_object_destroy (GTK_OBJECT (scroll_region_extender));
			scroll_region_extender = NULL;

			planner_gantt_chart_status_updated (chart, NULL);
		}
		else if (priv->state == STATE_DRAG_COMPLETE) {
			GValue      value = { 0 };
			gint        percent_complete = floor((MIN(MAX(0, event->button.x - priv->x), priv->width) * 100.0) / priv->width + 0.5);

			chart = g_object_get_data (G_OBJECT (item->canvas), "chart");
			tree = planner_gantt_chart_get_view (chart);

			g_value_init (&value, G_TYPE_INT);
			g_value_set_int (&value, percent_complete);

			task_cmd_edit_property (planner_task_tree_get_window (tree),
						tree,
						priv->task,
						"percent_complete",
						&value);

			if (priv->scroll_timeout_id) {
				g_source_remove (priv->scroll_timeout_id);
				priv->scroll_timeout_id = 0;
			}

			gtk_object_destroy (GTK_OBJECT (drag_item));
			drag_item = NULL;

			planner_gantt_chart_status_updated (chart, NULL);
		}
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
				GError            *error = NULL;
				PlannerCmd        *cmd;
				PlannerGanttChart *chart;
				PlannerTaskTree   *tree;
				PlannerWindow     *main_window;

				task = priv->task;
				target_task = PLANNER_GANTT_ROW (target_item)->priv->task;

				chart = g_object_get_data (G_OBJECT (item->canvas), "chart");
				tree = planner_gantt_chart_get_view (chart);
				main_window = planner_task_tree_get_window (tree);

				cmd = planner_task_cmd_link (main_window,
							     task,
							     target_task,
							     MRP_RELATION_FS,
							     0,
							     &error);

				if (!cmd) {
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

			chart = g_object_get_data (G_OBJECT (item->canvas), "chart");

			planner_gantt_chart_status_updated (chart, NULL);
		}

		/* We're done, reset the cursor and state, ungrab pointer. */
		gdk_window_set_cursor (canvas_widget->window, NULL);

		gnome_canvas_item_ungrab (item, event->button.time);

		/* Select the clicked on task in the treeview */
		chart = g_object_get_data (G_OBJECT (item->canvas), "chart");
		tree = planner_gantt_chart_get_view (chart);
		gtk_widget_grab_focus (GTK_WIDGET (tree));

		path = planner_gantt_model_get_path_from_task (
			PLANNER_GANTT_MODEL (planner_gantt_chart_get_model (chart)),
			priv->task);

		selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree));

		if (!(event->button.state & (GDK_CONTROL_MASK | GDK_SHIFT_MASK))) {
			gtk_tree_selection_unselect_all (selection);
			gtk_tree_selection_select_path (selection, path);
			planner_task_tree_set_anchor (tree, path);
		}
		else if (event->button.state & GDK_CONTROL_MASK) {
			if (gtk_tree_selection_path_is_selected (selection, path)) {
				gtk_tree_selection_unselect_path (selection, path);
			}
			else {
				gtk_tree_selection_select_path (selection, path);
			}
			planner_task_tree_set_anchor (tree, path);
		}
		else if (event->button.state & GDK_SHIFT_MASK) {
			GtkTreePath *anchor;

			anchor = planner_task_tree_get_anchor (tree);

			if (anchor) {
				gtk_tree_selection_unselect_all (selection);
				gtk_tree_selection_select_range(selection, anchor, path);
				gtk_tree_path_free (path);
			}
			else {
				gtk_tree_selection_unselect_all (selection);
				gtk_tree_selection_select_path (selection, path);
				planner_task_tree_set_anchor (tree, path);
			}
		}

		priv->state = STATE_NONE;

		return TRUE;
	}

	case GDK_2BUTTON_PRESS:
		if (event->button.button == 1) {
			drag_spot = get_drag_spot(event->button.x, event->button.y, priv);
			if (drag_spot == DRAG_RELATION_SPOT) {
				PlannerTaskTree  *tree;
				GtkTreePath      *path;
				GtkTreeSelection *selection;

				chart = g_object_get_data (G_OBJECT (item->canvas), "chart");
				tree = planner_gantt_chart_get_view (chart);
				gtk_widget_grab_focus (GTK_WIDGET (tree));

				path = planner_gantt_model_get_path_from_task (
					PLANNER_GANTT_MODEL (planner_gantt_chart_get_model (chart)),
					priv->task);

				selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree));

				gtk_tree_selection_unselect_all (selection);
				gtk_tree_selection_select_path (selection, path);
				planner_task_tree_set_anchor(tree, path);

				planner_task_tree_edit_task (tree, PLANNER_TASK_DIALOG_PAGE_GENERAL);

				return TRUE;
			}
		}

	default:
		break;
	}

	return FALSE;
}

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

static gboolean
gantt_row_canvas_scroll (GtkWidget *widget,
			 gint       delta_x,
			 gint       delta_y)
{
	GtkAdjustment *hadj, *vadj;
	int            old_h_value, old_v_value;

	hadj = gtk_layout_get_hadjustment (GTK_LAYOUT (widget));
	vadj = gtk_layout_get_vadjustment (GTK_LAYOUT (widget));

	/* Store the old adjustment values so we can tell if we ended up
	 * actually scrolling. We may not have in a case where the resulting
	 * value got pinned to the adjustment min or max.
	 */
	old_h_value = hadj->value;
	old_v_value = vadj->value;

	eel_gtk_adjustment_set_value (hadj, hadj->value + delta_x);
	eel_gtk_adjustment_set_value (vadj, vadj->value + delta_y);

	/* return TRUE if we did scroll */
	return hadj->value != old_h_value || vadj->value != old_v_value;
}

static gint
gantt_row_get_resource_index_at (PlannerGanttRow *row,
				 gint             x)
{
	PlannerGanttRowPriv *priv;
	gint                 i, len;
	gint                 left, right;
	gint                 offset;

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
				 gint             index,
				 gint            *x1,
				 gint            *x2)
{
	PlannerGanttRowPriv *priv;

	g_return_val_if_fail (index >= 0, FALSE);

	/* We can't catch too high indices using the g_return_val_if_fal, since
	 * the array might have changed under us.
	 */

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

void
planner_gantt_row_init_menu (PlannerGanttRow *row)
{
	PlannerGanttChart *chart;
	PlannerTaskTree   *tree;

	chart = g_object_get_data (G_OBJECT (GNOME_CANVAS_ITEM (row)->canvas), "chart");
    	tree = planner_gantt_chart_get_view (chart);

	row->priv->popup_factory = planner_task_popup_new (tree);
}

/* Save this code for later. */
#if 0
	gdouble              wx1, wx2;
	mrptime              t1, t2;    /* First and last exposed times */
	gint                 tx1, tx2;
	mrptime              ival_start, ival_end, ival_prev;
	MrpProject          *project;
	MrpCalendar         *calendar;
	MrpDay              *day;
	GList               *intervals, *l;
	MrpInterval         *ival;
	GdkColor             color_break;


                /* Draw non-working intervals shaded. FIXME: Disabled for now. */
		if (0) { g_object_get (priv->task, "project", &project, NULL);

		calendar = mrp_project_get_calendar (project);

		/* Get exposed world coordinates. */
		gnome_canvas_c2w (item->canvas, x, 0, &wx1, NULL);
		gnome_canvas_c2w (item->canvas, x + width, 0, &wx2, NULL);

		/* Convert to exposed time coordinates. */
		t1 = floor (wx1 / priv->scale + 0.5);
		t2 = floor (wx2 / priv->scale + 0.5);

		t1 = mrp_time_align_day (t1);
		t2 = mrp_time_align_day (t2 + 24*60*60);

		ival_prev = t1;

		gnome_canvas_get_color (item->canvas, "#b7c3c9", &color_break);
		gdk_gc_set_foreground (priv->fill_gc, &color_break);

		/*g_print ("-------------------\n");*/

		/* Loop through the days between t1 and t2. */
		while (t1 <= t2) {
			day = mrp_calendar_get_day (calendar, t1, TRUE);

			intervals = mrp_calendar_day_get_intervals (calendar, day, TRUE);

			/* Loop through the intervals for this day. */
			for (l = intervals; l; l = l->next) {
				ival = l->data;

				mrp_interval_get_absolute (ival,
							   t1,
							   &ival_start,
							   &ival_end);

				/*g_print ("%s %s (%s)\n",
					 mrp_time_format ("%H:%M", ival_prev),
					 mrp_time_format ("%H:%M", ival_start),
					 mrp_time_format ("%a %e %b", ival_prev));*/

				/* Don't draw if the interval is shorter than what we
				 * want at this zoom level.
				 */
				if (planner_scale_conf[level].nonworking_limit <= ival_start - ival_prev) {
					/* Draw the section between the end of the last working
					 * time interval and the start of the current one,
					 * i.e. [ival_prev, ival_start].
					 */
					wx1 = ival_prev * priv->scale;
					wx2 = ival_start * priv->scale;

					gnome_canvas_w2c (item->canvas, wx1, 0, &tx1, NULL);
					gnome_canvas_w2c (item->canvas, wx2, 0, &tx2, NULL);

					tx1 -= x;
					tx2 -= x;

					/* Don't draw if we're outside the exposed parts
					 * of the gantt bar.
					 */
					if (tx1 >= rx2) {
						break;
					}

					if (tx2 <= rx1) {
						ival_prev = ival_end;
						continue;
					}

					tx1 = MAX (tx1, rx1);
					tx2 = MIN (tx2, rx2);

					if (tx1 < tx2) {
						draw_cut_rectangle (drawable,
								    priv->fill_gc,
								    TRUE,
								    tx1,
								    cy1 + 1,
								    tx2 - tx1,
								    cy2 - cy1 - 1);
					}

					/*gdk_gc_set_foreground (priv->fill_gc, &color_high);
					draw_cut_line (drawable,
						       priv->fill_gc,
						       tx1,
						       cy1 + 1,
						       tx1,
						       cy2 - 1);

					draw_cut_line (drawable,
						       priv->fill_gc,
						       tx2 - 1,
						       cy1 + 1,
						       tx2 - 1,
						       cy2 - 1);*/
				}

				ival_prev = ival_end;
			}

			t1 += 60*60*24;

			/* Draw the remaining interval if any. */
			if (ival_prev < t1 && planner_scale_conf[level].nonworking_limit <= t1 - ival_prev) {
				g_print ("%s %s (%s) REMAINING\n",
					 mrp_time_format ("%H:%M", ival_prev),
					 mrp_time_format ("%H:%M", t1),
					 mrp_time_format ("%a %e %b", ival_prev));

				wx1 = ival_prev * priv->scale;
				wx2 = t1 * priv->scale;

				gnome_canvas_w2c (item->canvas, wx1, 0, &tx1, NULL);
				gnome_canvas_w2c (item->canvas, wx2, 0, &tx2, NULL);

				tx1 -= x;
				tx2 -= x;

				/* Don't draw if we're outside the exposed parts
				 * of the gantt bar.
				 */
				if (tx1 >= rx2 || tx2 <= rx1) {
					continue;
				}

				tx1 = MAX (tx1, rx1);
				tx2 = MIN (tx2, rx2);

				if (tx1 < tx2) {
					draw_cut_rectangle (drawable,
							    priv->fill_gc,
							    TRUE,
							    tx1,
							    cy1 + 1,
							    tx2 - tx1,
							    cy2 - cy1 - 1);
				}
			}
		}
		}

#endif


/* This should be ina shared file ideally, but there are some linking
 * problems.
 */

typedef struct {
	PlannerCmd         base;

	PlannerTaskTree   *tree;
	MrpProject        *project;

	GtkTreePath       *path;

	gchar             *property;
	GValue            *value;
	GValue            *old_value;
} TaskCmdEditProperty;

static gboolean
task_cmd_edit_property_do (PlannerCmd *cmd_base)
{
	TaskCmdEditProperty *cmd;
	PlannerGanttModel   *model;
	MrpTask             *task;

	cmd = (TaskCmdEditProperty*) cmd_base;

	model = PLANNER_GANTT_MODEL (gtk_tree_view_get_model (GTK_TREE_VIEW (cmd->tree)));
	task = planner_gantt_model_get_task_from_path (model, cmd->path);

	g_object_set_property (G_OBJECT (task),
			       cmd->property,
			       cmd->value);

	return TRUE;
}

static void
task_cmd_edit_property_undo (PlannerCmd *cmd_base)
{
	TaskCmdEditProperty *cmd;
	PlannerGanttModel   *model;
	MrpTask             *task;

	cmd = (TaskCmdEditProperty*) cmd_base;

	model = PLANNER_GANTT_MODEL (gtk_tree_view_get_model (GTK_TREE_VIEW (cmd->tree)));
	task = planner_gantt_model_get_task_from_path (model, cmd->path);

	g_object_set_property (G_OBJECT (task),
			       cmd->property,
			       cmd->old_value);
}

static void
task_cmd_edit_property_free (PlannerCmd *cmd_base)
{
	TaskCmdEditProperty *cmd;

	cmd = (TaskCmdEditProperty*) cmd_base;

	g_object_unref (cmd->project);

	g_free (cmd->property);
	g_value_unset (cmd->value);
	g_value_unset (cmd->old_value);

	g_free (cmd->value);
	g_free (cmd->old_value);
}

static PlannerCmd *
task_cmd_edit_property (PlannerWindow   *window,
			PlannerTaskTree *tree,
			MrpTask         *task,
			const gchar     *property,
			const GValue    *value)
{
	PlannerCmd          *cmd_base;
	TaskCmdEditProperty *cmd;
	PlannerGanttModel   *model;

	cmd_base = planner_cmd_new (TaskCmdEditProperty,
				    _("Edit task property"),
				    task_cmd_edit_property_do,
				    task_cmd_edit_property_undo,
				    task_cmd_edit_property_free);

	cmd = (TaskCmdEditProperty *) cmd_base;

	cmd->tree = tree;
	cmd->project = g_object_ref (planner_window_get_project (window));

	model = PLANNER_GANTT_MODEL (gtk_tree_view_get_model (GTK_TREE_VIEW (tree)));

	cmd->path = planner_gantt_model_get_path_from_task (model, task);

	cmd->property = g_strdup (property);

	cmd->value = g_new0 (GValue, 1);
	g_value_init (cmd->value, G_VALUE_TYPE (value));
	g_value_copy (value, cmd->value);

	cmd->old_value = g_new0 (GValue, 1);
	g_value_init (cmd->old_value, G_VALUE_TYPE (value));

	g_object_get_property (G_OBJECT (task),
			       cmd->property,
			       cmd->old_value);

	planner_cmd_manager_insert_and_do (planner_window_get_cmd_manager (window),
					   cmd_base);

	return cmd_base;
}

static  void
gantt_row_get_selected_func (GtkTreeModel *model,
			     GtkTreePath  *path,
			     GtkTreeIter  *iter,
			     gpointer      data)
{
	GList   **list = data;
	MrpTask  *task;

	gtk_tree_model_get (model,
			    iter,
			    COL_TASK, &task,
			    -1);

	*list = g_list_prepend (*list, task);
}

static GList *
gantt_row_get_selected_tasks (GtkTreeSelection *selection)
{
	GList *list;

	list = NULL;
	gtk_tree_selection_selected_foreach (selection,
					     gantt_row_get_selected_func,
					     &list);

	list = g_list_reverse (list);

	return list;
}
