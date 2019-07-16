import requests
import sys
import time

import sqrl_conv
from sqrl_url import SqrlUrl
from sqrl_crypto import *

# Computer\HKEY_CURRENT_USER\Software\Classes\sqrl\shell\open\command

def split_response(resp):
    records = {}
    for line in resp.rstrip(b'\r\n').split(b'\r\n'):
        key, value = line.split(b'=', 1)
        records[key] = value
    return records

def transaction_1(url, ssk, idk):
    client  =  b'ver=1\r\n'
    client  += b'cmd=query\r\n'
    client  += b'idk=%s\r\n' % sqrl_conv.base64_encode(bytes(idk))
    client  += b'opt=cps~suk\r\n'
    print('client', client)
    client  = sqrl_conv.base64_encode(client)

    server  =  bytes(url)
    print('server', server)
    server  = sqrl_conv.base64_encode(server)

    ids     = ssk.sign(client + server)
    form    = {'client': client,
               'server': server,
               'ids':    sqrl_conv.base64_encode(ids.signature)}
    return requests.post(url.get_http_url(), data=form)


def transaction_2(url, server, ssk, idk, ilk):
    records = split_response(sqrl_conv.base64_decode(server))
    print('url records', records)

    client  =  b'ver=1\r\n'
    client  += b'cmd=ident\r\n'
    client  += b'idk=%s\r\n' % sqrl_conv.base64_encode(bytes(idk))
    if b'sin' in records:
        ins = sqrl_hmac(EnHash(bytes(ssk)), records[b'sin'])
        client  += b'ins=%s\r\n' % sqrl_conv.base64_encode(ins)
    if b'suk' not in records:
        suk, vuk = sqrl_idlock_keys(ilk)
        client  += b'suk=%s\r\n' % sqrl_conv.base64_encode(suk)
        client  += b'vuk=%s\r\n' % sqrl_conv.base64_encode(vuk)
    client  += b'opt=cps~suk\r\n'
    print('client', client)
    client  = sqrl_conv.base64_encode(client)

    ids     = ssk.sign(client + server)
    form    = {'client': client,
               'server': server,
               'ids':    sqrl_conv.base64_encode(ids.signature)}
    return requests.post(url.get_resp_query_path(records[b'qry']), data=form)

def login_procedure(url_str):
    # Parse input parameter
    url = SqrlUrl(url_str)
    print('url      ', url)
    print('domain   ', url.domain)
    print('sks      ', url.get_sks())
    print('nut      ', url.get_nut())
    sys.stdout.flush()

    # Hard code identity (for now)
    imk = b'\xf3\xb78B\xda\x12E\xf3\xe7\xb2\xb8\xa1\x14/r\xbb\xc8\x97\xf8\xdd\xbaY\x83b\xb4\x93\r\xe2\xbbF\x0f\x1f'
    ilk = b'\xaa\xbdl*\xeei>\x81\xb2:Y;6\xa3\xb4\x96\xa6\xfa\x86\xfdhy~\xa2\xe4\xaf\x8d3\xd1\x9038'
    print('imk', imk)
    print('ilk', ilk)

    # Get site specific keys
    ssk, idk = sqrl_get_idk_for_site(imk, url.get_sks())

    # Transaction #1
    t1 = time.time()
    r = transaction_1(url, ssk, idk)
    print(r, 'time', time.time() - t1)
    # TODO: Check r.code == 200
    sys.stdout.flush()

    # Transaction #2
    t1 = time.time()
    r = transaction_2(url, bytes(r.text, encoding='utf-8'), ssk, idk, ilk)
    print(r, 'time', time.time() - t1)
    print(r.text)
    sys.stdout.flush()
    # TODO: Check r.code == 200

    # Transaction #3
    server = bytes(r.text, encoding='utf-8')
    print('server', server)
    records = split_response(sqrl_conv.base64_decode(server))
    print('url records', records)

    redirect = records[b'url'].decode('utf-8')
    print('redirect', repr(redirect))
    sys.stdout.flush()
    return redirect


import os
import http.server
import socketserver
from urllib.parse import urlparse
#from urllib.parse import parse_qs

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))

class Handler(http.server.SimpleHTTPRequestHandler):

    def do_GET(self):
        print('do_GET', repr(self.path))
        sys.stdout.flush()

        # http://localhost:25519/1561829900702.gif
        if self.path.endswith('.gif'):
            self.send_response(200)
            self.send_header('Content-Type', 'image/gif')
            self.send_header('Cache-Control', 'no-cache')
            self.send_header('Connection', 'close')
            self.send_header('Sqrl-Process', '12345')
            self.end_headers()
            self.wfile.write(open('onepixel.gif', 'rb').read())
            return

        if self.path in [ '/sqrl.ico', '/favicon.ico']:
            # image/x-icon
            self.send_response(200)
            self.send_header('Content-Type', 'image/x-icon')
            self.send_header('Connection', 'close')
            self.end_headers()
            self.wfile.write(open(self.path[1:], 'rb').read())
            return

        #'/c3FybDovL3d3dy5ncmMuY29tL3Nxcmw_bnV0PVdUVjBOZjZBajlQRjdrUDZzdEF6MHcmY2FuPWFIUjBjSE02THk5M2QzY3VaM0pqTG1OdmJTOXpjWEpzTDJScFlXY3VhSFJ0'
        url_str = sqrl_conv.base64_decode(self.path[1:])
        print('url_str', url_str, url_str.find(b'qrl://'))
        sys.stdout.flush()
        if url_str.find(b'qrl://') < 0:
            self.send_response(404)
            self.send_header('Content-Type', 'text/plain')
            self.send_header('Connection', 'close')
            self.end_headers()
            self.wfile.write(b'Not an sqrl url')
            print('Not and sqrl url')
            sys.stdout.flush()
            return

        redirect = login_procedure(url_str)

        self.send_response(302)
        self.send_header('Location', redirect)
        self.end_headers()
        print('done')
        sys.stdout.flush()

        #pprint (vars(self))
        #pprint (vars(self.headers))
        #pprint (dir(self))
        #pprint (self.connection)
        #pprint (self.request)
        #pprint (self.path)
        #pprint (self.rfile)
        #pprint (vars(self.server))
        #pprint (self.wfile)

class SimpleServer(socketserver.ThreadingMixIn, socketserver.TCPServer):
    # Ctrl-C will cleanly kill all spawned threads
    daemon_threads = True
    # much faster rebinding
    allow_reuse_address = True


def main():
    # http://localhost:25519/
    listen = ('localhost', 25519)

    print('Server listening on port %s:%s' % listen)
    with SimpleServer(listen, Handler) as httpd:
        # Activate the server; this will keep running until you
        # interrupt the program with Ctrl-C
        httpd.serve_forever()


if __name__ == "__main__":
    if len(sys.argv) > 1:
        print(login_procedure(sys.argv[1]))
    else:
        main()
