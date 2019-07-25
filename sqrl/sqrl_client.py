from .sqrl_conv import sqrl_base64_encode, sqrl_base64_decode
from .sqrl_url import SqrlUrl
from .sqrl_crypto import *

def sqrl_query(imk, sks, server):
    # Get site specific keys
    idk, ssk = sqrl_get_idk_for_site(imk, sks)

    client  =  b'ver=1\r\n'
    client  += b'cmd=query\r\n'
    client  += b'idk=%s\r\n' % sqrl_base64_encode(idk)
    client  += b'opt=cps~suk\r\n'
    print('client', client)
    client  = sqrl_base64_encode(client)

    print('server', server)
    server  = sqrl_base64_encode(server)

    ids     = sqrl_sign(ssk, client + server)
    print('ids', ids)
    form    = {'client': client,
               'server': server,
               'ids':    sqrl_base64_encode(ids)}
    return form


def sqrl_ident(ilk, imk, sks, server, sin, create_suk):
    # Get site specific keys
    idk, ssk = sqrl_get_idk_for_site(imk, sks)

    client  =  b'ver=1\r\n'
    client  += b'cmd=ident\r\n'
    client  += b'idk=%s\r\n' % sqrl_base64_encode(idk)
    if sin:
        ins = sqrl_hmac(EnHash(ssk), sin)
        client  += b'ins=%s\r\n' % sqrl_base64_encode(ins)
    if create_suk:
        suk, vuk = sqrl_idlock_keys(ilk)
        client  += b'suk=%s\r\n' % sqrl_base64_encode(suk)
        client  += b'vuk=%s\r\n' % sqrl_base64_encode(vuk)
    client  += b'opt=cps~suk\r\n' # TODO: Not always true?
    client  = sqrl_base64_encode(client)

    ids     = sqrl_sign(ssk, client + server)
    form    = {'client': client,
               'server': server,
               'ids':    sqrl_base64_encode(ids)}
    return form

def sqrl_disable(ilk, imk, sks, server, sin, create_suk):
    # Get site specific keys
    idk, ssk = sqrl_get_idk_for_site(imk, sks)

    client  =  b'ver=1\r\n'
    client  += b'cmd=disable\r\n'
    client  += b'idk=%s\r\n' % sqrl_base64_encode(idk)
    if sin:
        ins = sqrl_hmac(EnHash(ssk), sin)
        client  += b'ins=%s\r\n' % sqrl_base64_encode(ins)
    if create_suk:
        # TODO: Does this even make any sense?
        suk, vuk = sqrl_idlock_keys(ilk)
        client  += b'suk=%s\r\n' % sqrl_base64_encode(suk)
        client  += b'vuk=%s\r\n' % sqrl_base64_encode(vuk)
    client  += b'opt=cps~suk\r\n' # TODO: Not always true?
    client  = sqrl_base64_encode(client)

    ids     = sqrl_sign(ssk, client + server)
    form    = {'client': client,
               'server': server,
               'ids':    sqrl_base64_encode(ids)}
    return form

