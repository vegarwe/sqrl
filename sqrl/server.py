import ed25519
import logging
import random
import string

from sqrl import baseconv


def _id_generator(size=22, chars=string.ascii_uppercase + string.ascii_lowercase + string.digits):
    return ''.join(random.choice(chars) for _ in range(size))


def get_nut():
    return _id_generator()


class SqrlCallback(object):
    def ident(self, session_id, idk, suk, vuk):
        raise NotImplementedError()

    def id_found(self, idk):
        raise NotImplementedError()


class SqrlHandler(object):
    def _get_value(self, nameValuePair, name):
        if nameValuePair and nameValuePair.has_key(name) and nameValuePair[name]:
            return nameValuePair[name]
        return None

    def _get_url_nut(self, url):
        if not url:
            return None
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
        verifying_key = ed25519.VerifyingKey(baseconv.decode(idk))

        try:
            verifying_key.verify(baseconv.decode(ids), str(client + server))
            return True
        except ed25519.BadSignatureError:
            return False

    def post(self, client_str, server_str, ids, sqrl_callback):
        client_str = str(client_str) # TODO(vw): Do we want to support u'nicode' or just convert?
        server_str = str(server_str) # TODO(vw): Do we want to support u'nicode' or just convert?
        client = baseconv.decodeNameValue(client_str)
        server = baseconv.decodeNameValue(server_str)
        idk    = self._get_value(client, 'idk')

        logging.debug('client:')
        for key, value in client.iteritems():
            logging.debug("  %r: %r", key, value)
        logging.debug('server:')
        for key, value in server.iteritems():
            logging.debug("  %r: %r", key, value)
        logging.debug('ids:')
        logging.debug("  %r", ids)

        signature_ok = self._check_signature(idk, client_str, server_str, ids)

        # Handle post request
        tif = 0
        tif ^= 4 # TODO: Check if IP matches
        session_id = None
        suk = None
        sin = None
        url = None

        cmd    = self._get_value(client, 'cmd')
        logging.info("cmd: %r", cmd)
        if not signature_ok:
            logging.warn("signature failed")
            tif ^= 80
        elif cmd == 'query':
            session_id = self._get_url_nut(baseconv.decode(server_str))
            #if sqrl_callback.id_found(idk):
            #    tif ^= 1
            sin = 0
        elif cmd == 'ident':
            session_id  = self._get_value(server, 'session_id')
            if not session_id:
                tif ^= 80 # We always included a session_id, so something has gone wrong
            else:
                #if sqrl_callback.id_found(idk):
                #    tif ^= 1
                suk = self._get_value(client, 'suk') # TODO: Can be ''
                vuk = self._get_value(client, 'vuk')
                sqrl_callback.ident(session_id, idk, suk, vuk)
            url = 'https://kanskje.de/sqrl/success?session_id=%s' % session_id
        else:
            tif ^= 10 # Not supported

        new_nut = get_nut()
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

