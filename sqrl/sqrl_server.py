import ed25519
import logging
import random
import string

from sqrl_conv import sqrl_decode_response, sqrl_base64_encode, sqrl_base64_decode


def _id_generator(size=22, chars=string.ascii_uppercase + string.ascii_lowercase + string.digits):
    return ''.join(random.choice(chars) for _ in range(size))


class SqrlHandler(object):
    def ident(self, session_id, idk, suk, vuk):
        raise NotImplementedError()


    def id_found(self, idk):
        raise NotImplementedError()


    def get_nut(self):
        # TODO: Keep track of recently used nut's and issue tif=E1 when approriate
        return _id_generator()


    def _get_url_nut(self, url):
        if not url:
            return None
        if isinstance(url, bytes):
            url = url.decode()
        url.strip()
        find_index = url.find('nut=')
        if find_index < 0:
            return None
        url = url[find_index+len('nut='):]
        find_index = url.find('&')
        if find_index >= 0:
            url = url[:find_index]
        return url


    def _check_signature(self, idk, client, server, ids):
        verifying_key = ed25519.VerifyingKey(sqrl_base64_decode(idk))

        try:
            verifying_key.verify(sqrl_base64_decode(ids), client + server)
            return True
        except ed25519.BadSignatureError:
            return False

    def post(self, client_str, server_str, ids):
        client = sqrl_decode_response(client_str)
        server = sqrl_decode_response(server_str)
        idk    = client.get('idk')

        logging.debug('client:')
        for key, value in client.items():
            logging.debug("  %r: %r", key, value)
        logging.debug('server:')
        for key, value in server.items():
            logging.debug("  %r: %r", key, value)
        logging.debug('ids:')
        logging.debug("  %r", ids)

        signature_ok = self._check_signature(idk, client_str.encode(), server_str.encode(), ids.encode())

        # Handle post request
        tif = 0
        tif ^= 4 # TODO: Check if IP matches
        session_id = None
        suk = None
        sin = None
        url = None

        cmd    = client.get('cmd')
        logging.info("cmd: %r", cmd)
        if not signature_ok:
            logging.warn("signature failed")
            tif ^= 80
        elif cmd == 'query':
            session_id = self._get_url_nut(sqrl_base64_decode(server_str))
            if self.id_found(idk):
                tif ^= 1
            sin = 0
        elif cmd == 'ident':
            session_id  = server.get('session_id', None)
            if not session_id:
                tif ^= 80 # We always included a session_id, so something has gone wrong
            else:
                if self.id_found(idk):
                    tif ^= 1
                suk = client.get('suk', '')
                vuk = client.get('vuk', '')
                url = self.ident(session_id, idk, suk, vuk)
        else:
            tif ^= 10 # Not supported

        new_nut = self.get_nut()
        server  = "ver=1\r\n"
        server += "nut=%s\r\n" % new_nut
        server += "tif=%x\r\n" % tif
        server += "qry=/sqrl/sqrl?nut=%s\r\n" % new_nut
        if suk is not None:
            server += "suk=%s\r\n" % suk
        if sin is not None:
            server += "sin=%s\r\n" % sin
        if session_id:
            server += "session_id=%s\r\n" % session_id
        if url:
            server += "url=%s\r\n" % url
        logging.debug('response')
        for param in server.split('\r\n')[:-1]:
            logging.debug('  %r', param)
        return server

