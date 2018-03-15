#include <sqlite3.h>
#include <stdio.h>

extern int dnsql_init(void);

int main(void) {
    sqlite3 *db;

    dnsql_init();
    sqlite3_open("thing.db", &db);
    return 0;
}
