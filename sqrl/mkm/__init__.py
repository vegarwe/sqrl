#!/usr/bin/env python
import os
import scrypt
import ed25519
from datetime import datetime


class MKM:
    """
    Master Key Manager
    - Create accounts
    - Delete account
    - List accounts
    - Store accounts
    - Maintain active account
    """

    def __init__(self, path):
        self.path = path
        self.storageFile = path + ".secret_key"
        self._create_key()

    def _init_dir(self):
        if not os.path.exists(self.path):
            os.mkdir(self.path)

    def list_accounts(self):
        # Collect a list of .skey files
        # their name is the is the name of the account
        pass

    def _create_key(self):
        sk, vk = ed25519.create_keypair()
        self._store_key(sk)

    def _store_key(self, sk):
        # if the storageFile doesnt exists or force is set write file
        if not os.path.exists(self.storageFile):
            self._init_dir()
            open(self.storageFile, "wb").write(sk.to_seed())

    def get_key(self):
        seed = open(self.storageFile, "rb").read()
        key = ed25519.SigningKey(seed)
        return key.to_ascii(encoding="base64")


class Account:
    def __init__(self, id="", attr={}):
        if id == "":
            self._id = self._new_id()
            self._init_attr()
        else:
            self._id = id
            self._load(attr)

    def _init_attr(self):
        self._name = ""
        self._created = datetime.now()
        self._encrypted_key = ""
        ielf._modified = datetime.now()

    def _load(self, attr):
        self._name = attr['name']
        self._created = attr['created']
        self._encrypted_key = attr['key']
        self._modified = attr['modified']

    def set_name(self, name):
        if name:
            self._name = name

    def get_key(self, password):
        decrypted_key = scrypt.decrypt(self._key, password, maxtime=1)
        return decrypted_key

    def create_key(self, password, password_confirm):
        if password == password_confirm:
            key = self._new_key()
            self._key = scrypt.encrypt(key, password, maxtime=1)

    def _new_key(self):
        sk, vk = ed25519.create_keypair()
        return sk

    def _new_id(self):
        return "Test!!!"
