#ifndef __MG_TTABLE_TREE__
#define __MG_TTABLE_TREE__

#include <gtk/gtktreeview.h>
#include <libplanner/mrp-project.h>
#include "planner-ttable-model.h"
#include "app/planner-main-window.h"

#define MG_TYPE_TTABLE_TREE               (planner_ttable_tree_get_type ())
#define MG_TTABLE_TREE(obj)               (GTK_CHECK_CAST ((obj), MG_TYPE_TTABLE_TREE, MgTtableTree))
#define MG_TTABLE_TREE_CLASS(klass)       (GTK_CHECK_CLASS_CAST ((klass), MG_TYPE_TTABLE_TREE, MgTtableTreeClass))
#define MG_IS_TTABLE_TREE(obj)            (GTK_CHECK_TYPE ((obj), MG_TYPE_TTABLE_TREE))
#define MG_IS_TTABLE_TREE_CLASS(klass)    (GTK_CHECK_CLASS_TYPE ((obj), MG_TYPE_TTABLE_TREE))
#define MG_TTABLE_TREE_GET_CLASS(obj)     (GTK_CHECK_GET_CLASS ((obj), MG_TYPE_TTABLE_TREE, MgTtableTreeClass))

typedef struct _MgTtableTree           MgTtableTree;
typedef struct _MgTtableTreeClass      MgTtableTreeClass;
typedef struct _MgTtableTreePriv       MgTtableTreePriv;

struct _MgTtableTree {
	GtkTreeView       parent;
	MgTtableTreePriv *priv;
};

struct _MgTtableTreeClass {
	GtkTreeViewClass  parent_class;
};

GType		 planner_ttable_tree_get_type		(void);
GtkWidget	*planner_ttable_tree_new			(MgMainWindow	*main_window,
		                                	 MgTtableModel	*model);
void		 planner_ttable_tree_set_model		(MgTtableTree	*tree,
							 MgTtableModel	*model);
void		 planner_ttable_tree_edit_task		(MgTtableTree	*tree);
void		 planner_ttable_tree_edit_resource		(MgTtableTree	*tree);
GList*		 planner_ttable_tree_get_selected_items	(MgTtableTree	*tree);
void		 planner_ttable_tree_expand_all		(MgTtableTree	*tree);
void		 planner_ttable_tree_collapse_all		(MgTtableTree	*tree);
	
#endif
