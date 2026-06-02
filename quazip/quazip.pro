TEMPLATE = lib
CONFIG += qt warn_on
QT -= gui
QT += core
# core5compat is only needed for Qt6 builds; for Qt5 it's not available
DESTDIR = $$OUT_PWD/../lib

!win32:CONFIG += staticlib

# The ABI version.
!win32:VERSION = 1.0.0

# 1.0.0 is the first stable ABI.
# The next binary incompatible change will be 2.0.0 and so on.
# The existing QuaZIP policy on changing ABI requires to bump the
# major version of QuaZIP itself as well. Note that there may be
# other reasons for chaging the major version of QuaZIP, so
# in case where there is a QuaZIP major version bump but no ABI change,
# the VERSION variable will stay the same.

# For example:

# QuaZIP 1.0 is released after some 0.x, keeping binary compatibility.
# VERSION stays 1.0.0.
# Then some binary incompatible change is introduced. QuaZIP goes up to
# 2.0, VERSION to 2.0.0.
# And so on.


# This one handles dllimport/dllexport directives.
DEFINES += QUAZIP_BUILD
# DEFINES += QUAZIP_USE_QT_ZLIB=ON
DEFINES += QT_NO_CAST_FROM_ASCII
DEFINES += QT_NO_CAST_TO_ASCII
DEFINES += QUAZIP_QT_MAJOR_VERSION=6

# You'll need to define this one manually if using a build system other
# than qmake or using QuaZIP sources directly in your project.
CONFIG(staticlib): DEFINES += QUAZIP_STATIC

# Input
include(quazip.pri)

INCLUDEPATH += $$PWD/../zlib $$PWD

unix:!macx{
    LIBS += -L$$OUT_PWD/../lib -l:libz.a
}

macx{
    LIBS += -L$$OUT_PWD/../lib -lz
}

win32{
    LIBS += -L$$OUT_PWD/../lib -lz
}

unix {
    # headers.path=$$PREFIX/include/quazip
    # headers.files=$$HEADERS
    # target.path=$$PREFIX/lib/$${LIB_ARCH}
    # INSTALLS += headers target

	OBJECTS_DIR=.obj
	MOC_DIR=.moc
}

win32 {
    # headers.path=$$PREFIX/include/quazip
    # headers.files=$$HEADERS
    # target.path=$$PREFIX/lib
    # INSTALLS += headers target
    # workaround for qdatetime.h macro bug
    DEFINES += NOMINMAX
    QMAKE_LFLAGS_CONSOLE    = /SUBSYSTEM:CONSOLE,5.01
    QMAKE_LFLAGS_WINDOWS    = /SUBSYSTEM:WINDOWS,5.01
}
