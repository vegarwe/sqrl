import scrypt
import ed25519
import random
import base64
from datetime import datetime


KEYLENGTH = 5


class Account:
    """
    Account information for specific user in the key management system
    - creates key for user
    - authenticates users credentials
    -
    """

    def __init__(self, id="", attr={}):
        if id == "":
            self._id = random_id(KEYLENGTH)
            self._init_attr(attr)
        else:
            self._id = id
            self._load(attr)

    def _init_attr(self, attr):
        self._name = attr['name']
        self._created = datetime.now()
        self._encrypted_key = ""
        self._modified = datetime.now()
        self._active = True

    def _set_key(self, key, password):
        key = scrypt.encrypt(key, password, maxtime=1)
        self._encrypted_key = base64.b32encode(key)

    def _load(self, attr):
        self._name = attr['name']
        self._created = attr['created']
        self._encrypted_key = attr['key']
        self._modified = attr['modified']
        self._active = attr['active']

    def change_password(self, old_pass, new_pass, new_pass_conf):
        if new_pass == new_pass_conf:
            key = self.get_key(old_pass)
            if key is not False:
                self._set_key(key, new_pass)
                return True
            else:
                print "Password Incorrect"
                return False
        else:
            print "New Password Confirmation Doesnt Match"
            return False

    def store(self):
        attr = {'name': self._name,
                'active': self._active,
                'created': self._created,
                'id': self._id,
                'key': self._encrypted_key,
                'modified': self._modified
                }
        return attr

    def set_name(self, name):
        if name:
            self._name = name

    def get_key(self, password):
        try:
            key = base64.b32decode(self._encrypted_key)
            decrypted_key = scrypt.decrypt(key, password)
            return decrypted_key
        except scrypt.error:
            return False

    def create_key(self, password, password_confirm):
        if password == password_confirm:
            key = self._new_key()
            key = base64.b32encode(key.sk_s)
            self._set_key(key)
            return True
        else:
            return False

    def _new_key(self):
        sk, vk = ed25519.create_keypair()
        return sk


def random_id(length):
    number = '0123456789'
    alpha = 'abcdefghijklmnopqrstuvwxyz'
    id = ''
    for i in range(0, length, 2):
        id += random.choice(number)
        id += random.choice(alpha)
    return id
