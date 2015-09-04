#!/usr/bin/env python

import os
import sys
from mkm import MKM
from client.client import Client
from client import baseconv

VERSION = "0.1.0"
HOME = os.environ['HOME']
CONFIG_DIR = '.config/sqrl/'
WORKING_DIR = HOME + '/' + CONFIG_DIR

manager = None
def run(url):
    global manager
    if manager == None: return

    masterkey = manager.get_key('f')

    sqrlclient = Client(masterkey)
    return sqrlclient.login(url)


import BaseHTTPServer
class MyHandler(BaseHTTPServer.BaseHTTPRequestHandler):
    def do_GET(s):
        try:
            url = baseconv.decode(s.path[1:])
        except:
            return

        success, login = run(url)
        if success:
            s.send_response(301)
            s.send_header("Location", login)
            s.end_headers()
        else:
            s.send_response(200)
            s.send_header("Content-type", "text/html")
            s.end_headers()
            s.wfile.write("<html><head><title>Title goes here.</title></head>")
            s.wfile.write("<body><p>This is a test.</p>")
            s.wfile.write("<p>You accessed path: %s</p>" % login)
            s.wfile.write("</body></html>")

def main():
    global manager
    manager = MKM(WORKING_DIR)

    server_class = BaseHTTPServer.HTTPServer
    httpd = server_class(('localhost', 25519), MyHandler)
    print "Started"
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        print "Ending"
    httpd.server_close()

if __name__ == "__main__":
    main()
