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


def dns_get(key, retries=3):
    if not key:
        return b''

    resolver = dns.resolver.Resolver(configure=False)
    resolver.nameservers = ['8.8.8.8']
    try:
        answers = resolver.query('{}.dnsql.io'.format(key), 'TXT')
    except dns.resolver.NXDOMAIN:
        if retries == 0:
            return None
        else:
            return dns_get(key, retries - 1)

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
        #time.sleep(0.1)
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

    def __init__(self, filename):
        self.filename = filename
        with open(filename) as f:
            self.size = int(f.readline().strip())
            self.root = f.readline().strip()

        self.levels = calculate_levels(self.size)

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

    def write_data(self, block_hash, buf, offset):
        """Write a buffer to a single data block."""
        old_data = dns_get(block_hash)
        start = offset % CHUNK_SIZE

        new_data = old_data[:start] + buf[:CHUNK_SIZE - start]
        new_data += old_data[len(new_data):]
        if new_data == old_data:
            return hashlib.sha1(new_data).digest()
        return store_blob(new_data)

    def write_tree(self, block_hash, buf, offset, level):
        if level == 0:
            return self.write_data(block_hash, buf, offset)

        old_data = dns_get(block_hash)
        # The number of bytes stored in each subtree
        tree_size = CHUNK_SIZE * HASHES_PER_CHUNK ** (level - 1)
        # The index of the start and end child trees at the next level
        start_tree = offset // tree_size
        end_tree = (offset + len(buf) - 1) // tree_size

        # Since we're not overwriting all the child trees, initialize data to
        # the hashes of the trees up until the start tree
        new_data = old_data[:(start_tree % HASHES_PER_CHUNK) * 20]
        for tree in range(start_tree, end_tree + 1):
            ndx = tree % HASHES_PER_CHUNK
            child = encode_digest(old_data[ndx * 20 : ndx * 20 + 20])
            # Compute the offset and data slice for the subtree
            sub_off = max(tree * tree_size, offset)
            start = max(sub_off - offset, 0)
            end = tree_size - start
            sub_data = buf[start:end]
            new_data += self.write_tree(child, sub_data, start, level - 1)
        # Tack on trailing data after write
        new_data += old_data[len(new_data):]
        if new_data == old_data:
            return hashlib.sha1(new_data).digest()
        return store_blob(new_data)

    def write(self, buf, offset):
        if len(buf) == 0:
            return

        if offset + len(buf) > self.size:
            self.size = offset + len(buf)
            level = calculate_levels(self.size)
            if level != self.levels:
                # TODO: add a new level
                print('Oh no! Need to add new level')
                pass

        new_hash = self.write_tree(self.root, buf, offset, self.levels)
        new_root = encode_digest(new_hash)
        if new_root != self.root:
            self.root = new_root
            self.write_metafile()

    def write_metafile(self):
        with open(self.filename, 'w') as f:
            f.write(str(self.size) + '\n')
            f.write(self.root + '\n')

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

        root = encode_digest(store_file(f, size, levels))

    with open(sys.argv[2], 'w') as f:
        f.write(str(size) + '\n')
        f.write(root + '\n')


if __name__ == '__main__':
    main()
