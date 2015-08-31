import unittest
#from sqrl.mkm import Account

#class AccountTest(unittest.TestCase):
#    def setUp(self):
#        self.attr = {'name': "ChestCold",
#                     'created': "now",
#                     'key': 'KBGCYMM3C2QS\
#                     RZYPEHJWRJNZJLHIR7J4\
#                     OAITNMVNIUKGB72YW7F6\
#                     DUD3I6G776NYBIWQR2XS\
#                     5F4OOMFXXXV7UM6R7PYE\
#                     UETCHIOQZSY=',
#                     'modified': "now"
#                     }
#        self.acc = Account("SAMPLE", self.attr)
#
#    def test_get_key_fail_with_wrong_password(self):
#        self.acc.create_key("password", "password")
#        result = self.acc.get_key("pass")
#        assert result == False
#
#    def test_get_key_with_correct_password(self):
#        self.acc.create_key("password", "password")
#        result = self.acc.get_key("password")
#        assert result != False
#
#    def test_create_key_mismatch_password(self):
#        result = self.acc.create_key("password", "passwords")
#        assert result == False
