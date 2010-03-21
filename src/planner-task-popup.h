/*
 * Copyright (C) 2004 Chris Ladd <caladd@particlestorm.net>
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

#ifndef __PLANNER_TASK_POPUP_H__
#define __PLANNER_TASK_POPUP_H__

#include <gtk/gtk.h>

typedef enum {
	PLANNER_TASK_POPUP_NONE,
	PLANNER_TASK_POPUP_INSERT,
	PLANNER_TASK_POPUP_SUBTASK,
	PLANNER_TASK_POPUP_REMOVE,
	PLANNER_TASK_POPUP_UNLINK,
	PLANNER_TASK_POPUP_EDIT_TASK,
	PLANNER_TASK_POPUP_EDIT_RESOURCES
} PlannerTaskPopupItem;

GtkItemFactory *planner_task_popup_new               (PlannerTaskTree *tree);
void            planner_task_popup_update_sensitivity (GtkItemFactory  *factory,
						      GList           *tasks);


#endif
