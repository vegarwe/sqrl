import httplib
import baseconv

def post_form(url, body):
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

