#ifndef __MG_CELL_RENDERER_POPUP_H__
#define __MG_CELL_RENDERER_POPUP_H__

#include <pango/pango.h>
#include <gtk/gtkcellrenderertext.h>

#define MG_TYPE_CELL_RENDERER_POPUP		(planner_cell_renderer_popup_get_type ())
#define MG_CELL_RENDERER_POPUP(obj)		(GTK_CHECK_CAST ((obj), MG_TYPE_CELL_RENDERER_POPUP, MgCellRendererPopup))
#define MG_CELL_RENDERER_POPUP_CLASS(klass)	(GTK_CHECK_CLASS_CAST ((klass), MG_TYPE_CELL_RENDERER_POPUP, MgCellRendererPopupClass))
#define MG_IS_CELL_RENDERER_POPUP(obj)		(GTK_CHECK_TYPE ((obj), MG_TYPE_CELL_RENDERER_POPUP))
#define MG_IS_CELL_RENDERER_POPUP_CLASS(klass)	(GTK_CHECK_CLASS_TYPE ((obj), MG_TYPE_CELL_RENDERER_POPUP))
#define MG_CELL_RENDERER_POPUP_GET_CLASS(obj)   (GTK_CHECK_GET_CLASS ((obj), MG_TYPE_CELL_RENDERER_POPUP, MgCellRendererPopupClass))

typedef struct _MgCellRendererPopup      MgCellRendererPopup;
typedef struct _MgCellRendererPopupClass MgCellRendererPopupClass;

struct _MgCellRendererPopup
{
	GtkCellRendererText  parent;

	/* Cached width of the popup button. */
	gint                 button_width;
	
	/* The popup window. */
	GtkWidget           *popup_window;

	/* The widget that should grab focus on popup. */
	GtkWidget           *focus_window;

	/* The editable entry. */
	GtkWidget           *editable;

	gboolean             shown;
	gboolean             editing_canceled;
};

struct _MgCellRendererPopupClass
{
	GtkCellRendererTextClass parent_class;
	
	void   (* show_popup) (MgCellRendererPopup *cell,
			       const gchar         *path,
			       gint                 x1,
			       gint                 y1,
			       gint                 x2,
			       gint                 y2);
	
	void   (* hide_popup) (MgCellRendererPopup *cell);
};

GtkType          planner_cell_renderer_popup_get_type (void);

GtkCellRenderer *planner_cell_renderer_popup_new      (void);

void             planner_cell_renderer_popup_show     (MgCellRendererPopup *cell,
						  const gchar         *path,
						  gint                 x1,
						  gint                 y1,
						  gint                 x2,
						  gint                 y2);

void             planner_cell_renderer_popup_hide     (MgCellRendererPopup *cell);

#endif /* __MG_CELL_RENDERER_POPUP_H__ */
