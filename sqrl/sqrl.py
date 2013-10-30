#!/usr/bin/env python

# TODO Catch connection errors
# TODO Catch sqrlurl format errors
# TODO Add logging option

"""
Usage: sqrl [-d] [-n] [--path=<Dir>] [<SQRLURL>]
       sqrl [-l] [-s <AccountID>] [--create="<Name>"]

Options:
  -d                    Debugging output
  -l                    List Accounts
  -n                    Notify via libnotify (Gnome)
  -s                    Set an account as Default
  --path=<Dir>          Path for config and key storage
  --create=<Your Name>  Create Account

Example:
    sqrl -l
    sqrl --id 2a9s8x
    sqrl --create="John Doe"
    sqrl -d "sqrl://example.com/login/sqrl?d=6&nut=a95fa8e88dc499758"
"""

import os
import sys
from .mkm import MKM
from client import Client
from docopt import docopt
from getpass import getpass

VERSION = "0.0.2"
HOME = os.environ['HOME']
CONFIG_DIR = '.config/sqrl/'
WORKING_DIR = HOME + '/' + CONFIG_DIR


def main():
    arguments = docopt(__doc__, version=VERSION)

    # Collecting arguments
    url = arguments.get('<SQRLURL>')
    account_id = arguments.get('<AccountID>')
    create_acct = arguments.get('--create')
    bool_notify = arguments.get('-n')
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
        create_account(manager, create_acct)

    if not debug:
        debug = False

    run(url, manager, debug, bool_notify)


def list_accounts(manager):
    """
    List out ID and Name for each account
    """

    accounts = manager.list_accounts()
    output = []
    for k in accounts.keys():
        if accounts[k]['active']:
            output.insert(0, ("* " + accounts[k]['id'] +
                              " [Name: " + accounts[k]['name'] + "]"))
        else:
            output.append("  " + accounts[k]['id'] + " [Name: "
                          + accounts[k]['name'] + "]")
    print "\n".join(output)
    sys.exit()


def export_key(manager, id):
    pass


def delete_account(manager, id):
    pass


def select_account(manager, id):
    manager.set_account(id)
    list_accounts(manager)
    sys.exit()


def create_account(manager, name):
    pswd = getpass("Please Enter Master Password: ")
    pswd_confirm = getpass("Please Confirm Master Password: ")
    if manager.create_account({'name': name}, pswd, pswd_confirm):
        print "Account Created"
    else:
        print "Account NOT Created"
    sys.exit()


def unlock_account(manager):
    password = getpass("Please Enter Master Password: ")
    return manager.get_key(password)


def run(url, manager, debug, bool_notify=False):
    accounts = manager.list_accounts()

    if not accounts:
        print "Please Create an Account first!"
        name = raw_input("Please enter name of Account Owner: ")
        create_account(manager, name)

    masterkey = unlock_account(manager)

    if masterkey is not False:
        # Create sqrl client and submit request
        sqrlclient = Client(masterkey, url, bool_notify, debug)
        sqrlclient.submit()
    else:
        print "Incorrect Password"

if __name__ == "__main__":
    main()
