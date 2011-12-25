/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2003-2004 Imendio AB
 * Copyright (C) 2003 Benjamin BAYART <benjamin@sitadelle.com>
 * Copyright (C) 2003 Xavier Ordoquy <xordoquy@wanadoo.fr>
 * Copyright (C) 2006 Alvaro del Castillo <acs@barrapunto.com>
 * Copyright (C) 2008 Lee Baylis <lee@leebaylis.co.uk>
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
#include <glib/gi18n.h>
#include <libgnomecanvas/gnome-canvas.h>
#include <libgnomecanvas/gnome-canvas-util.h>
#include "planner-marshal.h"
#include "planner-format.h"
#include "planner-usage-row.h"
#include "planner-usage-chart.h"
#include "planner-canvas-line.h"
#include "eel-canvas-rect.h"
#include "planner-scale-utils.h"
#include "planner-usage-chart.h"
#include "planner-usage-model.h"

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
};

enum {
        /* For relation arrows. */
        GEOMETRY_CHANGED,
        VISIBILITY_CHANGED,
        LAST_SIGNAL
};

typedef enum {
        STATE_NONE = 0,
        STATE_DRAG_MOVE = 1 << 0,
        STATE_DRAG_DURATION = 1 << 1,

        STATE_DRAG_ANY = STATE_DRAG_MOVE | STATE_DRAG_DURATION
} State;

struct _PlannerUsageRowPriv {
        GdkGC         *complete_gc;
        GdkGC         *break_gc;
        GdkGC         *fill_gc;
        GdkGC         *frame_gc;
        PangoLayout   *layout;

        MrpAssignment *assignment;
        MrpResource   *resource;

        guint          visible:1;
        guint          fixed_duration:1;

        gdouble        scale;
        gdouble        zoom;

        gdouble        x;
        gdouble        y;
        gdouble        x_start;

        gdouble        width;
        gdouble        height;
        gdouble        text_width;

        guint          scroll_timeout_id;
        State          state;
};

static void     usage_row_class_init                   (PlannerUsageRowClass  *class);
static void     usage_row_init                         (PlannerUsageRow       *row);
static void     usage_row_destroy                      (GtkObject              *object);
static void     usage_row_set_property                 (GObject                *object,
							 guint                   param_id,
							 const GValue           *value,
							 GParamSpec             *pspec);
static void     usage_row_get_property                 (GObject                *object,
							 guint                   param_id,
							 GValue                 *value,
							 GParamSpec             *pspec);
static void     usage_row_update                       (GnomeCanvasItem        *item,
							 double                 *affine,
							 ArtSVP                 *clip_path,
							 int                     flags);
static void     usage_row_realize                      (GnomeCanvasItem        *item);
static void     usage_row_unrealize                    (GnomeCanvasItem        *item);
static void     usage_row_draw                         (GnomeCanvasItem        *item,
							 GdkDrawable            *drawable,
							 gint                    x,
							 gint                    y,
							 gint                    width,
							 gint                    height);
static double   usage_row_point                        (GnomeCanvasItem        *item,
							 double                  x,
							 double                  y,
							 gint                    cx,
							 gint                    cy,
							 GnomeCanvasItem       **actual_item);
static void     usage_row_bounds                       (GnomeCanvasItem        *item,
							 double                 *x1,
							 double                 *y1,
							 double                 *x2,
							 double                 *y2);
static gboolean usage_row_event                       (GnomeCanvasItem          *item,
						       GdkEvent                 *event);
static void     usage_row_ensure_layout               (PlannerUsageRow       *row);
static void     usage_row_update_resources            (PlannerUsageRow       *row);
static void     usage_row_geometry_changed             (PlannerUsageRow       *row);
static void     usage_row_ensure_layout                (PlannerUsageRow       *row);
static void     usage_row_resource_notify_cb           (MrpResource            *resource,
							 GParamSpec             *pspec,
							 PlannerUsageRow       *row);
static void     usage_row_assignment_notify_cb         (MrpAssignment          *assign,
							 GParamSpec             *pspec,
							 PlannerUsageRow       *row);
static void     usage_row_task_notify_cb               (MrpTask                *task,
							 GParamSpec             *pspec,
							 PlannerUsageRow       *row);
static void     usage_row_resource_assignment_added_cb (MrpResource            *resource,
							 MrpAssignment          *assign,
							 PlannerUsageRow       *row);


static GnomeCanvasItemClass *parent_class;
static guint                 signals[LAST_SIGNAL];
static GdkBitmap            *complete_stipple = NULL;
static gchar                 complete_stipple_pattern[] = { 0x02, 0x01 };
static GdkBitmap            *break_stipple = NULL;
static gchar                 break_stipple_pattern[] = { 0x03 };

static GdkColor              color_normal;
static GdkColor              color_normal_light;
static GdkColor              color_normal_dark;

static GdkColor              color_free;
static GdkColor              color_free_light;
static GdkColor              color_free_dark;

static GdkColor              color_underuse;
static GdkColor              color_underuse_light;
static GdkColor              color_underuse_dark;

static GdkColor              color_overuse;
static GdkColor              color_overuse_light;
static GdkColor              color_overuse_dark;


GType
planner_usage_row_get_type (void)
{
        static GType type = 0;

        if (!type) {
                static const GTypeInfo info = {
                        sizeof (PlannerUsageRowClass),
                        NULL,   /* base_init */
                        NULL,   /* base_finalize */
                        (GClassInitFunc) usage_row_class_init,
                        NULL,   /* class_finalize */
                        NULL,   /* class_data */
                        sizeof (PlannerUsageRow),
                        0,      /* n_preallocs */
                        (GInstanceInitFunc) usage_row_init
                };
                type = g_type_register_static (GNOME_TYPE_CANVAS_ITEM,
                                               "PlannerUsageRow", &info, 0);
        }
        return type;
};

static void
usage_row_class_init (PlannerUsageRowClass * class)
{
        GObjectClass         *gobject_class;
        GtkObjectClass       *object_class;
        GnomeCanvasItemClass *item_class;

        gobject_class = (GObjectClass *) class;
        object_class = (GtkObjectClass *) class;
        item_class = (GnomeCanvasItemClass *) class;

        parent_class = g_type_class_peek_parent (class);

        gobject_class->set_property = usage_row_set_property;
        gobject_class->get_property = usage_row_get_property;

	item_class->event = usage_row_event;

        signals[GEOMETRY_CHANGED] =
                g_signal_new ("geometry-changed",
                              G_TYPE_FROM_CLASS (class),
                              G_SIGNAL_RUN_LAST,
                              0,
                              NULL, NULL,
                              planner_marshal_VOID__DOUBLE_DOUBLE_DOUBLE_DOUBLE,
                              G_TYPE_NONE, 4,
                              G_TYPE_DOUBLE, G_TYPE_DOUBLE, G_TYPE_DOUBLE,
                              G_TYPE_DOUBLE);

        signals[VISIBILITY_CHANGED] =
                g_signal_new ("visibility-changed",
                              G_TYPE_FROM_CLASS (class),
                              G_SIGNAL_RUN_LAST,
                              0,
                              NULL, NULL,
                              planner_marshal_VOID__BOOLEAN,
                              G_TYPE_NONE, 1, G_TYPE_BOOLEAN);

        g_object_class_install_property
                (gobject_class,
                 PROP_SCALE,
                 g_param_spec_double ("scale", NULL, NULL,
                                      0.000001, G_MAXDOUBLE, 1.0,
                                      G_PARAM_READWRITE));

        g_object_class_install_property (gobject_class,
                                         PROP_ZOOM,
                                         g_param_spec_double ("zoom",
                                                              NULL,
                                                              NULL,
                                                              -G_MAXDOUBLE,
                                                              G_MAXDOUBLE,
                                                              7.0,
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
                                      MRP_TYPE_RESOURCE, G_PARAM_READWRITE));

        g_object_class_install_property
                (gobject_class,
                 PROP_HIGHLIGHT,
                 g_param_spec_boolean ("highlight", NULL, NULL,
                                       FALSE, G_PARAM_READWRITE));

        object_class->destroy = usage_row_destroy;

        item_class->update = usage_row_update;
        item_class->realize = usage_row_realize;
        item_class->unrealize = usage_row_unrealize;
        item_class->draw = usage_row_draw;
        item_class->point = usage_row_point;
        item_class->bounds = usage_row_bounds;
}

static void
usage_row_init (PlannerUsageRow * row)
{
        PlannerUsageRowPriv *priv;

        row->priv = g_new0 (PlannerUsageRowPriv, 1);
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
}

static void
usage_row_destroy (GtkObject * object)
{
        PlannerUsageRow     *row;
        PlannerUsageRowPriv *priv;

        row = PLANNER_USAGE_ROW (object);
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
                (*GTK_OBJECT_CLASS (parent_class)->destroy) (object);
        }
}

static void
usage_row_get_bounds (PlannerUsageRow *row,
                       double           *px1,
		       double           *py1,
		       double           *px2,
		       double           *py2)
{
        GnomeCanvasItem *item;
        gdouble          wx1, wy1, wx2, wy2;
        gint             cx1, cy1, cx2, cy2;

        item = GNOME_CANVAS_ITEM (row);

        /* Get the items bbox in canvas pixel coordinates. */

        wx1 = row->priv->x - MILESTONE_SIZE - 1;
        wy1 = row->priv->y;
        wx2 = row->priv->x + MAX (row->priv->width, 2 * MILESTONE_SIZE) + 1;
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
get_assignment_bounds (MrpAssignment *assign,
                       gdouble        scale,
                       gdouble       *x_debut,
                       gdouble       *x_fin,
		       gdouble       *x_start_reel)
{
        MrpTask *task;
        mrptime  t;

        task = mrp_assignment_get_task (assign);
        t = mrp_task_get_work_start (task);
        *x_debut = t * scale;

        t = mrp_task_get_finish (task);
        *x_fin = t * scale;

        t = mrp_task_get_start (task);
        *x_start_reel = t * scale;
}

static void
get_resource_bounds (MrpResource *resource,
                     gdouble      scale,
                     gdouble     *x_debut,
                     gdouble     *x_fin,
		     gdouble     *x_start_reel)
{
        MrpProject *project;
        MrpTask    *root;
        mrptime     t;

        project = mrp_object_get_project (MRP_OBJECT (resource));

        t = mrp_project_get_project_start (project);

        *x_debut = t * scale;
        *x_start_reel = t * scale;

        root = mrp_project_get_root_task (project);

	t = mrp_task_get_finish (root);
        *x_fin = t * scale;
}

static gboolean
recalc_bounds (PlannerUsageRow *row)
{
	PlannerUsageRowPriv *priv;
	gint                  width;
	gdouble               x_debut, x_fin, x_debut_real;
	gdouble               old_x, old_x_start, old_width;
	gboolean              changed;

	priv = row->priv;

	old_x = priv->x;
	old_x_start = priv->x_start;
	old_width = priv->width;

	usage_row_ensure_layout (row);

	if (priv->layout != NULL) {
		pango_layout_get_pixel_size (priv->layout,
					     &width,
					     NULL);
	}
	else {
		width = 0;
	}

	if (width > 0) {
		width += TEXT_PADDING;
	}

	priv->text_width = width;

	if (priv->assignment) {
		get_assignment_bounds (priv->assignment, priv->scale,
				       &x_debut, &x_fin, &x_debut_real);
	}
	else if (priv->resource) {
		get_resource_bounds (priv->resource, priv->scale, &x_debut,
				     &x_fin, &x_debut_real);
	}

	priv->x = x_debut;
	priv->width = x_fin - x_debut;
	priv->x_start = x_debut_real;

	changed = (old_x != priv->x || old_x_start != priv->x_start ||
		   old_width != priv->width);

	return changed;
}

static void
usage_row_set_property (GObject      *object,
                         guint         param_id,
                         const GValue *value,
			 GParamSpec   *pspec)
{
        PlannerUsageRowPriv *priv;
        GnomeCanvasItem      *item;
        PlannerUsageRow     *row;
        gboolean              changed = FALSE;
        gfloat                tmp_scale;
        gdouble               tmp_dbl;
        MrpTask              *task;

        item = GNOME_CANVAS_ITEM (object);
        row = PLANNER_USAGE_ROW (object);
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
                if (priv->resource != NULL) {
                        /* FIXME: check handlers to modify !!! */
                        g_object_unref (priv->resource);
                }
                if (g_value_get_object (value) == NULL) {
                        priv->resource = NULL;
                } else {
                        GList *a;
                        priv->resource = g_object_ref (g_value_get_object (value));
                        g_signal_connect_object (priv->resource, "notify",
                                                 G_CALLBACK
                                                 (usage_row_resource_notify_cb),
                                                 row, 0);
                        g_signal_connect_object (priv->resource,
                                                 "assignment_added",
                                                 G_CALLBACK
                                                 (usage_row_resource_assignment_added_cb),
                                                 row, 0);
                        a = mrp_resource_get_assignments (priv->resource);
                        for (; a; a = a->next) {
                                MrpAssignment *assign;
                                MrpTask       *tmp_task;

                                assign = a->data;

				tmp_task = mrp_assignment_get_task (assign);
                                g_signal_connect_object (assign,
                                                         "notify",
                                                         G_CALLBACK (usage_row_assignment_notify_cb),
                                                         row, 0);

                                g_signal_connect_object (tmp_task,
                                                         "notify",
                                                         G_CALLBACK (usage_row_task_notify_cb),
                                                         row, 0);
                        }
                }
                /*
                 * g_signal_connect_object (priv->resource,
                 * "assignment-added",
                 * G_CALLBACK (usage_row_res_assignment_added),
                 * row,
                 * 0);
                 *
                 * g_signal_connect_object (priv->resource,
                 * "assignment-removed",
                 * G_CALLBACK (usage_row_res_assignment_removed),
                 * row,
                 * 0);
                 */
                changed = TRUE;
                break;


        case PROP_ASSIGNMENT:
                if (priv->assignment != NULL) {
                        /* gantt_row_disconnect_all_resources (priv->task, row); */
                        g_object_unref (priv->assignment);
                        /* FIXME: Disconnect notify handlers. */
                }
                if (g_value_get_object (value) == NULL) {
                        priv->assignment = NULL;
                } else {
                        MrpTaskSched sched;

                        priv->assignment = g_object_ref (g_value_get_object (value));
                        task = mrp_assignment_get_task (priv->assignment);

                        sched = mrp_task_get_sched (task);
                        if (sched == MRP_TASK_SCHED_FIXED_DURATION) {
                                priv->fixed_duration = 1;
                        } else {
                                priv->fixed_duration = 0;
                        }

                        g_signal_connect_object (priv->assignment,
                                                 "notify",
                                                 G_CALLBACK (usage_row_assignment_notify_cb),
                                                 row, 0);

                        g_signal_connect_object (task,
                                                 "notify",
                                                 G_CALLBACK (usage_row_task_notify_cb),
                                                 row, 0);
                }

                /* usage_row_connect_all_resources (priv->assignment, row); */

                changed = TRUE;
                break;

        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
                break;
        }

        if (changed) {
                recalc_bounds (row);
                usage_row_geometry_changed (row);
                gnome_canvas_item_request_update (item);
        }
}

static void
usage_row_get_property (GObject    *object,
                         guint       param_id,
			 GValue     *value,
			 GParamSpec *pspec)
{
        PlannerUsageRow     *row;
        PlannerUsageRowPriv *priv;

        row = PLANNER_USAGE_ROW (object);
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

        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
                break;
        }
}

static void
usage_row_ensure_layout (PlannerUsageRow *row)
{
	if (row->priv->assignment != NULL && row->priv->layout == NULL) {
		row->priv->layout = gtk_widget_create_pango_layout (
			GTK_WIDGET (GNOME_CANVAS_ITEM (row)->canvas), NULL);

		usage_row_update_resources (row);
	}
}

static void
usage_row_update_resources (PlannerUsageRow *row)
{
	PlannerUsageRowPriv *priv;
	gint                  units;
	gchar                *units_string;

	priv = row->priv;

	units = mrp_assignment_get_units (priv->assignment);
	units_string = g_strdup_printf ("%i%%", units);
	pango_layout_set_text (priv->layout, units_string, -1);

	g_free (units_string);
}

static void
usage_row_update (GnomeCanvasItem *item,
                   double          *affine,
		   ArtSVP          *clip_path,
		   gint             flags)
{
        PlannerUsageRow *row;
        double            x1, y1, x2, y2;

        row = PLANNER_USAGE_ROW (item);

        GNOME_CANVAS_ITEM_CLASS (parent_class)->update (item,
                                                        affine,
                                                        clip_path, flags);

        usage_row_ensure_layout (row);
        usage_row_get_bounds (row, &x1, &y1, &x2, &y2);

        gnome_canvas_update_bbox (item, x1, y1, x2, y2);
}

static void
usage_row_realize (GnomeCanvasItem * item)
{
        PlannerUsageRow     *row;
        PlannerUsageRowPriv *priv;

        row = PLANNER_USAGE_ROW (item);
        priv = row->priv;

        GNOME_CANVAS_ITEM_CLASS (parent_class)->realize (item);

        if (complete_stipple == NULL) {
                complete_stipple = gdk_bitmap_create_from_data (NULL,
                                                                complete_stipple_pattern,
                                                                2, 2);

                g_object_add_weak_pointer (G_OBJECT (complete_stipple),
                                           (gpointer) & complete_stipple);

		gnome_canvas_get_color (item->canvas, "LightSkyBlue3", &color_normal);
		gnome_canvas_get_color (item->canvas, "#9ac7e0", &color_normal_light);
		gnome_canvas_get_color (item->canvas, "#7da1b5", &color_normal_dark);

		gnome_canvas_get_color (item->canvas, "indian red", &color_overuse);
		gnome_canvas_get_color (item->canvas, "#de6464", &color_overuse_light);
		gnome_canvas_get_color (item->canvas, "#ba5454", &color_overuse_dark);

		gnome_canvas_get_color (item->canvas, "grey", &color_underuse);
		gnome_canvas_get_color (item->canvas, "#d6d6d6", &color_underuse_light);
		gnome_canvas_get_color (item->canvas, "#a8a8a8", &color_underuse_dark);

		gnome_canvas_get_color (item->canvas, "medium sea green", &color_free);
		gnome_canvas_get_color (item->canvas, "#43c77e", &color_free_light);
		gnome_canvas_get_color (item->canvas, "#359e64", &color_free_dark);
	} else {
                g_object_ref (complete_stipple);
        }

        if (break_stipple == NULL) {
                break_stipple = gdk_bitmap_create_from_data (NULL,
                                                             break_stipple_pattern,
                                                             6, 1);

                g_object_add_weak_pointer (G_OBJECT (break_stipple),
                                           (gpointer) & break_stipple);
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
        gdk_gc_set_line_attributes (priv->frame_gc,
                                    0,
                                    GDK_LINE_SOLID,
                                    GDK_CAP_BUTT, GDK_JOIN_MITER);
}

static void
usage_row_unrealize (GnomeCanvasItem * item)
{
        PlannerUsageRow *row;

        row = PLANNER_USAGE_ROW (item);

        gdk_gc_unref (row->priv->complete_gc);
        row->priv->complete_gc = NULL;

        gdk_gc_unref (row->priv->break_gc);
        row->priv->break_gc = NULL;

        gdk_gc_unref (row->priv->fill_gc);
        row->priv->fill_gc = NULL;

	gdk_gc_unref (row->priv->frame_gc);
        row->priv->frame_gc = NULL;

        if (break_stipple) {
                g_object_unref (break_stipple);
        }

        if (complete_stipple) {
                g_object_unref (complete_stipple);
        }

        GNOME_CANVAS_ITEM_CLASS (parent_class)->unrealize (item);
}

typedef enum {
        START_ASSIGN,
        END_ASSIGN
} date_type;

typedef struct {
        date_type type;
        mrptime time;
        gint units;
        MrpAssignment *assignment;
        MrpTask *task;
} Date;

static gint
usage_row_date_compare (gconstpointer date1,
			 gconstpointer date2)
{
        const Date *a, *b;

	a = date1;
        b = date2;

	if (a->time < b->time) {
                return -1;
        }
	else if (a->time == b->time) {
                if (a->type < b->type) {
                        return -1;
                }
		else if (a->type == b->type) {
                        return 0;
                } else {
                        return 1;
                }
        } else {
                return 1;
        }
}

typedef enum {
        ROW_MIDDLE = 0,
        ROW_START  = 1 << 0,
        ROW_END    = 1 << 1,
        ROW_WHOLE  = ROW_START | ROW_END
} RowChunk;

static void
usage_row_draw_resource_ival (mrptime          start,
                               mrptime          end,
                               gint             units,
                               RowChunk         chunk,
                               GdkDrawable     *drawable,
                               GnomeCanvasItem *item,
                               gint             x,
			       gint             y,
			       gint             width,
			       gint             height)
{
        PlannerUsageRow     *row;
        PlannerUsageRowPriv *priv;

        /* World coord */
        gdouble               xoffset, yoffset;
        gdouble               xstart, ystart, xend, yend;

        /* Canvas coord */
        /*
         * c_XXX: global
         * cr_XXX: central rectangle
         * cs_XXX: shadow
         */
        gint                  c_xstart, c_ystart, c_xend, c_yend;
        gint                  cr_xstart, cr_ystart, cr_xend, cr_yend;
        gint                  cs_xstart, cs_ystart, cs_xend, cs_yend;

        /* Real coord (in the exposed area) */
        gint                  r_xstart, r_ystart, r_xend, r_yend;
        gint                  rr_xstart, rr_ystart, rr_xend, rr_yend;
        gint                  rs_xstart, rs_ystart, rs_xend, rs_yend;

        row = PLANNER_USAGE_ROW (item);
        priv = row->priv;

        /* Compute offset in world coord */
        xoffset = 0.0;
        yoffset = 0.0;
        gnome_canvas_item_i2w (item, &xoffset, &yoffset);

        /* world coordinates */
        xstart = start * priv->scale;
        ystart = priv->y + 0.15 * priv->height;
        xend = end * priv->scale;
        yend = priv->y + 0.70 * priv->height;

        /* Convert to canvas */
        gnome_canvas_w2c (item->canvas,
                          xstart + xoffset,
                          ystart + yoffset, &c_xstart, &c_ystart);
        gnome_canvas_w2c (item->canvas,
                          xend + xoffset, yend + yoffset, &c_xend, &c_yend);

        /* Shift by (x,y) */
        c_xstart -= x;
        c_xend -= x;
        c_ystart -= y;
        c_yend -= y;
        /*
         * c_xstart -= 2;
         * c_xend += 2;
         */

        /* Compute shadow coord: */
        cs_xstart = c_xstart + 1;
        cs_ystart = c_ystart + 1;
        cs_xend = c_xend - 1;
        cs_yend = c_yend - 1;

        /* Compute rectangle coord: */
        cr_xstart = c_xstart;
        cr_ystart = cs_ystart + 1;
        cr_xend = c_xend;
        cr_yend = cs_yend - 1;

        /* Clip to the expose area */
	r_xstart = MAX (c_xstart, 0);
        r_xend = MIN (c_xend, width);
        r_ystart = MAX (c_ystart, 0);
        r_yend = MIN (c_yend, height);

        rs_xstart = MAX (cs_xstart, 0);
        rs_xend = MIN (cs_xend, width);
        rs_ystart = MAX (cs_ystart, 0);
        rs_yend = MIN (cs_yend, height);

        rr_xstart = MAX (cr_xstart, 0);
        rr_xend = MIN (cr_xend, width);
        rr_ystart = MAX (cr_ystart, 0);
        rr_yend = MIN (cr_yend, height);

        if (r_xstart > r_xend || r_ystart > r_yend) {
                /* Nothing to draw */
                return;
        }

        if (units == 0) {
                gdk_gc_set_foreground (priv->fill_gc, &color_free);
        }
	else if (units < 100) {
                gdk_gc_set_foreground (priv->fill_gc, &color_underuse);
        }
	else if (units == 100) {
                gdk_gc_set_foreground (priv->fill_gc, &color_normal);
        } else {
                gdk_gc_set_foreground (priv->fill_gc, &color_overuse);
        }

        /* Draw the central part of the chunk */
	if (rr_xend >= rr_xstart && rr_yend >= rr_ystart) {
		gdk_draw_rectangle (drawable,
				    priv->fill_gc,
				    TRUE,
				    rr_xstart, rr_ystart,
				    rr_xend - rr_xstart + 1, rr_yend - rr_ystart + 1);
	}

        if (units == 0) {
                gdk_gc_set_foreground (priv->fill_gc, &color_free_light);
        }
	else if (units < 100) {
                gdk_gc_set_foreground (priv->fill_gc, &color_underuse_light);
        }
	else if (units == 100) {
                gdk_gc_set_foreground (priv->fill_gc, &color_normal_light);
        } else {
                gdk_gc_set_foreground (priv->fill_gc, &color_overuse_light);
        }

        //gdk_gc_set_foreground (priv->fill_gc, &color_high);

        /* Top of the shadow. */
        if (cs_ystart == rs_ystart) {
                gdk_draw_line (drawable, priv->fill_gc, r_xstart, rs_ystart,
                               r_xend, rs_ystart);
        }

        /* Left of the shadow. */
        if (chunk & ROW_START && cs_xstart == rs_xstart) {
                gdk_draw_line (drawable, priv->fill_gc, rs_xstart, rs_ystart,
                               rs_xstart, cs_yend);
        }

	if (units == 0) {
                gdk_gc_set_foreground (priv->fill_gc, &color_free_dark);
        }
	else if (units < 100) {
                gdk_gc_set_foreground (priv->fill_gc, &color_underuse_dark);
        }
	else if (units == 100) {
                gdk_gc_set_foreground (priv->fill_gc, &color_normal_dark);
        } else {
                gdk_gc_set_foreground (priv->fill_gc, &color_overuse_dark);
        }

	//gdk_gc_set_foreground (priv->fill_gc, &color_shadow);

        /* Bottom of the shadow. */
        if (cs_yend == rs_yend) {
                gdk_draw_line (drawable, priv->fill_gc, r_xstart, rs_yend,
                               r_xend, rs_yend);
        }

        /* Right of the shadow. */
        if (chunk & ROW_END && cs_xend == rs_xend) {
                gdk_draw_line (drawable, priv->fill_gc, rs_xend, rs_ystart,
                               rs_xend, cs_yend);
        }

	/* Interval separator. */
	if (!(chunk & ROW_START)) {
		gdk_gc_set_foreground (priv->fill_gc, &GTK_WIDGET (item->canvas)->style->white);
		gdk_draw_line (drawable, priv->fill_gc, c_xstart, rs_ystart, c_xstart,
                               rr_yend);
	}

        /* Top frame. */
        if (c_ystart == r_ystart) {
                gdk_draw_line (drawable, priv->frame_gc, r_xstart, r_ystart, r_xend,
                               r_ystart);
	}

        /* Bottom frame. */
        if (c_yend == r_yend) {
                gdk_draw_line (drawable, priv->frame_gc, r_xstart, r_yend, r_xend,
                               r_yend);
	}

        /* Left frame. */
        if (chunk & ROW_START && c_xstart == r_xstart) {
                gdk_draw_line (drawable, priv->frame_gc, r_xstart, r_ystart,
                               r_xstart, r_yend);
	}

        /* Right frame. */
        if (chunk & ROW_END && c_xend == r_xend) {
                gdk_draw_line (drawable, priv->frame_gc, r_xend, r_ystart, r_xend,
                               r_yend);
	}
}

static void
usage_row_draw_resource (PlannerUsageRow *row,
                          GdkDrawable      *drawable,
                          GnomeCanvasItem  *item,
                          gint              x,
			  gint              y,
			  gint              width,
			  gint              height)
{
        GList         *dates;
        MrpResource   *resource;
        MrpTask       *root;
        MrpAssignment *assignment;
        MrpTask       *task;
        MrpProject    *project;
        GList         *assignments;
        GList         *a, *d;
        Date          *date, *date0, *date1;
        mrptime        work_start, finish, previous_time;
        gint           units;
        RowChunk       chunk;

        resource = row->priv->resource;

        dates = NULL;

	project = mrp_object_get_project (MRP_OBJECT (resource));

	assignments = mrp_resource_get_assignments (resource);
        for (a = assignments; a; a = a->next) {
                assignment = a->data;

                task = mrp_assignment_get_task (assignment);
                work_start = mrp_task_get_work_start (task);
                finish = mrp_task_get_finish (task);

		units = mrp_assignment_get_units (assignment);

		date0 = g_new0 (Date, 1);
                date0->type = START_ASSIGN;
                date0->time = work_start;
                date0->units = units;
                date0->assignment = assignment;
                date0->task = task;

                date1 = g_new0 (Date, 1);
                date1->type = END_ASSIGN;
                date1->time = finish;
                date1->units = units;
                date1->assignment = assignment;
                date1->task = task;
                dates = g_list_insert_sorted (dates, date0, usage_row_date_compare);
                dates = g_list_insert_sorted (dates, date1, usage_row_date_compare);
        }

        units = 0;
        previous_time = mrp_project_get_project_start (project);

	root = mrp_project_get_root_task (project);
	finish = mrp_task_get_finish (root);

        chunk = ROW_START;

        for (d = dates; d; d = d->next) {
                date = d->data;

                if (date->time != previous_time) {
                        if (date->time == finish) {
				chunk |= ROW_END;
			}

                        usage_row_draw_resource_ival (previous_time,
                                                       date->time,
                                                       units,
                                                       chunk,
                                                       drawable, item,
                                                       x, y, width, height);

                        chunk &= ~ROW_START;
                        previous_time = date->time;
                }

                if (date->type == START_ASSIGN) {
                        units += date->units;
                } else {
                        units -= date->units;
                }
                g_free (date);
        }
        g_list_free (dates);

        if (!(chunk & ROW_END)) {
		chunk |= ROW_END;
                usage_row_draw_resource_ival (previous_time,
                                               finish,
                                               units,
                                               chunk,
                                               drawable, item,
                                               x, y, width, height);
        }
}

static void
usage_row_draw_assignment (PlannerUsageRow *row,
                            MrpAssignment    *assign,
                            GnomeCanvasItem  *item,
                            GdkDrawable      *drawable,
                            gint              x,
			    gint              y,
			    gint              width,
			    gint              height)
{
        PlannerUsageRowPriv *priv;
        MrpTask              *task;
        gdouble               i2w_dx;
        gdouble               i2w_dy;
        gdouble               dx1, dy1, dx2, dy2;
        gboolean              summary;
        gint                  percent_complete;
        gint                  complete_x2, complete_width;
        gint                  rx1;
        gint                  rx2;
        gint                  cx1, cy1, cx2, cy2;
        gdouble               ass_x, ass_xend, ass_x_start;

        priv = row->priv;
        task = mrp_assignment_get_task (assign);

        /* Get item area in canvas coordinates. */
        i2w_dx = 0.0;
        i2w_dy = 0.0;
        gnome_canvas_item_i2w (item, &i2w_dx, &i2w_dy);

        get_assignment_bounds (assign, priv->scale, &ass_x, &ass_xend, &ass_x_start);
        dx1 = ass_x;
        dy1 = priv->y + 0.15 * priv->height;
        dx2 = ass_xend;
        dy2 = priv->y + 0.70 * priv->height;

        gnome_canvas_w2c (item->canvas,
                          dx1 + i2w_dx, dy1 + i2w_dy, &cx1, &cy1);

        gnome_canvas_w2c (item->canvas,
                          dx2 + i2w_dx, dy2 + i2w_dy, &cx2, &cy2);

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

        summary = (mrp_task_get_n_children (task) > 0);
        complete_width = 0;
        complete_x2 = 0;

        if (!summary) {
                percent_complete = mrp_task_get_percent_complete (task);

		complete_width =  floor ((cx2 - cx1) * (percent_complete / 100.0) + 0.5);
                complete_x2 = MIN (cx1 + complete_width, rx2);
        }

        if (rx1 <= rx2) {
                if (complete_width > 0) {
                        gnome_canvas_set_stipple_origin (item->canvas,
                                                         priv->complete_gc);
                }

                gdk_gc_set_foreground (priv->fill_gc, &color_normal);

                gdk_draw_rectangle (drawable,
                                    priv->fill_gc,
                                    TRUE,
                                    rx1, cy1 + 1, rx2 - rx1, cy2 - cy1 - 1);

                if (rx1 <= complete_x2) {
                        gdk_draw_rectangle (drawable,
                                            priv->complete_gc,
                                            TRUE,
                                            rx1,
                                            cy1 + 4,
                                            complete_x2 - rx1, cy2 - cy1 - 7);
                }

                gdk_draw_line (drawable, priv->frame_gc, rx1, cy1, rx2, cy1);
                gdk_draw_line (drawable, priv->frame_gc, rx1, cy2, rx2, cy2);

                gdk_gc_set_foreground (priv->fill_gc, &color_normal_light);
                gdk_draw_line (drawable,
                               priv->fill_gc,
                               rx1 + 0, cy1 + 1, rx2 - 0, cy1 + 1);

                if (cx1 == rx1) {
                        gdk_draw_line (drawable,
                                       priv->fill_gc,
                                       rx1 + 1, cy1 + 1, rx1 + 1, cy2 - 1);
                }

                gdk_gc_set_foreground (priv->fill_gc, &color_normal_dark);
                gdk_draw_line (drawable,
                               priv->fill_gc,
                               rx1 + 0, cy2 - 1, rx2 - 0, cy2 - 1);

                if (cx2 == rx2) {
                        gdk_draw_line (drawable,
                                       priv->fill_gc,
                                       rx2 - 1, cy1 + 1, rx2 - 1, cy2 - 1);
                }

                if (cx1 == rx1) {
                        gdk_draw_line (drawable,
                                       priv->frame_gc, rx1, cy1, rx1, cy2);
                }

                if (cx2 == rx2) {
                        gdk_draw_line (drawable,
                                       priv->frame_gc, rx2, cy1, rx2, cy2);
                }
        }

        rx1 = MAX (cx2 + TEXT_PADDING, 0);
        rx2 = MIN (cx2 + TEXT_PADDING + priv->text_width, width);

        if (priv->layout != NULL && rx1 < rx2) {
		/* FIXME: Center the text vertically? */
		gdk_draw_layout (drawable,
				 GTK_WIDGET (item->canvas)->style->text_gc[GTK_STATE_NORMAL],
				 cx2 + TEXT_PADDING,
				 cy1,
				 priv->layout);

        }
}

static void
usage_row_draw (GnomeCanvasItem *item,
                 GdkDrawable     *drawable,
                 gint             x,
		 gint             y,
		 gint             width,
		 gint             height)
{
        PlannerUsageRow *row;

        row = PLANNER_USAGE_ROW (item);

	if (row->priv->assignment) {
                usage_row_draw_assignment (row,
                                            row->priv->assignment,
                                            item,
                                            drawable, x, y, width, height);
        }
	else if (row->priv->resource) {
                usage_row_draw_resource (row, drawable, item, x, y, width, height);
        }
}

static double
usage_row_point (GnomeCanvasItem  *item,
                  double            x,
                  double            y,
		  gint              cx,
		  gint              cy,
		  GnomeCanvasItem **actual_item)
{
        PlannerUsageRow     *row;
        PlannerUsageRowPriv *priv;
        gint                  text_width;
        gdouble               x1, y1, x2, y2;
        gdouble               dx, dy;

        row = PLANNER_USAGE_ROW (item);
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
usage_row_bounds (GnomeCanvasItem *item,
                   double          *x1,
		   double          *y1,
		   double          *x2,
		   double          *y2)
{
        PlannerUsageRow *row;

        row = PLANNER_USAGE_ROW (item);

        usage_row_get_bounds (row, x1, y1, x2, y2);

        if (GNOME_CANVAS_ITEM_CLASS (parent_class)->bounds) {
                GNOME_CANVAS_ITEM_CLASS (parent_class)->bounds (item, x1, y1,
                                                                x2, y2);
        }
}

static void
usage_row_resource_notify_cb (MrpResource      *resource,
			       GParamSpec       *pspec,
                               PlannerUsageRow *row)
{
        if (!recalc_bounds (row)) {
		return;
	}

        usage_row_geometry_changed (row);
        gnome_canvas_item_request_update (GNOME_CANVAS_ITEM (row));
}

static void
usage_row_resource_assignment_added_cb (MrpResource      *resource,
                                         MrpAssignment    *assign,
                                         PlannerUsageRow *row)
{
        MrpTask *task;

        task = mrp_assignment_get_task (assign);

	g_signal_connect_object (assign,
                                 "notify",
                                 G_CALLBACK (usage_row_assignment_notify_cb),
                                 row, 0);

        g_signal_connect_object (task,
                                 "notify",
                                 G_CALLBACK (usage_row_task_notify_cb),
                                 row, 0);
        recalc_bounds (row);
        usage_row_geometry_changed (row);
        gnome_canvas_item_request_update (GNOME_CANVAS_ITEM (row));
}

static void
usage_row_assignment_notify_cb (MrpAssignment    *assignment,
                                 GParamSpec       *pspec,
				 PlannerUsageRow *row)
{
        if (!recalc_bounds (row)) {
		return;
	}

        usage_row_geometry_changed (row);
        gnome_canvas_item_request_update (GNOME_CANVAS_ITEM (row));
}

static void
usage_row_task_notify_cb (MrpTask          *task,
			   GParamSpec       *pspec,
                           PlannerUsageRow *row)
{
        MrpTaskSched sched;

        sched = mrp_task_get_sched (task);

	if (sched == MRP_TASK_SCHED_FIXED_DURATION) {
                row->priv->fixed_duration = TRUE;
        } else {
                row->priv->fixed_duration = FALSE;
        }

        if (!recalc_bounds (row)) {
		return;
	}

        usage_row_geometry_changed (row);
        gnome_canvas_item_request_update (GNOME_CANVAS_ITEM (row));
}

/* Returns the geometry of the actual bars, not the bounding box, not including
 * the text labels.
 */
void
planner_usage_row_get_geometry (PlannerUsageRow *row,
                                 gdouble          *x1,
                                 gdouble          *y1,
				 gdouble          *x2,
				 gdouble          *y2)
{
        PlannerUsageRowPriv *priv;

        g_return_if_fail (PLANNER_IS_USAGE_ROW (row));

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
planner_usage_row_set_visible (PlannerUsageRow *row, gboolean is_visible)
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

        g_signal_emit (row, signals[VISIBILITY_CHANGED], 0, is_visible);
}

static void
usage_row_geometry_changed (PlannerUsageRow * row)
{
        gdouble x1, y1, x2, y2;

        if (row->priv->assignment) {
		usage_row_update_resources (row);
        }

        x1 = row->priv->x;
        y1 = row->priv->y;
        x2 = x1 + row->priv->width;
        y2 = y1 + row->priv->height;

        g_signal_emit (row, signals[GEOMETRY_CHANGED], 0, x1, y1, x2, y2);
}

static gboolean
usage_row_event (GnomeCanvasItem *item, GdkEvent *event)
{
	PlannerUsageRow     *row;
	PlannerUsageRowPriv *priv;
	PlannerUsageChart   *chart;
	GtkTreePath         *path;
	GtkTreeSelection    *selection;
	PlannerUsageTree    *tree;
	GtkTreeView         *tree_view;
	GtkTreeIter          iter;

	row = PLANNER_USAGE_ROW (item);
	priv = row->priv;
	chart = g_object_get_data (G_OBJECT (item->canvas), "chart");
	tree = planner_usage_chart_get_view (chart);

	switch (event->type) {
	case GDK_BUTTON_PRESS:
		if (priv->assignment != NULL) {
			path = planner_usage_model_get_path_from_assignment
				(PLANNER_USAGE_MODEL (planner_usage_chart_get_model (chart)),
				 priv->assignment);
		}
		else if (priv->resource != NULL) {
			path = planner_usage_model_get_path_from_resource
				(PLANNER_USAGE_MODEL (planner_usage_chart_get_model (chart)),
				 priv->resource);
		}
		else {
			break;
		}

		tree_view = GTK_TREE_VIEW (tree);
		selection = gtk_tree_view_get_selection (tree_view);

		gtk_tree_model_get_iter (gtk_tree_view_get_model (tree_view),
					 &iter, path);

		if (!gtk_tree_selection_iter_is_selected (selection, &iter)) {
			gtk_tree_selection_unselect_all (selection);
			gtk_tree_selection_select_path (selection, path);
		}

		break;

	case GDK_2BUTTON_PRESS:
		if (event->button.button == 1) {
			if (priv->assignment != NULL) {
				planner_usage_tree_edit_task (tree);
			}
			else if (priv->resource != NULL) {
				planner_usage_tree_edit_resource (tree);
			}
		}

		break;

	default:
		break;
	}



	if (TRUE) {
		return TRUE;
	}

	return FALSE;
}
