#ifndef __MG_TTABLE_CHART_H__
#define __MG_TTABLE_CHART_H__

#include <gtk/gtkwidget.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtktreemodel.h>
#include <libgnomecanvas/gnome-canvas.h>

#define MG_TYPE_TTABLE_CHART             (planner_ttable_chart_get_type ())
#define MG_TTABLE_CHART(obj)             (GTK_CHECK_CAST ((obj), MG_TYPE_TTABLE_CHART, MgTtableChart))
#define MG_TTABLE_CHART_CLASS(klass)     (GTK_CHECK_CLASS_CAST ((klass), MG_TYPE_TTABLE_CHART, MgTtableChartClass))
#define MG_IS_TTABLE_CHART(obj)          (GTK_CHECK_TYPE ((obj), MG_TYPE_TTABLE_CHART))
#define MG_IS_TTABLE_CHART_CLASS(klass)  (GTK_CHECK_CLASS_TYPE ((obj), MG_TYPE_TTABLE_CHART))
#define MG_TTABLE_CHART_GET_CLASS(obj)   (GTK_CHECK_GET_CLASS ((obj), MG_TYPE_TTABLE_CHART, MgTtableChartClass))

typedef struct _MgTtableChart           MgTtableChart;
typedef struct _MgTtableChartClass      MgTtableChartClass;
typedef struct _MgTtableChartPriv       MgTtableChartPriv;

struct _MgTtableChart {
	GtkVBox			parent_class;
	MgTtableChartPriv	*priv;
};

struct _MgTtableChartClass {
	GtkVBoxClass	parent_class;
	void (*set_scroll_adjustments) (MgTtableChart *chart,
				        GtkAdjustment *hadj,
				        GtkAdjustment *vadj);
};

GType		 planner_ttable_chart_get_type	(void);
GtkWidget	*planner_ttable_chart_new		(void);
GtkWidget	*planner_ttable_chart_new_with_model	(GtkTreeModel	*model);
GtkTreeModel	*planner_ttable_chart_get_model	(MgTtableChart	*chart);
void		 planner_ttable_chart_set_model	(MgTtableChart	*chart,
						 GtkTreeModel	*model);
void		 planner_ttable_chart_expand_row	(MgTtableChart	*chart,
						 GtkTreePath	*path);
void		 planner_ttable_chart_collapse_row	(MgTtableChart	*chart,
						 GtkTreePath	*path);
void		 planner_ttable_chart_expand_all	(MgTtableChart	*chart);
void		 planner_ttable_chart_collapse_all	(MgTtableChart	*chart);
void		 planner_ttable_chart_zoom_in	(MgTtableChart	*chart);
void		 planner_ttable_chart_zoom_out	(MgTtableChart	*chart);
void		 planner_ttable_chart_can_zoom	(MgTtableChart	*chart,
		                                 gboolean	*in,
			                         gboolean	*out);
void		 planner_ttable_chart_zoom_to_fit    (MgTtableChart	*chart);
gdouble		 planner_ttable_chart_get_zoom       (MgTtableChart	*chart);
void		 planner_ttable_chart_status_updated	(MgTtableChart	*chart,
						 gchar		*message);
#endif
