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

#include <Python.h>

#include <config.h>
#include <glib.h> 
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <glade/glade.h>
#include <gtk/gtkradiobutton.h>
#include <gtk/gtkmessagedialog.h>
#include <gtk/gtkentry.h>
#include <gtk/gtkstock.h>
#include <gtk/gtkfilesel.h>
#include <libgnome/libgnome.h>
#include <glib/gi18n.h>
#include <libgnomeui/gnome-file-entry.h>
#include "planner-window.h"
#include "planner-plugin.h"

#include "pygobject.h"



typedef struct _PlannerPythonEnv PlannerPythonEnv;

struct _PlannerPluginPriv {
	PlannerWindow	*main_window;
	GHashTable	*scripts;
};

struct _PlannerPythonEnv {
	char		*name;
	PyObject	*globals;
};

static PlannerPythonEnv *planner_python_env_new (const gchar       *name);
static void planner_python_env_free             (PlannerPythonEnv  *env);

static void python_plugin_execute               (const gchar       *filename,
						 PlannerWindow     *window,
						 GHashTable        *scripts);

void        plugin_init                         (PlannerPlugin     *plugin,
					         PlannerWindow     *main_window);
void        plugin_exit                         (PlannerPlugin     *plugin);


static PlannerPythonEnv *
planner_python_env_new (const gchar *name)
{
	PlannerPythonEnv *env;
	PyObject         *pDict, *pMain;

	env = g_new0 (PlannerPythonEnv,1);
	env->name = g_strdup(name);

	pDict = PyImport_GetModuleDict ();
	pMain = PyDict_GetItemString (pDict, "__main__");
	pDict = PyModule_GetDict (pMain);
	env->globals = PyDict_Copy (pDict);

	return env;
}

static void
planner_python_env_free (PlannerPythonEnv *env)
{
	g_free (env->name);
	PyDict_Clear (env->globals);
	Py_DECREF (env->globals);
	g_free (env);
}

static void
python_plugin_execute (const gchar   *filename,
		       PlannerWindow *window,
		       GHashTable    *scripts)
{
	PlannerPythonEnv  *env;
	/* MrpProject        *project; */

	FILE              *fp;
	PyObject          *pModule;
	PyObject          *py_widget;

	env = planner_python_env_new (filename);

	/* Import pygtk */
	pModule = PyRun_String ("import pygtk\n"
				"pygtk.require('2.0')\n"
				"import gtk\n"
				"import gnome\n",
				Py_file_input, env->globals, env->globals);
	if (pModule == NULL) {
		PyErr_Print ();
		planner_python_env_free (env);
		return;
	}

	/* Import planner */
	pModule = PyImport_ImportModuleEx ("planner", env->globals, env->globals, Py_None);

	if (pModule == NULL) {
		PyErr_Print ();
		planner_python_env_free (env);
		return;
	}

	py_widget = pygobject_new ((GObject *) window);
	PyDict_SetItemString (env->globals, "window", py_widget);
	Py_DECREF (py_widget);

	fp = fopen (filename,"r");
	if (fp != NULL) {
		if (PyRun_File (fp, (char *) filename, Py_file_input, env->globals, env->globals) == NULL) {
			PyErr_Print ();
		}
		fclose (fp);
		g_hash_table_insert (scripts,(gpointer)filename,env);
	} else {
		/* FIXME: do the free */
		g_message("Could not open file: %s",filename);
	}
}


G_MODULE_EXPORT void 
plugin_init (PlannerPlugin *plugin, PlannerWindow *main_window)
{
	PlannerPluginPriv *priv;
	GDir              *dir;
	gchar             *dirname, *fullfilename;
	const gchar       *filename;
	
	priv = g_new0 (PlannerPluginPriv, 1);
	plugin->priv = priv;
	priv->main_window = main_window;
	priv->scripts = g_hash_table_new (g_str_hash, g_str_equal);
	
	Py_Initialize ();
	
	/* Look in ~/.gnome2/planner/python/  and run the scripts that we find */
	dirname = g_build_path(G_DIR_SEPARATOR_S,g_get_home_dir(),GNOME_DOT_GNOME,"planner","python",NULL);
	dir = g_dir_open(dirname,0,NULL);
	if (dir == NULL) {
		g_free(dirname);
		return;
	}

	filename = g_dir_read_name(dir);
	while (filename != NULL) {
		fullfilename = g_build_filename(dirname,filename,NULL);
		python_plugin_execute(fullfilename,main_window,priv->scripts);
		g_free(fullfilename);
		filename = g_dir_read_name(dir);
	}
	g_free(dirname);
	g_dir_close(dir);
}

G_MODULE_EXPORT void 
plugin_exit (PlannerPlugin *plugin) 
{
	g_message ("%s(%i) FIXME: free the Python plugin priv structure !!!",__FILE__,__LINE__);
	if (plugin != NULL) {
		/*planner_python_env_free (NULL);*/
	}
	Py_Finalize ();
}
