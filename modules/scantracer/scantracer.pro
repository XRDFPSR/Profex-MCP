# -------------------------------------------------
# Project created by QtCreator 2011-04-14T14:55:51
# -------------------------------------------------
QT += xml widgets printsupport svg concurrent # core5compat
DEFINES  += QT_NO_SSL
TARGET = profexst
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
    RC_ICONS = $$PWD/../../profex/icons/profex5st.ico

    QMAKE_LFLAGS_CONSOLE    = /SUBSYSTEM:CONSOLE,5.01
    QMAKE_LFLAGS_WINDOWS    = /SUBSYSTEM:WINDOWS,5.01
}
macx{
    ICON = $$PWD/../../profex/icons/profex5st.icns
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
        $$PWD/scantracerscene.cpp \
        $$PWD/scantracerimagetracer.cpp \
        $$PWD/scantracergraphicsview.cpp \
        $$PWD/scantracercalibinfowidget.cpp \
        $$PWD/scantracerbaselineinfowidget.cpp \
        $$PWD/helpaboutdialog.cpp


HEADERS += $$PWD/mainwindow.h \
        $$PWD/scantracerscene.h \
        $$PWD/scantracerimagetracer.h \
        $$PWD/scantracergraphicsview.h \
        $$PWD/scantracercalibinfowidget.h \
        $$PWD/scantracerbaselineinfowidget.h \
        $$PWD/helpaboutdialog.h

FORMS   += $$PWD/mainwindow.ui \
        $$PWD/helpaboutdialog.ui

RESOURCES += $$PWD/../../profex/profex.qrc \
    $$PWD/../../profex/icons/profex-light/profex-light.qrc \
    $$PWD/../../profex/icons/profex-light-colored/profex-light-colored.qrc \
    $$PWD/../../profex/icons/profex-dark/profex-dark.qrc

# end of file

