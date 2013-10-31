#!/usr/bin/python
from gi.repository import Gtk


def gui_get_pass():
    dialog = Gtk.Dialog()
    dialog.set_modal(True)
    userEntry = Gtk.Entry()
    userEntry.set_visibility(False)
    dialogBox = dialog.get_content_area()
    dialogBox.pack_end(userEntry, False, False, 0)
    dialog.add_button("Ok", Gtk.ResponseType.OK)
    dialog.show_all()
    dialog.run()
    return userEntry.get_text()

