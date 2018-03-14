#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3ext.h>

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

typedef struct {
    sqlite3_vfs *parent;
} context;

typedef struct {
    sqlite3_file base;
    char *name;
    sqlite3_int64 size;
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
    // TODO
    printf("vfs_read(%d, %lld)\n", iAmt, iOfst);
    return SQLITE_OK;
}

int vfs_write(sqlite3_file *pFile, const void *zBuf, int iAmt,
              sqlite_int64 iOfst) {
    // TODO
    printf("vfs_write\n");
    return SQLITE_OK;
}

int vfs_truncate(sqlite3_file *pFile, sqlite_int64 size) {
    return SQLITE_OK;
}

int vfs_sync(sqlite3_file *pFile, int flags) {
    return SQLITE_OK;
}

int vfs_file_size(sqlite3_file *pFile, sqlite_int64 *pSize) {
    dnsql_file *file = (dnsql_file *)pFile;
    *pSize = file->size;
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
    return 1 << 14;
}

int vfs_device_characteristics(sqlite3_file *pFile) {
    // TODO
    return SQLITE_IOCAP_ATOMIC16K;
}

int vfs_open(sqlite3_vfs *pVfs, const char *zName, sqlite3_file *pFile,
             int flags, int *pOutFlags) {
    dnsql_file *file;

    // TODO
    printf("dnsql_open\n");

    file = (dnsql_file *)pFile;
    file->base.pMethods = &dnsql_io_methods;
    file->name = strdup(zName);
    file->size = 1000;

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
