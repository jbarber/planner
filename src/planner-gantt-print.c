/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2004 Imendio HB
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

#include <config.h>
#include <math.h>
#include <string.h>
#include <libplanner/mrp-project.h>
#include <libplanner/mrp-task.h>
#include <libplanner/mrp-resource.h>
#include <libgnome/gnome-i18n.h>
#include <libgnomeprint/gnome-print.h>
#include "planner-print-job.h"
#include "planner-format.h"
#include "planner-gantt-model.h"
#include "planner-gantt-print.h"
#include "planner-scale-utils.h"

#define d(x)
#define GET_PAGE(d,r,c) (&d->pages[r*d->cols_of_pages+c])
#define INDENT_FACTOR 4

typedef struct {
	mrptime  first_time;
	mrptime  last_time;

	gdouble  gantt_x0;
	gdouble  gantt_y0;

	GList   *elements;
	GList   *background_elements;
} Page;

struct _PlannerGanttPrintData {
	MrpProject        *project;
	PlannerView       *view;
	PlannerPrintJob   *job;

	/* Used to get the visible (expanded) tasks. */
	GtkTreeView       *tree_view;
	
	/* Settings. */
	gboolean           show_critical;

	gint               level;
	
	PlannerScaleUnit   major_unit;
	PlannerScaleFormat major_format;
	
	PlannerScaleUnit   minor_unit;
	PlannerScaleFormat minor_format;
	
	gdouble            header_height;
	
	gint               tasks_per_page_with_header;
	gint               tasks_per_page_without_header;

	/* The matrix of pages we will produce. */
	gint               rows_of_pages;
	gint               cols_of_pages;

	gdouble            tree_x1;
	gdouble            tree_x2;
	
	gdouble            name_x1;
	gdouble            name_x2;

	gdouble            work_x1;
	gdouble            work_x2;
	
	gdouble            row_height;

	GHashTable        *task_start_coords;
	GHashTable        *task_finish_coords;
	
	GnomeFont         *font;
	GnomeFont         *font_bold;

	GList             *tasks;

	gdouble            f;
	
	/* Summary drawing. */
	gdouble            summary_height;
	gdouble            summary_thick;
	gdouble            summary_slope;

	/* Milestone drawing. */
	gdouble            milestone_size;

	/* Relation arrow drawing. */
	gdouble            arrow_width;
	gdouble            arrow_height;
	
	/* Printed time span. */
	mrptime            start;
	mrptime            finish;

	Page              *pages;
};

typedef struct {
	/* Page */
	gint    row, col;

	/* Position on that page. */
	gdouble x, y;
} TaskCoord;

typedef struct {
	MrpTask *task;
	gint     depth;
} PrintTask;

typedef enum {
	TASK_WHOLE,
	TASK_LEFT,
	TASK_RIGHT,
	TASK_MIDDLE,

	SUMMARY_WHOLE,
	SUMMARY_LEFT,
	SUMMARY_RIGHT,
	SUMMARY_MIDDLE,

	MILESTONE,
	
	RELATION_HORIZ,
	RELATION_VERT,
	RELATION_ARROW_DOWN,
	RELATION_ARROW_UP,

	RESOURCES,

	SHADE
} ElementType;

typedef struct {
	ElementType  type;

	gdouble      x1, y1;
	gdouble      x2, y2;

	gdouble      x_complete;

	gboolean     is_critical;
	gchar       *resources;
} Element;


static void
print_table_header (PlannerGanttPrintData *data)
{
	gdouble x, y;

	gnome_print_setlinewidth (data->job->pc, 0);
	planner_print_job_set_font_bold (data->job);

	y = data->header_height + data->row_height / 4;

	planner_print_job_moveto (data->job, data->tree_x1, y);
	planner_print_job_lineto (data->job, data->tree_x2, y);
	gnome_print_stroke (data->job->pc);

	x = data->name_x1 + data->job->x_pad;
	y = data->row_height;

	planner_print_job_show_clipped (data->job,
					x, y,
					_("Name"),
					data->name_x1, 0, 
					data->name_x2 - data->job->x_pad / 2, y + data->row_height);

	x = data->work_x1 + data->job->x_pad;

	planner_print_job_show_clipped (data->job,
					x, y,
					_("Work"),
					data->work_x1, 0, 
					data->work_x2 - data->job->x_pad / 2, y + data->row_height);

	planner_print_job_set_font_regular (data->job);
}

static void
print_table_tasks (PlannerGanttPrintData *data,
		   gboolean               header,
		   GList                 *tasks,
		   gint                   first)
{
	gchar     *str;
	gint       work;
	gdouble    x, y;
	GList     *l, *to;
	PrintTask *ptask;
	gint       last, i;

	if (header) {
		last = first + data->tasks_per_page_with_header;
	} else {
		last = first + data->tasks_per_page_without_header;
	}
	
	l = g_list_nth (tasks, first);
	to = g_list_nth (tasks, last);
	i = 1;

	gnome_print_setlinewidth (data->job->pc, 0);
	
	while (l && l != to) {
		ptask = l->data;
		
		g_object_get (ptask->task,
			      "name", &str,
			      "work", &work,
			      NULL);

		if (mrp_task_get_n_children (ptask->task) > 0) {
			planner_print_job_set_font_bold (data->job);
		} else {
			planner_print_job_set_font_regular (data->job);
		}

		x = data->name_x1 + data->job->x_pad + ptask->depth * INDENT_FACTOR * data->job->x_pad;
		y = i * data->row_height;
		
		if (header) {
			y += data->header_height;
		}

		planner_print_job_show_clipped (data->job,
						x, y,
						str,
						data->name_x1, 0, 
						data->name_x2 - data->job->x_pad / 2, y + data->row_height);

		g_free (str);
		
		x = data->work_x1 + data->job->x_pad;
		
		str = planner_format_duration (work, 8);
		planner_print_job_show_clipped (data->job,
						x, y,
						str,
						data->work_x1, 0, 
						data->work_x2 - data->job->x_pad / 2, y + data->row_height);
		g_free (str);

		y += data->row_height / 4;

		planner_print_job_moveto (data->job, 0, y);
		planner_print_job_lineto (data->job, data->tree_x2, y);
		gnome_print_stroke (data->job->pc);

		i++;
		l = l->next;
	}

	gnome_print_setlinewidth (data->job->pc, 1);
}

static void
print_time_header (PlannerGanttPrintData *data,
		   gdouble                x1,
		   gdouble                x2,
		   mrptime                start,
		   mrptime                finish)
{
	gdouble  x, y;
	gdouble  y1, y2, y3;
	gdouble  width;
	mrptime  t;
	gchar   *str;

	y1 = 0;
	y2 = data->header_height / 2 + data->row_height / 4;
	y3 = data->header_height + data->row_height / 4;
	
	gnome_print_setlinewidth (data->job->pc, 0);

	planner_print_job_moveto (data->job, x1, y2);
	planner_print_job_lineto (data->job, x2, y2);
	gnome_print_stroke (data->job->pc);

	planner_print_job_moveto (data->job, x1, y3);
	planner_print_job_lineto (data->job, x2, y3);
	gnome_print_stroke (data->job->pc);

	/* Major scale. */
	x = x1;
	y = data->row_height;
	
	t = planner_scale_time_prev (start, data->major_unit);
	width = (planner_scale_time_next (t, data->major_unit) - t) / data->f - data->job->x_pad / 2;
	
	while (t <= finish) {
		x = x1 + (t - start) / data->f;

		if (x + width > x1) {
			if (x > x1) {
				planner_print_job_moveto (data->job, x, y1);
				planner_print_job_lineto (data->job, x, y2);
				gnome_print_stroke (data->job->pc);
			}
			
			str = planner_scale_format_time (t, data->major_unit, data->major_format);
			
			planner_print_job_show_clipped (data->job,
							x + data->job->x_pad, y,
							str,
							MAX (x, x1), y1, 
							x + width, y2);
			
			g_free (str);
		}
		
		t = planner_scale_time_next (t, data->major_unit);
	}

	/* Minor scale. */
	x = x1;	
	y = 2 * data->row_height;

	t = planner_scale_time_prev (start, data->minor_unit);
	width = (planner_scale_time_next (t, data->minor_unit) - t) / data->f - data->job->x_pad / 2;

	while (t <= finish) {
		x = x1 + (t - start) / data->f;

		if (x + width > x1) {
			if (x > x1) {
				planner_print_job_moveto (data->job, x, y2);
				planner_print_job_lineto (data->job, x, y3);
				gnome_print_stroke (data->job->pc);
			}
			
			str = planner_scale_format_time (t, data->minor_unit, data->minor_format);
			
			planner_print_job_show_clipped (data->job,
							x + data->job->x_pad, y,
							str,
							MAX (x, x1), y2, 
							x + width, y3);
			
			g_free (str);
		}
		
		t = planner_scale_time_next (t, data->minor_unit);
	}
}

typedef struct {
	GtkTreeView *tree_view;
	GList       *list;
} ForeachVisibleData;

static gboolean
foreach_visible_task (GtkTreeModel *model,
		      GtkTreePath  *path,
		      GtkTreeIter  *iter,
		      gpointer      data)
{
	ForeachVisibleData *fvd = data;
	MrpTask            *task;
	GtkTreeIter         parent_iter;
	GtkTreePath        *parent_path;
	PrintTask          *ptask;

	gtk_tree_model_iter_parent (model, &parent_iter, iter);
	parent_path = gtk_tree_model_get_path (model, &parent_iter);
	if (gtk_tree_path_get_depth (path) == 1 ||
	    gtk_tree_view_row_expanded (fvd->tree_view, parent_path)) {
		gtk_tree_model_get (model,
				    iter, 
				    COL_TASK, &task,
				    -1);
		
		ptask = g_new0 (PrintTask, 1);
		ptask->task = task;
		ptask->depth = gtk_tree_path_get_depth (path);

		fvd->list = g_list_prepend (fvd->list, ptask);
	}
	gtk_tree_path_free (parent_path);
	
	return FALSE;
}

static GList *
gantt_print_get_visible_tasks (PlannerGanttPrintData *data)
{
	ForeachVisibleData  fvd;
	GtkTreeModel       *model;

	model = gtk_tree_view_get_model (data->tree_view);

	fvd.list = NULL;
	fvd.tree_view = data->tree_view;
	
	gtk_tree_model_foreach (model,
				foreach_visible_task,
				&fvd);

	return g_list_reverse (fvd.list);
}

static void
gantt_print_free_print_tasks (GList *tasks)
{
	GList *l;

	for (l = tasks; l; l = l->next) {
		g_free (l->data);
	}
}	

static GList *
gantt_print_get_relations (PlannerGanttPrintData *data)
{
	GList   *tasks, *l;
	GList   *predecessors, *p;
	GList  *relations = NULL;

	tasks = mrp_project_get_all_tasks (data->project);
	for (l = tasks; l; l = l->next) {
		predecessors = mrp_task_get_predecessor_relations (l->data);
		
		for (p = predecessors; p; p = p->next) {
			relations = g_list_prepend (relations, p->data);
		}
	}

	g_list_free (tasks);

	return relations;
}

static void
gantt_print_task (PlannerGanttPrintData *data, Element *element)
{
	gnome_print_newpath (data->job->pc);
	planner_print_job_moveto (data->job, element->x1, element->y1);
	planner_print_job_lineto (data->job, element->x2, element->y1);
	planner_print_job_lineto (data->job, element->x2, element->y2);
	planner_print_job_lineto (data->job, element->x1, element->y2);
	gnome_print_closepath (data->job->pc);
	
	gnome_print_gsave (data->job->pc);

	if (data->show_critical && element->is_critical) {
		gnome_print_setrgbcolor (data->job->pc, 205/255.0, 92/255.0, 92/255.0);
	} else {
		gnome_print_setrgbcolor (data->job->pc, 235/255.0, 235/255.0, 235/255.0);
	}

	gnome_print_fill (data->job->pc);
	gnome_print_grestore (data->job->pc);

	gnome_print_stroke (data->job->pc);

	/* Percent complete. */
	if (element->x_complete > 0) {
		gdouble pad;

		pad = (element->y2 - element->y1) * 0.25;
		
		gnome_print_gsave (data->job->pc);

		gnome_print_newpath (data->job->pc);
		planner_print_job_moveto (data->job, element->x1, element->y1 + pad);
		planner_print_job_lineto (data->job, element->x_complete, element->y1 + pad);
		planner_print_job_lineto (data->job, element->x_complete, element->y2 - pad);
		planner_print_job_lineto (data->job, element->x1, element->y2 - pad);
		gnome_print_closepath (data->job->pc);
		
		gnome_print_setrgbcolor (data->job->pc, 135/255.0, 135/255.0, 135/255.0);
		gnome_print_fill (data->job->pc);
		
		gnome_print_grestore (data->job->pc);
	}
}

static gboolean
gantt_print_get_allocated_resources_string (PlannerGanttPrintData  *data,
					    MrpTask                *task,
					    gchar                 **str,
					    gdouble                *width)
{
	GList         *l;
	GList         *assignments;
	MrpAssignment *assignment;
	MrpResource   *resource;
	gchar         *name, *tmp_str, *name_unit;
	gchar         *text = NULL;
	gdouble        w = 0;
	gint           units;

	assignments = mrp_task_get_assignments (task);
	for (l = assignments; l; l = l->next) {
		assignment = l->data;

		resource = mrp_assignment_get_resource (assignment);
		units = mrp_assignment_get_units (assignment);

		/* Use the resource short_name in preference to the resource
		 * name.
		 */
		g_object_get (resource, 
			      "short_name", &name, 
			      NULL);
				
		if (name && name[0] == 0) {
			g_free (name);

			g_object_get (resource, 
				      "name", &name, 
				      NULL);
			
			if (name && name[0] == 0) {
				g_free (name);
				
				name = g_strdup (_("Unnamed"));
			}
		}

 		if (units != 100) {
			name_unit = g_strdup_printf ("%s [%i]", name, units);
		} else {
			name_unit = g_strdup_printf ("%s", name);
		}
		
		if (!text) { /* First resource */
			text = g_strdup_printf ("%s", name_unit);
			g_free (name_unit);
			continue;
		}
		
		tmp_str = g_strdup_printf ("%s, %s", text, name_unit);
		
		g_free (name);
		g_free (text);
		g_free (name_unit);

		text = tmp_str;
	}

	if (text && width) {
		w = gnome_font_get_width_utf8 (
			planner_print_job_get_font (data->job), text);
	}

	if (width) {
		*width = w;
	}
	
	if (str) {
		*str = text;
	} else {
		g_free (text);
	}

	return text != NULL;
}

void
planner_gantt_print_do (PlannerGanttPrintData *data)
{
	GList      *relations;
	GList      *l;
	gdouble      x1, x2;
	gdouble      y1, y2;
	gint         num_tasks;
	mrptime      t1, t2, t0;
	mrptime      start, finish;
	mrptime      complete;
	gboolean     is_summary;
	gboolean     is_critical;
	gboolean     boundary_overlap = FALSE;
	MrpTaskType  type;
	PrintTask   *ptask;
	TaskCoord   *task_coord;
	Element     *element;
	gdouble      x0, y0;
	gint         i, row, col;
	gint         tasks_on_this_page;
	gchar       *name;
	Page        *page;
	gchar       *str;
	MrpCalendar *calendar;
	MrpDay      *day;
	GList       *ivals;
	MrpInterval *ival;

	calendar = mrp_project_get_calendar (data->project);
	
	num_tasks = g_list_length (data->tasks);

	/* Go through all tasks in a first pass and layout them so that we know
	 * where to draw relation arrows.
	 */

	l = data->tasks;
	row = 0;
	col = 0;
	while (l) {
		if (row == 0) {
			/* Top-most row has the header. */
			y0 = data->header_height;
			tasks_on_this_page = data->tasks_per_page_with_header;
		} else {
			y0 = 0;
			tasks_on_this_page = data->tasks_per_page_without_header;
		}
		
		x1 = data->tree_x2;

		/* Loop through the tasks on this row of pages. */
		for (i = 0; i < tasks_on_this_page; i++) {
			ptask = l->data;
			
			start = mrp_task_get_work_start (ptask->task);
			finish = mrp_task_get_finish (ptask->task);
			complete = start + (finish - start) *
				mrp_task_get_percent_complete (ptask->task) / 100.0;

			is_summary = mrp_task_get_n_children (ptask->task) > 0;

			g_object_get (ptask->task,
				      "name", &name,
				      "type", &type,
				      "critical", &is_critical,
				      NULL);

			if (!is_summary && type == MRP_TASK_TYPE_MILESTONE) {
				finish = start;
			}
			
			d(g_print ("%s: ", name));

			col = 0;
			t1 = data->start;

			y1 = y0 + data->row_height * (i + 1.5 * 0.25);
			y2 = y1 + 0.75 * data->row_height;

			
			/* Loop through the columns that this task covers. */
			while (t1 <= finish) {
				if (col == 0) {
					/* Left-most column has the task tree. */
					x0 = data->tree_x2;
					t2 = t1 + (data->job->width - (data->tree_x2 - data->tree_x1)) * data->f;
				} else {
					x0 = 0;
					t2 = t1 + data->job->width * data->f;
				}

				element = g_new0 (Element, 1);

				element->y1 = y1;
				element->y2 = y2;
				element->is_critical = is_critical;
				
				/* Identify the cases: only left-most part on
				 * page, only right-most part, the whole task,
				 * or only the mid-section.
				 */ 
				if (start >= t1 && start <= t2 && finish > t2) {
					/* Left */
					d(g_print ("left "));

					if (is_summary) {
						element->type = SUMMARY_LEFT;
					}
					else if (type == MRP_TASK_TYPE_MILESTONE) {
						element->type = MILESTONE;
					} else {
						element->type = TASK_LEFT;
					}
					
					element->x1 = x0 + (start - t1) / data->f;
					element->x2 = data->job->width;
					element->x_complete = x0 + (complete - t1) / data->f;
				}
				else if (start < t1 && finish >= t1 && finish <= t2) {
					/* Right */
					d(g_print ("right "));

					if (is_summary) {
						element->type = SUMMARY_RIGHT;
					}
					else if (type == MRP_TASK_TYPE_MILESTONE) {
						element->type = MILESTONE;
					} else {
						element->type = TASK_RIGHT;
					}
					
					element->x1 = x0;
					element->x2 = x0 + (finish - t1) / data->f;
					element->x_complete = x0 + (complete - t1) / data->f;
				}
				else if (start >= t1 && finish <= t2) {
					/* Whole */
					d(g_print ("whole "));

					if (is_summary) {
						element->type = SUMMARY_WHOLE;
					}
					else if (type == MRP_TASK_TYPE_MILESTONE) {
						element->type = MILESTONE;
					} else {
						element->type = TASK_WHOLE;
					}
					
					element->x1 = x0 + (start - t1) / data->f;
					element->x2 = x0 + (finish - t1) / data->f;
					element->x_complete = x0 + (complete - t1) / data->f;
				}
				else if (start < t1 && finish > t2) {
					/* Middle */
					d(g_print ("middle "));

					if (is_summary) {
						element->type = SUMMARY_MIDDLE;
					}
					else if (type == MRP_TASK_TYPE_MILESTONE) {
						element->type = MILESTONE;
					} else {
						element->type = TASK_MIDDLE;
					}
					
					element->x1 = x0;
					element->x2 = data->job->width;
					element->x_complete = data->job->width;
				} else {
					d(g_print ("nothing"));
					g_free (element);
					element = NULL;
				}

				if (element) {
					switch (element->type) {
					case TASK_WHOLE:
					case TASK_LEFT:
					case SUMMARY_WHOLE:
					case SUMMARY_LEFT:
					case MILESTONE:
						task_coord = g_new0 (TaskCoord, 1);
						task_coord->row = row;
						task_coord->col = col;

						task_coord->x = element->x1;
						task_coord->y = y1 - (y2 - y1) / 2;
						
						g_hash_table_insert (data->task_start_coords,
								     ptask->task,
								     task_coord);
						break;
					default:
						break;
					}
					
					switch (element->type) {
					case TASK_WHOLE:
					case TASK_RIGHT:
					case SUMMARY_WHOLE:
					case SUMMARY_RIGHT:
					case MILESTONE:
						task_coord = g_new0 (TaskCoord, 1);
						task_coord->row = row;
						task_coord->col = col;
						
						task_coord->x = element->x2;
						task_coord->y = y1 + (y2 - y1) / 2;
						
						g_hash_table_insert (data->task_finish_coords,
								     ptask->task,
								     task_coord);
						break;
					default:
						break;
					}

					page = GET_PAGE (data, row, col);
					page->elements = g_list_prepend (page->elements, element);
				}
				
				t1 = t2;
				col++;
			}

			d(g_print ("\n"));

			/* Handle the allocated resources. */
			if (gantt_print_get_allocated_resources_string (data, ptask->task, &str, NULL)) {
				task_coord = g_hash_table_lookup (data->task_finish_coords,
								  ptask->task);

				/* FIXME: Remove this check when the scheduler
				 * is fixed. We get a crash here if tasks get
				 * really odd start/finish values. This
				 * sometimes happens for complex projects now.
				 */
				if (!task_coord) {
					goto fail;
				}

				element = g_new0 (Element, 1);
				element->type = RESOURCES;

				element->x1 = task_coord->x + data->job->x_pad;
				element->y1 = y0 + data->row_height * (i + 1);

				element->resources = str;

				page = GET_PAGE (data, row, col - 1);
				page->elements = g_list_prepend (page->elements, element);
			}

		fail:
			
			l = l->next;
			if (!l) {
				break;
			}
		}

		row++;
		col = 0;
	}

	/* Second pass, go through all relations and add elements for them. */
	relations = gantt_print_get_relations (data);
	for (l = relations; l; l = l->next) {
		MrpTask     *predecessor, *successor;
		TaskCoord   *pre_coord, *suc_coord;
		Page        *page;
		MrpRelation *relation = l->data;
		gint         from_row, to_row;
		gdouble      top;

		predecessor = mrp_relation_get_predecessor (relation);
		successor = mrp_relation_get_successor (relation);

		pre_coord = g_hash_table_lookup (data->task_finish_coords, predecessor);
		suc_coord = g_hash_table_lookup (data->task_start_coords, successor);

		/* One of the tasks might not be visible. */
		if (!pre_coord || !suc_coord) {
			continue;
		}
		
		element = g_new0 (Element, 1);
		element->x1 = suc_coord->x;

		from_row = MIN (pre_coord->row, suc_coord->row);
		to_row = MAX (pre_coord->row, suc_coord->row);

		/* Get the right direction and position of the arrow depending on
		 * the order of the predecessor and successor.
		 */
		if ((pre_coord->row == suc_coord->row && pre_coord->y < suc_coord->y) ||
		    (pre_coord->row < suc_coord->row)) {
			element->y1 = suc_coord->y + 0.75 * data->row_height * 0.5;
			element->type = RELATION_ARROW_DOWN;
		} else {
			element->y1 = suc_coord->y + 0.75 * data->row_height * 1.5;
			element->type = RELATION_ARROW_UP;
		}

		page = GET_PAGE (data, suc_coord->row, suc_coord->col);
		page->elements = g_list_prepend (page->elements, element);

		for (row = from_row; row <= to_row; row++) {
			if (row == pre_coord->row) {
				for (col = pre_coord->col; col <= suc_coord->col; col++) {
					element = g_new0 (Element, 1);
					element->type = RELATION_HORIZ;

					if (col == pre_coord->col) {
						element->x1 = pre_coord->x;
					} else {
						if (col == 0) {
							element->x1 = data->tree_x2;
						} else {
							element->x1 = 0;
						}						
					}
					
					if (col == suc_coord->col) {
						element->x2 = suc_coord->x;
					} else {
						element->x2 = data->job->width;
					}

					element->y1 = pre_coord->y;
					
					page = GET_PAGE (data, row, col);
					page->elements = g_list_prepend (page->elements, element);
				}
			}

			element = g_new0 (Element, 1);
			element->type = RELATION_VERT;
			element->x1 = suc_coord->x;

			if (row == 0) {
				top = data->header_height;
			} else {
				top = 0;
			}
			
			if (row == pre_coord->row) {
				element->y1 = pre_coord->y;
			} else {
				if (pre_coord->row <= suc_coord->row) { 
					element->y1 = top;
				} else {
					element->y1 = data->job->height;
				}
			}

			if (row == suc_coord->row) {
				element->y2 = suc_coord->y + data->row_height / 2;
			} else {
				if (pre_coord->row <= suc_coord->row) { 
					element->y2 = data->job->height;
				} else {
					element->y2 = top;
				}
			}
			
			page = GET_PAGE (data, row, suc_coord->col);
			page->elements = g_list_prepend (page->elements, element);
		}
	}

	g_list_free (relations);

	/* Third pass, add background shading for non-work intervals. */
	for (row = 0; row < data->rows_of_pages; row++) {

		t0 = mrp_time_align_day (data->start);

		if (row == 0) {
			/* Top-most row has the header. */
			y0 = data->header_height;
		} else {
			y0 = 0;
		}

		for (col = 0; col < data->cols_of_pages; col++) {
			mrptime ival_start, ival_end, ival_prev;
			
			if (col == 0) {
				/* Left-most col has the tree. */
				x0 = data->tree_x2;
			} else {
				x0 = 0;
			}

			t2 = t0 + (data->job->width - x0) * data->f;
						
			/* Loop through the days between t0 and t2. */
			t1 = t0;

			while (t1 <= t2) {
				day = mrp_calendar_get_day (calendar, t1, TRUE);
				
				ivals = mrp_calendar_day_get_intervals (calendar, day, TRUE);
				
				ival_prev = t1;
				
				/* Loop through the intervals for this day. */
				for (l = ivals; l; l = l->next) { 
					ival = l->data;

					mrp_interval_get_absolute (ival,
								   mrp_time_align_day (t1),
								   &ival_start,
								   &ival_end);

					if (planner_scale_conf[data->level].nonworking_limit <= ival_start - ival_prev ||
					    (boundary_overlap && ival_start >= t1)) {

						boundary_overlap = FALSE;

						/* Check for corner case of
						 * non-working interval
						 * overlapping page boundary.
						 */

						if (ival_start > t2) {
							boundary_overlap = TRUE;
						}

						element = g_new0 (Element, 1);
						element->type = SHADE;
						element->y1 = y0 + data->row_height / 4;
						element->y2 = data->job->height;

						element->x1 = x0 + (ival_prev - t0) / data->f;
						element->x2 = x0 + (ival_start - t0) / data->f;
						
						page = GET_PAGE (data, row, col);
						page->background_elements = g_list_prepend (page->background_elements, element);
					}
					
					ival_prev = ival_end;
				}

				t1 += 60*60*24;
				t1 =  mrp_time_align_day (t1);

				/* Draw the remaining interval if there is one. */
				if ((ival_prev < t1 && planner_scale_conf[data->level].nonworking_limit <= t1 - ival_prev) ||
				    boundary_overlap) {

					boundary_overlap = FALSE;

					/* Check for corner case of non-working
					 * interval overlapping page
					 * boundary.
					 */

					if (t1 > t2) {
						boundary_overlap = TRUE;
					}

					element = g_new0 (Element, 1);
					element->type = SHADE;
					element->y1 = y0 + data->row_height / 4;
					element->y2 = data->job->height;
					
					element->x1 = x0 + (ival_prev - t0) / data->f;
					element->x2 = x0 + (t1 - t0) / data->f;
					
					page = GET_PAGE (data, row, col);
					page->background_elements = g_list_prepend (page->background_elements, element);
				}
			}

			t0 = t2;
		}
	}
	
	/* Fourth pass, generate pages. */
	x2 = data->job->width;
	i = 0;
	for (row = 0; row < data->rows_of_pages; row++) {
		t1 = data->start;
		
		for (col = 0; col < data->cols_of_pages; col++) {
			planner_print_job_begin_next_page (data->job);

			if (col == 0) {
				x1 = data->tree_x2;
				t2 = t1 + (data->job->width - (data->tree_x2 - data->tree_x1)) * data->f;

				planner_print_job_moveto (data->job,
							  data->tree_x2,
							  0);
				planner_print_job_lineto (data->job,
							  data->tree_x2,
							  data->job->height);
				gnome_print_stroke (data->job->pc);
				
				planner_print_job_moveto (data->job,
							  data->name_x2,
							  0);
				planner_print_job_lineto (data->job,
							  data->name_x2,
							  data->job->height);
				gnome_print_stroke (data->job->pc);

				print_table_tasks (data,
						   row == 0,
						   data->tasks,
						   i);
			} else {
				x1 = 0;
				t2 = t1 + data->job->width * data->f;
			}

			page = GET_PAGE (data, row, col);
			for (l = page->background_elements; l; l = l->next) {
				element = l->data;

				switch (element->type) {
				case SHADE:
					gnome_print_newpath (data->job->pc);
					planner_print_job_moveto (data->job, element->x1, element->y1);
					planner_print_job_lineto (data->job, element->x2, element->y1);
					planner_print_job_lineto (data->job, element->x2, element->y2);
					planner_print_job_lineto (data->job, element->x1, element->y2);
					gnome_print_closepath (data->job->pc);
					
					gnome_print_setrgbcolor (data->job->pc, 249/255.0, 249/255.0, 249/255.0);
					gnome_print_fill (data->job->pc);

					gnome_print_setlinewidth (data->job->pc, 0);
					gnome_print_setrgbcolor (data->job->pc, 150/255.0, 150/255.0, 150/255.0);
					planner_print_job_moveto (data->job, element->x1, element->y1);
					planner_print_job_lineto (data->job, element->x1, element->y2);
					gnome_print_stroke (data->job->pc);
					break;
				default:
					break;
				}
			}

			gnome_print_setrgbcolor (data->job->pc, 0, 0, 0);

			if (row == 0) {
				print_time_header (data, x1, x2, t1, t2);
				if (col == 0) {
					print_table_header (data);
				}
			}
			
			for (l = page->elements; l; l = l->next) {
				element = l->data;

				gnome_print_setrgbcolor (data->job->pc, 0, 0, 0);
				
				switch (element->type) {
				case TASK_LEFT:
				case TASK_RIGHT:
				case TASK_WHOLE:
				case TASK_MIDDLE:
					gantt_print_task (data, element);
					break;
				case SUMMARY_LEFT:
					planner_print_job_moveto (data->job,
								  element->x1,
								  element->y1 + data->summary_thick);
					planner_print_job_lineto (data->job,
								  element->x1,
								  element->y1 + data->summary_thick + data->summary_height);
					planner_print_job_lineto (data->job,
								  element->x1 + data->summary_slope,
								  element->y1 + data->summary_thick);
					gnome_print_closepath (data->job->pc);
					gnome_print_fill (data->job->pc);

					planner_print_job_moveto (data->job,
								  element->x2,
								  element->y1 + 2 * data->summary_thick);
					planner_print_job_lineto (data->job,
								  element->x1,
								  element->y1 + 2 * data->summary_thick);
					planner_print_job_lineto (data->job,
								  element->x1,
								  element->y1 + data->summary_thick);
					planner_print_job_lineto (data->job,
								  element->x2,
								  element->y1 + data->summary_thick);
					gnome_print_closepath (data->job->pc);
					gnome_print_fill (data->job->pc);
					break;
				case SUMMARY_RIGHT:
					planner_print_job_moveto (data->job,
								  element->x2,
								  element->y1 + data->summary_thick);
					planner_print_job_lineto (data->job,
								  element->x2,
								  element->y1 + data->summary_thick + data->summary_height);
					planner_print_job_lineto (data->job,
								  element->x2 - data->summary_slope,
								  element->y1 + data->summary_thick);
					gnome_print_closepath (data->job->pc);
					gnome_print_fill (data->job->pc);

					planner_print_job_moveto (data->job,
								  element->x2,
								  element->y1 + 2 * data->summary_thick);
					planner_print_job_lineto (data->job,
								  element->x1,
								  element->y1 + 2 * data->summary_thick);
					planner_print_job_lineto (data->job,
								  element->x1,
								  element->y1 + data->summary_thick);
					planner_print_job_lineto (data->job,
								  element->x2,
								  element->y1 + data->summary_thick);
					gnome_print_closepath (data->job->pc);
					gnome_print_fill (data->job->pc);
					break;
				case SUMMARY_WHOLE:
					planner_print_job_moveto (data->job,
								  element->x1,
								  element->y1 + data->summary_thick);
					planner_print_job_lineto (data->job,
								  element->x1,
								  element->y1 + data->summary_thick + data->summary_height);
					planner_print_job_lineto (data->job,
								  element->x1 + data->summary_slope,
								  element->y1 + data->summary_thick);
					gnome_print_closepath (data->job->pc);
					gnome_print_fill (data->job->pc);
					
					planner_print_job_moveto (data->job,
								  element->x2,
								  element->y1 + data->summary_thick);
					planner_print_job_lineto (data->job,
								  element->x2,
								  element->y1 + data->summary_thick + data->summary_height);
					planner_print_job_lineto (data->job,
								  element->x2 - data->summary_slope,
								  element->y1 + data->summary_thick);
					gnome_print_closepath (data->job->pc);
					gnome_print_fill (data->job->pc);
					
					planner_print_job_moveto (data->job,
								  element->x2,
								  element->y1 + 2 * data->summary_thick);
					planner_print_job_lineto (data->job,
								  element->x1,
								  element->y1 + 2 * data->summary_thick);
					planner_print_job_lineto (data->job,
								  element->x1,
								  element->y1 + data->summary_thick);
					planner_print_job_lineto (data->job,
								  element->x2,
								  element->y1 + data->summary_thick);
					gnome_print_closepath (data->job->pc);
					gnome_print_fill (data->job->pc);
					break;
				case SUMMARY_MIDDLE:
					planner_print_job_moveto (data->job,
								  element->x2,
								  element->y1 + 2 * data->summary_thick);
					planner_print_job_lineto (data->job,
								  element->x1,
								  element->y1 + 2 * data->summary_thick);
					planner_print_job_lineto (data->job,
								  element->x1,
								  element->y1 + data->summary_thick);
					planner_print_job_lineto (data->job,
								  element->x2,
								  element->y1 + data->summary_thick);
					gnome_print_closepath (data->job->pc);
					gnome_print_fill (data->job->pc);
					break;
				case RELATION_ARROW_DOWN:
					planner_print_job_moveto (data->job,
								  element->x1,
								  element->y1);
					planner_print_job_lineto (data->job,
								  element->x1 - data->arrow_width,
								  element->y1 - data->arrow_height);
					planner_print_job_lineto (data->job,
								  element->x1 + data->arrow_width,
								  element->y1 - data->arrow_height);
					gnome_print_closepath (data->job->pc);
					gnome_print_fill (data->job->pc);
					break;
				case RELATION_ARROW_UP:
					planner_print_job_moveto (data->job,
								  element->x1,
								  element->y1);
					planner_print_job_lineto (data->job,
								  element->x1 - data->arrow_width,
								  element->y1 + data->arrow_height);
					planner_print_job_lineto (data->job,
								  element->x1 + data->arrow_width,
								  element->y1 + data->arrow_height);
					gnome_print_closepath (data->job->pc);
					gnome_print_fill (data->job->pc);
					break;
				case RELATION_HORIZ:
					planner_print_job_moveto (data->job, element->x1, element->y1);
					planner_print_job_lineto (data->job, element->x2, element->y1);
					gnome_print_stroke (data->job->pc);
					break;
				case RELATION_VERT:
					planner_print_job_moveto (data->job, element->x1, element->y1);
					planner_print_job_lineto (data->job, element->x1, element->y2);
					gnome_print_stroke (data->job->pc);
					break;
				case MILESTONE:
					planner_print_job_moveto (data->job,
								  element->x1,
								  element->y1);
					planner_print_job_lineto (data->job,
								  element->x1 + data->milestone_size,
								  element->y1 + data->milestone_size);
					planner_print_job_lineto (data->job,
								  element->x1,
								  element->y1 + 2 * data->milestone_size);
					planner_print_job_lineto (data->job,
								  element->x1 - data->milestone_size,
								  element->y1 + data->milestone_size);
					gnome_print_closepath (data->job->pc);
					gnome_print_fill (data->job->pc);
					break;
				case RESOURCES:
					planner_print_job_show_clipped (data->job,
									element->x1,
									element->y1,
									element->resources,
									0, 0,
									data->job->width,
									data->job->height);
					break;
				default:
					break;
				}
			}

			planner_print_job_finish_page (data->job, TRUE);
			t1 = t2;
		}

		if (row == 0) {
			i += data->tasks_per_page_with_header;
		} else {
			i += data->tasks_per_page_without_header;
		}
	}
}

PlannerGanttPrintData *
planner_gantt_print_data_new (PlannerView     *view,
			      PlannerPrintJob *job,
			      GtkTreeView     *tree_view,
			      gint             level,
			      gboolean         show_critical)
{
	PlannerGanttPrintData *data;
	GnomeFont             *font;
	GList                 *tasks = NULL, *l;
	gint                   num_tasks;
	gdouble                max_name_width = 0.0;

	data = g_new0 (PlannerGanttPrintData, 1);

	data->view = view;
	data->job = job;
	data->project = planner_window_get_project (view->main_window);

	data->tree_view = tree_view;

	data->show_critical = show_critical;
	data->level = level;
	
	/* Note: This looks hackish, but it's the same equation used for the
	 * zoom level in the gantt chart, which actually is calculated to have a
	 * "good feel" :). We scale it with the paper size here though...
	 */
	data->f = 1000 / pow (2, level - 19) / data->job->width;

	data->major_unit = planner_scale_conf[level].major_unit;
	data->major_format = planner_scale_conf[level].major_format;

	data->minor_unit = planner_scale_conf[level].minor_unit;
	data->minor_format = planner_scale_conf[level].minor_format;

	font = planner_print_job_get_font (job);
	
	data->task_start_coords = g_hash_table_new_full (NULL, NULL, NULL, g_free);
	data->task_finish_coords = g_hash_table_new_full (NULL, NULL, NULL, g_free);

	/* Start and finish of the project. */
	data->start = mrp_project_get_project_start (data->project);
	
	tasks = gantt_print_get_visible_tasks (data);
	data->tasks = tasks;
	num_tasks = g_list_length (tasks);

	data->finish = data->start;
	
	/* Go through the tasks and get the end time by checking the right-most
	 * resource label we will print.
	 */
	for (l = tasks; l; l = l->next) {
		PrintTask *ptask = l->data;
		MrpTask   *task  = ptask->task;
		gchar     *name;
		mrptime    finish;
		gdouble    width;
		gdouble    name_width;
		
		g_object_get (task,
			      "name", &name,
			      "finish", &finish,
			      NULL);

		name_width = gnome_font_get_width_utf8 (font, name) + 
			ptask->depth * INDENT_FACTOR * data->job->x_pad;
		
		if (max_name_width < name_width) {
			max_name_width = name_width;
		}

		gantt_print_get_allocated_resources_string (data, task, NULL, &width);

		data->finish = MAX (data->finish, finish);/* + width * data->f);*/
	}
	
	data->name_x1 = 0;
	data->name_x2 = data->name_x1 + max_name_width + gnome_font_get_width_utf8 (font, "WW");
	
	data->work_x1 = data->name_x2;
	data->work_x2 = data->work_x1 + gnome_font_get_width_utf8 (font, "WORKWO");

	data->tree_x1 = 0;
	data->tree_x2 = data->work_x2;
	
	data->row_height = 2 * planner_print_job_get_font_height (job);
	
	data->header_height = 2 * data->row_height;
	
	/* Calculate drawing "constants". */
	data->summary_height = 0.36 * data->row_height;
	data->summary_thick  = 0.12 * data->row_height;
	data->summary_slope  = 0.28 * data->row_height;
	data->milestone_size = 0.40 * data->row_height;
	data->arrow_height   = 0.24 * data->row_height;
	data->arrow_width    = 0.16 * data->row_height;
	
	if (num_tasks > 0) {
		data->tasks_per_page_without_header = data->job->height / data->row_height;
		data->tasks_per_page_with_header = (data->job->height - data->header_height) /
			data->row_height;
		
		data->cols_of_pages = ceil (((data->finish - data->start) /
					     data->f + data->tree_x2 - data->tree_x1) /
					    data->job->width);
		
		data->rows_of_pages = ceil ((num_tasks * data->row_height + data->header_height) /
					    (data->job->height - data->row_height));
 		
		if (data->tasks_per_page_without_header * (data->rows_of_pages - 2) +
		    data->tasks_per_page_with_header >= num_tasks) {
			data->rows_of_pages--;
		}
		
		data->cols_of_pages = MAX (1, data->cols_of_pages);
		data->rows_of_pages = MAX (1, data->rows_of_pages);
		
 		data->pages = g_new0 (Page, data->cols_of_pages * data->rows_of_pages);
	}
	
	return data;
}

static void
free_page (Page *page)
{
	GList   *l;
	Element *element;

	for (l = page->elements; l; l = l->next) {
		element = l->data;
		g_free (element->resources);
	}

	for (l = page->background_elements; l; l = l->next) {
		element = l->data;
		g_free (element->resources);
	}

	g_list_free (page->elements);
	g_list_free (page->background_elements);
}

void
planner_gantt_print_data_free (PlannerGanttPrintData *data)
{
	gint i, num_pages;
	
	g_return_if_fail (data != NULL);

	g_hash_table_destroy (data->task_start_coords);
	g_hash_table_destroy (data->task_finish_coords);
	
	gantt_print_free_print_tasks (data->tasks);
	data->tasks = NULL;

	num_pages = data->cols_of_pages * data->rows_of_pages;
	
	for (i = 0; i < num_pages; i++) {
		free_page (&data->pages[i]);
	}
	
	g_free (data->pages);
	data->pages = NULL;

	g_free (data);
}

gint
planner_gantt_print_get_n_pages (PlannerGanttPrintData *data)
{
	g_return_val_if_fail (data != NULL, 0);
	
	return data->cols_of_pages * data->rows_of_pages;
}
