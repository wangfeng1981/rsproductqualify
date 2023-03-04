QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    ../sharedcodes2/filenameutil.cpp \
    ../sharedcodes2/simple_linear_regression.cpp \
    ../sharedcodes2/wogrcoorconverter.cpp \
    ../sharedcodes2/wprocessqueue.cpp \
    ../sharedcodes2/wstringutils.cpp \
    assertlineedit.cpp \
    computeapu.cpp \
    main.cpp \
    mainwindow.cpp \
    maskvaluevalidator.cpp \
    plotorder.cpp \
    plotvarreplaceutil.cpp \
    wcomparetask.cpp \
    wfilepair.cpp \
    wrastercompare.cpp

HEADERS += \
    ../sharedcodes2/filenameutil.h \
    ../sharedcodes2/simple_linear_regression.h \
    ../sharedcodes2/wGdalRaster.h \
    ../sharedcodes2/wogrcoorconverter.h \
    ../sharedcodes2/wprocessqueue.h \
    ../sharedcodes2/wstringutils.h \
    ../sharedcodes2/wtextfilereader.h \
    assertlineedit.h \
    computeapu.h \
    mainwindow.h \
    maskvaluevalidator.h \
    plotorder.h \
    plotvarreplaceutil.h \
    wcomparetask.h \
    wfilepair.h \
    wrastercompare.h

FORMS += \
    mainwindow.ui

INCLUDEPATH += "D:\coding\sharedcodes2"
INCLUDEPATH += "C:\OSGeo4W\include"
LIBS += -L"C:\OSGeo4W\lib" -lgdal_i

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
