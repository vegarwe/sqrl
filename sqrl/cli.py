import sys
from . import GNOME_ON
from getpass import getpass
from sqrlgui import gui_get_pass


class mkmCLI():

    @classmethod
    def update_password(self, manager):
        name = manager.get_account_name()
        print "Changing Password for: " + name
        old_pass = getpass("Please Enter Master Password: ")
        new_pass = getpass("Please Enter New Master Password: ")
        pass_conf = getpass("Please Confirm New Master Password: ")
        if manager.change_password(old_pass, new_pass, pass_conf):
            print "Password Updated for: " + name
        sys.exit()

    @classmethod
    def list_accounts(self, manager):
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
            self.create_account(manager)
        sys.exit()

    @classmethod
    def export_key(self, manager, id):
        pass

    @classmethod
    def delete_account(self, manager, id):
        pass

    @classmethod
    def select_account(self, manager, id):
        if manager.set_account(id):
            self.list_accounts(manager)
        else:
            print "Invalid Account ID"
        sys.exit()

    @classmethod
    def create_account(self, manager):
        try:
            name = raw_input("Please enter name of Account Owner: ")
            pswd = getpass("Please Enter Master Password: ")
            pswd_confirm = getpass("Please Confirm Master Password: ")
            if manager.create_account({'name': name}, pswd, pswd_confirm):
                print "Account Created"
        except:
            print "Account NOT Created"
        sys.exit()

    @classmethod
    def unlock_account(self, manager):
        try:
            if GNOME_ON:
                name = manager.get_account_name()
                value = "Enter Password for:\n" + name
                password = gui_get_pass(value)
            else:
                password = getpass("Please Enter Master Password: ")
        except:
            sys.exit()

        key = manager.get_key(password)
        if key:
            return key
        else:
            print "Invalid Password"
            return False
