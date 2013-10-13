#!/usr/bin/python

import hmac
import ed25519
import argparse
import base64
from urlparse import urlparse
from urlparse import parse_qs


class URIParser():
    def __init__(self, uri):
        self.orig_uri = uri
        uri_parsed = urlparse(uri)
        self._processURI(uri_parsed)

    def _processURI(self, uri):
        self.domain = uri.netloc
        self.path = uri.path
        self.scheme = uri.scheme
        self.query = uri.query
        self.queries = parse_qs(uri.query)

    def getURI(self):
        return self.orig_uri

    def getDomain(self):
        domain = ''
        if "d" in self.queries:
            depth = int(self.queries['d'][0])
            domain = self.domain + self.path[0:depth]
        else:
            domain = self.domain
        return domain


# Master Key Manager
class MKM:
    def create_key(self):
        sk, vk = ed25519.create_keypair()
        self._store_key(sk)

    def _store_key(self, sk):
        open("my-secret-seed", "wb").write(sk.to_seed())

    def get_key(self):
        seed = open("my-secret-seed", "rb").read()
        return ed25519.SigningKey(seed)


# Encryptor
# Responsible for:
# - Creating site specific key pair
# - Signing SRQL response
# - Providing public key
class Encryptor:
    def __init__(self, masterkey, domain):
        self.masterkey = masterkey
        self.domain = domain
        self.sk, self.vk = self._site_key_pair()

    def _site_key_pair(self):
        seed = self._site_seed()
        sk = ed25519.SigningKey(seed)
        vk = sk.get_verifying_key()
        return sk, vk

    def _site_seed(self):
        key = self.masterkey.to_ascii(encoding="base64")
        local_hmac = hmac.new(key)
        local_hmac.update(self.domain)
        return local_hmac.hexdigest()

    def sign(self, value):
        signed = self.sk.sign(value, encoding="base64")
        signed = BaseConverter.encode(signed)
        return BaseConverter.cleanUp(signed)

    def getPublicKey(self):
        key = self.vk.to_ascii(encoding="base64")
        key = BaseConverter.encode(key)
        return BaseConverter.cleanUp(key)
class BaseConverter:
    @classmethod
    def cleanUp(self, value):
        while value[-1] == "=":
            value = value.rstrip("=")
        return value

    @classmethod
    def decode(self, value):
        return base64.urlsafe_b64decode(value)

    @classmethod
    def encode(self, value):
        return base64.urlsafe_b64encode(value)


def test(uri, signed_uri, public_key):
    verifying_key = ed25519.VerifyingKey(public_key, encoding="base64")
    key = BaseConverter.decode(public_key + "==")
    verifying_key = ed25519.VerifyingKey(key, encoding="base64")

    print "URI: " + uri
    print "Domin: \"" + domain + "\""
    print "Publick Key: " + public_key
    print "Signed URI: " + signed_uri
    try:

        signed_uri = BaseConverter.decode(signed_uri + "=")
        verifying_key.verify(signed_uri, uri, encoding="base64")
        print "signature is good"
    except ed25519.BadSignatureError:
        print "signature is bad!"


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("sqrlurl")

    args = parser.parse_args()
    uri = args.sqrlurl

    uriparsed = URIParser(uri)
    domain = uriparsed.getDomain()

    manager = MKM()
    #manager.create_key()
    masterkey = manager.get_key()

    enc = Encryptor(masterkey, domain)
    public_key = enc.getPublicKey()
    signed_uri = enc.sign(uri)

    test(uri, signed_uri, public_key)
