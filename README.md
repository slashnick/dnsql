# DNSQL

> SQLite over DNS

Final project for Networks II at Cal Poly, by Nick Frost and Nick Gonella.

# Client setup

    cd client
    python3 -m venv venv
    source venv/bin/activate
    pip install -r requirements.txt
    make

You can run `python dnsmap.py <infile> <outfile>` to upload an entire file to
DNS. `outfile` will be a new DNSQL metadata file containing the file size and
the hash of the root node in the tree.

To run the demo:

    ./demo hogwarts

# Server setup

The server is currently running at dnsql.io, but if you want to run it locally
it will be similar to .

Note that the `sudo service bind9 reload` command in `reload_bind9()` is
specific to our setup, as is the `ZONE_FILE_PATH`.

    cd server
    python3 -m venv venv
    source venv/bin/activate
    pip install Flask
    venv/bin/python server.py
