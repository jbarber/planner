
/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 * Copyright (C) 2003 Benjamin BAYART <benjamin@sitadelle.com>
 * Copyright (C) 2003 Xavier Ordoquy <xordoquy@wanadoo.fr>
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

#ifndef __PLANNER_TTABLE_PRINT_JOB_H__
#define __PLANNER_TTABLE_PRINT_JOB_H__

#include <gtk/gtktreeview.h>
#include "planner-print-job.h"
#include "planner-view.h"

typedef struct _PlannerTtablePrintData PlannerTtablePrintData;

void planner_ttable_print_do (PlannerTtablePrintData * print_data);
gint planner_ttable_print_get_n_pages (PlannerTtablePrintData * print_data);
PlannerTtablePrintData *planner_ttable_print_data_new (PlannerView * view,
                                                       PlannerPrintJob * job);
void planner_ttable_print_data_free (PlannerTtablePrintData * print_data);

#endif
