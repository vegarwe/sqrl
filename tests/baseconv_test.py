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

