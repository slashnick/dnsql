import base64
import dns.resolver
import dns.rdataclass
import hashlib
import math
import os
import requests
import sys
import time


# TODO: CHUNK_SIZE = 16384
CHUNK_SIZE = 4096
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
        if dns_get(key) is None:
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

def calculate_levels(filesize):
    if filesize < CHUNK_SIZE:
        return 0

    return int(math.log(filesize // CHUNK_SIZE, HASHES_PER_CHUNK)) + 1


class DnsVfs(object):

    def __init__(self, root):
        self.root = root
        self.size = 0
        self.levels = 1
        self.filename = root
        # TODO
        self.root = 'v5s6euxjva7zpnfnvxeaasnmrrnc2obm'
        self.size = 8192

    def get_block_hash(self, index, level=0):
        """Get the hash where a block is located. level 0 = leaf

        TODO: optimize
        """
        if level == self.levels:
            return self.root

        parent_id = self.get_block_hash(index // HASHES_PER_CHUNK, level + 1)
        parent = dns_get(parent_id)
        offset = index % HASHES_PER_CHUNK
        digest = parent[offset * 20 : offset * 20 + 20]
        return encode_digest(digest)

    def block_range(self, offset, amount, increment=CHUNK_SIZE):
        """Return the range of blocks that contain [offset:offset + amount]."""
        start_block = offset // increment
        end_block = (offset + amount - 1) // increment
        return range(start_block, end_block + 1)

    def read(self, offset, amount):
        # 0-length reads are sketchy
        if amount == 0:
            return b''

        start_block = offset // CHUNK_SIZE
        end_block = (offset + amount - 1) // CHUNK_SIZE

        data = b''.join(dns_get(self.get_block_hash(index))
                        for index in range(start_block, end_block + 1))
        start = offset % CHUNK_SIZE
        rv = data[offset % CHUNK_SIZE : offset % CHUNK_SIZE + amount]
        return rv

    def write_tree(self, block_hash, buf, offset, level):
        print('write? wtf')
        return
        old_data = dns_get(block_hash)
        # The number of bytes stored in each subtree
        tree_size = CHUNK_SIZE * HASHES_PER_CHUNK ** (level - 1)
        # The index of the start and end child trees at the next level
        start_tree = offset // tree_size
        end_tree = (offset + amount - 1) // tree_size
        # Since we're not overwriting all the child trees, initialize data to
        # the hashes of the trees up until the start tree
        new_data = old_data[:(start_tree % HASHES_PER_CHUNK) * 20]
        for tree in range(start_tree, end_tree + 1):
            ndx = tree % HASHES_PER_CHUNK
            child = encode_digest(old_data[ndx : ndx + 20])
            # Compute the offset and data slice for the subtree
            sub_off = max(tree * tree_size, offset)
            # Don't pass too much data: (sub_off % tree_size) + len(sub_data)
            sub_data = None #TODO
            new_data += self.write_tree(child, )

    def write(self, buf, offset):
        if len(buf) == 0:
            return

        # TODO: increase size?
        # TODO: add a level to the tree?

        self.root = self.write_tree(self.root, buf, offset)
        print("I'm gonna write '{}' at {}".format(buffer, offset))

    """TODO: truncate"""


def main():
    if len(sys.argv) != 3:
        print('usage: python %s infile outfile' % sys.argv[0])
        return

    with open(sys.argv[1], 'rb') as f:
        f.seek(0, os.SEEK_END)
        size = f.tell()
        f.seek(0, os.SEEK_SET)

        levels = calculate_levels(size)

        root = store_file(f, size, levels)
    print(encode_digest(root))

    with open(sys.argv[2], 'wb') as f:
        fetch_file(f, encode_digest(root), levels)


if __name__ == '__main__':
    main()
