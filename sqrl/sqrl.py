#!/usr/bin/env python

# TODO Catch sqrlurl format errors
# TODO Add logging option

"""
Usage: sqrl account ([list] | [create] | [password])
       sqrl account <accountID>
       sqrl [-d] [-n] [--path=<Dir>] <SQRLURL>

Options:
    -d            Debugging output
    -n            Notify via libnotify (Gnome)
    --path=<Dir>  Path for config and key storage
    list          List Accounts
    create        Create New Account
    password      Update password of active account
    <accountID>   Set an account as active

Example:
    sqrl account list
    sqrl account create
    sqrl account 2a9s8x
    sqrl -d "sqrl://example.com/login/sqrl?d=6&nut=a95fa8e88dc499758"
"""

from . import WORKING_DIR, VERSION
from cli import mkmCLI
from mkm import MKM
from client import Client
from docopt import docopt
import sys


def main():
    arguments = docopt(__doc__, version=VERSION)

    # Collecting arguments
    args = {
        'url': arguments.get('<SQRLURL>'),
        'account_id': arguments.get('<accountID>'),
        'create_acct': arguments.get('create'),
        'account_mode': arguments.get('account'),
        'bool_notify': arguments.get('-n'),
        'path': arguments.get('--path'),
        'debug': arguments.get('-d'),
        'update_pass': arguments.get('password'),
        'account_list': arguments.get('list')
    }
    process(args)


def process(args):
    if not args['path']:
        path = WORKING_DIR

    manager = MKM(path)

    if args['account_mode']:
        if args['update_pass']:
            mkmCLI.update_password(manager)
        elif args['account_id']:
            mkmCLI.select_account(manager, args['account_id'])
        elif args['account_list']:
            mkmCLI.list_accounts(manager)
        elif args['create_acct']:
            mkmCLI.create_account(manager)
        print "Account requires more options. sqrl -h"
        sys.exit()

    if not args['debug']:
        args['debug'] = False

    if args['url'] is not None:
        run(args['url'], manager, args['debug'], args['bool_notify'])
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
