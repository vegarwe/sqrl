import unittest

from sqrl.client import baseconv

class TestNoPaddingBase64(unittest.TestCase):

    def test_encode_no_padding(self):
        self.assertEqual(baseconv.encode("abc"), 'YWJj')

    def test_encode_padding_stripped(self):
        self.assertEqual(baseconv.encode("abcd"), 'YWJjZA')
        self.assertEqual(baseconv.encode("abcde"), 'YWJjZGU')

    def test_decode_no_padding(self):
        self.assertEqual(baseconv.decode("YWJj"), 'abc')

    def test_decode_padding_added_back(self):
        self.assertEqual(baseconv.decode("YWJjZA"), 'abcd')
        self.assertEqual(baseconv.decode("YWJjZGU"), 'abcde')

    def test_decodeNameValue_server_response(self):
        server = baseconv.decodeNameValue('dmVyPTENCm51dD1BaHZoZHk4U3hucy1QaXdhTUtBbG53DQp0aWY9NDI0DQpxcnk9L3Nxcmw_bnV0PUFodmhkeThTeG5zLVBpd2FNS0FsbncNCnNmbj1HUkMNCg')

        expected = {
            'ver': '1',
            'nut': 'Ahvhdy8Sxns-PiwaMKAlnw',
            'qry': '/sqrl?nut=Ahvhdy8Sxns-PiwaMKAlnw',
            'sfn': 'GRC',
            'tif': '424'}
        self.assertEqual(expected, server)

    def test_encodeNameValue_ver_always_first(self):
        server = baseconv.encodeNameValue({'ver': '1', 'nut': 'Ahvhdy8Sxns-PiwaMKAlnw'})
        self.assertEqual('dmVyPTENCm51dD1BaHZoZHk4U3hucy1QaXdhTUtBbG53DQo', server)

        server = baseconv.encodeNameValue({'nut': 'Ahvhdy8Sxns-PiwaMKAlnw', 'ver': '1'})
        self.assertEqual('dmVyPTENCm51dD1BaHZoZHk4U3hucy1QaXdhTUtBbG53DQo', server)
