#-------------------------------------------------
#
# Project created by QtCreator 2016-01-28T11:11:11
#
#-------------------------------------------------

TARGET = xrdio
TEMPLATE = lib
QT += core gui xml qml concurrent sql svg
CONFIG += c++17
DESTDIR = $$OUT_PWD/../lib
!win32:CONFIG += static
win32:CONFIG += shared
DEFINES += XRDIO

INCLUDEPATH += $$PWD/../zlib $$PWD/../quazip $$PWD

unix:!macx{
    LIBS += -L$$OUT_PWD/../lib -l:libquazip.a
    LIBS += -L$$OUT_PWD/../lib -l:libz.a
}

macx{
    LIBS += -L$$OUT_PWD/../lib -lquazip
    LIBS += -L$$OUT_PWD/../lib -lz
}

win32{
    LIBS += -L$$OUT_PWD/../lib -lquazip
    LIBS += -L$$OUT_PWD/../lib -lz
}

include(../version.pri)

DEFINES += "VERSION_MAJOR=$$VERSION_MAJOR" \
           "VERSION_MINOR=$$VERSION_MINOR" \
           "VERSION_BUILD=$$VERSION_BUILD"

#Target version
VERSION = $${VERSION_MAJOR}.$${VERSION_MINOR}.$${VERSION_BUILD}

include(3rdparty/alglib/alglib.pri)

SOURCES += bgmnfileio.cpp \
    absorptioncoefficientcalculator.cpp \
    chemtablestruct.cpp \
    coddbmanager.cpp \
    crystal/crystalhklgenerator.cpp \
    curveFitting/cubiccurve.cpp \
    curveFitting/fwhmmodelchernyshovfocused.cpp \
    curveFitting/lorentziancurve.cpp \
    curveFitting/polynom4curve.cpp \
    curveFitting/pseudovoigtcurve.cpp \
    curveFitting/quadraticcurve.cpp \
    elementscatteringdatadownloader.cpp \
    elementscatteringdatamanager.cpp \
    export/asciitxtexport.cpp \
    export/profexpgxexport.cpp \
    graphtosvg.cpp \
    hkl.cpp \
    import/chiimport.cpp \
    import/pyfaidatimport.cpp \
    import/xyeimport.cpp \
    parser/bgmndelayedparser.cpp \
    parser/bgmnhtmlreportgenerator.cpp \
    scan.cpp \
    settingsmanager.cpp \
    import/asciixyimport.cpp \
    import/bgmndiaimport.cpp \
    import/brukerbrmlimport.cpp \
    import/brukerrawimport.cpp \
    import/fullprofdat10import.cpp \
    import/fullprofprfimport.cpp \
    import/fullprofsubimport.cpp \
    import/genericimport.cpp \
    import/importhandler.cpp \
    import/jademdiimport.cpp \
    import/jadexmlimport.cpp \
    import/panalyticalxrdmlimport.cpp \
    import/pdcifimport.cpp \
    import/philipsrdimport.cpp \
    import/philipsudfimport.cpp \
    import/reynoldspltimport.cpp \
    import/rigakubinimport.cpp \
    import/rigakudatimport.cpp \
    import/rigakudifimport.cpp \
    import/rigakurawimport.cpp \
    import/rigakurasimport.cpp \
    import/rigakurasximport.cpp \
    import/rigakuxmlimport.cpp \
    import/seifertvalimport.cpp \
    import/stoeproimport.cpp \
    import/stoerawimport.cpp \
    import/texplusovlimport.cpp \
    import/gsasstdimport.cpp \
    import/gnresgimport.cpp \
    import/thermotxlimport.cpp \
    import/nexusrawimport.cpp \
    export/asciihklexport.cpp \
    export/asciixyexport.cpp  \
    export/exporthandler.cpp  \
    export/fullprofdatexport.cpp \
    export/genericexport.cpp  \
    export/gnuplotexport.cpp \
    export/graceexport.cpp \
    export/pdcifexport.cpp \
    export/philipsudfexport.cpp \
    export/textureplusexport.cpp \
    export/gsasstdexport.cpp \
    parser/bgmnlamparser.cpp \
    parser/bgmnlstparser.cpp \
    parser/bgmnparparser.cpp \
    parser/bgmnresparser.cpp \
    parser/bgmnsavparser.cpp \
    parser/bgmnstrparser.cpp \
    parser/cifparser.cpp  \
    parser/fppcrparser.cpp \
    parser/fpsumparser.cpp \
    parser/icddxmlparser.cpp \
    parser/spgrdatparser.cpp \
    crystal/crystalatom.cpp \
    crystal/crystalstructure.cpp \
    crystal/crystalunitcell.cpp \
    crystal/crystalstructurefactor.cpp \
    scanops.cpp \
    parser/bgmngeqparser.cpp \
    parser/convolutiondata.cpp \
    parser/bgmnsgdatparser.cpp \
    parser/bgmngeqdata.cpp \
    parser/bgmnlamdata.cpp \
    parser/bgmnsampledata.cpp \
    parser/eflechparparser.cpp \
    parser/bgmngerparser.cpp \
    hklphasedata.cpp \
    import/thermorawimport.cpp \
    import/thermoniimport.cpp \
    parser/bgmnprotocolparser.cpp \
    functions.cpp \
    curveFitting/genericcurve.cpp \
    curveFitting/linearcurve.cpp \
    curveFitting/gaussiancurve.cpp \
    curveFitting/pearsoncurve.cpp \
    curveFitting/gaussiansplitcurve.cpp \
    curveFitting/lorentziansplitcurve.cpp \
    curveFitting/pseudovoigtsplitcurve.cpp \
    curveFitting/pearsonsplitcurve.cpp \
    curveFitting/bgmnl2curve.cpp \
    curveFitting/curvefittingmanager.cpp \
    curveFitting/curvehandler.cpp \
    curveFitting/fwhmmodelchernyshov.cpp \
    parser/bgmninstrumentsavparser.cpp \
    parser/rruffdifparser.cpp \
    crystal/crystalsymop.cpp \
    parser/bgmnpdbparser.cpp \
    parser/codhklparser.cpp \
    parser/cifparser2.cpp \
    elementscatteringdata.cpp \
    colorMaps/lutgenerator.cpp \
    colorMaps/imageeffects.cpp \
    colorMaps/sobelfilter.cpp \
    bgmnpresethandler.cpp \
    chemtabledata.cpp

HEADERS  += bgmnfileio.h \
    absorptioncoefficientcalculator.h \
    chemtablestruct.h \
    coddbmanager.h \
    crystal/crystalhklgenerator.h \
    curveFitting/cubiccurve.h \
    curveFitting/fwhmmodelchernyshovfocused.h \
    curveFitting/lorentziancurve.h \
    curveFitting/polynom4curve.h \
    curveFitting/pseudovoigtcurve.h \
    curveFitting/quadraticcurve.h \
    elementscatteringdatadownloader.h \
    elementscatteringdatamanager.h \
    export/asciitxtexport.h \
    export/profexpgxexport.h \
    graphtosvg.h \
    hkl.h \
    import/chiimport.h \
    import/pyfaidatimport.h \
    import/xyeimport.h \
    parser/bgmndelayedparser.h \
    parser/bgmnhtmlreportgenerator.h \
    scan.h \
    settingsmanager.h \
    structs.h \
    import/asciixyimport.h \
    import/bgmndiaimport.h \
    import/brukerbrmlimport.h \
    import/brukerrawimport.h \
    import/fullprofdat10import.h \
    import/fullprofprfimport.h \
    import/fullprofsubimport.h \
    import/genericimport.h \
    import/importhandler.h \
    import/jademdiimport.h \
    import/jadexmlimport.h \
    import/panalyticalxrdmlimport.h \
    import/pdcifimport.h \
    import/philipsrdimport.h \
    import/philipsudfimport.h \
    import/reynoldspltimport.h \
    import/rigakubinimport.h \
    import/rigakudatimport.h \
    import/rigakudifimport.h \
    import/rigakurawimport.h \
    import/rigakurasimport.h \
    import/rigakurasximport.h \
    import/rigakuxmlimport.h \
    import/seifertvalimport.h \
    import/stoeproimport.h \
    import/stoerawimport.h \
    import/texplusovlimport.h \
    import/gsasstdimport.h \
    import/gnresgimport.h \
    import/thermotxlimport.h \
    import/nexusrawimport.h \
    export/asciihklexport.h \
    export/asciixyexport.h  \
    export/exporthandler.h  \
    export/fullprofdatexport.h \
    export/genericexport.h  \
    export/gnuplotexport.h \
    export/graceexport.h \
    export/pdcifexport.h \
    export/philipsudfexport.h \
    export/textureplusexport.h \
    export/gsasstdexport.h \
    parser/bgmnlamparser.h \
    parser/bgmnlstparser.h \
    parser/bgmnparparser.h \
    parser/bgmnresparser.h \
    parser/bgmnsavparser.h \
    parser/bgmnstrparser.h \
    parser/cifparser.h  \
    parser/fppcrparser.h \
    parser/fpsumparser.h \
    parser/icddxmlparser.h \
    parser/spgrdatparser.h \
    crystal/crystalatom.h \
    crystal/crystalstructure.h \
    crystal/crystalunitcell.h \
    crystal/crystalstructurefactor.h \
    scanops.h \
    parser/bgmngeqparser.h \
    parser/convolutiondata.h \
    parser/bgmnsgdatparser.h \
    parser/bgmnlamdata.h \
    parser/bgmngeqdata.h \
    parser/bgmnsampledata.h \
    parser/eflechparparser.h \
    parser/bgmngerparser.h \
    hklphasedata.h \
    import/thermorawimport.h \
    import/thermoniimport.h \
    parser/bgmnprotocolparser.h \
    functions.h \
    curveFitting/genericcurve.h \
    curveFitting/linearcurve.h \
    curveFitting/gaussiancurve.h \
    curveFitting/pearsoncurve.h \
    curveFitting/gaussiansplitcurve.h \
    curveFitting/lorentziansplitcurve.h \
    curveFitting/pseudovoigtsplitcurve.h \
    curveFitting/pearsonsplitcurve.h \
    curveFitting/bgmnl2curve.h \
    curveFitting/curvefittingmanager.h \
    curveFitting/curvehandler.h \
    curveFitting/fwhmmodelchernyshov.h \
    parser/bgmninstrumentsavparser.h \
    parser/rruffdifparser.h \
    crystal/crystalsymop.h \
    parser/bgmnpdbparser.h \
    parser/codhklparser.h \
    parser/cifparser2.h \
    elementscatteringdata.h \
    colorMaps/lutstructs.h \
    colorMaps/lutgenerator.h \
    colorMaps/imageeffects.h \
    colorMaps/sobelfilter.h \
    bgmnpresethandler.h \
    chemtabledata.h



