/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2005 Francisco Moraes
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

#include <windows.h>
#include <shellapi.h>
#include <stdio.h>

#include "planner-util.h"

static GQuark quark = 0;

static GQuark
planner_win32_get_quark ()
{
	if (quark == 0)
		quark = g_quark_from_static_string ("planner-win32-quark");
	return quark;
}

gboolean
planner_util_show_url (const gchar *url,
		       GError     **error)
{
	ShellExecute (NULL, "open", url, NULL, NULL, SW_SHOWNORMAL);

	// Fix me later
	return TRUE;
}

gboolean
planner_util_show_help (GError      **error)
{
	int    res;
	gchar *path;
	gchar *file;

	// should use HtmlHelp but it is not part of MingW yet
	path = g_win32_get_package_installation_subdirectory (NULL, NULL, ".");
	file = g_build_filename (path, "planner.chm", NULL);

	res = (int) ShellExecute (NULL, "open", file, NULL, NULL, SW_SHOWNORMAL);
	g_free (file);
	g_free (path);
	
	if (res <= 32) {
		g_set_error (error,
			     planner_win32_get_quark (),
			     0,
			     "Unable to open help");
		return FALSE;
	}
	return TRUE;
}
