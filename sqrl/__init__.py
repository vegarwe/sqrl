import os
import sys

VERSION = "0.1.0"
HOME = os.environ['HOME']
CONFIG_DIR = '.config/sqrl/'
WORKING_DIR = HOME + '/' + CONFIG_DIR
GNOME_ON = False

if os.environ.get('DESKTOP_SESSION') == 'gnome':
    if sys.stdout.isatty():
        GNOME_ON = False
    else:
        GNOME_ON = True
