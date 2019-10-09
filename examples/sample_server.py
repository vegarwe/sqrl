import sys, os
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
sys.path.append(os.path.join(SCRIPT_DIR, '..'))

import json
import logging
import re
import sqlite3
import tornado.httpserver
import tornado.ioloop
import tornado.web
import tornado.websocket

from pysqrl import sqrl_conv
from pysqrl.server import SqrlHandler

#PORT = 8080
#SCHEME = 'http'
#if PORT == 443:
#    SCHEME = 'https'
#    URL = "%s://raiom.no" % (SCHEME)
#elif PORT == 80:
#    URL = "%s://raiom.no" % (SCHEME)
#else:
#    URL = "%s://raiom.no:%i" % (SCHEME, PORT)
PORT = 8080
SCHEME = 'http'
URL = "https://kanskje.de/sqrl"

sqrl_callback = None # TODO: Come up with a better apprach than global variable...?

class SqrlCallbackHandler(SqrlHandler):
    def __init__(self):
        self._websockets = []
        self._conn = sqlite3.connect(os.path.join(SCRIPT_DIR, 'sample_server.sqlite'))
        self._conn.row_factory = sqlite3.Row
        self._db = self._conn.cursor()
        try:
            self._db.execute('select count(*) from sqrl_user')
        except sqlite3.OperationalError:
            self._db.execute("""create table sqrl_user (id INT, username TEXT, idk TEXT, suk TEXT, vuk TEXT)""")
        self._sessions = {}

    def close(self):
        logging.info("closing db")
        self._db.close()
        self._conn.close()

    def ident(self, session_id, idk, suk, vuk):
        if not self.id_found(idk):
            pass
            self._db.execute("insert into sqrl_user (idk, suk, vuk) values(?, ?, ?)", [str(idk), str(suk), str(vuk)])
            self._conn.commit()
        if session_id in self._sessions:
            # TODO: Reauthenticating a session?
            logging.error("Ehhh, what?")
        self._sessions[session_id] = idk

        redirect_url = '%s/user?session_id=%s&msg=Session+authenticated' % (URL, session_id)
        for ws in self._websockets:
            if ws._session_id == session_id:
                ws.redirect_socket_endpoint(redirect_url)
        return redirect_url

    def id_found(self, idk):
        return self.get_user(idk) is not None

    def get_user(self, idk):
        return self._db.execute("select * from sqrl_user where idk = ?", [idk]).fetchone()

    def update_user(self, idk, username):
        self._db.execute("update sqrl_user set username = ? where idk = ?", [username, idk])
        self._conn.commit()

    def add_ws(self, ws):
        self._websockets.append(ws)

    def remove_ws(self, ws):
        if ws in self._websockets:
            self._websockets.remove(ws)

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

class SqrlRequestHandler(tornado.web.RequestHandler):
    def post(self):
        server = sqrl_callback.post(
                self.get_argument('client', ""),
                self.get_argument('server', ""),
                self.get_argument('ids', ""))
        self.write(sqrl_conv.base64_encode(server))


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
        self.set_header('Content-Type', 'text/css')

    def get(self, path):
        logging.debug("path: %r", path)
        if path == 'style.css':
            self.get_style_css()
        elif path.startswith('user'):
            self.get_user()
        elif path.startswith('cancel'):
            self.get_cancel()
        elif path == '' or path == 'index.html':
            self.get_index_html()
        else:
            self.send_error(404)

    def get_index_html(self):
        nut     = sqrl_callback.get_nut()
        ws_url  = URL.replace('http', 'ws')
        if URL.startswith('https'):
            sqrl_url = URL.replace('https', 'sqrl')
        else:
            sqrl_url = URL.replace('http', 'qrl')
        sqrl_url = '%s/sqrl?nut=%s&sfn=%s&can=%s' % (sqrl_url,
                nut, sqrl_conv.base64_encode("Fisken").decode(),
                sqrl_conv.base64_encode(URL + '/cancel').decode())
        localhost_url = 'http://localhost:25519/' + sqrl_conv.base64_encode(sqrl_url).decode()

        self.writeln("<html><head><title>Title goes here.</title></head>")
        self.writeln("<body>")
        self.writeln("  <p>Blipp fisken</p>")
        self.writeln("  <a href='%s'>login</a>" % (sqrl_url))
        self.writeln("  <a href='%s'>localhost</a>" % (localhost_url))
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
        try:
            idk = sqrl_callback._sessions[session_id]
        except KeyError:
            return self.redirect('/sqrl')
        user = sqrl_callback.get_user(idk)
        msg = self.get_argument('msg', None)

        self.writeln("<html>")
        self.writeln("  <head>")
        self.writeln("    <title>Title goes here.</title></head>")
        self.writeln('    <link href="/sqrl/style.css" rel="stylesheet" type="text/css"/>')
        self.writeln("  </head>")
        self.writeln("<body>")
        self.writeln("  <p>Blipp fisken</p>")
        self.writeln("  <p class='fadeShadow'>%s</p>" % msg)
        self.writeln("  <p>Session: %s</p>" % (session_id))
        self.writeln("  <p>idk: %s</p>" % (user['idk']))
        self.writeln("  <p>suk: %s</p>" % (user['suk']))
        self.writeln("  <p>vuk: %s</p>" % (user['vuk']))
        self.writeln("  <form method='post'>")
        self.writeln("    <input type='hidden' name='session_id' value='%s'>" % (session_id))
        self.writeln("    <label for='blapp'>Display name:")
        self.writeln("      <input type='text' name='blapp' value='%s'>" % (user['username'] if user['username'] else ''))
        self.writeln("    </label>")
        self.writeln("    <input type='submit' value='submit'>")
        self.writeln("  </form>")
        self.writeln("</body></html>")

    def get_cancel(self):
        self.writeln("<html>")
        self.writeln("  <head>")
        self.writeln("    <title>Title goes here.</title></head>")
        self.writeln("  </head>")
        self.writeln("<body>")
        self.writeln("  <p>Authentication process was cancelled</p>")
        self.writeln("</body></html>")

    def post(self, path):
        session_id = self.get_argument('session_id', None)
        username = self.get_argument('blapp', None)
        try:
            idk = sqrl_callback._sessions[session_id]
        except KeyError:
            return self.redirect('/sqrl')
        sqrl_callback.update_user(idk, username)
        self.redirect('/sqrl/user?session_id=%s&msg=User+updated' % session_id)

    def get_argument(self, key, default):
        argument = tornado.web.RequestHandler.get_argument(self, key, default)
        if re.match(r'^[A-Za-z0-9_ +-]*$', argument):
            return argument
        logging.error("Input did not match! %r", argument)
        return default

    def writeln(self, text):
        self.write(text)
        self.write('\n')


def log_setup(verbose=False, logfilename=None, name=None):
    formatter = logging.Formatter('%(asctime)s %(levelname)8s: %(message)s')
    console = logging.StreamHandler()
    console.setFormatter(formatter)
    console.setLevel(logging.ERROR)
    if verbose:
        console.setLevel(logging.DEBUG)

    logger = logging.getLogger(name)
    logger.setLevel(logging.DEBUG)
    logger.addHandler(console)

    if logfilename is None:
        return
    logfile = logging.FileHandler(logfilename)
    logfile.setFormatter(formatter)
    logfile.setLevel(logging.INFO)
    if verbose:
        logfile.setLevel(logging.DEBUG)
    logger.addHandler(logfile)


if __name__ == "__main__":
    log_setup(verbose=True)

    sqrl_callback = SqrlCallbackHandler()

    application = tornado.web.Application([
        (r'/ws',    SocketHandler),
        (r"/sqrl",  SqrlRequestHandler),
        (r"/(.*)",  HtmlHandler),
    ])

    ssl_options = None
    if SCHEME == 'https':
        ssl_options = {
                "certfile": os.path.join(__dir__, "ssl", "signed.crt"),
                "keyfile": os.path.join(__dir__,  "ssl", "domain.key"),
                }
    http_server = tornado.httpserver.HTTPServer(application, ssl_options=ssl_options)

    http_server.listen(PORT)
    try:
        tornado.ioloop.IOLoop.current().start()
    except KeyboardInterrupt:
        sqrl_callback.close()
