#ifndef __MG_TTABLE_ROW_H__
#define __MG_TTABLE_ROW_H__

#include <gtk/gtk.h>
#include <libgnomecanvas/gnome-canvas.h>
#include <libgnomecanvas/gnome-canvas-util.h>

#define MG_TYPE_TTABLE_ROW            (planner_ttable_row_get_type ())
#define MG_TTABLE_ROW(obj)            (GTK_CHECK_CAST ((obj), MG_TYPE_TTABLE_ROW, MgTtableRow))
#define MG_TTABLE_ROW_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), MG_TYPE_TTABLE_ROW, MgTtableRowClass))
#define MG_IS_TTABLE_ROW(obj)         (GTK_CHECK_TYPE ((obj), MG_TYPE_TTABLE_ROW))
#define MG_IS_TTABLE_ROW_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), MG_TYPE_TTABLE_ROW))
#define MG_TTABLE_ROW_GET_CLASS(obj)  (GTK_CHECK_GET_CLASS ((obj), MG_TYPE_TTABLE_ROW, MgTtableRowClass))


typedef struct _MgTtableRow      MgTtableRow;
typedef struct _MgTtableRowClass MgTtableRowClass;
typedef struct _MgTtableRowPriv  MgTtableRowPriv;

struct _MgTtableRow {
	GnomeCanvasItem parent;
	MgTtableRowPriv *priv;
};

struct _MgTtableRowClass {
	GnomeCanvasItemClass prent_class;
};

GType planner_ttable_row_get_type (void) G_GNUC_CONST;
void  planner_ttable_row_get_geometry (MgTtableRow *row,
				  gdouble *x1, gdouble *y1,
				  gdouble *x2, gdouble *y2);
void  planner_ttable_row_set_visible  (MgTtableRow *row, gboolean is_visible);

#endif
