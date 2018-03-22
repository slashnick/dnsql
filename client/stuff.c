#include <assert.h>
#include <sqlite3.h>
#include <stddef.h>
#include <stdio.h>

extern int dnsql_init(void);

static int print_row(void *_, int argc, char **argv, char **colname) {
    printf("%9s %8s %11s\n", argv[0], argv[1], argv[2]);
    return 0;
}

int main(void) {
    sqlite3 *fake_db, *db;
    char *error;
    int rc;

    rc = sqlite3_open(":memory:", &fake_db);

    rc = sqlite3_enable_load_extension(fake_db, 1);
    rc = sqlite3_load_extension(fake_db, "./dnsql", "sqlite3_dnsql_init",
                                &error);
    if (rc) {
        printf("%s\n", error);
    }

    sqlite3_open("thing.db", &db);

    // Disable temporary journal files
    rc = sqlite3_exec(db, "PRAGMA journal_mode = OFF;", NULL, NULL, NULL);
    assert(rc == SQLITE_OK);

    printf("   First      Last       House\n");
    printf("------------------------------\n");
    if (sqlite3_exec(db, "SELECT * from students", print_row, NULL, &error)) {
        printf("%s\n", error);
    }

    return 0;
}
