/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*-
   
   self-checks.c: The self-check framework.
 
   Copyright (C) 1999 Eazel, Inc.
  
   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.
  
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.
  
   You should have received a copy of the GNU Library General Public
   License along with this program; if not, write to the
   Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
  
   Author: Darin Adler <darin@eazel.com>
*/

#include <config.h>
#include "self-check.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static gboolean failed;

static const char *current_expression;
static const char *current_file_name;
static int current_line_number;


void
report_check_failure (char *result, char *expected)
{
	if (!failed) {
		fprintf (stderr, "\n");
	}

	fprintf (stderr, "FAIL: check failed in %s, line %d\n", current_file_name, current_line_number);
	fprintf (stderr, "      evaluated: %s\n", current_expression);
	fprintf (stderr, "       expected: %s\n", expected == NULL ? "NULL" : expected);
	fprintf (stderr, "            got: %s\n", result == NULL ? "NULL" : result);
	
	failed = TRUE;

	g_free (result);
	g_free (expected);

	exit (EXIT_FAILURE);
}

void
before_check (const char *expression,
	      const char *file_name,
	      int line_number)
{
	current_expression = expression;
	current_file_name = file_name;
	current_line_number = line_number;
}

void
after_check (void)
{
	/* It would be good to check here if there was a memory leak. */
}

void
check_string_result (char *result, const char *expected)
{
	gboolean match;
	
	/* Stricter than eel_strcmp.
	 * NULL does not match "" in this test.
	 */
	if (expected == NULL) {
		match = result == NULL;
	} else {
		match = result != NULL && strcmp (result, expected) == 0;
	}

	if (!match) {
		report_check_failure (result, g_strdup (expected));
	} else {
		g_free (result);
	}
	after_check ();
}

void
check_integer_result (long result, long expected)
{
	if (result != expected) {
		report_check_failure (g_strdup_printf ("%ld", result),
					  g_strdup_printf ("%ld", expected));
	}
	after_check ();
}

void
check_pointer_result (gconstpointer result, gconstpointer expected)
{
	if (result != expected) {
		report_check_failure (g_strdup_printf ("%p", result),
				      g_strdup_printf ("%p", expected));
	}
	after_check ();
}
