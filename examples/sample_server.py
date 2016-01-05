import sys, os
sys.path.append(os.path.join(os.path.dirname(__file__), '..'))

import json
import logging
import re
import tornado.httpserver
import tornado.ioloop
import tornado.web
import tornado.websocket

from sqrl import log_setup
from sqrl import baseconv
from sqrl.server import handler

PORT = 443
URL = "https://raiom.no:%i" % PORT

class User(object):
    def __init__(self, idk, username=None):
        self.idk = idk
        self.username = username

class SqrlCallback(handler.SqrlCallback):
    def __init__(self):
        self._websockets = []
        self._idks = {}
        self._sessions = {}

    def ident(self, session_id, idk, suk, vuk):
        if idk not in self._idks:
            self._idks[idk] = User(idk)
        if session_id not in self._sessions:
            self._sessions[session_id] = self._idks[idk]
        else:
            logging.error("Ehhh, what?")
            # TODO: Reauthenticating a session?
            self._sessions[session_id] = self._idks[idk]

        redirect_url = '%s/user?session_id=%s&msg=Session+authenticated' % (URL, session_id)
        for ws in self._websockets:
            if ws._session_id == session_id:
                ws.redirect_socket_endpoint(redirect_url)

    def id_found(self, idk):
        logging.info('%r', self._idks)
        return idk in self._idks

    def add_ws(self, ws):
        self._websockets.append(ws)

    def remove_ws(self, ws):
        if ws in self._websockets:
            self._websockets.remove(ws)

sqrl_callback = SqrlCallback()

class SocketHandler(tornado.websocket.WebSocketHandler):
    def __init__(self, application, request, **kwargs):
        tornado.websocket.WebSocketHandler.__init__(self, application, request, **kwargs)
        self._session_id = None

    def check_origin(self, origin):
        # http://tornadokevinlee.readthedocs.org/en/latest/websocket.html#tornado.websocket.WebSocketHandler.check_origin
        return True

    def open(self):
        logging.info('opened')

    def on_message(self, message):
        logging.info("on_message: " + message)
        data = json.loads(message)
        self._session_id = data['session_id']
        sqrl_callback.add_ws(self)

    def on_close(self):
        logging.info('closed')
        sqrl_callback.remove_ws(self)

    def redirect_socket_endpoint(self, url):
        self.write_message('{"url": "%s"}' % url)

class SqrlHandler(tornado.web.RequestHandler):
    def post(self):
        server = handler.SqrlHandler().post(
                self.get_argument('client', ""),
                self.get_argument('server', ""),
                self.get_argument('ids', ""),
                sqrl_callback)
        self.write(baseconv.encode(server))


class HtmlHandler(tornado.web.RequestHandler):
    def get_style_css(self):
        self.writeln("@-webkit-keyframes fadeIt {")
        self.writeln("  0%  { text-shadow: 0 0 25px red; }")
        self.writeln("}")
        self.writeln("@-moz-keyframes    fadeIt {")
        self.writeln("  0%  { text-shadow: 0 0 25px red; }")
        self.writeln("}")
        self.writeln("@-o-keyframes      fadeIt {")
        self.writeln("  0%  { text-shadow: 0 0 25px red; }")
        self.writeln("}")
        self.writeln("@keyframes         fadeIt {")
        self.writeln("  0%  { text-shadow: 0 0 25px red; }")
        self.writeln("}")
        self.writeln(".fadeShadow {")
        self.writeln("    background-image:none !important;")
        self.writeln("    -webkit-animation: fadeIt 3s linear;")
        self.writeln("       -moz-animation: fadeIt 3s linear;")
        self.writeln("         -o-animation: fadeIt 3s linear;")
        self.writeln("            animation: fadeIt 3s linear;")
        self.writeln("}")

    def get(self, path):
        logging.debug("path: %r", path)
        if path == 'style.css':
            self.get_style_css()
        elif path.startswith('user'):
            self.get_user()
        elif path == '' or path == 'index.html':
            self.get_index_html()
        else:
            self.send_error(404)

    def get_index_html(self):
        nut      = handler.get_nut()
        if URL.startswith('https'):
            sqrl_url = URL.replace('https', 'sqrl')
        else:
            sqrl_url = URL.replace('http', 'qrl')
        sqrl_url = '%s/sqrl?nut=%s&sfn=%s' % (sqrl_url, nut, baseconv.encode("Fisken"))
        ws_url = URL.replace('http', 'ws')

        self.writeln("<html><head><title>Title goes here.</title></head>")
        self.writeln("<body>")
        self.writeln("  <p>Blipp fisken</p>")
        self.writeln("  <a href='%s'>login</a>" % (sqrl_url))
        self.writeln('  <script>')
        self.writeln('    var ws = new WebSocket("%s/ws");' % (ws_url))
        self.writeln('    ws.onopen = function(){')
        self.writeln('      console.log("onopen");')
        self.writeln('      ws.send("{\\\"session_id\\\": \\\"%s\\\"}");' % (nut))
        self.writeln('    };')
        self.writeln('    ws.onmessage = function(ev){')
        self.writeln('      console.log("onmessage ev.data " + ev.data);')
        self.writeln('      var json = JSON.parse(ev.data);')
        self.writeln('      window.location.href = json.url;')
        self.writeln('    };')
        self.writeln('    ws.onclose = function(ev){')
        self.writeln('      console.log("onclose");')
        self.writeln('    };')
        self.writeln('    ws.onerror = function(ev){')
        self.writeln('      console.log("onerror");')
        self.writeln('    };')
        self.writeln('  </script>')
        self.writeln("  <br/>")
        self.writeln("</body></html>")

    def get_user(self):
        session_id = self.get_argument('session_id', None)
        user = sqrl_callback._sessions[session_id]
        msg = self.get_argument('msg', None)

        self.writeln("<html>")
        self.writeln("  <head>")
        self.writeln("    <title>Title goes here.</title></head>")
        self.writeln('    <link href="/style.css" rel="stylesheet" type="text/css"/>')
        self.writeln("  </head>")
        self.writeln("<body>")
        self.writeln("  <p>Blipp fisken</p>")
        self.writeln("  <p class='fadeShadow'>%s</p>" % msg)
        self.writeln("  <p>Session: %s</p>" % (session_id))
        self.writeln("  <p>IDK: %s</p>" % (user.idk))
        self.writeln("  <form method=post>")
        self.writeln("    <input type='hidden' name='session_id' value='%s'>" % (session_id))
        self.writeln("    <label for='blapp'>Display name:")
        self.writeln("      <input type='text' name='blapp' value='%s'>" % (user.username if user.username else ''))
        self.writeln("    </label>")
        self.writeln("    <input type='submit' value='submit'>")
        self.writeln("  </form>")
        self.writeln("</body></html>")

    def post(self, path):
        session_id = self.get_argument('session_id', None)
        username = self.get_argument('blapp', None)
        user = sqrl_callback._sessions[session_id]
        user.username = username
        self.redirect('/user?session_id=%s&msg=User+updated' % session_id)

    def get_argument(self, key, default):
        argument = tornado.web.RequestHandler.get_argument(self, key, default)
        if re.match(r'^[A-Za-z0-9_ +-]*$', argument):
            return argument
        logging.error("Input did not match! %r", argument)
        return default

    def writeln(self, text):
        self.write(text)
        self.write('\n')


if __name__ == "__main__":
    log_setup.log_setup(verbose=True)

    application = tornado.web.Application([
        (r'/ws', SocketHandler),
        (r"/sqrl", SqrlHandler),
        (r"/(.*)", HtmlHandler),
    ])
    http_server = tornado.httpserver.HTTPServer(application, ssl_options = {
        "certfile": os.path.join(os.path.dirname(__file__), "ssl", "signed.crt"),
        "keyfile": os.path.join(os.path.dirname(__file__),  "ssl", "domain.key"),
    })

    http_server.listen(4443)
    tornado.ioloop.IOLoop.current().start()
