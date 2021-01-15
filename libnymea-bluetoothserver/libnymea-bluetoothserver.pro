TARGET = nymea-bluetoothserver

QT -= gui
QT += bluetooth dbus network

QMAKE_CXXFLAGS *= -Werror -std=c++11 -g
QMAKE_LFLAGS *= -std=c++11

CONFIG += link_pkgconfig
PKGCONFIG += nymea-networkmanager libsodium

TEMPLATE = lib

DEFINES += VERSION_STRING=\\\"$${VERSION_STRING}\\\"

SOURCES += \
    bluetoothserver.cpp \
    bluetoothservicedatahandler.cpp \
    encryptionhandler.cpp \
    encryptionservice.cpp \
    loggingcategories.cpp \
    networkmanager/networkmanagerservice.cpp \
    networkmanager/networkservice.cpp \
    networkmanager/wirelessservice.cpp

HEADERS += \
    bluetoothserver.h \
    bluetoothservice.h \
    bluetoothservicedatahandler.h \
    encryptionhandler.h \
    encryptionservice.h \
    loggingcategories.h \
    networkmanager/networkmanagerservice.h \
    networkmanager/networkservice.h \
    networkmanager/wirelessservice.h

target.path = $$[QT_INSTALL_LIBS]
INSTALLS += target

# install header file with relative subdirectory
for(header, HEADERS) {
    path = $$[QT_INSTALL_PREFIX]/include/nymea-bluetoothserver/$${dirname(header)}
    eval(headers_$${path}.files += $${header})
    eval(headers_$${path}.path = $${path})
    eval(INSTALLS *= headers_$${path})
}

# Create pkgconfig file
CONFIG += create_pc create_prl no_install_prl
QMAKE_PKGCONFIG_NAME = nymea-bluetoothserver
QMAKE_PKGCONFIG_DESCRIPTION = nymea bluetooth server development library
QMAKE_PKGCONFIG_PREFIX = $$[QT_INSTALL_PREFIX]
QMAKE_PKGCONFIG_INCDIR = $$[QT_INSTALL_PREFIX]/include/nymea-bluetoothserver/
QMAKE_PKGCONFIG_LIBDIR = $$target.path
QMAKE_PKGCONFIG_VERSION = $$VERSION_STRING
QMAKE_PKGCONFIG_FILE = nymea-bluetoothserver
QMAKE_PKGCONFIG_DESTDIR = pkgconfig
