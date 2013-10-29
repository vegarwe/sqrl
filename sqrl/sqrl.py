#!/usr/bin/env python

# TODO Catch connection errors
# TODO Catch sqrlurl format errors
# TODO Add logging option

"""
Usage: sqrl [-d] [-n] [-l] [--create="<Users Name>"] [--path=<Dir>] [<SQRLURL>]

Options:
  -d               Debugging output
  -l               List Accounts
  -c <Your Name>   Create Account
  -n               Notify via libnotify (Gnome)
  -p --path=<Dir>  Path for config and key storage

Example:
    sqrl "sqrl://example.com/login/sqrl?d=6&nut=a95fa8e88dc499758"
"""

import os
import sys
from .mkm import MKM
from client import Client
from docopt import docopt

VERSION = "0.0.2"
HOME = os.environ['HOME']
CONFIG_DIR = '.config/sqrl/'
WORKING_DIR = HOME + '/' + CONFIG_DIR


def main():
    arguments = docopt(__doc__, version=VERSION)

    # Collecting arguments
    url = arguments.get('<SQRLURL>')
    create_acct = arguments.get('--create')
    bool_notify = arguments.get('-n')
    path = arguments.get('--path')
    debug = arguments.get('-d')
    list = arguments.get('-l')

    if not path:
        path = WORKING_DIR

    manager = MKM(path)

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
    for k in accounts.keys():
        print accounts[k]['id'] + " [Name: " + accounts[k]['name'] + "]"
    sys.exit()


def create_account(manager, name):
    password = raw_input("Please Enter Master Password: ")
    password_confirm = raw_input("Please Confirm Master Password: ")

    if manager.create_account({'name': name},
                              password, password_confirm):
        print "Account Created"
    else:
        print "Account NOT Created"
    sys.exit()



    # Create sqrl client and submit request
    sqrlclient = Client(masterkey, url, bool_notify, debug)
    sqrlclient.submit()

if __name__ == "__main__":
    main()
