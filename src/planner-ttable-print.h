#ifndef __MG_TTABLE_PRINT_JOB_H__
#define __MG_TTABLE_PRINT_JOB_H__

#include <gtk/gtktreeview.h>
#include "src/util/planner-print-job.h"
#include "src/app/planner-view.h"

typedef struct _MgTtablePrintData MgTtablePrintData;

void			planner_ttable_print_do		(MgTtablePrintData	*print_data);
gint			planner_ttable_print_get_n_pages	(MgTtablePrintData	*print_data);
MgTtablePrintData *  	planner_ttable_print_data_new	(MgView		*view,
							 MgPrintJob	*job);
void			planner_ttable_print_data_free	(MgTtablePrintData	*print_data);

#endif //__MG_TTABLE_PRINT_JOB_H__
