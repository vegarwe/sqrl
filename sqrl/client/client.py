import request
import baseconv
from crypt import Crypt
from parser import URLParser


class Client:
    def __init__(self, masterkey):
        self.enc = Crypt(masterkey)


    def login(self, urlString, site_url=None):
        url = URLParser(urlString)

        success, data = self.query(url, site_url)

        if not success:
            return False, "Auth failed. " + data

        success, data = self.ident(url, data)
        if not success:
            return False, "Auth failed. " + data

        resp = baseconv.decodeNameValue(data)
        if not resp.has_key('url'):
            return False, "Auth failed. No success url returned"

        return True, resp['url']

    def query(self, url, site_url):
        client  = "ver=1\r\n"
        client += "cmd=query\r\n"
        client += "idk=%s\r\n" % self.enc.getPublicKey(url.getDomain())
        client += "opt=cps\r\n"
        #client += "url=https://www.grc.com/sqrl/diag.htm\r\n" # TODO: Need to start hard coding this
        client = baseconv.encode(client)

        server = url.orig_url
        server = baseconv.encode(server)

        ids = self.enc.sign(client + server)

        return request.post_form(url, "client=%s&server=%s&ids=%s" % (client, server, ids))

    def ident(self, url, server):
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
        return request.post_form(url, "client=%s&server=%s&ids=%s" % (client, server, ids))
