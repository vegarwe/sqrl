#!/usr/bin/env python

from crypt import Crypt
from parser import URLParser
from request import SQRLRequest


class Client:
    def __init__(self, masterkey, url):
        self.sqrlreq = SQRLRequest(URLParser(url))
        self.signed_body = self.sqrlreq.get_signed_body(Crypt(masterkey))

    def submit(self):
        result, msg = self.sqrlreq.send(self.signed_body)
