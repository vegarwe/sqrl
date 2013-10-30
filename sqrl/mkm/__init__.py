#!/usr/bin/env python
import os
import pickle
from account import Account


class MKM:
    """
    Master Key Manager
    - Create accounts
    - Delete account
    - List accounts
    - Store accounts
    - Maintain active account
    """

    account = None
    accounts = {}

    def __init__(self, path):
        self.path = path
        self.storageFile = path + "pysqrl.dat"
        self._init_dir()

    def _init_dir(self):
        if not os.path.exists(self.path):
            os.mkdir(self.path)
        if not os.path.exists(self.storageFile):
            self._store()
        else:
            self._load_accounts()

    def _load_accounts(self):
        try:
            file = open(self.storageFile, "rb")
            self.accounts = pickle.load(file)
            for k in self.accounts.keys():
                if self.accounts[k]['active'] is True:
                    self.set_account(self.accounts[k]['id'])
        except:
            # Raise FileNotFound on Error
            return False

    def _store(self):
        try:
            file = open(self.storageFile, "wb")
            pickle.dump(self.accounts, file)
            return True
        except:
            return False

    def create_account(self, attr, password, password_confirm):
        account = Account("", attr)
        if account.create_key(password, password_confirm):
            self.accounts[account._id] = account.store()
            self.set_account(account._id)
            return self._store()
        else:
            return False

    def delete_account(self, account_id, password):
        pass

    def list_accounts(self):
        output = {}
        for k in self.accounts.keys():
            output[k] = {'active': self.accounts[k]['active'],
                         'name': self.accounts[k]['name'], 'id': k}
        return output

    def set_account(self, account_id):
        for k in self.accounts.keys():
            self.accounts[k]['active'] = False

        if account_id in self.accounts:
            self.accounts[account_id]['active'] = True
            self.account = self.accounts[account_id]

        self._store()

    def get_key(self, password):
        account = Account(self.account['id'], self.account)
        return account.get_key(password)
