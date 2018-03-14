import sqlite3


def main():
    # This connection just lets us load the dnsql extension
    dummy = sqlite3.connect(':memory:')
    dummy.enable_load_extension(True)
    dummy.load_extension('./dnsql')

    con = sqlite3.connect('dns.db')
    con.execute('create table if not exists students(name,house)')
    con.execute("insert into students (name, house) values ('Harry', 'Gryffindor')")
    con.execute("insert into students (name, house) values ('Hermione', 'Gryffindor')")
    con.execute("insert into students (name, house) values ('Ron', 'Gryffindor')")


if __name__ == '__main__':
    main()
