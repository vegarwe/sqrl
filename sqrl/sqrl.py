#!/usr/bin/env python

# TODO Catch connection errors
# TODO Catch sqrlurl format errors
# TODO Add logging option

"""
Usage: sqrl [-d] [-n] [--path=<Dir>] [<SQRLURL>]
       sqrl [-l] [-s <AccountID>] [--create]

Options:
  -d              Debugging output
  -l              List Accounts
  -n              Notify via libnotify (Gnome)
  -s              Set an account as Default
  --create        Create New Account
  --path=<Dir>    Path for config and key storage

Example:
    sqrl -l
    sqrl --id 2a9s8x
    sqrl --create
    sqrl -d "sqrl://example.com/login/sqrl?d=6&nut=a95fa8e88dc499758"
"""

import os
import sys
from mkm import MKM
from client import Client
from docopt import docopt
from getpass import getpass

VERSION = "0.1.0"
HOME = os.environ['HOME']
CONFIG_DIR = '.config/sqrl/'
WORKING_DIR = HOME + '/' + CONFIG_DIR


def main():
    arguments = docopt(__doc__, version=VERSION)

    # Collecting arguments
    url = arguments.get('<SQRLURL>')
    account_id = arguments.get('<AccountID>')
    create_acct = arguments.get('--create')
    path = arguments.get('--path')
    debug = arguments.get('-d')
    list = arguments.get('-l')

    if not path:
        path = WORKING_DIR

    manager = MKM(path)

    if account_id:
        select_account(manager, account_id)

    if list:
        list_accounts(manager)

    if create_acct:
        create_account(manager)

    run(url, manager)


def list_accounts(manager):
    """
    List out ID and Name for each account
    or
    Create account is there are none
    """
    accounts = manager.list_accounts()
    output = []
    if accounts:
        for k in accounts.keys():
            line = accounts[k]['id'] + " [Name: " + accounts[k]['name'] + "]"
            if accounts[k]['active']:
                output.append("* " + line)
            else:
                output.append("  " + line)
        print "\n".join(output)
    else:
        create_account(manager)
    sys.exit()


def export_key(manager, id):
    pass


def delete_account(manager, id):
    pass


def select_account(manager, id):
    if manager.set_account(id):
        list_accounts(manager)
    else:
        print "Invalid Account ID"
    sys.exit()


def create_account(manager):
    try:
        name = raw_input("Please enter name of Account Owner: ")
        pswd = getpass("Please Enter Master Password: ")
        pswd_confirm = getpass("Please Confirm Master Password: ")
        if manager.create_account({'name': name}, pswd, pswd_confirm):
            print "Account Created"
    except:
        print "Account NOT Created"
    sys.exit()


def unlock_account(manager):
    password = getpass("Please Enter Master Password: ")
    key = manager.get_key(password)
    if key:
        return key
    else:
        print "Invalid Password"
        return False


def run(url, manager):
    accounts = manager.list_accounts()

    if not accounts:
        create_account(manager)

    masterkey = unlock_account(manager)

    if masterkey is not False:
        sqrlclient = Client(masterkey)
        sqrlclient.query(url)


if __name__ == "__main__":
    main()
