#include <Python.h>

#define PY_SCRIPT "dnsmap"
#define PY_OBJECT "DnsVfs"

typedef struct PythonObjects {
    PyObject *module;
    PyObject *class;
} PythonObjects;

int dns_fs_init(PythonObjects *state) {
    PyObject *name, *module, *DnsVfs_class;

    Py_Initialize();
    PyObject* sysPath = PySys_GetObject((char*)"path");
    PyObject* programName = Py_BuildValue("s", ".");
    PyList_Append(sysPath, programName);
    Py_DECREF(programName);

    module = PyImport_ImportModule("dnsmap");
    if (module == NULL) {
        PyErr_Print();
        fprintf(stderr, "Failed to load script\n");
        return 1;
    }
    else {
        puts("Woot");
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

    *state = (PythonObjects){module, DnsVfs_class};

    return 0;
}

int dns_fs_read(PythonObjects *state, int amount, int64_t offset) {
    PyObject *result;

    printf("I'm gonna try to read in c\n");
    result = PyObject_CallMethod(state->class, "read", 
                                 "(O,l,i)", state->class, offset, amount);

    if(result == NULL) {
        // Probably should do something in this case
        PyErr_Print();
        fprintf(stderr, "Failed to call read\n");
        return 1;
    }

    Py_DECREF(result);
    return 0;
}

int dns_fs_write(PythonObjects *state, void *bytes, int num_bytes, 
        int64_t offset) {
    PyObject *result;

    result = PyObject_CallMethod(state->class, "write", 
                                 "(b#, l)", bytes, num_bytes, offset);

    if(result == NULL) {
        // Probably should do something in this case
        return 1;
    }

    Py_DECREF(result);
    return 0;
}

int main(int argc, char *argv[]) {
    PythonObjects state;

    dns_fs_init(&state);
    dns_fs_read(&state, 12, 100);
    dns_fs_write(&state, "Some words", strlen("Some words"), 100);
}
