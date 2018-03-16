#include <sqlite3.h>
#include <stdio.h>

extern int dnsql_init(void);

int main(void) {
    sqlite3 *fake_db, *db;
    char *error;
    int rc;

    rc = sqlite3_open(":memory:", &fake_db);

    rc = sqlite3_enable_load_extension(fake_db, 1);
    rc = sqlite3_load_extension(fake_db, "./dnsql", "sqlite3_dnsql_init", &error);
    if (rc) {
        printf("%s\n", error);
    }

    sqlite3_open("thing.db", &db);
    return 0;
}
