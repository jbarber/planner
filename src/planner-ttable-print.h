#ifndef __PLANNER_TTABLE_PRINT_JOB_H__
#define __PLANNER_TTABLE_PRINT_JOB_H__

#include <gtk/gtktreeview.h>
#include "src/util/planner-print-job.h"
#include "src/app/planner-view.h"

typedef struct _PlannerTtablePrintData PlannerTtablePrintData;

void			planner_ttable_print_do		(PlannerTtablePrintData	*print_data);
gint			planner_ttable_print_get_n_pages	(PlannerTtablePrintData	*print_data);
PlannerTtablePrintData *  	planner_ttable_print_data_new	(PlannerView		*view,
							 PlannerPrintJob	*job);
void			planner_ttable_print_data_free	(PlannerTtablePrintData	*print_data);

#endif //__PLANNER_TTABLE_PRINT_JOB_H__
