import base64

"""
BaseConverter
Uitility to encode and decode base64url to spec
"""

#def decode(value):
#    return base64.urlsafe_b64decode(value).rstrip("=")

def encode(value):
    return base64.urlsafe_b64encode(value).rstrip("=")
