import base64

def sqrl_base64_encode(data):
    if isinstance(data, str):
        data = data.encode()
    return base64.urlsafe_b64encode(data).rstrip(b'=')

def sqrl_base64_decode(data):
    if isinstance(data, str):
        return base64.urlsafe_b64decode(data + '==')
    else:
        return base64.urlsafe_b64decode(data + b'==')

def sqrl_decode_response(resp):
    records = {}
    for line in sqrl_base64_decode(resp).rstrip(b'\r\n').split(b'\r\n'):
        key, value = line.split(b'=', 1)
        if isinstance(resp, bytes):
            records[key] = value
        else:
            records[key.decode()] = value.decode()

    return records

