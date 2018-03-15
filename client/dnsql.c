#include <sqlite3ext.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Python.h>

SQLITE_EXTENSION_INIT1

int sqlite3_dnsql_init(sqlite3 *, char **, const sqlite3_api_routines *);
static int dnsql_init(void);

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
static int vfs_delete(sqlite3_vfs *, const char *, int);
static int vfs_access(sqlite3_vfs *, const char *, int, int *);

typedef struct { sqlite3_vfs *parent; } context;

typedef struct {
  sqlite3_file base;
  PyObject *module;
  PyObject *class;
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

int vfs_close(sqlite3_file *file) { return SQLITE_OK; }

int vfs_read(sqlite3_file *pFile, void *zBuf, int iAmt, sqlite_int64 iOfst) {
  PyObject *result;
  char *result_buffer;
  Py_ssize_t result_size;
  dnsql_file *file = (dnsql_file *)pFile;

  result = PyObject_CallMethod(file->class, "read", "(O,l,i)", file->class,
                               iOfst, iAmt);

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

  result = PyObject_CallMethod(file->class, "write", "(O, y#, l)", file->class,
                               zBuf, iAmt, iOfst);

  if (result == NULL) {
    PyErr_Print();
    fprintf(stderr, "Failed to call write\n");
    return 1;
  }

  Py_DECREF(result);
  return SQLITE_OK;
}

int vfs_truncate(sqlite3_file *pFile, sqlite_int64 size) { return SQLITE_OK; }

int vfs_sync(sqlite3_file *pFile, int flags) { return SQLITE_OK; }

int vfs_file_size(sqlite3_file *pFile, sqlite_int64 *pSize) {
    PyObject *size_attr;
    dnsql_file *file = (dnsql_file *)pFile;

    size_attr = PyObject_GetAttrString(file->class, "size");

    if(size_attr == NULL) {
        PyErr_Print();
        fprintf(stderr, "Failed to get size from object");
        return SQLITE_ERROR;
    }

    *pSize = PyLong_AsLong(size_attr);
    return SQLITE_OK;
}

int vfs_lock(sqlite3_file *pFile, int eLock) { return SQLITE_OK; }

int vfs_unlock(sqlite3_file *pFile, int eLock) { return SQLITE_OK; }

int vfs_check_reserved_lock(sqlite3_file *pFile, int *pResOut) {
  return SQLITE_OK;
}

int vfs_file_control(sqlite3_file *pFile, int op, void *pArg) {
  return SQLITE_OK;
}

int vfs_sector_size(sqlite3_file *pFile) {
  // TODO: double-check this
  return 1 << 14;
}

int vfs_device_characteristics(sqlite3_file *pFile) {
  // TODO
  return SQLITE_IOCAP_ATOMIC16K;
}

int vfs_open(sqlite3_vfs *pVfs, const char *zName, sqlite3_file *pFile,
             int flags, int *pOutFlags) {
  dnsql_file *file;
  PyObject *sys_path, *dot_str, *module, *DnsVfs_class;

  printf("dnsql_open\n");

  /* PyEval_InitThreads(); */
  /* Py_Initialize(); */

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

  DnsVfs_class = PyObject_GetAttrString(module, "dnsmap");
  // Implicit Py_INCREF(DnsVfs_class)

  if (DnsVfs_class == NULL) {
    Py_DECREF(DnsVfs_class);
    Py_DECREF(module);
    PyErr_Print();
    fprintf(stderr, "Could not load viable function\n");
    return SQLITE_ERROR;
  }

  file = (dnsql_file *)pFile;
  file->base.pMethods = &dnsql_io_methods;
  file->module = module;
  file->class = DnsVfs_class;

  return SQLITE_OK;
}

int vfs_delete(sqlite3_vfs *pVfs, const char *zName, int syncDir) {
  return SQLITE_OK;
}

int vfs_access(sqlite3_vfs *pVfs, const char *zName, int flags, int *pResOut) {
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
    vfs->pAppData = &ctx;
    vfs->szOsFile = sizeof(dnsql_file);
    vfs->xOpen = vfs_open;
    vfs->xAccess = vfs_access;
    vfs->xDelete = vfs_delete;

    sqlite3_vfs_register(vfs, 1);

    return SQLITE_OK;
}

int sqlite3_dnsql_init(sqlite3 *db, char **pzErrMsg,
                       const sqlite3_api_routines *pApi) {
  SQLITE_EXTENSION_INIT2(pApi);
  ((void)db);
  ((void)pzErrMsg);
  return dnsql_init();
}
