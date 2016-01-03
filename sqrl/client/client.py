import httplib
import logging

import baseconv
from crypt import Crypt
from parser import URLParser


class Client:
    def __init__(self, masterkey):
        self.enc = Crypt(masterkey)

    def login(self, urlString):
        url = URLParser(urlString)

        success, data = self.query(url)
        if not success:
            return False, "Auth failed. " + data

        success, data = self.ident(url, data)
        if not success:
            return False, "Auth failed. " + data

        resp = baseconv.decodeNameValue(data)
        if not resp.has_key('url'):
            return False, "Auth failed. No success url returned"

        return True, resp['url']

    def query(self, url):
        client  = "ver=1\r\n"
        client += "cmd=query\r\n"
        client += "idk=%s\r\n" % self.enc.getPublicKey(url.getDomain())
        #client += "opt=cps\r\n"
        client = baseconv.encode(client)

        server = url.orig_url
        server = baseconv.encode(server)

        ids = self.enc.sign(client + server)

        return self._post_form(url, "client=%s&server=%s&ids=%s" % (client, server, ids))

    def ident(self, url, server):
        resp = baseconv.decodeNameValue(server)

        client  = "ver=1\r\n"
        client += "cmd=ident\r\n"
        client += "idk=%s\r\n" % self.enc.getPublicKey(url.getDomain())
        #client += "opt=cps\r\n"
        client += "suk=dMRXbs49XNmVUhsKzta7ESD-cP2QlnxkSaORsswOAj4\r\n"
        client += "vuk=q13E_hd5CR0WE0A9ZD8571te0Ul47YfsDCWpETuCGcI\r\n"
        client = baseconv.encode(client)

        ids = self.enc.sign(client + server)

        url = URLParser(url.scheme + "://" + url.netloc + resp['qry'])
        return self._post_form(url, "client=%s&server=%s&ids=%s" % (client, server, ids))

    def _post_form(self, url, body):
        headers = { "Content-type": "application/x-www-form-urlencoded", "Accept": "text/plain"}

        if url.isSecure():
            http = httplib.HTTPSConnection(url.netloc, timeout=9)
        else:
            http = httplib.HTTPConnection(url.netloc, timeout=9)

        # TODO: Use try-catch
        http.request("POST", url.path + "?" + url.query, body, headers)
        response = http.getresponse()

        if response.status == 200:
            return True, response.read()
        else:
            return False, "%s (Error: %s)" % (response.reason, response.status)

