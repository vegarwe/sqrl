# TODO Catch connection errors
# TODO Catch sqrl_url format errors
# TODO Look at libnotify (Gnome)

import os
import logging
import argparse
from getpass import getpass

from sqrl import log_setup
from sqrl.mkm import MKM
from sqrl.client.client import Client

VERSION = "0.1.0"
HOME = os.environ['HOME']
CONFIG_DIR = '.config/sqrl/'
SQRL_DIR = HOME + '/' + CONFIG_DIR


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("-l", "--list",            action="store_true", help="List accounts")
    parser.add_argument("-s", "--select",                   nargs=1,    help="Set an account as default")
    parser.add_argument("--create",                action="store_true", help="Create new account")
    parser.add_argument("--path",       default=SQRL_DIR,   nargs=1,    help="Path for config and key storage")
    parser.add_argument("--cps",                   action="store_true", help="Client Provided Session")
    parser.add_argument("-v", "--verbose",         action="store_true", help="DEBUG output in log")
    parser.add_argument('sqrl_url',     metavar='SQRLURL',  nargs='?',  help='An SQRL url to authenticate')

    return parser.parse_args()


def main():
    args = parse_args()

    log_setup.log_setup(verbose=args.verbose)

    manager = MKM(args.path)

    if args.select:
        select_account(manager, args.select[0])
    elif args.list:
        list_accounts(manager)
    elif args.create:
        create_account(manager)
    else:
        run(manager, args.sqrl_url, args.cps)


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


def export_key(manager, id):
    pass


def delete_account(manager, id):
    pass


def select_account(manager, id):
    if manager.set_account(id):
        list_accounts(manager)
    else:
        print "Invalid Account ID"


def create_account(manager):
    try:
        name = raw_input("Please enter name of Account Owner: ")
        pswd = getpass("Please Enter Master Password: ")
        pswd_confirm = getpass("Please Confirm Master Password: ")
        if manager.create_account({'name': name}, pswd, pswd_confirm):
            print "Account Created"
    except:
        print "Account NOT Created"


def unlock_account(manager):
    password = getpass("Please Enter Master Password: ")
    return manager.get_key(password)


def run(manager, url, cps):
    accounts = manager.list_accounts()
    if not accounts:
        print "No account(s) found"
        return

    #masterkey = unlock_account(manager)
    masterkey = manager.get_key('f')

    if not masterkey:
        print "Invalid Password"

    sqrlclient = Client(masterkey, cps)
    success, data = sqrlclient.login(url)
    if success:
        print "Authenticated"
        if data:
            print "On Linux, run xdg-open %s" % data
    else:
        print "Authentication failed"
