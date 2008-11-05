/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2002 CodeFactory AB
 * Copyright (C) 2002 Mikael Hallendal <micke@imendio.com>
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

#ifndef __PLANNER_PRINT_DIALOG_H__
#define __PLANNER_PRINT_DIALOG_H__

#include "planner-window.h"

GtkWidget *       planner_print_dialog_new                 (PlannerWindow    *window,
							    GtkPrintOperation *job,
							    GList            *views);
GtkWidget *       print_dialog_create_page                 (PlannerWindow *window,
							    GtkWidget     *dialog,
							    GList         *views);
GList *           planner_print_dialog_get_print_selection (GtkWidget        *widget,
							    gboolean         *summary);
void              planner_print_dialog_save_page_setup     (GtkPageSetup *page_setup);
GtkPageSetup     *planner_print_dialog_load_page_setup     (void);
void              planner_print_dialog_save_print_settings (GtkPrintSettings *settings);
GtkPrintSettings *planner_print_dialog_load_print_settings (void);


#endif /* __PLANNER_PRINT_DIALOG_H__ */












