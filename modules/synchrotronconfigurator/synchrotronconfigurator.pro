# -------------------------------------------------
# Project created by QtCreator 2011-04-14T14:55:51
# -------------------------------------------------
QT += xml widgets printsupport svgwidgets concurrent # core5compat
DEFINES  += QT_NO_SSL
TARGET = profexsc
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
    RC_ICONS = $$PWD/../../profex/icons/profex5sc.ico

    QMAKE_LFLAGS_CONSOLE    = /SUBSYSTEM:CONSOLE,5.01
    QMAKE_LFLAGS_WINDOWS    = /SUBSYSTEM:WINDOWS,5.01
}
macx{
    ICON = $$PWD/../../profex/icons/profex5sc.icns
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

include(qcustomplot/qcustomplot.pri)

SOURCES += $$PWD/main.cpp \
        $$PWD/mainwindow.cpp \
        $$PWD/helpaboutdialog.cpp \
        arrowlineitem.cpp \
        bgmngeqexport.cpp \
        centeredsimpletextitem.cpp \
        lorentzparam.cpp \
        modelfitterchernyshov.cpp \
        modelmanagerchernyshov.cpp \
        parameterstorage.cpp \
        peakfitterchernyshov.cpp \
        peakmanagerchernyshov.cpp \
        peakmodelchernyshov.cpp \
        synchrotronpreferencesdialog.cpp \
        synchrotronxmlio.cpp \
        threadsafeplotter.cpp \
        treewidgetmodelpeaks.cpp \
        treewidgetsupportpeaks.cpp \
        instrumentgraphicsview.cpp \
        instrumentscene.cpp \
        opticsitem.cpp

HEADERS += $$PWD/mainwindow.h \
        $$PWD/helpaboutdialog.h \
        arrowlineitem.h \
        bgmngeqexport.h \
        centeredsimpletextitem.h \
        lorentzparam.h \
        modelfitterchernyshov.h \
        modelmanagerchernyshov.h \
        noeditdelegate.h \
        parameterstorage.h \
        peakfitterchernyshov.h \
        peakmanagerchernyshov.h \
        peakmodelchernyshov.h \
        structssc.h \
        synchrotronpreferencesdialog.h \
        synchrotronxmlio.h \
        threadsafeplotter.h \
        treewidgetmodelpeaks.h \
        treewidgetsupportpeaks.h \
        instrumentgraphicsview.h \
        instrumentscene.h \
        opticsitem.h

FORMS   += $$PWD/mainwindow.ui \
        $$PWD/helpaboutdialog.ui \
        synchrotronpreferencesdialog.ui

RESOURCES += $$PWD/../../profex/profex.qrc \
    $$PWD/../../profex/icons/profex-light/profex-light.qrc \
    $$PWD/../../profex/icons/profex-light-colored/profex-light-colored.qrc \
    $$PWD/../../profex/icons/profex-dark/profex-dark.qrc \
    $$PWD/synchrotronconfigurator.qrc \
    $$PWD/../../profex/profex.qrc

# end of file

