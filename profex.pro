TEMPLATE = subdirs
CONFIG += ordered warn_off
SUBDIRS = zlib \
    quazip \
    libXrdIO \
    profex \
    modules/waterfall \
    modules/electrondensity \
    modules/scantracer \
    modules/synchrotronconfigurator \
    cmdtools/pxanytoxy \
    cmdtools/pxapplypreset \

profex.depends = zlib quazip libXrdIO
waterfall.depends = zlib quazip libXrdIO
electrondensity.depends = zlib quazip libXrdIO
scantracer.depends = zlib quazip libXrdIO
synchrotronconfigurator.depends = zlib quazip libXrdIO
pxanytoxy.depends = zlib quazip libXrdIO
pxapplypreset.depends = zlib quazip libXrdIO

# make these files show up in Qt Creator
OTHER_FILES += .gitignore \
    todo.txt \
    changelog.txt \
    org.profex_xrd.Profex.appdata.xml \
    profex5.desktop \
    version.pri \
    scripts/deploy-win-64.bat \
    scripts/deploy-win-64-final.bat \
    scripts/pull-and-compile.sh \
    scripts/pull-and-compile-m1.sh \
    scripts/pull-and-compile-final.sh \
    scripts/pull-and-compile-m1-final.sh \
    scripts/deploy-source.sh \
    scripts/deploy-source-final.sh \
    scripts/deploy-linux-qtifw.sh
