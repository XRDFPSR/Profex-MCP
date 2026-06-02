INCLUDEPATH += $$PWD

HEADERS += $$PWD/src/alglibinternal.h \
           $$PWD/src/alglibmisc.h \
           $$PWD/src/ap.h \
           $$PWD/src/dataanalysis.h \
           $$PWD/src/diffequations.h \
           $$PWD/src/fasttransforms.h \
           $$PWD/src/integration.h \
           $$PWD/src/interpolation.h \
           $$PWD/src/kernels_avx2.h \
           $$PWD/src/kernels_fma.h \
           $$PWD/src/kernels_sse2.h \
           $$PWD/src/linalg.h \
           $$PWD/src/optimization.h \
           $$PWD/src/solvers.h \
           $$PWD/src/specialfunctions.h \
           $$PWD/src/statistics.h \
           $$PWD/src/stdafx.h

SOURCES += $$PWD/src/alglibinternal.cpp \
           $$PWD/src/alglibmisc.cpp \
           $$PWD/src/ap.cpp \
           $$PWD/src/dataanalysis.cpp \
           $$PWD/src/diffequations.cpp \
           $$PWD/src/fasttransforms.cpp \
           $$PWD/src/integration.cpp \
           $$PWD/src/interpolation.cpp \
           $$PWD/src/kernels_avx2.cpp \
           $$PWD/src/kernels_fma.cpp \
           $$PWD/src/kernels_sse2.cpp \
           $$PWD/src/linalg.cpp \
           $$PWD/src/optimization.cpp \
           $$PWD/src/solvers.cpp \
           $$PWD/src/specialfunctions.cpp \
           $$PWD/src/statistics.cpp

OTHER_FILES += $$PWD/manual.cpp.html
