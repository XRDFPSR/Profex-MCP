# -------------------------------------------------
# Project created by QtCreator 2011-04-14T14:55:51
# -------------------------------------------------
QT += xml widgets printsupport svg concurrent qml # core5compat
DEFINES  += QT_NO_SSL
TARGET = profexed
TEMPLATE = app
# CONFIG += c++11
DESTDIR = $$OUT_PWD/../../bin

include(../../version.pri)

DEFINES += "VERSION_MAJOR=$$VERSION_MAJOR" \
           "VERSION_MINOR=$$VERSION_MINOR" \
           "VERSION_BUILD=$$VERSION_BUILD"

#Target version
VERSION = $${VERSION_MAJOR}.$${VERSION_MINOR}.$${VERSION_BUILD}

win32{
    RC_ICONS = $$PWD/../../profex/icons/profex5ed.ico

    QMAKE_LFLAGS_CONSOLE    = /SUBSYSTEM:CONSOLE,5.01
    QMAKE_LFLAGS_WINDOWS    = /SUBSYSTEM:WINDOWS,5.01
}
macx{
    ICON = $$PWD/../../profex/icons/profex5ed.icns
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
           $$PWD/mainwindow.cpp \
           $$PWD/emapscale2d.cpp \
           $$PWD/threadfouriersynth.cpp \
           $$PWD/threadrenderemap.cpp \
           $$PWD/emapwidget2d.cpp \
           $$PWD/emapimporthandler.cpp \
           $$PWD/emapsettingsdialog.cpp \
           $$PWD/helpaboutdialog.cpp \
           emapdatahandler.cpp \
           emapexportcoordinatedialog.cpp


HEADERS += mainwindow.h \
           $$PWD/emapscale2d.h \
           $$PWD/threadfouriersynth.h \
           $$PWD/threadrenderemap.h \
           $$PWD/emapwidget2d.h \
           $$PWD/emapimporthandler.h \
           $$PWD/emapsettingsdialog.h \
           $$PWD/emapstructs.h \
           $$PWD/helpaboutdialog.h \
           emapdatahandler.h \
           emapexportcoordinatedialog.h

FORMS   += mainwindow.ui \
           $$PWD/emapsettingsdialog.ui \
           $$PWD/helpaboutdialog.ui \
           emapexportcoordinatedialog.ui

RESOURCES += $$PWD/../../profex/profex.qrc \
    $$PWD/../../profex/icons/profex-light/profex-light.qrc \
    $$PWD/../../profex/icons/profex-light-colored/profex-light-colored.qrc \
    $$PWD/../../profex/icons/profex-dark/profex-dark.qrc

# end of file

