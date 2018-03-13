#include <Python.h>

#define PY_SCRIPT "dns_map.py"
#define PY_OBJECT "DnsVfs"

struct PythonObjects {
    PyObject *module;
    PyObject *class;
}

int dns_fs_init(PyObject *state) {
    Py_Initialize();

    name = PyUnicode_DecodeFSDefaultAndSize(PY_SCRIPT, strlen(PY_SCRIPT));
    // Implicit Py_INCREF(pname)

    if (name == NULL) {
        return 1;
    }

    module = PyImport_Import(pName);
    // Implicit Py_INCREF(pModule)
    Py_DECREF(name);

    if (module == NULL) {
        PyErr_Print();
        fprintf(stderr, "Failed to load script");
        return 1;
    }

    DnsVfs_class = PyObject_GetAttrString(module, PY_OBJECT);
    // Implicit Py_INCREF(DnsVfs_class)

    if (DnsVfs_class == NULL) {
        Py_DECREF(DnsVfs_class);
        Py_DECREF(module);
        PyErr_Print();
        fprintf(stderr,"Could not load viable function\n");
        return 1;
    }

    *state = (PythonObjects ){module, DnsVfs_class};

    return 0;
}

int dns_fs_read(PyObject *state) {
    PyObject *result;

    result = PyObject_CallMethod(state->class, "read", NULL);

    if(result == NULL) {
        // Probably should do something in this case
        return 1;
    }

    return 0;
}

int dns_fs_write(PyObject *state) {
    PyObject *result;

    result = PyObject_CallMethod(state->class, "write", NULL);

    if(result == NULL) {
        // Probably should do something in this case
        return 1;
    }

    return 0;
}

int main(int argc, char *argv[]) {
    PyObject *pName, *pModule, *pDict, *pFunc;
    PyObject *pArgs, *pValue;
    int i;
 

    pArgs = PyTuple_New(argc - 3);

    for (i = 0; i < argc - 3; ++i) {
        pValue = PyLong_FromLong(atoi(argv[i + 3]));
        if (!pValue) {
            Py_DECREF(pArgs);
            Py_DECREF(pModule);
            fprintf(stderr, "Cannot convert argument\n");
            return 1;
        }
        // pValue reference stolen here:
        PyTuple_SetItem(pArgs, i, pValue);
    }

    pValue = PyObject_CallObject(pFunc, pArgs);
    // Implicit Py_INCREF(pValue)
    Py_DECREF(pArgs);

    if (pValue != NULL) {
        printf("Result of call: %ld\n", PyLong_AsLong(pValue));
        Py_DECREF(pValue);
    }
    else {
        if (PyErr_Occurred())
            PyErr_Print();
        fprintf(stderr, "Cannot find function \"%s\"\n", argv[2]);
    }

    Py_XDECREF(pFunc);
    Py_DECREF(pModule);
    Py_Finalize();
    // If we get Python 3.6, replace the above line with this
    /* if (Py_FinalizeEx() < 0) { */
    /*     return 120; */
    /* } */
    return 0;
}
