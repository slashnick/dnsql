from server import format_txt_line, split_value


def test_format_txt_line():
    assert format_txt_line(u'foo', b'bar') == u'foo IN TXT "bar"'
    assert format_txt_line(u'foo', b'bar', ttl=10) == u'foo 10 IN TXT "bar"'
    assert format_txt_line(u'abc', b'My string \0"\r\n stuff \xff abc') == \
        u'abc IN TXT "My string \\000\\"\\013\\010 stuff \\255 abc"'


def test_split_value():
    lst = list(split_value(b'1' * 254 + b'2' * 254 + b'3'))
    assert lst == [b'\0' + b'1' * 254, b'\1' + b'2' * 254, b'\x023']
