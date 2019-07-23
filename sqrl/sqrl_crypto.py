import hashlib
import hmac
import scrypt
import nacl.bindings
import nacl.signing
import nacl.utils


def _bxor(b1, b2):
    result = bytearray(b1)
    for i, b in enumerate(b2):
        result[i] ^= b
    return bytes(result)

def EnScrypt(password, salt, N, iteration_count):
    """ Working test of test's
    print(binascii.hexlify(EnScrypt('', '', 512, 1)))
    print(binascii.hexlify(EnScrypt('', '', 512, 100)))
    print(binascii.hexlify(EnScrypt('', '', 512, 1000)))
    print(binascii.hexlify(EnScrypt('password', '', 512, 123)))
    print(binascii.hexlify(EnScrypt('password',
        binascii.unhexlify('0000000000000000000000000000000000000000000000000000000000000000'), 512, 123)))

    """

    r = 256
    p = 1 # disable parallelism

    key = None
    for _ in range(iteration_count):
        salt = scrypt.hash(password, salt, N=N, r=r, p=p, buflen=32)
        if key is None:
            key = salt
        else:
            key = _bxor(salt, key)
        #print(binascii.hexlify(key), binascii.hexlify(salt))

    return key

def EnHash(data):
    digest = None
    for _ in range(16):
        data = hashlib.sha256(data).digest()
        if digest is None:
            digest = data
        else:
            digest = _bxor(data, digest)
    return digest

def sqrl_hmac(key, msg):
    if isinstance(msg, str):
        msg = msg.encode()
    return hmac.new(key, msg, hashlib.sha256).digest()

def sqrl_get_ins_from_sin(ssk, sin):
    return sqrl_hmac(EnHash(ssk[:32]), sin)

def sqrl_keypair(raw_sign_key):
    verify, sign = nacl.bindings.crypto_sign_seed_keypair(raw_sign_key)
    # TODO: Is raw_sign_key == sign[:32] always true?
    return verify, sign

def sqrl_make_public(priv):
    return nacl.bindings.crypto_scalarmult_base(priv)

def sqrl_idlock_keys(ilk):
    rlk = nacl.utils.random(size=32)
    print('rlk', rlk)
    suk = sqrl_make_public(rlk)
    dhka = nacl.bindings.crypto_scalarmult(rlk, ilk)
    vuk, ursk = sqrl_keypair(dhka)
    return suk, vuk

def sqrl_get_unlock_request_signing_key(suk, iuk):
    dhka = nacl.bindings.crypto_scalarmult(iuk, suk)
    vuk, ursk = sqrl_keypair(dhka)
    # TODO: Is dhka == ursk always true?
    # TODO: Don't really need to return vuk...
    return ursk, vuk

def sqrl_get_imk_from_iuk(iuk):
    return EnHash(iuk)

def sqrl_get_ilk_from_iuk(iuk):
    return sqrl_make_public(iuk)

def sqrl_get_idk_for_site(imk, sks):
    idk, ssk = sqrl_keypair(sqrl_hmac(imk, sks))
    return idk, ssk

def sqrl_sign(signing_key, message):
    raw_signed = nacl.bindings.crypto_sign(message, signing_key)
    return raw_signed[:nacl.bindings.crypto_sign_BYTES]
