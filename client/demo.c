#include <assert.h>
#include <sqlite3.h>
#include <stddef.h>
#include <stdio.h>

static void load_extension(void) {
    sqlite3 *db;
    char *error;

    sqlite3_open(":memory:", &db);
    sqlite3_enable_load_extension(db, 1);
    if (sqlite3_load_extension(db, "./dnsql", "sqlite3_dnsql_init", &error)) {
        printf("load_extension: %s\n", error);
    }
}

static void pause(void) {
    while (getchar() != '\n')
        ;
}

static int print_row(void *_, int argc, char **argv, char **colname) {
    printf("%9s %8s %11s\n", argv[0], argv[1], argv[2]);
    return 0;
}

static void query(sqlite3 *db, const char *sql, int print_result) {
    char *error;

    printf("%s [enter] ", sql);
    fflush(stdout);
    pause();
    printf("\n");

    if (print_result) {
        printf("%9s %8s %11s\n", "First", "Last", "House");
        printf("------------------------------\n");
    }
    if (sqlite3_exec(db, sql, print_row, NULL, &error)) {
        printf("%s\n", error);
    }
    if (print_result) {
        printf("\n");
    }
}

int main(int argc, char *argv[]) {
    sqlite3 *db;

    if (argc != 2) {
        fprintf(stderr, "usage: %s file\n", argv[0]);
        return 1;
    }

    load_extension();

    sqlite3_open(argv[1], &db);

    // Disable journaling
    sqlite3_exec(db, "PRAGMA journal_mode = DELETE;", NULL, NULL, NULL);

    query(db, "SELECT * FROM students", 1);
    query(db, "SELECT * FROM students WHERE house='Gryffindor'", 1);
    query(db, "INSERT INTO students (first, last, house) VALUES "
                 "('Draco', 'Malfoy', 'Slytherin')", 0);
    query(db, "DELETE FROM students WHERE first='Ron'", 0);

    sqlite3_close(db);

    return 0;
}
