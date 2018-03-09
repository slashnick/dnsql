import dns.resolver
import dns.rdataclass

import requests

def dns_get(key):
    resolver = dns.resolver.Resolver()
    resolver.nameservers = ['8.8.8.8']
    answers = resolver.query('{}.dnsql.io'.format(key),'TXT')
    entries = [entry.strings[0] for entry in answers]
    entries.sort(key=lambda x : x[0])

    value = b''
    for entry in entries:
        value += entry[1:]

    return value


def dns_put(key, value):
    requests.post('https://dnsql.io/records/?key={}'.format(key), data=value)


if __name__ == '__main__':
    size = 30995
    name = 'a' * 20
    dns_put('{}{}'.format(name, size), 'a' * size)
    print(dns_get('{}{}'.format(name, size)))
