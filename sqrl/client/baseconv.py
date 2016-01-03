import base64

"""
BaseConverter
Uitility to encode and decode base64url to spec
"""

def decode(data):
    data = str(data) # To be able to handle u'Unicode strings'
    data += '=' * (4 - len(data) % 4)
    return base64.urlsafe_b64decode(data)

def encode(data):
    return base64.urlsafe_b64encode(data).rstrip("=")

def decodeNameValue(data):
    ret = {}
    for line in decode(data).split("\r\n"):
        if line == '': continue
        key, value = line.split('=', 1)
        ret[key] = value
    return ret

def encodeNameValue(data):
    if not data.has_key("ver"):
        raise Exception("SQRL protocol requires all protocl exchanges to cary version information")

    ret = "ver=%s\r\n" % data['ver']
    for key, value in data.iteritems():
        if key == 'ver': continue
        ret += "%s=%s\r\n" % (key, value)

    return encode(ret)
