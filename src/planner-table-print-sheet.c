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

#include <config.h>
#include <math.h>
#include <string.h>
#include <gtk/gtk.h>
#include <libplanner/planner.h>
#include <glib/gi18n.h>
#include "planner-print-job.h"
#include "planner-table-print-sheet.h"

#define d(x)

#define TEXT_IN_CELL_MULTI 0.75
#define INDENT_FACTOR 4

#define PRINT_ROW(o)  ((PrintRow *) o)
#define PRINT_COL(o)  ((PrintColumn *) o)
#define PRINT_PAGE(o) ((PrintPage *) o)

typedef struct {
	GSList  *columns;
	GSList  *rows;

	gdouble  height;
	gdouble  width;
} PrintPage;

typedef struct {
	GtkTreeViewColumn   *tree_column;
	gchar               *name;

	gdouble              width;

	gboolean             expander_column;

	GtkTreeCellDataFunc  data_func;
	gpointer             user_data;
} PrintColumn;

typedef struct {
	GtkTreePath *path;
	gdouble      height;
	gboolean     header;
} PrintRow;

struct _PlannerTablePrintSheet {
	PlannerView     *view;
	PlannerPrintJob *job;
	GtkTreeView     *tree_view;

	gdouble          x_pad;

	GSList          *pages;

	/* Used during creation */
	PangoFontDescription *font;
	GSList          *columns;
	GSList          *rows;
	gdouble          row_height;
	gdouble          page_width;
	gdouble          page_height;
};

static void         table_print_sheet_print_header_cell (PlannerTablePrintSheet *sheet,
							 PrintColumn            *column,
							 PrintRow               *row,
							 gdouble                 x,
							 gdouble                 y);
static void         table_print_sheet_print_cell        (PlannerTablePrintSheet *sheet,
							 PrintColumn            *column,
							 PrintRow               *row,
							 gdouble                 x,
							 gdouble                 y);
static void         table_print_sheet_print_page        (PlannerTablePrintSheet *sheet,
							 PrintPage              *page);
static gboolean     table_print_sheet_foreach_row       (GtkTreeModel           *model,
							 GtkTreePath            *path,
							 GtkTreeIter            *iter,
							 gpointer                user_data);
static PrintColumn *table_print_sheet_create_column     (PlannerTablePrintSheet *sheet,
							 GtkTreeViewColumn      *tree_column,
							 gboolean                expander_column);
static void         table_print_sheet_create_pages      (PlannerTablePrintSheet *sheet);
static GSList *     table_print_sheet_add_row_of_pages  (PlannerTablePrintSheet *sheet,
							 GSList                 *page_row,
							 GSList                 *rows,
							 gboolean                new_row);
static void         table_print_sheet_fill_page         (PlannerTablePrintSheet *sheet,
							 PrintPage              *page);


static void
table_print_sheet_print_header_cell (PlannerTablePrintSheet *sheet,
				     PrintColumn            *column,
				     PrintRow               *row,
				     gdouble                 x,
				     gdouble                 y)
{
	gdouble text_x;
	gdouble text_y;

	text_x = x + sheet->x_pad;
	text_y = y + TEXT_IN_CELL_MULTI * row->height;

	planner_print_job_set_font_bold (sheet->job);

	planner_print_job_text (sheet->job, text_x, text_y, column->name);

	planner_print_job_set_font_regular (sheet->job);
}

static void
table_print_sheet_print_cell (PlannerTablePrintSheet *sheet,
			      PrintColumn            *column,
			      PrintRow               *row,
			      gdouble                 x,
			      gdouble                 y)
{
	GtkTreeModel    *model;
	gdouble          text_x;
	gdouble          text_y;
	gchar           *str;
	GtkTreeIter      iter;
	GtkCellRenderer *cell;
	PangoWeight      weight;
	gint             depth = 0;

	if (row->header) {
		table_print_sheet_print_header_cell (sheet, column, row, x, y);
		return;
	}

	cell  = gtk_cell_renderer_text_new ();
	if (column->expander_column) {
		depth = gtk_tree_path_get_depth (row->path);
	}

	model = gtk_tree_view_get_model (sheet->tree_view);
	gtk_tree_model_get_iter (model, &iter, row->path);

	(* column->data_func) (column->tree_column,
			       cell,
			       model,
			       &iter,
			       column->user_data);

	g_object_get (cell,
		      "text", &str,
		      "weight", &weight,
		      NULL);
	g_object_ref_sink (cell);
	g_object_unref (cell);

	if (!str) {
		return;
	}

	if (weight == PANGO_WEIGHT_BOLD) {
		planner_print_job_set_font_bold (sheet->job);
	}

	d(g_print ("Writing: [%s]\n", str));

	text_x = x + sheet->x_pad + INDENT_FACTOR * depth;
	text_y = y + TEXT_IN_CELL_MULTI * row->height;

	planner_print_job_text (sheet->job, text_x, text_y, str);
	g_free (str);

	planner_print_job_set_font_regular (sheet->job);
}

static void
table_print_sheet_print_page (PlannerTablePrintSheet *sheet, PrintPage *page)
{
	GSList  *r, *c;
	gdouble  x = 0, y = 0;

	planner_print_job_begin_next_page (sheet->job);

	cairo_set_line_width (sheet->job->cr, THIN_LINE_WIDTH);

	for (r = page->rows; r; r = r->next) {
		for (c = page->columns; c; c = c->next) {
			table_print_sheet_print_cell (sheet,
						      PRINT_COL (c->data),
						      PRINT_ROW (r->data),
						      x, y);
			x += PRINT_COL (c->data)->width;
			planner_print_job_moveto (sheet->job,
					     x, y);
			planner_print_job_lineto (sheet->job,
					     x,
					     y + PRINT_ROW (r->data)->height);
			cairo_stroke (sheet->job->cr);
		}


		y += PRINT_ROW (r->data)->height;
		planner_print_job_moveto (sheet->job, 0, y);
		planner_print_job_lineto (sheet->job, x, y);
		cairo_stroke (sheet->job->cr);

		x = 0;
	}

	planner_print_job_finish_page (sheet->job, TRUE);
}

static PrintColumn *
table_print_sheet_create_column (PlannerTablePrintSheet *sheet,
				 GtkTreeViewColumn      *tree_column,
				 gboolean                expander_column)
{
	PrintColumn *column;
	gdouble ext;

	column = g_new0 (PrintColumn, 1);

	column->tree_column = tree_column;
	column->expander_column = expander_column;
	column->name = g_strdup (gtk_tree_view_column_get_title (tree_column));
	planner_print_job_set_font_bold (sheet->job);
	ext = planner_print_job_get_extents (sheet->job, column->name);
	column->width = ext + 3 * sheet->x_pad;

	column->data_func = g_object_get_data (G_OBJECT (tree_column),
					       "data-func");
	column->user_data = g_object_get_data (G_OBJECT (tree_column),
					       "user-data");

	return column;
}

static gboolean
table_print_sheet_foreach_row (GtkTreeModel *model,
			       GtkTreePath  *path,
			       GtkTreeIter  *iter,
			       gpointer      user_data)
{
	PlannerTablePrintSheet *sheet = user_data;
	PrintRow               *row;
	GSList                 *l;
	GtkCellRenderer        *cell;
	gint                    depth;
	GtkTreeIter             parent_iter;
	GtkTreePath            *parent_path = NULL;
	gdouble                 ext;

	d(g_print ("%s\n", gtk_tree_path_to_string (path)));

	depth = gtk_tree_path_get_depth (path);

	if (gtk_tree_model_iter_parent (model, &parent_iter, iter)) {
		parent_path = gtk_tree_model_get_path (model, &parent_iter);
	}

	if (depth == 1 ||
	    gtk_tree_view_row_expanded (sheet->tree_view, parent_path)) {
		/* Create a new row */
		row = g_new0 (PrintRow, 1);
		row->path   = gtk_tree_path_copy (path);
		row->height = sheet->row_height;
		row->header = FALSE;

		sheet->rows = g_slist_prepend (sheet->rows, row);

		cell  = gtk_cell_renderer_text_new ();

		/* Loop through the columns to update widths */
		for (l = sheet->columns; l; l = l->next) {
			PrintColumn *column = PRINT_COL (l->data);
			gchar       *str;
			gdouble      extra = 3 * sheet->x_pad;

			(* column->data_func) (column->tree_column,
					       cell,
					       model,
					       iter,
					       column->user_data);
			g_object_get (cell,
				      "text", &str,
				      NULL);

 			if (column->expander_column) {
				extra += depth * INDENT_FACTOR * sheet->x_pad;
 			}

			ext = 0;
			if(str) {
				ext = planner_print_job_get_extents (sheet->job, str);
			}
			column->width = MAX (column->width,
					     ext
					     + extra);
/* 		d(g_print ("New width: %f\n", column->width)); */
			g_free (str);
		}

		g_object_ref_sink (cell);
		g_object_unref (cell);
	}
	if (parent_path) {
		gtk_tree_path_free (parent_path);
	}

	return FALSE;
}

static void
table_print_sheet_create_pages (PlannerTablePrintSheet *sheet)
{
	GSList    *l, *p = NULL;
	PrintPage *page = NULL;
	GSList    *rows, *page_row;

	sheet->pages = NULL;

	page = g_new0 (PrintPage, 1);
	page->height  = 0;
	page->width   = 0;
	page->columns = NULL;
	page->rows    = NULL;
	d(g_print ("sheet_create_pages: Adding a page\n"));

	sheet->pages = g_slist_prepend (sheet->pages, page);

	/* Create pages for all columns */
	for (l = sheet->columns; l; l = l->next) {
		PrintColumn *column = PRINT_COL (l->data);

		if ((column->width + page->width) > sheet->page_width) {
			d(g_print ("Switching page: %f > %f\n",
				   column->width + page->width,
				   sheet->page_width));

			if (p) {
				p->next = NULL;
			}

			table_print_sheet_fill_page (sheet, page);

			page          = g_new0 (PrintPage, 1);
			page->height  = 0;
			page->width   = 0;
			page->columns = NULL;
			page->rows    = NULL;
			d(g_print ("sheet_create_pages (in loop): Adding a page\n"));
			sheet->pages  = g_slist_prepend (sheet->pages, page);
		}

		page->width   += column->width;
		page->columns  = g_slist_append (page->columns, column);
		p = l;
	}

	sheet->pages = g_slist_reverse (sheet->pages);
	rows = sheet->rows;

	page_row = g_slist_copy (sheet->pages);
	rows = table_print_sheet_add_row_of_pages (sheet,
						   page_row,
						   rows,
						   FALSE);
	while (rows) {
		rows = table_print_sheet_add_row_of_pages (sheet, page_row,
							   rows, TRUE);
	}
	g_slist_free (page_row);
}

static GSList *
table_print_sheet_add_row_of_pages (PlannerTablePrintSheet *sheet,
				    GSList                 *page_row,
				    GSList                 *rows,
				    gboolean                new_row)
{
	GSList  *l;
	GSList  *added_rows = NULL;
	GSList  *ret_val = NULL;
	gdouble  added_height = 0;

	for (l = rows; l; l = l->next) {
		PrintRow *row = PRINT_ROW (l->data);

		if (added_height + row->height > sheet->page_height) {
			ret_val = l;
			break;
		}

		added_height += row->height;
		added_rows = g_slist_prepend (added_rows, l->data);
	}

	if (!l) {
		ret_val = NULL;
	}

	added_rows = g_slist_reverse (added_rows);

	for (l = page_row; l; l = l->next) {
		PrintPage *page = PRINT_PAGE (l->data);
		if (new_row) {
			PrintPage *new_page;

			new_page = g_new0 (PrintPage, 1);
			new_page->columns = g_slist_copy (page->columns);
			new_page->rows    = g_slist_copy (added_rows);
			new_page->height  = added_height;
			new_page->width   = page->width;
			d(g_print ("sheet_add_row_of_pages (in loop): Adding a page\n"));

			sheet->pages = g_slist_append (sheet->pages, new_page);
		} else {
			page->rows = g_slist_copy (added_rows);
			page->height = added_height;
		}
	}

	g_slist_free (added_rows);

	return ret_val;
}

static void
table_print_sheet_fill_page (PlannerTablePrintSheet *sheet, PrintPage *page)
{
	GSList  *l;
	gdouble  extra;
	gdouble  extra_per_column;
	gint     columns;

	d(g_print ("Fill page\n"));

	extra            = sheet->page_width - page->width;
	columns          = g_slist_length (page->columns);
	extra_per_column = extra / columns;

	for (l = page->columns; l; l = l->next) {
		PrintColumn *column = PRINT_COL (l->data);

		column->width += extra_per_column;
	}
}

PlannerTablePrintSheet *
planner_table_print_sheet_new (PlannerView     *view,
			       PlannerPrintJob *job,
			       GtkTreeView     *tree_view)
{
	PlannerTablePrintSheet *sheet;
	GtkTreeModel           *model;
	GList                  *tree_columns, *l;
	PrintRow               *row;
	gboolean                expander_column;

	sheet = g_new0 (PlannerTablePrintSheet, 1);

	sheet->view        = view;
	sheet->job         = job;
	sheet->tree_view   = tree_view;
	sheet->font        = planner_print_job_get_font (job);
	sheet->columns     = NULL;
	sheet->row_height  = 2 * planner_print_job_get_font_height (sheet->job);
	sheet->page_width  = job->width;
	sheet->page_height = job->height;
	sheet->x_pad       = job->x_pad;

	/* Add the header row */
	row = g_new0 (PrintRow, 1);
	row->path = NULL;
	row->height = 1.5 * sheet->row_height;
	row->header = TRUE;
	sheet->rows = g_slist_prepend (NULL, row);

	model = gtk_tree_view_get_model (tree_view);

	/* Create all the columns */
	tree_columns = gtk_tree_view_get_columns (tree_view);
	for (l = tree_columns; l; l = l->next) {
		PrintColumn       *column;
		GtkTreeViewColumn *tree_column;

		tree_column = GTK_TREE_VIEW_COLUMN (l->data);
		if (!gtk_tree_view_column_get_visible (tree_column)) {
			continue;
		}

		if (tree_column == gtk_tree_view_get_expander_column (tree_view)) {
			expander_column = TRUE;
		} else {
			expander_column = FALSE;
		}

		column = table_print_sheet_create_column (sheet,
							  tree_column,
							  expander_column);

		sheet->columns = g_slist_prepend (sheet->columns, column);
	}
	g_list_free (tree_columns);
	sheet->columns = g_slist_reverse (sheet->columns);

	/* Create the rows and calculate column widths */
	gtk_tree_model_foreach (model,
				table_print_sheet_foreach_row,
				sheet);

	if (g_slist_length (sheet->rows) == 1) {
		/* Only contains the header row */
		return sheet;
	}

	sheet->rows = g_slist_reverse (sheet->rows);

	table_print_sheet_create_pages (sheet);

	return sheet;
}

void
planner_table_print_sheet_output (PlannerTablePrintSheet *sheet,
				  gint page_nr)
{
	PrintPage *page = PRINT_PAGE(g_slist_nth_data (sheet->pages, page_nr));
	g_assert(page != NULL);
	table_print_sheet_print_page(sheet, page);
}

gint
planner_table_print_sheet_get_n_pages (PlannerTablePrintSheet *sheet)
{
	d(g_print ("Number of pages: %d\n", g_slist_length (sheet->pages)));

	return g_slist_length (sheet->pages);
}

void
planner_table_print_sheet_free (PlannerTablePrintSheet *sheet)
{
	GSList *l;

	for (l = sheet->pages; l; l = l->next) {
		PrintPage *page = PRINT_PAGE (l->data);

		/* The sheet holds a list with all the structs and */
		/* is responsible for freeing those */
		g_slist_free (page->columns);
		g_slist_free (page->rows);

		g_free (page);
	}

	for (l = sheet->columns; l; l = l->next) {
		PrintColumn *column = PRINT_COL (l->data);

		g_free (column->name);
		g_free (column);
	}

	for (l = sheet->rows; l; l = l->next) {
		PrintRow *row = PRINT_ROW (l->data);

		if (row->path) {
			gtk_tree_path_free (row->path);
		}
		g_free (row);
	}

	g_slist_free (sheet->pages);
	g_slist_free (sheet->columns);
	g_slist_free (sheet->rows);

	g_free (sheet);
}
