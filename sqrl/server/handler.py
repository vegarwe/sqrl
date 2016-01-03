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

    def post(self, client_str, server_str, ids, sqrl_callback):
        client = baseconv.decodeNameValue(client_str)
        server = baseconv.decodeNameValue(server_str)
        cmd    = self._get_value(client, 'cmd')
        idk    = self._get_value(client, 'idk')

        logging.info("cmd: %r", cmd)
        logging.debug('client:')
        for key, value in client.iteritems():
            logging.debug("  %r: %r", key, value)
        logging.debug('server:')
        for key, value in server.iteritems():
            logging.debug("  %r: %r", key, value)
        logging.debug('ids:')
        logging.debug("  %r", ids)

        tif = 0
        # TODO: Verify ids
        tif ^= 4 # TODO: Check if IP matches

        if cmd == 'query':
            session_id = self._get_url_nut(baseconv.decode(server_str))
            if sqrl_callback.id_found(idk):
                tif ^= 1
        elif cmd == 'ident':
            session_id  = self._get_value(server, 'session_id')
            if not session_id:
                tif ^= 80 # We always included a session_id, so something has gone wrong
            else:
                if sqrl_callback.id_found(idk):
                    tif ^= 1
                suk = self._get_value(server, 'suk')
                vuk = self._get_value(server, 'vuk')
                sqrl_callback.ident(session_id, idk, suk, vuk)
        else:
            tif ^= 10 # Not supported

        new_nut = get_nut()
        server  = "ver=1\r\n"
        server += "nut=%s\r\n" % new_nut
        server += "qry=/sqrl?nut=%s\r\n" % new_nut
        server += "tif=%x\r\n" % tif
        server += "session_id=%s\r\n" % session_id
        logging.debug('response')
        for param in server.split('\r\n'):
            logging.debug('  %r', param)
        return server

