# -------------------------------------------------
# Project created by QtCreator 2011-04-14T14:55:51
# -------------------------------------------------
QT += xml core5compat qml

CONFIG += c++17 console
CONFIG -= app_bundle

DEFINES  += QT_NO_SSL
TARGET = pxapplypreset
TEMPLATE = app
DESTDIR = $$OUT_PWD/../../bin

include(../../version.pri)

DEFINES += "VERSION_MAJOR=$$VERSION_MAJOR" \
           "VERSION_MINOR=$$VERSION_MINOR" \
           "VERSION_BUILD=$$VERSION_BUILD"

#Target version
VERSION = $${VERSION_MAJOR}.$${VERSION_MINOR}.$${VERSION_BUILD}

win32{
    RC_ICONS = $$PWD/../../profex/icons/profex5.ico

    QMAKE_LFLAGS_CONSOLE    = /SUBSYSTEM:CONSOLE,5.01
    QMAKE_LFLAGS_WINDOWS    = /SUBSYSTEM:WINDOWS,5.01
}
macx{
    ICON = $$PWD/../../profex/icons/profex5.icns
    CONFIG += sdk_no_version_check

#    QMAKE_MAC_SDK = macosx11.3
#    QMAKE_MACOSX_DEPLOYMENT_TARGET = 11.3
}

INCLUDEPATH += $$PWD/../../zlib $$PWD/../../quazip $$PWD/../../libXrdIO $$PWD

unix:!macx{
    LIBS += -L$$OUT_PWD/../../lib -l:libxrdio.a
    LIBS += -L$$OUT_PWD/../../lib -l:libquazip.a
    LIBS += -L$$OUT_PWD/../../lib -l:libz.a
}

macx{
    LIBS += -L$$OUT_PWD/../../lib -lxrdio
    LIBS += -L$$OUT_PWD/../../lib -lquazip
    LIBS += -L$$OUT_PWD/../../lib -lz
}

win32{
    LIBS += -L$$OUT_PWD/../../lib -lxrdio5
    LIBS += -L$$OUT_PWD/../../lib -lquazip
    LIBS += -L$$OUT_PWD/../../lib -lz
}

SOURCES += $$PWD/main.cpp \
        $$PWD/pxapplypreset.cpp

HEADERS += $$PWD/pxapplypreset.h


# end of file
