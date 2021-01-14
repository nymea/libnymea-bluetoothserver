TEMPLATE = subdirs
SUBDIRS += libnymea-bluetoothserver

VERSION_STRING=$$system('dpkg-parsechangelog | sed -n -e "s/^Version: //p"')

