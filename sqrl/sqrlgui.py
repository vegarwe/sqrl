#!/usr/bin/python
from gi.repository import Gtk


class pyInput(Gtk.Window):

    def __init__(self):
        Gtk.Window.__init__(self, title="SQRL Password Prompt")

        vbox = Gtk.VBox()
        self.input = Gtk.Entry()
        self.input.set_visibility(False)
        self.button = Gtk.Button(label="Unlock Account")
        self.button.connect("clicked", self.on_button_clicked)
        self.add(vbox)
        vbox.add(self.input)
        vbox.add(self.button)

    def on_button_clicked(self, widget):
        self.func(self.input.get_text())
        Gtk.main_quit()

    def get_password(self, func):
        self.func = func
        self.show_all()
        Gtk.main()
