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
#include <bonobo/bonobo-ui-component.h>
#include <bonobo/bonobo-ui-util.h>
#include <glade/glade.h>
#include <gtk/gtkradiobutton.h>
#include <gtk/gtkmessagedialog.h>
#include <gtk/gtkentry.h>
#include <gtk/gtkstock.h>
#include <gtk/gtkfilesel.h>
#include <libgnome/gnome-i18n.h>
#include <libgnomeui/gnome-file-entry.h>
#include "planner-window.h"
#include "planner-plugin.h"

#include "pygobject.h"

struct _PlannerPluginPriv {
	PlannerWindow *main_window;
	GtkWidget     *dialog;
};

static void python_plugin_execute             (BonoboUIComponent *component,
					       gpointer           user_data,
					       const gchar       *cname);

void        plugin_init                       (PlannerPlugin     *plugin,
					       PlannerWindow     *main_window);
void        plugin_exit                       (PlannerPlugin     *plugin);


static BonoboUIVerb verbs[] = {
	BONOBO_UI_VERB ("Python script", python_plugin_execute),
	BONOBO_UI_VERB_END
};




static gboolean
window_file_is_dir (const gchar *file)
{
	struct stat sb;

	if ((stat (file, &sb) == 0) && S_ISDIR (sb.st_mode)) {
		return TRUE;
	}

	return FALSE;
}

/*
static gchar *
get_last_dir (PlannerWindow *window)
{
	PlannerWindowPriv *priv;
	GConfClient      *gconf_client;
	gchar            *last_dir;
	
	priv = window->priv;
	
	gconf_client = planner_application_get_gconf_client (priv->application);
	
	last_dir = gconf_client_get_string (gconf_client,
					    GCONF_PATH "/general/last_dir",
					    NULL);
	
	if (last_dir == NULL) {
		last_dir = g_strdup (g_get_home_dir ());
	}
	
	if (last_dir[strlen (last_dir)] != G_DIR_SEPARATOR) {
		gchar *tmp;
		
		tmp = g_strconcat (last_dir, G_DIR_SEPARATOR_S, NULL);
		g_free (last_dir);
		last_dir = tmp;
	}

	return last_dir;
}
*/

static void
python_plugin_execute (BonoboUIComponent *component,
		       gpointer           user_data,
		       const gchar       *cname)
{
	PlannerWindow     *window;
	PlannerPluginPriv *priv;

	GtkWidget        *file_sel;
	gint              response;
	const gchar      *filename = NULL;
	gchar            *last_dir;

	priv = PLANNER_PLUGIN (user_data)->priv;
	window = priv->main_window;

	file_sel = gtk_file_selection_new (_("Open a file"));

	/* last_dir = get_last_dir (window); */
	last_dir = g_strdup_printf ("%s/",g_get_home_dir ());
	gtk_file_selection_set_filename (GTK_FILE_SELECTION (file_sel), last_dir);
	g_free (last_dir);

	gtk_window_set_modal (GTK_WINDOW (file_sel), TRUE);

	gtk_widget_show (file_sel);

	response = gtk_dialog_run (GTK_DIALOG (file_sel));

	if (response == GTK_RESPONSE_OK) {
		filename = gtk_file_selection_get_filename (
			GTK_FILE_SELECTION (file_sel));
		if (window_file_is_dir (filename)) {
			filename = NULL;
		}
	}
	
	gtk_widget_destroy (file_sel);

	if (filename != NULL) {
		FILE *fp;
		PyObject *pModule, *pName;
		/* PyObject *pDict, *pMain; */

		fp = fopen(filename,"r");
		Py_Initialize();

		/* Import pygtk */
		PyRun_SimpleString("import pygtk\n");
		PyRun_SimpleString("pygtk.require('2.0')\n");
		PyRun_SimpleString("import gtk\n");

		/* Import planner */
		pName = PyString_FromString("planner");
		pModule = PyImport_Import(pName);
		Py_DECREF(pName);
		/*
		pDict = PyImport_GetModuleDict();
		pMain = PyDict_GetItemString(pDict,"__main__");
		pDict = PyModule_GetDict(pMain);
		PyDict_SetItemString(pDict,"planner",pModule);
		*/

		if (pModule != NULL) {
			PyObject   *py_widget;
			PyObject   *pDict, *pMain;
			MrpProject *project;

			pDict = PyImport_GetModuleDict();
			pMain = PyDict_GetItemString(pDict,"__main__");
			pDict = PyModule_GetDict(pMain);

			project = planner_window_get_project(window);
			py_widget = pygobject_new((GObject *)project);
			PyDict_SetItemString(pDict, "project", py_widget);
			Py_DECREF(py_widget);

			PyRun_SimpleFile(fp,filename);
		}
		Py_Finalize();
		fclose(fp);
	}
}

G_MODULE_EXPORT void 
plugin_init (PlannerPlugin *plugin, PlannerWindow *main_window)
{
	PlannerPluginPriv *priv;
	BonoboUIContainer *ui_container;
	BonoboUIComponent *ui_component;
	
	priv = g_new0 (PlannerPluginPriv, 1);
	plugin->priv = priv;
	priv->main_window = main_window;
	
	ui_container = planner_window_get_ui_container (main_window);
	ui_component = bonobo_ui_component_new_default ();
	
	bonobo_ui_component_set_container (ui_component, 
					   BONOBO_OBJREF (ui_container),
					   NULL);
	bonobo_ui_component_freeze (ui_component, NULL);
	bonobo_ui_component_add_verb_list_with_data (ui_component, 
						     verbs,
						     plugin);
	bonobo_ui_util_set_ui (ui_component,
			       DATADIR,
			       "/planner/ui/python-plugin.ui",
			       "pythonplugin",
			       NULL);
	
	bonobo_ui_component_thaw (ui_component, NULL);
}

G_MODULE_EXPORT void 
plugin_exit (PlannerPlugin *plugin) 
{
	/*g_message ("Test exit");*/
}
