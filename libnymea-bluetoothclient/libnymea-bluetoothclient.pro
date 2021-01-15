TARGET = nymea-bluetoothclient

QT -= gui
QT += bluetooth network

QMAKE_CXXFLAGS *= -Werror -std=c++11 -g
QMAKE_LFLAGS *= -std=c++11

CONFIG += link_pkgconfig
PKGCONFIG += libsodium

TEMPLATE = lib

DEFINES += VERSION_STRING=\\\"$${VERSION_STRING}\\\"

SOURCES += \
    bluetoothclient.cpp \
    encryptionhandler.cpp \
    loggingcategories.cpp

HEADERS += \
    bluetoothclient.h \
    encryptionhandler.h \
    loggingcategories.h

target.path = $$[QT_INSTALL_LIBS]
INSTALLS += target

# install header file with relative subdirectory
for(header, HEADERS) {
    path = $$[QT_INSTALL_PREFIX]/include/nymea-bluetoothclient/$${dirname(header)}
    eval(headers_$${path}.files += $${header})
    eval(headers_$${path}.path = $${path})
    eval(INSTALLS *= headers_$${path})
}

# Create pkgconfig file
CONFIG += create_pc create_prl no_install_prl
QMAKE_PKGCONFIG_NAME = nymea-bluetoothclient
QMAKE_PKGCONFIG_DESCRIPTION = nymea bluetooth client development library
QMAKE_PKGCONFIG_PREFIX = $$[QT_INSTALL_PREFIX]
QMAKE_PKGCONFIG_INCDIR = $$[QT_INSTALL_PREFIX]/include/nymea-bluetoothclient/
QMAKE_PKGCONFIG_LIBDIR = $$target.path
QMAKE_PKGCONFIG_VERSION = $$VERSION_STRING
QMAKE_PKGCONFIG_FILE = nymea-bluetoothclient
QMAKE_PKGCONFIG_DESTDIR = pkgconfig
