/* -*- Mode: C; c-basic-offset: 4 -*- */

#include <config.h>

/* include this first, before NO_IMPORT_PYGOBJECT is defined */
#include <pygobject.h>

void initplannerui(void);
void plannerui_register_classes (PyObject *d);
//void plannerui_add_constants(PyObject *module, const gchar *strip_prefix);

extern PyMethodDef plannerui_functions[];

DL_EXPORT(void)
initplannerui(void)
{
    PyObject *m, *d;

    init_pygobject ();

    m = Py_InitModule ("plannerui", plannerui_functions);
    d = PyModule_GetDict (m);

    plannerui_register_classes (d);

    if (PyErr_Occurred ()) {
        PyErr_Print ();
        Py_FatalError ("can't initialise module plannerui");
    }
}
