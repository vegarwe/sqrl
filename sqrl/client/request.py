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
        self.headers = SQRLHeader()

        self.url = url
        if self.url.isSecure():
            self.http = httplib.HTTPSConnection(self.url.netloc, timeout=9)
        else:
            self.http = httplib.HTTPConnection(self.url.netloc, timeout=9)

    def get_signed_body(self, enc):
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
        res = [self.url.path, "?", self.url.query]
        return "".join(res)

    def get_url(self):
        return self.url.scheme + "://" + self.url.netloc + self._path()

    def _body(self, body):
        return "sqrlsig=" + body

    def send(self, body):
        sigbody = self._body(body)
        path = self._path()

        print "path", path
        print "body", body
        print "headers", self.headers.get()

        try:
            self.http.request("POST", path, body, self.headers.get())
            response = self.http.getresponse()
        except Exception as e:
            code, msg = e
            print (msg)
            return False, msg

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


class SQRLHeader:
    """
    SQRLHeader
    """
    def __init__(self):
        self.headers = {"Content-type": "application/x-www-form-urlencoded",
                        "Accept": "text/plain"}

    def get(self):
        return self.headers


class SQRLParams:
    """
    SQRLParam
    - Builds out the parameters specific to SQRL
    - Formats and checks each param
    """
    def __init__(self):
        self.buffer = []
        self.set = ""
        self.key = ""
        self.opt = []

    def set_ver(self, ver):
        self.ver = "sqrlver=" + ver

    def set_key(self, key):
        self.key = "sqrlkey=" + key

    def add_opt(self, option):
        if self.opt.length == 0:
            self.opt = ["sqrlopt=" + option]
        else:
            self.opt.append(option)

    def get(self):
        opts = ",".join(self.opt)
        params = filter(None, [self.ver, opts, self.key])
        result = "&".join(params)
        return result
