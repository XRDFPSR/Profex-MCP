# -------------------------------------------------
# Project created by QtCreator 2011-04-14T14:55:51
# -------------------------------------------------
QT += xml widgets printsupport svg sql qml concurrent network svgwidgets core5compat
DEFINES  += QT_NO_SSL
TARGET = profex
TEMPLATE = app
DESTDIR = $$OUT_PWD/../bin

include(../version.pri)

DEFINES += "VERSION_MAJOR=$$VERSION_MAJOR" \
           "VERSION_MINOR=$$VERSION_MINOR" \
           "VERSION_BUILD=$$VERSION_BUILD"

#Target version
VERSION = $${VERSION_MAJOR}.$${VERSION_MINOR}.$${VERSION_BUILD}

win32{
    QT += axcontainer
    RC_ICONS = $$PWD/../profex/icons/profex5.ico

    QMAKE_LFLAGS_CONSOLE    = /SUBSYSTEM:CONSOLE,5.01
    QMAKE_LFLAGS_WINDOWS    = /SUBSYSTEM:WINDOWS,5.01
}
macx{
    ICON = $$PWD/../profex/icons/profex5.icns
    CONFIG += sdk_no_version_check
}

INCLUDEPATH += $$PWD/../zlib $$PWD/../quazip $$PWD/../libXrdIO $$PWD

unix:!macx{
    LIBS += -L$$OUT_PWD/../lib -l:libxrdio.a
    LIBS += -L$$OUT_PWD/../lib -l:libquazip.a
    LIBS += -L$$OUT_PWD/../lib -l:libz.a
}

macx{
    LIBS += -L$$OUT_PWD/../lib -lxrdio
    LIBS += -L$$OUT_PWD/../lib -lquazip
    LIBS += -L$$OUT_PWD/../lib -lz
}

win32{
    LIBS += -L$$OUT_PWD/../lib -lxrdio5
    LIBS += -L$$OUT_PWD/../lib -lquazip
    LIBS += -L$$OUT_PWD/../lib -lz
}

include(tools/tools.pri)
include(projectWidget/projectwidget.pri)
include(preferencesDialog/preferencesdialog.pri)
include(3rdparty/3rdparty.pri)
include(projectWidget/searchMatchWidget/searchmatchwidget.pri)
include(strucImportDialog/strucimportdialog.pri)
include(searchInFilesDialog/searchinfilesdialog.pri)
include(editInstrumentDialog/editinstrumentdialog.pri)

SOURCES += main.cpp \
    helptextmanager.cpp \
    indexingprogressdialog.cpp \
    mainwindow.cpp \
    exportgraphdialog.cpp \
    contexthelpdisplay.cpp \
    imageresolutiondialog.cpp \
    oqprojectdialog.cpp \
    oqprojecthandler.cpp \
    presetmenumanager.cpp \
    projectwidgetdock.cpp \
    statusbarlabel.cpp \
    updatemanager.cpp \
    zoomrangedialog.cpp \
    projectselectdialog.cpp \
    newinstrumentdialog.cpp \
    helpaboutdialog.cpp \
    learnprofiledialog.cpp \
    searchreplacealldialog.cpp \
    datatable.cpp \
    cifexportdialog.cpp \
    projectstreewidget.cpp \
    xrdcustomplot.cpp \
    backdropwidget.cpp \
    wavelengthcombobox.cpp \
    wavelengthselectdialog.cpp

HEADERS += mainwindow.h \
    exportgraphdialog.h \
    contexthelpdisplay.h \
    helptextmanager.h \
    imageresolutiondialog.h \
    indexingprogressdialog.h \
    oqprojectdialog.h \
    oqprojecthandler.h \
    presetmenumanager.h \
    projectwidgetdock.h \
    statusbarlabel.h \
    updatemanager.h \
    zoomrangedialog.h \
    projectselectdialog.h \
    newinstrumentdialog.h \
    helpaboutdialog.h \
    learnprofiledialog.h \
    searchreplacealldialog.h \
    datatable.h \
    cifexportdialog.h \
    projectstreewidget.h \
    xrdcustomplot.h \
    backdropwidget.h \
    wavelengthcombobox.h \
    wavelengthselectdialog.h

FORMS += mainwindow.ui \
    exportgraphdialog.ui \
    imageresolutiondialog.ui \
    indexingprogressdialog.ui \
    oqprojectdialog.ui \
    zoomrangedialog.ui \
    projectselectdialog.ui \
    newinstrumentdialog.ui \
    helpaboutdialog.ui \
    addremovephasedialog.ui \
    learnprofiledialog.ui \
    searchreplacealldialog.ui \
    cifexportdialog.ui \
    wavelengthselectdialog.ui

RESOURCES += profex.qrc \
    icons/profex-light/profex-light.qrc \
    icons/profex-light-colored/profex-light-colored.qrc \
    icons/profex-dark/profex-dark.qrc \
    resources/oqproject.qrc

DISTFILES += resources/report.css

# end of file
