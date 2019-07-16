import sys, os
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
sys.path.append(os.path.join(SCRIPT_DIR, '..'))

import sys
import binascii
from sqrl.sqrl_s4 import Identity
from sqrl.sqrl_url import SqrlUrl
from sqrl.sqrl_crypto import *
from sqrl import sqrl_conv

def main():
    # Documents/SQRL/vegarwe_test5.sqrl
    password = '1234567890ab'
    rescue_code = '9491-0649-1269-8522-6922-0540'
    sqrlbinary = b'sqrldata}\x00\x01\x00-\x00"wQ\x122\x0e\xb5\x891\xfep\x97\xef\xf2e]\xf6\x0fg\x07\x8c_\xda\xd4\xe0Z\xe0\xb8\t\x96\x00\x00\x00\xf3\x01\x04\x05\x0f\x00\x023\x88\xcd\xa0\xd7WN\xf7\x8a\xd19\xf8\x1c]\x13\x87\x06\xc6\xe8\xf8\xb08\xf6\x14\xd9m\x9e\xf6|\x94\xa4\x1fF\xab}\x0e\xd3\xbf\xa3r\xa3^\xb4\xfb\xcc\xe7\x8cQ\x8d\x8dyRl\x05\xf1\x19|\x90\x03\x06\t\xe0\xb3\x85H\x8c\xe0\xa6\x0fQm\xf6\x94q6-\xee\xe0\xe9I\x00\x02\x00\xea\xde\x04q\xa1\xfaO\x8f\x1c\xf5e\xea\xb3)-^\t\xa5\x00\x00\x00\xf9o$"\x9e\x91\xa6\xa9k\xde\xe2z^&j\xa6\x15\xb5\x04\xf4P\x01e\xcc\xfa\xa8V\xd7\xf4\x94L\xea\xea\xdd><\xcbC\xc5+\xeb\xaf\x18\x88\xf9\xa6\xd4\xce'

    # Parse identity
    identity = Identity(sqrlbinary)
    print(repr(identity))
    print(identity)

    # Decrypt Type2 section
    print("Decoding data, this will take a while")
    sys.stdout.flush()
    iuk = identity.get_uik_from_rescue_code(rescue_code)
    print('iuk ', binascii.hexlify(iuk))
    #iuk = binascii.unhexlify(b'a243bfb0ed04565d0461a7731a8fad18de4bddf8e3833853926ccfaf2e2e04a6')
    assert binascii.hexlify(iuk) == b'a243bfb0ed04565d0461a7731a8fad18de4bddf8e3833853926ccfaf2e2e04a6'
    imk = sqrl_get_imk_from_iuk(iuk)
    print('imk ', binascii.hexlify(imk))
    assert binascii.hexlify(imk) == b'21d70894575e6b6efe991fb86a9868a49f3a72040e88252d82be5a3ac6c3aa23'
    ilk = sqrl_get_ilk_from_iuk(iuk)
    print('ilk ', binascii.hexlify(ilk))
    assert binascii.hexlify(ilk) == b'00d3a56b500bca7908eb89a6f5fe0931388797d42930798d2ffe88d436c94878'

    # Decrypt Type1 section
    print("Decoding data, this will take a while")
    sys.stdout.flush()
    imk, ilk = identity.get_imk_ilk_from_password(password)
    print('imk ', binascii.hexlify(imk))
    print('ilk ', binascii.hexlify(ilk))
    #imk = binascii.unhexlify(b'21d70894575e6b6efe991fb86a9868a49f3a72040e88252d82be5a3ac6c3aa23')
    #ilk = binascii.unhexlify(b'00d3a56b500bca7908eb89a6f5fe0931388797d42930798d2ffe88d436c94878')
    assert binascii.hexlify(imk) == b'21d70894575e6b6efe991fb86a9868a49f3a72040e88252d82be5a3ac6c3aa23'
    assert binascii.hexlify(ilk) == b'00d3a56b500bca7908eb89a6f5fe0931388797d42930798d2ffe88d436c94878'

    # Test per site key
    print('Test crypto')
    url = SqrlUrl(b'sqrl://www.grc.com/sqrl?nut=oGXEUEmTkPG0z0Eka3pHJQ')
    ssk, idk = sqrl_get_idk_for_site(imk, url.get_sks())
    print('idk ', binascii.hexlify(bytes(idk)))
    assert binascii.hexlify(bytes(idk)) == b'cd9f76fdcdbfb99a72c3f64abd318bbebbac7d2c730906369499f0b8c6bb64dd'

    # Test recreating ursk and vuk from suk
    suk = sqrl_conv.base64_decode('YHgTQbQ2MPttIU0g7Uv4d6_tQPN8hxwGE4m8t9C-5C0') # From diagnostics page
    ursk, vuk = sqrl_get_unlock_request_signing_key(suk, iuk)
    print('vuk ', binascii.hexlify(vuk))
    print('ursk', binascii.hexlify(ursk))
    assert vuk == sqrl_conv.base64_decode('369S6x9Yl2sX4cCAGQ03F4DdcUuOgtpPquUJ0WfW4qU') # From diagnostics page

    # Generate new suk and vuk (for new user registration)
    print('Create new keys')
    suk, vuk = sqrl_idlock_keys(ilk)
    print('suk ', binascii.hexlify(suk))
    print('vuk ', binascii.hexlify(vuk))

    # Recreate ursk (for identity unlock/upgrade)
    ursk, vuk = sqrl_get_unlock_request_signing_key(suk, iuk)
    print('vuk ', binascii.hexlify(vuk))
    print('ursk', binascii.hexlify(ursk))

if __name__ == "__main__":
    main()
