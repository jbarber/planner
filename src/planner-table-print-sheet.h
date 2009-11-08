/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2003 CodeFactory AB
 * Copyright (C) 2003 Richard Hult <richard@imendio.com>
 * Copyright (C) 2003 Mikael Hallendal <micke@imendio.com>
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

#ifndef __PLANNER_TABLE_PRINT_SHEET_H__
#define __PLANNER_TABLE_PRINT_SHEET_H__

#include <gtk/gtk.h>
#include "planner-print-job.h"
#include "planner-view.h"

typedef struct _PlannerTablePrintSheet PlannerTablePrintSheet;

void                    planner_table_print_sheet_output      (PlannerTablePrintSheet *sheet,
							       gint                    page_nr);
gint                    planner_table_print_sheet_get_n_pages (PlannerTablePrintSheet *sheet);
PlannerTablePrintSheet *planner_table_print_sheet_new         (PlannerView            *view,
							       PlannerPrintJob        *job,
							       GtkTreeView            *tree);
void                    planner_table_print_sheet_free        (PlannerTablePrintSheet *sheet);

#endif /* __PLANNER_TABLE_PRINT_SHEET_H__ */
