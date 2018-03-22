#include <sqlite3ext.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Python.h>

SQLITE_EXTENSION_INIT1

int sqlite3_dnsql_init(sqlite3 *, char **, const sqlite3_api_routines *);
int dnsql_init(void);

static int vfs_close(sqlite3_file *);
static int vfs_read(sqlite3_file *, void *, int iAmt, sqlite3_int64);
static int vfs_write(sqlite3_file *, const void *, int, sqlite3_int64);
static int vfs_truncate(sqlite3_file *, sqlite3_int64);
static int vfs_sync(sqlite3_file *, int);
static int vfs_file_size(sqlite3_file *, sqlite3_int64 *);
static int vfs_lock(sqlite3_file *, int);
static int vfs_unlock(sqlite3_file *, int);
static int vfs_check_reserved_lock(sqlite3_file *, int *);
static int vfs_file_control(sqlite3_file *, int, void *);
static int vfs_sector_size(sqlite3_file *);
static int vfs_device_characteristics(sqlite3_file *);

static int vfs_open(sqlite3_vfs *, const char *, sqlite3_file *, int, int *);
static int vfs_access(sqlite3_vfs *, const char *, int, int *);

typedef struct {
    sqlite3_vfs *parent;
} context;

typedef struct {
    sqlite3_file base;
    PyObject *module;
    PyObject *object;
} dnsql_file;

static const sqlite3_io_methods dnsql_io_methods = {
    3,                          /* iVersion */
    vfs_close,                  /* xClose */
    vfs_read,                   /* xRead */
    vfs_write,                  /* xWrite */
    vfs_truncate,               /* xTruncate */
    vfs_sync,                   /* xSync */
    vfs_file_size,              /* xFileSize */
    vfs_lock,                   /* xLock */
    vfs_unlock,                 /* xUnlock */
    vfs_check_reserved_lock,    /* xCheckReservedLock */
    vfs_file_control,           /* xFileControl */
    vfs_sector_size,            /* xSectorSize */
    vfs_device_characteristics, /* xDeviceCharacteristics */
    NULL,                       /* xShmMap */
    NULL,                       /* xShmLock */
    NULL,                       /* xShmBarrier */
    NULL,                       /* xShmUnmap */
    NULL,                       /* xFetch */
    NULL                        /* xUnfetch */
};

int vfs_close(sqlite3_file *file) {
    return SQLITE_OK;
}

int vfs_read(sqlite3_file *pFile, void *zBuf, int iAmt, sqlite_int64 iOfst) {
    PyObject *result;
    char *result_buffer;
    Py_ssize_t result_size;
    dnsql_file *file = (dnsql_file *)pFile;

    result = PyObject_CallMethod(file->object, "read", "(l,i)", iOfst, iAmt);

    if (result == NULL) {
        PyErr_Print();
        fprintf(stderr, "Failed to call read\n");
        return SQLITE_ERROR;
    }

    if (PyBytes_AsStringAndSize(result, &result_buffer, &result_size) < 0) {
        Py_DECREF(result);
        PyErr_Print();
        fprintf(stderr, "Failed to extract read result\n");
        return SQLITE_ERROR;
    }

    if (result_size != iAmt) {
        Py_DECREF(result);
        fprintf(stderr, "Mismatch in read result vs buffer sizes\n");
        return SQLITE_ERROR;
    }

    memcpy(zBuf, result_buffer, iAmt);

    Py_DECREF(result);
    return SQLITE_OK;
}

int vfs_write(sqlite3_file *pFile, const void *zBuf, int iAmt,
              sqlite_int64 iOfst) {
    PyObject *result;
    dnsql_file *file = (dnsql_file *)pFile;

    result =
        PyObject_CallMethod(file->object, "write", "(y#,l)", zBuf, iAmt, iOfst);

    if (result == NULL) {
        PyErr_Print();
        fprintf(stderr, "Failed to call write\n");
        return 1;
    }

    Py_DECREF(result);
    return SQLITE_OK;
}

int vfs_truncate(sqlite3_file *pFile, sqlite_int64 size) {
    return SQLITE_OK;
}

int vfs_sync(sqlite3_file *pFile, int flags) {
    return SQLITE_OK;
}

int vfs_file_size(sqlite3_file *pFile, sqlite_int64 *pSize) {
    PyObject *size_attr;
    dnsql_file *file = (dnsql_file *)pFile;

    size_attr = PyObject_GetAttrString(file->object, "size");

    if (size_attr == NULL) {
        PyErr_Print();
        fprintf(stderr, "Failed to get size from object");
        return SQLITE_ERROR;
    }

    *pSize = PyLong_AsLong(size_attr);
    return SQLITE_OK;
}

int vfs_lock(sqlite3_file *pFile, int eLock) {
    return SQLITE_OK;
}

int vfs_unlock(sqlite3_file *pFile, int eLock) {
    return SQLITE_OK;
}

int vfs_check_reserved_lock(sqlite3_file *pFile, int *pResOut) {
    return SQLITE_OK;
}

int vfs_file_control(sqlite3_file *pFile, int op, void *pArg) {
    return SQLITE_OK;
}

int vfs_sector_size(sqlite3_file *pFile) {
    // TODO: double-check this
    return 1 << 12;
}

int vfs_device_characteristics(sqlite3_file *pFile) {
    // TODO
    return 0;
}

int vfs_open(sqlite3_vfs *pVfs, const char *zName, sqlite3_file *pFile,
             int flags, int *pOutFlags) {
    sqlite3_vfs *parent;
    dnsql_file *file;
    PyObject *sys_path, *dot_str, *module, *DnsVfs_class, *dns_vfs_object;

    parent = ((context *)pVfs->pAppData)->parent;
    if ((flags & SQLITE_OPEN_MAIN_DB) == 0) {
        return parent->xOpen(parent, zName, pFile, flags, pOutFlags);
    }

    Py_Initialize();

    sys_path = PySys_GetObject("path");

    if (sys_path == NULL) {
        PyErr_Print();
        fprintf(stderr, "Failed to get system path");
        return SQLITE_ERROR;
    }

    dot_str = Py_BuildValue("s", ".");
    // Implicit Py_INCREF(prog_name)

    if (dot_str == NULL) {
        PyErr_Print();
        fprintf(stderr, "Failed to build dot string");
        return SQLITE_ERROR;
    }

    PyList_Append(sys_path, dot_str);
    Py_DECREF(dot_str);

    module = PyImport_ImportModule("dnsmap");
    // Implicit Py_INCREF(module)

    if (module == NULL) {
        PyErr_Print();
        fprintf(stderr, "Failed to load script\n");
        return SQLITE_ERROR;
    }

    DnsVfs_class = PyObject_GetAttrString(module, "DnsVfs");
    // Implicit Py_INCREF(DnsVfs_class)

    if (DnsVfs_class == NULL) {
        Py_DECREF(DnsVfs_class);
        Py_DECREF(module);
        PyErr_Print();
        fprintf(stderr, "Could not load viable function\n");
        return SQLITE_ERROR;
    }

    dns_vfs_object = PyObject_CallFunction(DnsVfs_class, "(s)", zName);
    Py_DECREF(DnsVfs_class);

    if (dns_vfs_object == NULL) {
        Py_DECREF(DnsVfs_class);
        Py_DECREF(module);
        PyErr_Print();
        fprintf(stderr, "Could not construct object\n");
        return SQLITE_ERROR;
    }

    file = (dnsql_file *)pFile;
    file->base.pMethods = &dnsql_io_methods;
    file->module = module;
    file->object = dns_vfs_object;

    return SQLITE_OK;
}

int vfs_access(sqlite3_vfs *pVfs, const char *zName, int flags, int *pResOut) {
    sqlite3_vfs *parent;
    const char *journal_suffix = "-journal";
    size_t suffix_len, name_len;

    parent = ((context *)pVfs->pAppData)->parent;

    suffix_len = strlen(journal_suffix);
    name_len = strlen(zName);
    if (name_len > suffix_len &&
        !strcmp(zName + name_len - suffix_len, journal_suffix)) {
        return parent->xAccess(parent, zName, flags, pResOut);
    }

    *pResOut = 0;
    return SQLITE_OK;
}

int dnsql_init() {
    context *ctx;
    sqlite3_vfs *vfs;

    ctx = malloc(sizeof(context));
    vfs = malloc(sizeof(sqlite3_vfs));

    ctx->parent = sqlite3_vfs_find(NULL);
    memcpy(vfs, ctx->parent, sizeof(*vfs));
    vfs->zName = "dnsql";
    vfs->pAppData = ctx;
    vfs->xOpen = vfs_open;
    vfs->xAccess = vfs_access;
    vfs->xDlOpen = NULL;
    // Make sure we allocate enough space for both the DNSQL VFS and the
    // fallback unix VFS
    if (sizeof(dnsql_file) > (size_t)vfs->szOsFile) {
        vfs->szOsFile = sizeof(dnsql_file);
    }

    sqlite3_vfs_register(vfs, 1 /* make default */);

    return SQLITE_OK;
}

int sqlite3_dnsql_init(sqlite3 *db, char **pzErrMsg,
                       const sqlite3_api_routines *pApi) {
    SQLITE_EXTENSION_INIT2(pApi);
    ((void)db);
    ((void)pzErrMsg);
    return dnsql_init();
}
