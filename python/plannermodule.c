/* -*- Mode: C; c-basic-offset: 4 -*- */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* include this first, before NO_IMPORT_PYGOBJECT is defined */
#include <pygobject.h>

void initplanner(void);
void planner_register_classes (PyObject *d);
void planner_add_constants(PyObject *module, const gchar *strip_prefix);

extern PyMethodDef planner_functions[];

DL_EXPORT(void)
initplanner(void)
{
    PyObject *m, *d;
	
    init_pygobject ();

    m = Py_InitModule ("planner", planner_functions);
    d = PyModule_GetDict (m);
	
    planner_register_classes (d);
    planner_add_constants (m, "MRP_");

    if (PyErr_Occurred ()) {
	Py_FatalError ("can't initialise module planner");
    }
}
