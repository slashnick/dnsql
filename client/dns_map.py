import base64
import dns.resolver
import dns.rdataclass
import hashlib
import math
import os
import requests
import sys
import time


CHUNK_SIZE = 30000
HASHES_PER_CHUNK = CHUNK_SIZE // 20


def dns_get(key):
    resolver = dns.resolver.Resolver(configure=False)
    resolver.nameservers = ['8.8.8.8']
    try:
        answers = resolver.query('{}.dnsql.io'.format(key), 'TXT')
    except dns.resolver.NXDOMAIN:
        return None

    entries = [entry.strings[0] for entry in answers]
    entries.sort(key=lambda x : x[0])

    value = b''
    for entry in entries:
        value += entry[1:]

    return value


def dns_put(key, value, ttl=None):
    if len(key) + len(value) > 31015:
        raise ValueError("Key/Value pair too large")

    params = {'key': key}
    if ttl is not None:
        params['ttl'] = str(ttl)
    rv = requests.post('https://dnsql.io/records/', params=params, data=value)
    rv.raise_for_status()


def encode_digest(digest):
    """Return the base32-encoded version of a digest, without any padding."""
    encoded = base64.b32encode(digest)
    return encoded.decode('utf-8').rstrip('=').lower()


def store_blob(blob, get_count=11):
    """Write a blob to DNS at the appropriate key. Return the SHA-1 digest."""
    digest = hashlib.sha1(blob).digest()
    key = encode_digest(digest)
    dns_put(key, blob, ttl=21600)
    for _ in range(get_count):
        if dns_get(key) is not None:
            raise Exception('TXT record for {} not found :('.format(key))
        time.sleep(0.5)
    return digest


def store_file(f, size, level):
    """level = which level we are at in the Merkle tree (0 = leaf)"""
    if level == 0:
        chunk = f.read(size)
        return store_blob(chunk)
    else:
        chunk_size = HASHES_PER_CHUNK ** (level - 1) * CHUNK_SIZE
        blobs = []
        for pos in range(0, size, chunk_size):
            blobs.append(store_file(f, min(size - pos, chunk_size), level - 1))
        blob = b''.join(blobs)
        return store_blob(blob)


def fetch_file(f, key, level):
    chunk = dns_get(key)
    if level == 0:
        f.write(chunk)
    else:
        for i in range(0, len(chunk), 20):
            digest = chunk[i : i + 20]
            fetch_file(f, encode_digest(digest), level - 1)


def main():
    if len(sys.argv) != 3:
        print('usage: python %s infile outfile' % sys.argv[0])
        return

    with open(sys.argv[1], 'rb') as f:
        f.seek(0, os.SEEK_END)
        size = f.tell()
        f.seek(0, os.SEEK_SET)

        if size < CHUNK_SIZE:
            levels = 0
        else:
            levels = int(math.log(size // CHUNK_SIZE, HASHES_PER_CHUNK)) + 1

        root = store_file(f, size, levels)
    print(encode_digest(root))

    with open(sys.argv[2], 'wb') as f:
        fetch_file(f, encode_digest(root), levels)


if __name__ == '__main__':
    main()
