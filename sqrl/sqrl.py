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

from . import WORKING_DIR, VERSION
from cli import mkmCLI
from mkm import MKM
from client import Client
from docopt import docopt


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

    if update_pass:
        mkmCLI.update_password(manager)
    elif account_id:
        mkmCLI.select_account(manager, account_id)
    elif list:
        mkmCLI.list_accounts(manager)
    elif create_acct:
        mkmCLI.create_account(manager)
    elif not debug:
        debug = False

    if url is not None:
        run(url, manager, debug, bool_notify)
    else:
        print "Please supply valid SQRL URL"


def run(url, manager, debug, bool_notify=False):
    accounts = manager.list_accounts()

    if not accounts:
        mkmCLI.create_account(manager)

    masterkey = mkmCLI.unlock_account(manager)

    if masterkey is not False:
        # Create sqrl client and submit request
        sqrlclient = Client(masterkey, url, bool_notify, debug)
        sqrlclient.submit()


if __name__ == "__main__":
    main()
