#!/usr/bin/env python

import httplib
import baseconv
#from sqrl.test import test

__sqrlver__ = "1"


class SQRLRequest():
    """
    SQRLRequest
    - Formats SQRL request
    - Submits the SQRL reuqest
    """

    def __init__(self, url):
        self.headers = {
                "Content-type": "application/x-www-form-urlencoded",
                "Accept": "text/plain"}

        self.url = url
        if self.url.isSecure():
            self.http = httplib.HTTPSConnection(self.url.netloc, timeout=9)
        else:
            self.http = httplib.HTTPConnection(self.url.netloc, timeout=9)

    def get_signed_body(self, enc):
        # TODO: Replace with SQRLBody or some other class later
        client  = "ver=1\r\n"
        client += "cmd=query\r\n"
        client += "idk=%s\r\n" % enc.getPublicKey(self.url.getDomain())
        client += "opt=cps\r\n"
        client += "url=https://www.grc.com/sqrl/diag.htm\r\n"
        client = baseconv.encode(client)

        server = self.url.orig_url
        server = baseconv.encode(server)
        ids = enc.sign(client + server)
        #ids = baseconv.encode(ids)

        return "client=%s&server=%s&ids=%s" % (client, server, ids)

    def _path(self):
        return self.url.path + "?" + self.url.query

    def get_url(self):
        return self.url.scheme + "://" + self.url.netloc + self._path()

    def send(self, body):
        try:
            self.http.request("POST", self._path(), body, self.headers)
            response = self.http.getresponse()
        except Exception as e:
            code, msg = e
            print (msg)
            return False, msg

        blipp = response.read()
        print "response:\n%r" % blipp
        if response.status == 200:
            msg = "===\nAuthentication Successful!"
            print msg
            return True, msg
        else:
            msg = "===\nAuthentication Failed!\n" + \
                response.reason + " (Error: " + \
                str(response.status) + ")"
            print msg
            return False, msg

