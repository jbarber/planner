#ifndef __PLANNER_TTABLE_ROW_H__
#define __PLANNER_TTABLE_ROW_H__

#include <gtk/gtk.h>
#include <libgnomecanvas/gnome-canvas.h>
#include <libgnomecanvas/gnome-canvas-util.h>

#define PLANNER_TYPE_TTABLE_ROW            (planner_ttable_row_get_type ())
#define PLANNER_TTABLE_ROW(obj)            (GTK_CHECK_CAST ((obj), PLANNER_TYPE_TTABLE_ROW, PlannerTtableRow))
#define PLANNER_TTABLE_ROW_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), PLANNER_TYPE_TTABLE_ROW, PlannerTtableRowClass))
#define PLANNER_IS_TTABLE_ROW(obj)         (GTK_CHECK_TYPE ((obj), PLANNER_TYPE_TTABLE_ROW))
#define PLANNER_IS_TTABLE_ROW_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), PLANNER_TYPE_TTABLE_ROW))
#define PLANNER_TTABLE_ROW_GET_CLASS(obj)  (GTK_CHECK_GET_CLASS ((obj), PLANNER_TYPE_TTABLE_ROW, PlannerTtableRowClass))


typedef struct _PlannerTtableRow      PlannerTtableRow;
typedef struct _PlannerTtableRowClass PlannerTtableRowClass;
typedef struct _PlannerTtableRowPriv  PlannerTtableRowPriv;

struct _PlannerTtableRow {
	GnomeCanvasItem parent;
	PlannerTtableRowPriv *priv;
};

struct _PlannerTtableRowClass {
	GnomeCanvasItemClass prent_class;
};

GType planner_ttable_row_get_type (void) G_GNUC_CONST;
void  planner_ttable_row_get_geometry (PlannerTtableRow *row,
				  gdouble *x1, gdouble *y1,
				  gdouble *x2, gdouble *y2);
void  planner_ttable_row_set_visible  (PlannerTtableRow *row, gboolean is_visible);

#endif
