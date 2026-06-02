TARGET = z
TEMPLATE = lib
CONFIG += static
CONFIG -= gui core
DESTDIR = $$OUT_PWD/../lib
QMAKE_LFLAGS_CONSOLE    = /SUBSYSTEM:CONSOLE,5.01
QMAKE_LFLAGS_WINDOWS    = /SUBSYSTEM:WINDOWS,5.01

HEADERS += crc32.h  \
 gzguts.h \
 inffixed.h \
 inftrees.h \
 zconf.h \
 zutil.h \
 deflate.h \
 inffast.h \
 inflate.h  \
 trees.h  \
 zlib.h

SOURCES += adler32.c  \
 deflate.c \
 gzread.c \
 inffast.c \
 trees.c \
 compress.c \
 gzclose.c \
 gzwrite.c \
 inflate.c \
 uncompr.c \
 crc32.c  \
 gzlib.c \
 infback.c \
 inftrees.c \
 zutil.c
