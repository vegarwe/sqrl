#!/usr/bin/env python
import os


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
        self._create_key()
        self.storageFile = path + "pysqrl.dat"

    def _init_dir(self):
        if not os.path.exists(self.path):
            os.mkdir(self.path)


    def create_account(self, attr, password, password_confirm):
        account = Account("", attr)
        if account.create_key(password, password_confirm):
            self.accounts[account._id] = account.store()
            self.set_account(account._id)
            return self._store()
        else:
            return False


    def list_accounts(self):
        output = {}
        for k in self.accounts.keys():
            output[k] = {'name': self.accounts[k]['name'],
                         'id': k}
        return output


    def get_key(self, password):
        account = Account(self.account['id'], self.account)
        return account.get_key(password)
