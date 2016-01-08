import httplib
import logging

from sqrl import baseconv
from sqrl.client.crypt import Crypt
from sqrl.client.parser import URLParser


class Client:
    def __init__(self, masterkey, cps=False):
        self.enc = Crypt(masterkey)
        self.cps = cps

    def login(self, qry):
        url = URLParser(qry)

        success, data = self.query(url, baseconv.encode(url.orig_url))
        if not success:
            return False, "Auth failed. " + data

        success, data = self.ident(url, data)
        if not success:
            return False, "Auth failed. " + data

        return True, data

    def disable_account(self, qry):
        url = URLParser(qry)

        success, data = self.query(url, baseconv.encode(url.orig_url))
        if not success:
            return False, "Auth failed. " + data

        success, data = self.disable(url, data)
        if not success:
            return False, "Auth failed. " + data

        return True, data

    def query(self, url, server, automatic_retry=True):
        client  = "ver=1\r\n"
        client += "cmd=query\r\n"
        client += "idk=%s\r\n" % self.enc.getPublicKey(url.getDomain())
        if self.cps:
            client += "opt=cps\r\n"
        client = baseconv.encode(client)

        ids = self.enc.sign(client + server)

        success, data = self._post_form(url, "client=%s&server=%s&ids=%s" % (client, server, ids))
        logging.debug("client")
        for param in baseconv.decode(client).rstrip('\r\n').split('\r\n'):
            logging.debug('  %r', param)
        logging.debug("server")
        for param in baseconv.decode(server).rstrip('\r\n').split('\r\n'):
            logging.debug('  %r', param)
        logging.debug('  ids    %r', ids)

        if success:
            resp = baseconv.decodeNameValue(data)
            logging.debug("resp %r", resp)
            tif = int(resp['tif'], 16)
            if tif & 0x01 == 0x01:
                logging.info("ID match")
            if tif & 0x04 == 0x04:
                logging.info("IP matched")
            #if tif > 0x04: # TODO: Check tif failure values
            #    logging.warn("Problems with query, tif 0x%02x", tif)

            if tif & 0x20 == 0x20:
                if automatic_retry:
                    logging.debug("0x20 Transient error. Trying again")
                    url = URLParser(url.scheme + "://" + url.netloc + resp['qry'])
                    return self.query(url, data, False)
        return success, data


    def ident(self, url, server):
        resp = baseconv.decodeNameValue(server)

        tif = int(resp['tif'], 16)

        client  = "ver=1\r\n"
        client += "cmd=ident\r\n"
        client += "idk=%s\r\n" % self.enc.getPublicKey(url.getDomain())
        if self.cps:
            client += "opt=cps\r\n"
        if tif & 0x01 == 0x00:
            client += "suk=dMRXbs49XNmVUhsKzta7ESD-cP2QlnxkSaORsswOAj4\r\n" # TODO: ehhh...
            client += "vuk=q13E_hd5CR0WE0A9ZD8571te0Ul47YfsDCWpETuCGcI\r\n" # TODO: ehhh...
        client = baseconv.encode(client)

        ids = self.enc.sign(client + server)

        url = URLParser(url.scheme + "://" + url.netloc + resp['qry'])
        success, data = self._post_form(url, "client=%s&server=%s&ids=%s" % (client, server, ids))
        logging.debug("client")
        for param in baseconv.decode(client).rstrip('\r\n').split('\r\n'):
            logging.debug('  %r', param)
        logging.debug("server")
        for param in baseconv.decode(server).rstrip('\r\n').split('\r\n'):
            logging.debug('  %r', param)
        logging.debug('  ids    %r', ids)

        if success:
            resp = baseconv.decodeNameValue(data)
            logging.debug("resp %r", resp)

            tif = int(resp['tif'], 16)
            if tif & 0x08 == 0x08:
                return False, "0x08 SQRL disabled"
            if tif & 0x10 == 0x10:
                return False, "0x10 Function(s) not supported"
            #if tif > 0x04: # TODO: Check tif failure values
            #    return False, "Tif 0x%02x" % tif

            if 'url' in resp:
                return True, resp['url']
            else:
                return True, None
        return False, data

    def disable(self, url, server):
        resp = baseconv.decodeNameValue(server)

        client  = "ver=1\r\n"
        client += "cmd=disable\r\n"
        client += "idk=%s\r\n" % self.enc.getPublicKey(url.getDomain())
        client = baseconv.encode(client)

        ids = self.enc.sign(client + server)

        url = URLParser(url.scheme + "://" + url.netloc + resp['qry'])
        success, data = self._post_form(url, "client=%s&server=%s&ids=%s" % (client, server, ids))
        logging.debug("client")
        for param in baseconv.decode(client).rstrip('\r\n').split('\r\n'):
            logging.debug('  %r', param)
        logging.debug("server")
        for param in baseconv.decode(server).rstrip('\r\n').split('\r\n'):
            logging.debug('  %r', param)
        logging.debug('  ids    %r', ids)

        if success:
            resp = baseconv.decodeNameValue(data)
            logging.debug("resp %r", resp)

            tif = int(resp['tif'], 16)
            if tif & 0x10 == 0x10:
                return False, "0x10 Function(s) not supported"

            if tif & 0x08 == 0x08:
                return True, "0x08 SQRL disabled"
        return False, data

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

