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

    def __init__(self):
        self.headers = {
                "Content-type": "application/x-www-form-urlencoded",
                "Accept": "text/plain"}

    def _path(self, url):
        return url.path + "?" + url.query

    def send(self, url, body):
        if url.isSecure():
            http = httplib.HTTPSConnection(url.netloc, timeout=9)
        else:
            http = httplib.HTTPConnection(url.netloc, timeout=9)

        # TODO: Use try-catch
        #try:
        #    http.request("POST", self._path(url), body, self.headers)
        #    response = http.getresponse()
        #except Exception as e:
        #    code, msg = e
        #    return False, msg

        #print "POST", self._path(url), body, self.headers
        http.request("POST", self._path(url), body, self.headers)
        response = http.getresponse()

        if response.status == 200:
            return True, response.read()
        else:
            return False, "%s (Error: %s)" % (response.reason, response.status)

