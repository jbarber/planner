/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2004 Xavier Ordoquy <xordoquy@wanadoo.fr>
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
#include <Python.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <pygobject.h>
#include "planner-window.h"
#include "planner-plugin.h"

struct _PlannerPluginPriv {
	GHashTable    *scripts;
};

typedef struct {
	gchar    *filename;
	PyObject *globals;
} PlannerPythonEnv;

static PlannerPythonEnv *planner_python_env_new  (const gchar      *name);
static void              planner_python_env_free (PlannerPythonEnv *env);
static void              python_plugin_execute   (const gchar      *filename,
						  PlannerWindow    *window,
						  GHashTable       *scripts);
void                     plugin_init             (PlannerPlugin    *plugin);
void                     plugin_exit             (PlannerPlugin    *plugin);



static PlannerPythonEnv *
planner_python_env_new (const gchar *filename)
{
	PlannerPythonEnv *env;
	PyObject         *pDict, *pMain;

	env = g_new0 (PlannerPythonEnv,1);
	env->filename = g_strdup (filename);

	pDict = PyImport_GetModuleDict ();
	pMain = PyDict_GetItemString (pDict, "__main__");
	pDict = PyModule_GetDict (pMain);
	env->globals = PyDict_Copy (pDict);

	return env;
}

static void
planner_python_env_free (PlannerPythonEnv *env)
{
	g_free (env->filename);
	PyDict_Clear (env->globals);
	Py_DECREF (env->globals);
	g_free (env);
}

static void
python_plugin_execute (const gchar   *filename,
		       PlannerWindow *window,
		       GHashTable    *scripts)
{
	PlannerPythonEnv *env;
	FILE             *fp;
	PyObject         *pModule;
	PyObject         *py_object;

	env = planner_python_env_new (filename);

	pModule = PyRun_String ("import pygtk\n"
				"pygtk.require('2.0')\n"
				"import gtk\n"
				"import planner\n",
				Py_file_input, env->globals, env->globals);
	if (!pModule) {
		PyErr_Print ();
		planner_python_env_free (env);
		return;
	}

	pModule = PyImport_ImportModuleEx ("plannerui", env->globals, env->globals, Py_None);
	if (!pModule) {
		PyErr_Print ();
		planner_python_env_free (env);
		return;
	}

	py_object = pygobject_new (G_OBJECT (window));
	PyDict_SetItemString (env->globals, "window", py_object);
	Py_DECREF (py_object);

	py_object = pygobject_new (G_OBJECT (planner_window_get_application (window)));
	PyDict_SetItemString (env->globals, "application", py_object);
	Py_DECREF (py_object);

	fp = fopen (filename,"r");
	if (fp) {
		if (PyRun_File (fp, (gchar *) filename, Py_file_input, env->globals, env->globals) == NULL) {
			PyErr_Print ();
		}
		fclose (fp);
		g_hash_table_insert (scripts, env->filename, env);
	} else {
		planner_python_env_free (env);

		/* FIXME: do the free */
		g_warning ("Could not open python script: %s", filename);
	}
}

G_MODULE_EXPORT void
plugin_init (PlannerPlugin *plugin)
{
	PlannerPluginPriv *priv;
	GDir              *dir;
	gchar             *dirname, *full_filename;
	const gchar       *filename;

	priv = g_new0 (PlannerPluginPriv, 1);
	plugin->priv = priv;

	priv->scripts = g_hash_table_new (g_str_hash, g_str_equal);

	Py_Initialize ();

	/* Look in ~/.gnome2/planner/python/  and run the scripts that we find */
	dirname = g_build_filename (g_get_home_dir(), ".gnome2", "planner", "python", NULL);
	dir = g_dir_open (dirname, 0, NULL);
	if (dir == NULL) {
		g_free (dirname);
		return;
	}

	filename = g_dir_read_name (dir);
	while (filename != NULL) {
		if (g_str_has_suffix (filename, ".py")) {
			full_filename = g_build_filename (dirname, filename, NULL);
			python_plugin_execute (full_filename, plugin->main_window, priv->scripts);
			g_free (full_filename);
		}

		filename = g_dir_read_name (dir);
	}

	g_free (dirname);
	g_dir_close (dir);
}

G_MODULE_EXPORT void
plugin_exit (PlannerPlugin *plugin)
{
	PlannerPluginPriv *priv;

	priv = plugin->priv;

	/* FIXME: free everything in the hash table */

	Py_Finalize ();

	g_free (priv);
}
