import binascii
import json
import requests
import serial
import sys
import time

import http.server
import socketserver
from urllib.parse import urlparse

import sys, os
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
sys.path.append(os.path.join(SCRIPT_DIR, '..', 'pysqrl'))

from pysqrl.sqrl_conv import sqrl_decode_response, sqrl_base64_decode, sqrl_base64_encode
from pysqrl.sqrl_url import SqrlUrl
from pysqrl.sqrl_client import sqrl_query, sqrl_ident, sqrl_disable

# Computer\HKEY_CURRENT_USER\Software\Classes\sqrl\shell\open\command
# "C:\Program Files\Python37\pythonw.exe" "C:\Users\vegar.westerlund\devel\sqrl\examples\sample_client.py" "%1"

# Hard code identity (for now) (only needed if com is None)
#imk = binascii.unhexlify(b'f3b73842da1245f3e7b2b8a1142f72bbc897f8ddba598362b4930de2bb460f1f') # vegarwe-testing
#ilk = binascii.unhexlify(b'aabd6c2aee693e81b23a593b36a3b496a6fa86fd68797ea2e4af8d33d1903338') # vegarwe-testing
imk = binascii.unhexlify(b'21d70894575e6b6efe991fb86a9868a49f3a72040e88252d82be5a3ac6c3aa23') # vegarwe_test5
ilk = binascii.unhexlify(b'00d3a56b500bca7908eb89a6f5fe0931388797d42930798d2ffe88d436c94878') # vegarwe_test5
#print('imk', binascii.hexlify(imk))
#print('ilk', binascii.hexlify(ilk))

def readCommand(ser):
    i = 3
    resp = None
    while i > 0:
        a = ser.read(1)
        if a == b'':
            print("i =", i)
            sys.stdout.flush()
            i -=1
            continue
        else:
            i = 3

        if a == b'\x02':
            resp = a
        elif a == b'\x03':
            resp += a
            resp_parts = resp[1:-1].split(b'\x1e')
            if resp_parts[0] == b'log':
                log = b' '.join(resp_parts[1:]).strip()
                print('log:', log.decode())
            elif resp_parts[0] == b'resp':
                print('Found command response')
                print('  cmd    ', resp_parts[1])
                print('  client ', resp_parts[2])
                print('  server ', resp_parts[3])
                print('  ids    ', resp_parts[4])
                form = {'client': resp_parts[2],
                        'server': resp_parts[3],
                        'ids':    resp_parts[4]}
                return form
            else:
                print('err: unknown resp: ', repr(resp.decode()))
            resp = None
        elif resp:
            resp += a
        else:
            if a != b'\n':
                print('garbage: %r' % a.decode())


def query_cmd(url, com=None):
    t1 = time.time()
    if com:
        command = "\x02query\x1e%s\x1e%s\x03" % (url.get_sks(), sqrl_base64_encode(bytes(url)).decode())
        com.write(command.encode())

        form = readCommand(com)
    else:
        form = sqrl_query(imk, url.get_sks(), bytes(url))
    print('time', time.time() - t1, form)

    t1 = time.time()
    r = requests.post(url.get_http_url(), data=form)
    print('time', time.time() - t1, r)
    # TODO: Check r.code == 200
    sys.stdout.flush()
    return bytes(r.text, encoding='utf-8')

def login_procedure(url_str, com=None):
    # Parse input parameter
    url = SqrlUrl(url_str)
    print('url      ', url)
    print('domain   ', url.domain)
    print('sks      ', url.get_sks())
    print('nut      ', url.get_nut())
    sys.stdout.flush()

    # Transaction #1
    server = query_cmd(url, com)

    # TODO: Check for for tif=E*

    # Transaction #2
    records = sqrl_decode_response(server)
    print('server records', records)

    t1 = time.time()
    if com:
        command = "\x02ident\x1e%s\x1e%s\x1e%s\x1e%s\x03" % (
                url.get_sks(), server.decode(),
                records.get(b'sin', b"").decode(),
                'true' if b'suk' not in records else 'false')
        com.write(command.encode())

        form = readCommand(com)
    else:
        form = sqrl_ident(ilk, imk, url.get_sks(), server,
                records.get(b'sin', None), b'suk' not in records)
    print('time', time.time() - t1, form)

    t1 = time.time()
    r = requests.post(url.get_resp_query_path(records[b'qry']), data=form)
    print('time', time.time() - t1, r)
    sys.stdout.flush()
    # TODO: Check r.code == 200

    # Parse and redirect
    server = bytes(r.text, encoding='utf-8')
    #print('server', server)
    records = sqrl_decode_response(server)
    print('url records', records)

    redirect = records[b'url'].decode('utf-8')
    print('redirect', repr(redirect))
    sys.stdout.flush()
    return redirect

def disable_procedure(url_str, com=None):
    # Parse input parameter
    url = SqrlUrl(url_str)
    print('url      ', url)
    print('domain   ', url.domain)
    print('sks      ', url.get_sks())
    print('nut      ', url.get_nut())
    sys.stdout.flush()

    # Transaction #1
    server = query_cmd(url, com)

    # TODO: Check for for tif=E*

    # Transaction #2
    records = sqrl_decode_response(server)
    print('server records', records)

    t1 = time.time()
    if com:
        raise NotImplemented("No support for disable (yet)")
    else:
        # TODO: Not possible if suk not in records, right? Or not possible to unlock after?
        #       Or is this the 'last' opertunity to provide a suk?
        form = sqrl_disable(ilk, imk, url.get_sks(), server,
                records.get(b'sin', None), b'suk' not in records)
    print('time', time.time() - t1, form)

    t1 = time.time()
    r = requests.post(url.get_resp_query_path(records[b'qry']), data=form)
    print('time', time.time() - t1, r)
    sys.stdout.flush()
    # TODO: Check r.code == 200

    # Parse and handle
    server = bytes(r.text, encoding='utf-8')
    #print('server', server)
    records = sqrl_decode_response(server)
    print('url records', records)

    redirect = records[b'url'].decode('utf-8')
    print('redirect', repr(redirect))
    sys.stdout.flush()
    return redirect


class Handler(http.server.SimpleHTTPRequestHandler):
    com_port = None

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
            self.wfile.write(open(os.path.join(SCRIPT_DIR, 'onepixel.gif'), 'rb').read())
            return

        if self.path in [ '/sqrl.ico', '/favicon.ico']:
            # image/x-icon
            self.send_response(200)
            self.send_header('Content-Type', 'image/x-icon')
            self.send_header('Connection', 'close')
            self.end_headers()
            self.wfile.write(open(os.path.join(SCRIPT_DIR, 'favicon.ico'), 'rb').read())
            return

        #'/c3FybDovL3d3dy5ncmMuY29tL3Nxcmw_bnV0PVdUVjBOZjZBajlQRjdrUDZzdEF6MHcmY2FuPWFIUjBjSE02THk5M2QzY3VaM0pqTG1OdmJTOXpjWEpzTDJScFlXY3VhSFJ0'
        url_str = sqrl_base64_decode(self.path[1:])
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

        redirect = login_procedure(url_str, com_port)

        self.send_response(302)
        self.send_header('Location', redirect)
        self.end_headers()
        print('done')
        sys.stdout.flush()

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
    if len(sys.argv) > 2:
        #print(login_procedure(sys.argv[1]))
        #with serial.Serial('com22', baudrate=115200, timeout=1) as com:
        #    print(login_procedure(sys.argv[1], com))
        print(disable_procedure(sys.argv[1]))
    elif len(sys.argv) > 1:
        pass
    else:
        #Handler.com_port = serial.Serial('com7', baudrate=115200, timeout=1)
        main()
