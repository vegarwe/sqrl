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
        self.storageFile = path + ".secret_key"
        self._create_key()

    def _init_dir(self):
        if not os.path.exists(self.path):
            os.mkdir(self.path)

    def list_accounts(self):
        # Collect a list of .skey files
        # their name is the is the name of the account
        pass

    def create_account(self, attr, password, password_confirm):
        account = Account("", attr)
        if account.create_key(password, password_confirm):
            self.accounts[account._id] = account.store()
            self.set_account(account._id)
            return self._store()
        else:
            return False




    def get_key(self, password):
        account = Account(self.account['id'], self.account)
        return account.get_key(password)
