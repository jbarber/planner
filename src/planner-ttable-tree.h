#ifndef __PLANNER_TTABLE_TREE__
#define __PLANNER_TTABLE_TREE__

#include <gtk/gtktreeview.h>
#include <libplanner/mrp-project.h>
#include "planner-ttable-model.h"
#include "app/planner-main-window.h"

#define PLANNER_TYPE_TTABLE_TREE               (planner_ttable_tree_get_type ())
#define PLANNER_TTABLE_TREE(obj)               (GTK_CHECK_CAST ((obj), PLANNER_TYPE_TTABLE_TREE, PlannerTtableTree))
#define PLANNER_TTABLE_TREE_CLASS(klass)       (GTK_CHECK_CLASS_CAST ((klass), PLANNER_TYPE_TTABLE_TREE, PlannerTtableTreeClass))
#define PLANNER_IS_TTABLE_TREE(obj)            (GTK_CHECK_TYPE ((obj), PLANNER_TYPE_TTABLE_TREE))
#define PLANNER_IS_TTABLE_TREE_CLASS(klass)    (GTK_CHECK_CLASS_TYPE ((obj), PLANNER_TYPE_TTABLE_TREE))
#define PLANNER_TTABLE_TREE_GET_CLASS(obj)     (GTK_CHECK_GET_CLASS ((obj), PLANNER_TYPE_TTABLE_TREE, PlannerTtableTreeClass))

typedef struct _PlannerTtableTree           PlannerTtableTree;
typedef struct _PlannerTtableTreeClass      PlannerTtableTreeClass;
typedef struct _PlannerTtableTreePriv       PlannerTtableTreePriv;

struct _PlannerTtableTree {
	GtkTreeView       parent;
	PlannerTtableTreePriv *priv;
};

struct _PlannerTtableTreeClass {
	GtkTreeViewClass  parent_class;
};

GType		 planner_ttable_tree_get_type		(void);
GtkWidget	*planner_ttable_tree_new			(PlannerWindow	*main_window,
		                                	 PlannerTtableModel	*model);
void		 planner_ttable_tree_set_model		(PlannerTtableTree	*tree,
							 PlannerTtableModel	*model);
void		 planner_ttable_tree_edit_task		(PlannerTtableTree	*tree);
void		 planner_ttable_tree_edit_resource		(PlannerTtableTree	*tree);
GList*		 planner_ttable_tree_get_selected_items	(PlannerTtableTree	*tree);
void		 planner_ttable_tree_expand_all		(PlannerTtableTree	*tree);
void		 planner_ttable_tree_collapse_all		(PlannerTtableTree	*tree);
	
#endif
