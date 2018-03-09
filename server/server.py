import flask
import subprocess
import sys


ZONE_FILE_PATH = '/etc/bind/zones/db.dnsql.io'
ZONE_PREFIX = \
"""$TTL 604800
@ IN SOA dnsql1.nfrost.me. admin.dnsql.nfrost.me. (
       4   ; Serial
  604800   ; Refresh
   86400   ; Retry
 2419200   ; Expire
  604800 ) ; Negative Cache TTL
@ IN NS dnsql1.nfrost.me.
@ IN NS dnsql2.nfrost.me.
@ IN CAA 0 issue "letsencrypt.org"
@ IN CAA 0 issuewild ";"
@ IN A 165.227.24.13
www IN CNAME @
"""


def format_byte(ch):
    if ch < 0x20 or ch > 0x7e or ch == 0x5c:  # 0x5c = /
        return '\\%03d' % ch
    elif ch == 0x22:  # '"'
        return '\\"'
    else:
        return chr(ch)


def format_txt_line(subdomain, contents, ttl=None):
    """Return a zone-file-formatted TXT record line."""
    if len(contents) > 255:
        raise ValueError("Contents too long for single TXT line: %d bytes" %
                         len(contents))

    data = ''.join(format_byte(ch) for ch in contents)
    ttl_part = ' %d' % ttl if ttl is not None else ''
    return subdomain + ttl_part + ' IN TXT "' + data + '"'


def split_value(value):
    """Convert a byte string into a list of TXT-sized strings.

    Each string will be up to 255 bytes. The first byte is an identifier, which
    indicates the position of that substring in the original value.

        >>> list(split_value(b'a' * 254 + b'b' * 254 + b'c' * 254))
        [b'\x00aaaa...', b'\x01bbbb...', b'\x03cccc...']
    
    value must not be longer than 254 * 256, or 65024 bytes.
    """
    if len(value) > 254 * 256:
        raise ValueError("Value too large: %d bytes" %
                         len(value))

    for i in range(0, len(value), 254):
        yield bytes([i // 254]) + value[i : i + 254]


def write_zone_file(filename, key, value, ttl=None):
    with open(filename, 'w') as f:
        f.write(ZONE_PREFIX)
        for value_part in split_value(value):
            f.write(format_txt_line(key, value_part, ttl=ttl) + '\n')


def reload_bind9():
    subprocess.run(['/usr/bin/sudo', '/usr/sbin/service', 'bind9', 'reload'],
                   check=True)


def main():
    if len(sys.argv) < 2:
        print('usage: python %s zonefile' % sys.argv[0])
        return
    zone_file_name = sys.argv[1]

    key = 'test-key50'
    value = bytes([i % 256 for i in range(3 * 256)])
    write_zone_file(zone_file_name, key, value)


app = flask.Flask(__name__)


@app.route('/records/', methods=['POST'])
def add_record():
    key = flask.request.args['key']
    value = flask.request.get_data(cache=False)
    if 'ttl' in flask.request.args:
        ttl = int(flask.request.args['ttl'])
    else:
        ttl = None

    write_zone_file(ZONE_FILE_PATH, key, value, ttl=ttl)
    reload_bind9()
    return flask.Response('', status=204)


if __name__ == '__main__':
    app.run()
