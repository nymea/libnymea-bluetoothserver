TEMPLATE = subdirs
SUBDIRS += libnymea-bluetoothserver libnymea-bluetoothclient

VERSION_STRING=$$system('dpkg-parsechangelog | sed -n -e "s/^Version: //p"')

