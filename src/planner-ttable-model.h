#ifndef __MG_TTABLE_MODEL_H__
#define __MG_TTABLE_MODEL_H__

#include <libplanner/mrp-project.h>
#include <libplanner/mrp-task.h>
#include <libplanner/mrp-resource.h>
#include <gtk/gtktreemodel.h>

#define MG_TYPE_TTABLE_MODEL             (planner_ttable_model_get_type ())
#define MG_TTABLE_MODEL(obj)             (GTK_CHECK_CAST ((obj), MG_TYPE_TTABLE_MODEL, MgTtableModel))
#define MG_TTABLE_MODEL_CLASS(klass)     (GTK_CHECK_CLASS_CAST ((klass), MG_TYPE_TTABLE_MODEL, MgTtableModelClass))
#define MG_IS_TTABLE_MODEL(obj)          (GTK_CHECK_TYPE ((obj), MG_TYPE_TTABLE_MODEL))
#define MG_IS_TTABLE_MODEL_CLASS(klass)  (GTK_CHECK_CLASS_TYPE ((obj), MG_TYPE_TTABLE_MODEL))

typedef struct _MgTtableModel      MgTtableModel;
typedef struct _MgTtableModelClass MgTtableModelClass;
typedef struct _MgTtableModelPriv  MgTtableModelPriv;

struct _MgTtableModel {
	GObject            parent;
	gint               stamp;
	MgTtableModelPriv *priv;
};

struct _MgTtableModelClass {
	GObjectClass parent_class;
};

enum {
	COL_RESNAME,
	COL_TASKNAME,
	COL_RESOURCE,
	COL_ASSIGNMENT,
	NUM_COLS
};

GType		 planner_ttable_model_get_type		(void);

MgTtableModel 	*planner_ttable_model_new			(MrpProject *project);

GtkTreePath 	*planner_ttable_model_get_path_from_resource	(MgTtableModel *model,
							 MrpResource *resource);

//MrpTask 	*planner_ttable_model_get_indent_task_target	(MgTtableModel *model,
//							 MrpTask *task);

MrpProject 	*planner_ttable_model_get_project		(MgTtableModel *model);

MrpAssignment 	*planner_ttable_model_get_assignment		(MgTtableModel *model,
							 GtkTreeIter *iter);
MrpResource	*planner_ttable_model_get_resource		(MgTtableModel *model,
							 GtkTreeIter *iter);
gboolean	 planner_ttable_model_is_resource		(MgTtableModel *model,
							 GtkTreeIter *iter);
gboolean	 planner_ttable_model_is_assignment		(MgTtableModel *model,
							 GtkTreeIter *iter);
MrpAssignment 	*planner_ttable_model_path_get_assignment	(MgTtableModel *model,
							 GtkTreePath *path);
MrpResource	*planner_ttable_model_path_get_resource	(MgTtableModel *model,
							 GtkTreePath *path);
gboolean	 planner_ttable_model_path_is_resource	(MgTtableModel *model,
							 GtkTreePath *path);
gboolean	 planner_ttable_model_path_is_assignment	(MgTtableModel *model,
							 GtkTreePath *Path);

#endif //__MG_TTABLE_MODEL_H__
