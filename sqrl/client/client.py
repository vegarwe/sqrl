#!/usr/bin/env python

from crypt import Crypt
from parser import URLParser
from request import SQRLRequest


class Client:
    def __init__(self, masterkey):
        self.sqrlreq = SQRLRequest()
        self.enc = Crypt(masterkey)


    def query(self, urlString):
        url = URLParser(urlString)


        client  = "ver=1\r\n"
        client += "cmd=query\r\n"
        client += "idk=%s\r\n" % self.enc.getPublicKey(url.getDomain())
        client += "opt=cps\r\n"
        client += "url=https://www.grc.com/sqrl/diag.htm\r\n" # TODO: Need to start hard coding this
        client = baseconv.encode(client)

        server = url.orig_url
        server = baseconv.encode(server)

        ids = self.enc.sign(client + server)

        success, data = self.sqrlreq.send(url, "client=%s&server=%s&ids=%s" % (client, server, ids))

        if success:
            self._query_response(url, data)
        else:
            # TODO: How to report failure
            print "Auth failed"

    def _query_response(self, url, server):
        resp = baseconv.decodeNameValue(server)

        client  = "ver=1\r\n"
        client += "cmd=ident\r\n"
        client += "idk=%s\r\n" % self.enc.getPublicKey(url.getDomain())
        client += "opt=cps\r\n"
        client += "url=https://www.grc.com/sqrl/diag.htm\r\n"
        client += "suk=dMRXbs49XNmVUhsKzta7ESD-cP2QlnxkSaORsswOAj4\r\n"
        client += "vuk=q13E_hd5CR0WE0A9ZD8571te0Ul47YfsDCWpETuCGcI\r\n"
        client = baseconv.encode(client)

        ids = self.enc.sign(client + server)

        url = URLParser(url.scheme + "://" + url.netloc + resp['qry'])
        success, data = self.sqrlreq.send(url, "client=%s&server=%s&ids=%s" % (client, server, ids))

        resp = baseconv.decodeNameValue(data)
