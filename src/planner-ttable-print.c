
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
#include "planner-ttable-print.h"

typedef struct {
        MrpResource *resource;
        MrpAssignment *assignment;
} Ligne;

typedef struct {
        gboolean has_resource;
        gboolean has_task;
        GList *lines;
        gint n_lines;
} Page;


struct _PlannerTtablePrintData {
        MrpProject *project;
        PlannerView *view;
        PlannerPrintJob *job;
        Page *pages;

        gint pages_x;
        gint pages_y;
        gint n_pages;
        gint lines;
        gint lines_per_page;

        gboolean task_on_first;

        gdouble res_width;
        gdouble task_width;
        gdouble row_height;
        gdouble page_width;
        gdouble page_height;

        gdouble free_right_first;
};

#define TEXT_PAD 15.0

void
planner_ttable_print_do (PlannerTtablePrintData * data)
{
        int i, j;
        gdouble cur_x;

        return;                 /* FIXME: Release hook to avoid printing */

        for (i = 0; i < data->n_pages; i++) {
                Page *page;
                cur_x = 0;
                page = data->pages + i;
                planner_print_job_begin_next_page (data->job);
                if (page->has_resource) {
                        GList *l;
                        planner_print_job_moveto (data->job, 0, 0);
                        planner_print_job_lineto (data->job,
                                                  data->res_width +
                                                  2 * TEXT_PAD, 0);
                        planner_print_job_lineto (data->job,
                                                  data->res_width +
                                                  2 * TEXT_PAD,
                                                  (page->n_lines +
                                                   2) * data->row_height);
                        planner_print_job_lineto (data->job, 0,
                                                  (page->n_lines +
                                                   2) * data->row_height);
                        gnome_print_closepath (data->job->pc);
                        gnome_print_stroke (data->job->pc);
                        j = 1;
                        planner_print_job_set_font_bold (data->job);
                        planner_print_job_show_clipped (data->job,
                                                        cur_x + TEXT_PAD,
                                                        j * data->row_height,
                                                        _("Name"),
                                                        cur_x + TEXT_PAD,
                                                        (j -
                                                         1) *
                                                        data->row_height,
                                                        cur_x + TEXT_PAD +
                                                        data->res_width,
                                                        (j +
                                                         1) *
                                                        data->row_height);
                        j++;
                        planner_print_job_set_font_regular (data->job);
                        for (l = page->lines; l; l = l->next) {
                                Ligne *line;
                                line = (Ligne *) l->data;
                                j++;
                                planner_print_job_moveto (data->job, 0,
                                                          j *
                                                          data->row_height);
                                planner_print_job_lineto (data->job,
                                                          data->res_width +
                                                          2 * TEXT_PAD,
                                                          j *
                                                          data->row_height);
                                gnome_print_stroke (data->job->pc);
                                if (line->resource) {
                                        gchar *name;
                                        g_object_get (line->resource, "name",
                                                      &name, NULL);
                                        planner_print_job_show_clipped (data->
                                                                        job,
                                                                        cur_x
                                                                        +
                                                                        TEXT_PAD,
                                                                        (j +
                                                                         1) *
                                                                        data->
                                                                        row_height,
                                                                        name,
                                                                        cur_x
                                                                        +
                                                                        TEXT_PAD,
                                                                        (j) *
                                                                        data->
                                                                        row_height,
                                                                        cur_x
                                                                        +
                                                                        TEXT_PAD
                                                                        +
                                                                        data->
                                                                        res_width,
                                                                        (j +
                                                                         1) *
                                                                        data->
                                                                        row_height);
                                        g_free (name);
                                }
                        }
                        cur_x += data->res_width + 2 * TEXT_PAD;
                }
                if (page->has_task) {
                        GList *l;
                        planner_print_job_moveto (data->job, cur_x, 0);
                        planner_print_job_lineto (data->job,
                                                  data->task_width +
                                                  2 * TEXT_PAD, 0);
                        planner_print_job_lineto (data->job,
                                                  data->task_width +
                                                  2 * TEXT_PAD,
                                                  (page->n_lines +
                                                   2) * data->row_height);
                        planner_print_job_lineto (data->job, cur_x,
                                                  (page->n_lines +
                                                   2) * data->row_height);
                        gnome_print_closepath (data->job->pc);
                        gnome_print_stroke (data->job->pc);
                        j = 1;
                        planner_print_job_set_font_bold (data->job);
                        planner_print_job_show_clipped (data->job,
                                                        cur_x + TEXT_PAD,
                                                        j * data->row_height,
                                                        _("Task"),
                                                        cur_x + TEXT_PAD,
                                                        (j -
                                                         1) *
                                                        data->row_height,
                                                        cur_x + TEXT_PAD +
                                                        data->task_width,
                                                        (j +
                                                         1) *
                                                        data->row_height);
                        j++;
                        planner_print_job_set_font_regular (data->job);
                        for (l = page->lines; l; l = l->next) {
                                Ligne *line;
                                line = (Ligne *) l->data;
                                j++;
                                planner_print_job_moveto (data->job, 0,
                                                          j *
                                                          data->row_height);
                                planner_print_job_lineto (data->job,
                                                          data->task_width +
                                                          2 * TEXT_PAD,
                                                          j *
                                                          data->row_height);
                                gnome_print_stroke (data->job->pc);
                                if (line->assignment) {
                                        MrpTask *task;
                                        gchar *name;
                                        task = mrp_assignment_get_task (line->
                                                                        assignment);
                                        g_object_get (task, "name", &name,
                                                      NULL);
                                        planner_print_job_show_clipped (data->
                                                                        job,
                                                                        cur_x
                                                                        +
                                                                        TEXT_PAD,
                                                                        (j +
                                                                         1) *
                                                                        data->
                                                                        row_height,
                                                                        name,
                                                                        cur_x
                                                                        +
                                                                        TEXT_PAD,
                                                                        (j) *
                                                                        data->
                                                                        row_height,
                                                                        cur_x
                                                                        +
                                                                        TEXT_PAD
                                                                        +
                                                                        data->
                                                                        task_width,
                                                                        (j +
                                                                         1) *
                                                                        data->
                                                                        row_height);
                                        g_free (name);
                                }
                        }
                        cur_x += data->task_width + 2 * TEXT_PAD;
                }
                planner_print_job_finish_page (data->job, TRUE);
        }
        /*
         * planner_print_job_begin_next_page (data->job);
         * planner_print_job_moveto(data->job,0,0);
         * planner_print_job_lineto(data->job,data->job->width,0);
         * planner_print_job_lineto(data->job,data->job->width,data->job->height);
         * planner_print_job_lineto(data->job,0,data->job->height);
         * gnome_print_closepath(data->job->pc);
         * gnome_print_stroke(data->job->pc);
         * planner_print_job_finish_page (data->job, TRUE);
         */
}

PlannerTtablePrintData *
planner_ttable_print_data_new (PlannerView * view, PlannerPrintJob * job)
{
        PlannerTtablePrintData *data;

        data = g_new0 (PlannerTtablePrintData, 1);
        data->view = view;
        data->job = job;
        data->project = planner_window_get_project (view->main_window);
        return data;
}

void
planner_ttable_print_data_free (PlannerTtablePrintData * data)
{
        g_return_if_fail (data != NULL);
        g_free (data);
}

gint
planner_ttable_print_get_n_pages (PlannerTtablePrintData * data)
{
        GnomeFont *font;
        GList *r, *a;
        gdouble width;
        gint cur_page, cur_line;
        gint i;

        g_return_val_if_fail (data != NULL, 0);

        return data->pages_x * data->pages_y;   /* FIXME: Release hook to avoid printing */

        data->lines = 0;
        font = planner_print_job_get_font (data->job);
        r = mrp_project_get_resources (data->project);
        for (; r; r = r->next) {
                MrpResource *res;
                gchar *name;
                res = MRP_RESOURCE (r->data);
                g_object_get (res, "name", &name, NULL);
                width = gnome_font_get_width_utf8 (font, name);
                data->res_width = MAX (data->res_width, width);
                g_free (name);

                data->lines++;
                a = mrp_resource_get_assignments (res);
                for (; a; a = a->next) {
                        MrpAssignment *assign;
                        MrpTask *task;
                        assign = MRP_ASSIGNMENT (a->data);
                        task = mrp_assignment_get_task (assign);
                        g_object_get (task, "name", &name, NULL);
                        width = gnome_font_get_width_utf8 (font, name);
                        data->task_width = MAX (data->task_width, width);
                        g_free (name);
                        data->lines++;
                }
        }
        data->row_height = 2 * planner_print_job_get_font_height (data->job);
        data->page_height = data->job->height;
        data->page_width = data->job->width;
        data->res_width =
                MIN (data->res_width, data->page_width - 2 * TEXT_PAD);
        data->task_width =
                MIN (data->task_width, data->page_width - 2 * TEXT_PAD);

        data->lines_per_page =
                floor (data->page_height / data->row_height) - 2;
        data->pages_y = data->lines / data->lines_per_page + 1;
        data->pages_x = 1;

        data->free_right_first = data->page_width;
        data->free_right_first -= TEXT_PAD;
        data->free_right_first -= data->res_width;
        data->free_right_first -= TEXT_PAD;
        if (data->free_right_first > data->task_width + 2 * TEXT_PAD) {
                data->free_right_first -= data->task_width + 2 * TEXT_PAD;
                data->task_on_first = TRUE;
        } else {
                data->pages_x++;
                data->task_on_first = FALSE;
                data->free_right_first =
                        data->page_width - 2 * TEXT_PAD - data->task_width;
        }
        data->n_pages = data->pages_x * data->pages_y;

        data->pages = g_new0 (Page, data->n_pages);
        for (i = 0; i < data->n_pages; i++) {
                data->pages[i].has_resource = FALSE;
                data->pages[i].has_task = FALSE;
                data->pages[i].n_lines = 0;
        }

        r = mrp_project_get_resources (data->project);
        cur_line = 0;
        cur_page = 0;
        data->pages[0].has_resource = TRUE;
        if (data->task_on_first) {
                data->pages[0].has_task = TRUE;
        } else {
                data->pages[1].has_task = TRUE;
        }
        for (; r; r = r->next) {
                MrpResource *res;
                Ligne *line;
                res = MRP_RESOURCE (r->data);
                a = mrp_resource_get_assignments (res);
                line = g_new0 (Ligne, 1);
                line->assignment = NULL;
                line->resource = res;
                for (i = 0; i < data->pages_x; i++) {
                        data->pages[cur_page + i].lines =
                                g_list_append (data->pages[cur_page + i].
                                               lines, line);
                        data->pages[cur_page + i].n_lines++;
                }
                for (; a; a = a->next) {
                        MrpAssignment *assign;
                        assign = MRP_ASSIGNMENT (a->data);
                        line = g_new0 (Ligne, 1);
                        line->assignment = assign;
                        line->resource = NULL;
                        for (i = 0; i < data->pages_x; i++) {
                                data->pages[cur_page + i].lines =
                                        g_list_append (data->
                                                       pages[cur_page +
                                                             i].lines, line);
                                data->pages[cur_page + i].n_lines++;
                        }
                        cur_line++;
                        if (cur_line == data->lines_per_page) {
                                cur_page += data->pages_x;
                                cur_line = 0;
                                data->pages[cur_page].has_resource = TRUE;
                                if (data->task_on_first) {
                                        data->pages[cur_page].has_task = TRUE;
                                } else {
                                        data->pages[cur_page + 1].has_task =
                                                TRUE;
                                }
                        }
                }
        }

        return data->pages_x * data->pages_y;
}
