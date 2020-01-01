from urllib.parse import urlparse
from urllib.parse import parse_qs

# TODO: lower base domain and scheme part?

class SqrlUrl():
    def __init__(self, orig_url):
        if isinstance(orig_url, (bytes, bytearray)):
            orig_url = orig_url.decode()

        self.orig_url = orig_url

        # Validate URL
        parsed_url = urlparse(orig_url)
        if parsed_url.scheme not in ["sqrl", "qrl"]:
            raise ValueError("Bad Scheme: %s" % parsed_url.scheme)

        if parsed_url.netloc is "":
            raise ValueError("No Domain")

        if parsed_url.query is "":
            raise ValueError("No Query String")

        self.domain     = parsed_url.netloc.split(":")[0] # Remove any port number
        self.netloc     = parsed_url.netloc
        self.path       = parsed_url.path
        self.scheme     = parsed_url.scheme
        self.query      = parsed_url.query
        self.queries    = parse_qs(parsed_url.query)

    def __repr__(self):
        return '%s(%s)' % (self.__class__.__name__, self.orig_url)

    def __bytes__(self):
        return self.orig_url.encode('utf-8')

    def is_secure(self):
        return self.scheme == "sqrl"

    def get_nut(self):
        if 'nut' in self.queries:
            return self.queries['nut']

    def get_sks(self):
        """Get 'site key string'"""
        # https://sqrl.grc.com/?sqrl/authenticate/&token=zvXn9XVXOehBH0Dj8s862bmM
        if "x" in self.queries: # TODO: What about 'd='?
            depth = int(self.queries['x'][0])
            return self.domain + self.path[:depth]
        else:
            return self.domain

    def get_http_url(self):
        if self.is_secure:
            return 'https' + self.orig_url[4:]
        else:
            return 'http'  + self.orig_url[3:]

    def get_resp_query_path(self, qry):
        if isinstance(qry, (bytes, bytearray)):
            qry = qry.decode()
        if self.is_secure():
            return 'https://' + self.domain + qry
        else:
            return 'http://'  + self.domain + qry
