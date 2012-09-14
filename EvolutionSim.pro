# -------------------------------------------------
# Project created by QtCreator 2010-02-10T13:30:03
# -------------------------------------------------
QT += multimedia
TARGET = EvolutionSim
TEMPLATE = app
SOURCES += main.cpp \
    mainwindow.cpp \
    simmanager.cpp \
    critter.cpp \
    populationscene.cpp \
    environmentscene.cpp \
    settings.cpp \
    sortablegenome.cpp \
    analyser.cpp \
    genomecomparison.cpp \
    fossilrecord.cpp \
    fossrecwidget.cpp \
    resizecatcher.cpp \
    analysistools.cpp
HEADERS += mainwindow.h \
    simmanager.h \
    critter.h \
    populationscene.h \
    environmentscene.h \
    settings.h \
    sortablegenome.h \
    analyser.h \
    genomecomparison.h \
    fossilrecord.h \
    fossrecwidget.h \
    resizecatcher.h \
    analysistools.h
FORMS += mainwindow.ui \
    dialog.ui \
    fossrecwidget.ui \
    genomecomparison.ui

OTHER_FILES += \
    README.TXT

RESOURCES += \
    resources.qrc
